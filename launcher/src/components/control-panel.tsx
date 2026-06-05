"use client";

import { useState, useEffect } from "react";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { ScrollArea } from "@/components/ui/scroll-area";

interface AVD {
  name: string;
  target: string;
  abi: string;
  ram: string;
  cores: string;
  display: string;
  gpu: string;
}

interface Status {
  emulatorPath: string | null;
  avds: AVD[];
  runningPid: number | null;
}

export function ControlPanel() {
  const [status, setStatus] = useState<Status | null>(null);
  const [selected, setSelected] = useState("");
  const [launching, setLaunching] = useState(false);
  const [message, setMessage] = useState("");

  useEffect(() => {
    fetch("/api/status")
      .then((r) => r.json())
      .then(setStatus)
      .catch(() => setMessage("Failed to connect to backend"));
  }, []);

  const handleLaunch = async () => {
    if (!selected) return;
    setLaunching(true);
    setMessage("");
    try {
      const res = await fetch("/api/launch", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ avd: selected }),
      });
      const data = await res.json();
      if (data.error) {
        setMessage(data.error);
      } else {
        setMessage(`Launched (PID ${data.pid})`);
      }
    } catch (e) {
      setMessage("Launch failed");
    } finally {
      setLaunching(false);
    }
  };

  return (
    <ScrollArea className="h-full">
      <div className="p-3 space-y-3">
        {/* Emulator status */}
        <div className="flex items-center gap-2 text-[11px] text-muted-foreground">
          <div className={`w-1.5 h-1.5 rounded-full ${status?.emulatorPath ? "bg-green-500" : "bg-red-500"}`} />
          {status?.emulatorPath ? "Emulator found" : "No emulator"}
        </div>

        {/* AVD list */}
        <div className="space-y-1.5">
          <div className="text-[10px] font-semibold text-muted-foreground uppercase tracking-wider px-1">
            Devices
          </div>
          {!status && (
            <div className="text-[11px] text-muted-foreground px-1">Loading...</div>
          )}
          {status?.avds.map((avd) => (
            <button
              key={avd.name}
              onClick={() => setSelected(avd.name)}
              className={`w-full text-left p-2.5 rounded-lg border transition-all text-xs ${
                selected === avd.name
                  ? "border-blue-500/50 bg-blue-500/10 ring-1 ring-blue-500/20"
                  : "border-border/50 hover:border-muted-foreground/30 hover:bg-accent/30"
              }`}
            >
              <div className="flex items-center justify-between mb-1">
                <span className="font-semibold text-[12px]">{avd.name}</span>
                <Badge variant="secondary" className="text-[9px] font-mono h-4 px-1.5">
                  {avd.abi}
                </Badge>
              </div>
              <div className="flex gap-2 text-[10px] text-muted-foreground flex-wrap">
                <span>{avd.target}</span>
                <span>{avd.display}</span>
                <span>{avd.ram}MB</span>
                <span>{avd.cores} core{avd.cores !== "1" ? "s" : ""}</span>
              </div>
            </button>
          ))}
          {status && status.avds.length === 0 && (
            <div className="text-[11px] text-muted-foreground px-1">
              No AVDs found. Create one with avdmanager.
            </div>
          )}
        </div>

        {/* Launch button */}
        <Button
          className="w-full"
          size="sm"
          disabled={!selected || launching || !status?.emulatorPath}
          onClick={handleLaunch}
        >
          {launching ? "Launching..." : selected ? `Launch ${selected}` : "Select a device"}
        </Button>

        {/* Status message */}
        {message && (
          <div className={`text-[10px] px-2 py-1.5 rounded ${
            message.includes("PID") ? "bg-green-500/10 text-green-400" : "bg-red-500/10 text-red-400"
          }`}>
            {message}
          </div>
        )}

        {/* Running status */}
        {status?.runningPid && (
          <div className="flex items-center gap-2 text-[10px] text-green-400 px-1">
            <div className="w-1.5 h-1.5 rounded-full bg-green-500 animate-pulse" />
            Running (PID {status.runningPid})
          </div>
        )}

        {/* Info */}
        <div className="text-[9px] text-muted-foreground/50 px-1 pt-2 space-y-0.5">
          <div>Emulator: v35.6.3 (WHPX ARM64)</div>
          <div>Toolbar buttons send ADB commands</div>
        </div>
      </div>
    </ScrollArea>
  );
}
