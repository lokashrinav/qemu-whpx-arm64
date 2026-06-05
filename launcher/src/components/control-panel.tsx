"use client";

import { useState, useEffect, useCallback } from "react";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { ScrollArea } from "@/components/ui/scroll-area";

interface SystemImage {
  name: string;
  apiLevel: string;
  abi: string;
  dir: string;
  hasKernel: boolean;
  hasRamdisk: boolean;
  hasSystem: boolean;
  systemReady: boolean;
}

interface Status {
  qemuPath: string | null;
  images: SystemImage[];
  state: "idle" | "preparing" | "booting" | "running" | "stopped";
  pid: number | null;
  prepareProgress: string;
  consoleLines: number;
}

export function ControlPanel() {
  const [status, setStatus] = useState<Status | null>(null);
  const [selected, setSelected] = useState("");
  const [launching, setLaunching] = useState(false);
  const [message, setMessage] = useState("");

  const fetchStatus = useCallback(() => {
    fetch("/api/status")
      .then((r) => r.json())
      .then((data: Status) => {
        setStatus(data);
        if (!selected && data.images.length > 0) {
          setSelected(data.images[0].dir);
        }
      })
      .catch(() => setMessage("Failed to connect to backend"));
  }, [selected]);

  useEffect(() => {
    fetchStatus();
    const id = setInterval(fetchStatus, 3000);
    return () => clearInterval(id);
  }, [fetchStatus]);

  const handleLaunch = async () => {
    if (!selected) return;
    setLaunching(true);
    setMessage("");
    try {
      const res = await fetch("/api/launch", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ imageDir: selected }),
      });
      const data = await res.json();
      if (data.error) {
        setMessage(data.error);
      } else {
        setMessage("Boot sequence started");
      }
    } catch {
      setMessage("Launch failed");
    } finally {
      setLaunching(false);
      fetchStatus();
    }
  };

  const handleStop = async () => {
    try {
      await fetch("/api/launch", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "stop" }),
      });
      setMessage("Emulator stopped");
      fetchStatus();
    } catch {
      setMessage("Stop failed");
    }
  };

  const isActive = status?.state === "booting" || status?.state === "running" || status?.state === "preparing";
  const selectedImage = status?.images.find(i => i.dir === selected);

  return (
    <ScrollArea className="h-full">
      <div className="p-3 space-y-3">
        {/* QEMU binary status */}
        <div className="flex items-center gap-2 text-[11px] text-muted-foreground">
          <div className={`w-1.5 h-1.5 rounded-full ${status?.qemuPath ? "bg-green-500" : "bg-red-500"}`} />
          {status?.qemuPath ? "QEMU ARM64 found" : "QEMU binary missing"}
        </div>

        {/* System images */}
        <div className="space-y-1.5">
          <div className="text-[10px] font-semibold text-muted-foreground uppercase tracking-wider px-1">
            System Images
          </div>
          {!status && (
            <div className="text-[11px] text-muted-foreground px-1">Loading...</div>
          )}
          {status?.images.map((img) => (
            <button
              key={img.dir}
              onClick={() => !isActive && setSelected(img.dir)}
              disabled={isActive}
              className={`w-full text-left p-2.5 rounded-lg border transition-all text-xs ${
                selected === img.dir
                  ? "border-blue-500/50 bg-blue-500/10 ring-1 ring-blue-500/20"
                  : "border-border/50 hover:border-muted-foreground/30 hover:bg-accent/30"
              } ${isActive ? "opacity-60 cursor-not-allowed" : ""}`}
            >
              <div className="flex items-center justify-between mb-1">
                <span className="font-semibold text-[12px]">{img.name}</span>
                <Badge variant="secondary" className="text-[9px] font-mono h-4 px-1.5">
                  {img.abi}
                </Badge>
              </div>
              <div className="flex gap-2 text-[10px] text-muted-foreground flex-wrap">
                <span>API {img.apiLevel}</span>
                {img.systemReady ? (
                  <span className="text-green-400">system.img ready</span>
                ) : img.hasSystem ? (
                  <span className="text-yellow-400">needs decompress</span>
                ) : (
                  <span className="text-red-400">no system image</span>
                )}
              </div>
            </button>
          ))}
          {status && status.images.length === 0 && (
            <div className="text-[11px] text-muted-foreground px-1">
              No ARM64 system images found in prebuilts.
            </div>
          )}
        </div>

        {/* Launch / Stop button */}
        {isActive ? (
          <Button
            className="w-full bg-red-600 hover:bg-red-700"
            size="sm"
            onClick={handleStop}
          >
            Stop Emulator
          </Button>
        ) : (
          <Button
            className="w-full"
            size="sm"
            disabled={!selected || launching || !status?.qemuPath || !selectedImage?.hasSystem}
            onClick={handleLaunch}
          >
            {launching ? "Starting..." :
              selectedImage && !selectedImage.systemReady ? `Decompress & Boot` :
              selected ? `Boot` : "Select an image"}
          </Button>
        )}

        {/* Status message */}
        {message && (
          <div className={`text-[10px] px-2 py-1.5 rounded ${
            message.includes("started") || message.includes("stopped") ? "bg-green-500/10 text-green-400" : "bg-red-500/10 text-red-400"
          }`}>
            {message}
          </div>
        )}

        {/* Emulator state */}
        {status && status.state !== "idle" && (
          <div className="flex items-center gap-2 text-[10px] px-1">
            <div className={`w-1.5 h-1.5 rounded-full ${
              status.state === "running" ? "bg-green-500 animate-pulse" :
              status.state === "booting" ? "bg-yellow-500 animate-pulse" :
              status.state === "preparing" ? "bg-blue-500 animate-pulse" :
              "bg-gray-500"
            }`} />
            <span className={
              status.state === "running" ? "text-green-400" :
              status.state === "booting" ? "text-yellow-400" :
              status.state === "preparing" ? "text-blue-400" :
              "text-muted-foreground"
            }>
              {status.state === "running" && `Running (PID ${status.pid})`}
              {status.state === "booting" && `Booting (PID ${status.pid})`}
              {status.state === "preparing" && status.prepareProgress}
              {status.state === "stopped" && "Stopped"}
            </span>
          </div>
        )}

        {/* Boot config info */}
        <div className="text-[9px] text-muted-foreground/50 px-1 pt-2 space-y-0.5">
          <div>Accelerator: WHPX (ARM64)</div>
          <div>CPU: Cortex-A57 | RAM: 2048 MB</div>
          <div>Console: serial stdio</div>
        </div>
      </div>
    </ScrollArea>
  );
}
