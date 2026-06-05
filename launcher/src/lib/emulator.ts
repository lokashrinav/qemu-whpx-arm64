import { spawn, ChildProcess } from "child_process";
import { existsSync, readdirSync, createReadStream, createWriteStream, statSync } from "fs";
import { join } from "path";
import { createGunzip } from "zlib";
import { pipeline } from "stream/promises";

const QEMU_BIN = "C:\\emu-src\\external\\qemu\\objs\\qemu\\windows-x86_64\\qemu-system-aarch64.exe";
const SYSTEM_IMAGES_ROOT = "C:\\emu-src\\prebuilts\\android-emulator-build\\system-images\\generic\\system-images";

let runningProcess: ChildProcess | null = null;
let consoleBuffer: string[] = [];
let emulatorState: "idle" | "preparing" | "booting" | "running" | "stopped" = "idle";
let prepareProgress = "";
let pendingImageDir = "";

export interface SystemImage {
  name: string;
  apiLevel: string;
  abi: string;
  dir: string;
  hasKernel: boolean;
  hasRamdisk: boolean;
  hasSystem: boolean;
  systemReady: boolean;
}

export function getQemuPath(): string | null {
  return existsSync(QEMU_BIN) ? QEMU_BIN : null;
}

export function discoverSystemImages(): SystemImage[] {
  const images: SystemImage[] = [];
  if (!existsSync(SYSTEM_IMAGES_ROOT)) return images;

  for (const apiDir of readdirSync(SYSTEM_IMAGES_ROOT)) {
    const apiPath = join(SYSTEM_IMAGES_ROOT, apiDir);
    try { if (!statSync(apiPath).isDirectory()) continue; } catch { continue; }

    for (const variant of readdirSync(apiPath)) {
      const variantPath = join(apiPath, variant);
      const arm64Path = join(variantPath, "arm64-v8a");
      if (!existsSync(arm64Path)) continue;

      const files = readdirSync(arm64Path);
      const hasKernel = files.includes("kernel-ranchu");
      const hasRamdisk = files.includes("ramdisk.img");
      const hasSystemGz = files.includes("system.img.gz");
      const hasSystemRaw = files.includes("system.img");

      if (!hasKernel || !hasRamdisk) continue;

      const apiLevel = apiDir.replace("android-", "");
      images.push({
        name: `Android ${apiLevel} (${variant})`,
        apiLevel,
        abi: "arm64-v8a",
        dir: arm64Path,
        hasKernel,
        hasRamdisk,
        hasSystem: hasSystemGz || hasSystemRaw,
        systemReady: hasSystemRaw,
      });
    }
  }

  return images.sort((a, b) => parseInt(b.apiLevel) - parseInt(a.apiLevel));
}

async function decompressAndBoot(imageDir: string): Promise<void> {
  const gzPath = join(imageDir, "system.img.gz");
  const imgPath = join(imageDir, "system.img");

  if (!existsSync(imgPath)) {
    if (!existsSync(gzPath)) {
      prepareProgress = "No system.img or system.img.gz found";
      emulatorState = "stopped";
      return;
    }

    emulatorState = "preparing";
    const gzSize = statSync(gzPath).size;
    prepareProgress = `Decompressing system.img.gz (${(gzSize / 1e9).toFixed(1)} GB compressed)...`;

    try {
      const src = createReadStream(gzPath);
      const gunzip = createGunzip();
      const dest = createWriteStream(imgPath);

      let written = 0;
      dest.on("drain", () => {});
      const origWrite = dest.write.bind(dest);
      dest.write = function(chunk: any, ...args: any[]) {
        written += chunk.length;
        if (written % (100 * 1024 * 1024) < chunk.length) {
          prepareProgress = `Decompressing: ${(written / 1e9).toFixed(1)} GB written...`;
        }
        return origWrite(chunk, ...args);
      } as typeof dest.write;

      await pipeline(src, gunzip, dest);
      prepareProgress = "Decompression complete";
    } catch (e) {
      prepareProgress = `Decompress failed: ${(e as Error).message}`;
      emulatorState = "stopped";
      return;
    }
  }

  bootQemu(imageDir);
}

