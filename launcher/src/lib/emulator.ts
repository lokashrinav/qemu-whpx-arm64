import { execFile, exec, ChildProcess } from "child_process";
import { existsSync, readFileSync, readdirSync } from "fs";
import { join } from "path";
import { promisify } from "util";

const execAsync = promisify(exec);

const SDK_EMULATOR_DIR = join(
  process.env.ANDROID_SDK_ROOT ||
    join(process.env.USERPROFILE || "C:\\Users\\lokas", "AppData", "Local", "Android", "Sdk"),
  "emulator"
);
const CUSTOM_EMULATOR_DIR = "C:\\tmp\\emu-win-arm64-objs";
const EMULATOR_DIR = existsSync(join(SDK_EMULATOR_DIR, "emulator.exe")) ? SDK_EMULATOR_DIR : CUSTOM_EMULATOR_DIR;
const EMULATOR_BIN = join(EMULATOR_DIR, "emulator.exe");
const AVD_HOME = join(
  process.env.ANDROID_AVD_HOME ||
    join(process.env.USERPROFILE || "C:\\Users\\lokas", ".android", "avd")
);
const ADB_PATHS = [
  join(
    process.env.ANDROID_SDK_ROOT ||
      join(process.env.USERPROFILE || "C:\\Users\\lokas", "AppData", "Local", "Android", "Sdk"),
    "platform-tools",
    "adb.exe"
  ),
  "adb",
];

let runningProcess: ChildProcess | null = null;

function findAdb(): string | null {
  for (const p of ADB_PATHS) {
    try {
      if (existsSync(p)) return p;
    } catch {}
  }
  return "adb";
}

export interface AVD {
  name: string;
  target: string;
  abi: string;
  ram: string;
  cores: string;
  display: string;
  gpu: string;
}

export function getEmulatorPath(): string | null {
  return existsSync(EMULATOR_BIN) ? EMULATOR_BIN : null;
}

export function discoverAVDs(): AVD[] {
  const avds: AVD[] = [];
  if (!existsSync(AVD_HOME)) return avds;

  for (const f of readdirSync(AVD_HOME)) {
    if (!f.endsWith(".ini") || f.endsWith(".avd")) continue;
    const name = f.replace(".ini", "");
    const avdDir = join(AVD_HOME, `${name}.avd`);
    const configPath = join(avdDir, "config.ini");
    const config: Record<string, string> = {};

    if (existsSync(configPath)) {
      for (const line of readFileSync(configPath, "utf-8").split("\n")) {
        const eq = line.indexOf("=");
        if (eq > 0) config[line.slice(0, eq).trim()] = line.slice(eq + 1).trim();
      }
    }

    avds.push({
      name,
      target: config["tag.display"] || "Android",
      abi: config["abi.type"] || "arm64-v8a",
      ram: config["hw.ramSize"] || "2048",
      cores: config["hw.cpu.ncore"] || "2",
      display: `${config["hw.lcd.width"] || "1080"}x${config["hw.lcd.height"] || "1920"}`,
      gpu: config["hw.gpu.mode"] || "auto",
    });
  }
  return avds;
}

export function launchEmulator(avdName: string): { pid: number; cmd: string } | { error: string } {
  if (!existsSync(EMULATOR_BIN)) {
    return { error: "Emulator binary not found at " + EMULATOR_BIN };
  }

  if (runningProcess && !runningProcess.killed) {
    return { error: "Emulator already running (PID " + runningProcess.pid + ")" };
  }

  const args = ["-avd", avdName, "-gpu", "swiftshader_indirect"];
  const proc = execFile(EMULATOR_BIN, args, {
    cwd: EMULATOR_DIR,
    windowsHide: false,
  });

  runningProcess = proc;
  proc.on("exit", () => { runningProcess = null; });

  return { pid: proc.pid || 0, cmd: `emulator ${args.join(" ")}` };
}

export async function sendAdbCommand(command: string): Promise<string> {
  const adb = findAdb();
  if (!adb) return "ADB not found";

  try {
    const { stdout, stderr } = await execAsync(`"${adb}" ${command}`, { timeout: 5000 });
    return stdout || stderr || "ok";
  } catch (e: unknown) {
    return (e as Error).message || "command failed";
  }
}

export function getRunningPid(): number | null {
  if (runningProcess && !runningProcess.killed) return runningProcess.pid || null;
  return null;
}