function bootQemu(imageDir: string) {
  const kernel = join(imageDir, "kernel-ranchu");
  const ramdisk = join(imageDir, "ramdisk.img");
  const system = join(imageDir, "system.img");

  if (!existsSync(kernel) || !existsSync(ramdisk) || !existsSync(system)) {
    consoleBuffer.push("[Error: Missing boot files]");
    emulatorState = "stopped";
    return;
  }

  const args = [
    "-machine", "virt",
    "-cpu", "cortex-a57",
    "-accel", "whpx",
    "-m", "2048",
    "-nographic",
    "-serial", "stdio",
    "-net", "none",
    "-kernel", kernel,
    "-initrd", ramdisk,
    "-drive", `if=none,id=system,file=${system},format=raw,readonly=on`,
    "-device", "virtio-blk-device,drive=system",
    "-append", "console=ttyAMA0 androidboot.hardware=ranchu",
  ];

  emulatorState = "booting";
  consoleBuffer.push(`[Launching] qemu-system-aarch64 ${args.slice(0, 6).join(" ")} ...`);

  const proc = spawn(QEMU_BIN, args, {
    windowsHide: false,
    stdio: ["pipe", "pipe", "pipe"],
  });

  runningProcess = proc;

  let stdoutPartial = "";
  proc.stdout?.on("data", (data: Buffer) => {
    stdoutPartial += data.toString();
    const lines = stdoutPartial.split("\n");
    stdoutPartial = lines.pop() || "";
    for (const line of lines) {
      const trimmed = line.replace(/\r$/, "");
      if (trimmed) consoleBuffer.push(trimmed);
      if (trimmed.includes("Freeing unused kernel memory")) {
        emulatorState = "running";
      }
    }
  });

  let stderrPartial = "";
  proc.stderr?.on("data", (data: Buffer) => {
    stderrPartial += data.toString();
    const lines = stderrPartial.split("\n");
    stderrPartial = lines.pop() || "";
    for (const line of lines) {
      const trimmed = line.replace(/\r$/, "");
      if (trimmed) consoleBuffer.push(`[qemu] ${trimmed}`);
    }
  });

  proc.on("exit", (code) => {
    consoleBuffer.push(`\n[Emulator exited with code ${code}]`);
    emulatorState = "stopped";
    runningProcess = null;
  });

  proc.on("error", (err) => {
    consoleBuffer.push(`[Error: ${err.message}]`);
    emulatorState = "stopped";
    runningProcess = null;
  });
}

export function startBoot(imageDir: string): { ok: boolean; error?: string } {
  if (!existsSync(QEMU_BIN)) {
    return { ok: false, error: `QEMU binary not found: ${QEMU_BIN}` };
  }
  if (runningProcess && !runningProcess.killed) {
    return { ok: false, error: `Emulator already running (PID ${runningProcess.pid})` };
  }
  if (emulatorState === "preparing") {
    return { ok: false, error: "Already preparing system image" };
  }

  consoleBuffer = [];
  pendingImageDir = imageDir;

  decompressAndBoot(imageDir).catch((e) => {
    consoleBuffer.push(`[Fatal: ${(e as Error).message}]`);
    emulatorState = "stopped";
  });

  return { ok: true };
}

export function stopEmulator(): boolean {
  if (runningProcess && !runningProcess.killed) {
    runningProcess.kill();
    runningProcess = null;
    emulatorState = "stopped";
    return true;
  }
  return false;
}

export function getConsoleOutput(since = 0): { lines: string[]; total: number } {
  return {
    lines: consoleBuffer.slice(since),
    total: consoleBuffer.length,
  };
}

export function getState(): {
  state: typeof emulatorState;
  pid: number | null;
  prepareProgress: string;
  consoleLines: number;
} {
  return {
    state: emulatorState,
    pid: runningProcess?.pid || null,
    prepareProgress,
    consoleLines: consoleBuffer.length,
  };
}
