"use client";

import { useState, useEffect, useRef } from "react";
import { ScrollArea } from "@/components/ui/scroll-area";

export function ConsoleView() {
  const [lines, setLines] = useState<string[]>([]);
  const [lineCount, setLineCount] = useState(0);
  const [autoScroll, setAutoScroll] = useState(true);
  const bottomRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    let mounted = true;
    let cursor = 0;

    const poll = async () => {
      if (!mounted) return;
      try {
        const res = await fetch(`/api/console?since=${cursor}`);
        const data = await res.json();
        if (data.lines.length > 0) {
          setLines(prev => [...prev, ...data.lines]);
          cursor = data.total;
          setLineCount(data.total);
        }
      } catch {}
      if (mounted) setTimeout(poll, 500);
    };

    poll();
    return () => { mounted = false; };
  }, []);

  useEffect(() => {
    if (autoScroll && bottomRef.current) {
      bottomRef.current.scrollIntoView({ behavior: "smooth" });
    }
  }, [lines, autoScroll]);

  const highlightLine = (line: string) => {
    if (line.startsWith("[stderr]")) {
      if (line.includes("operational")) return "text-green-400";
      if (line.includes("WHPX:")) return "text-blue-400";
      return "text-yellow-500/70";
    }
    if (line.includes("Booting Linux")) return "text-green-400 font-semibold";
    if (line.includes("Freeing unused kernel memory")) return "text-green-400 font-semibold";
    if (line.includes("[Emulator exited")) return "text-red-400";
    if (line.includes("[Error:")) return "text-red-400";
    if (line.match(/^\[\s+\d+\.\d+\]/)) return "text-white/70";
    return "text-white/50";
  };

  return (
    <div className="flex flex-col h-full">
      <div className="flex items-center justify-between px-3 py-1.5 border-b border-white/[0.06]">
        <div className="flex items-center gap-2">
          <span className="text-[11px] font-semibold text-white/70">Serial Console</span>
          {lineCount > 0 && (
            <span className="text-[9px] text-muted-foreground font-mono">{lineCount} lines</span>
          )}
        </div>
        <div className="flex items-center gap-2">
          <button
            onClick={() => setAutoScroll(!autoScroll)}
            className={`text-[9px] px-1.5 py-0.5 rounded transition-colors ${
              autoScroll ? "bg-blue-500/20 text-blue-400" : "text-white/30 hover:text-white/50"
            }`}
          >
            Auto-scroll
          </button>
          <button
            onClick={() => { setLines([]); setLineCount(0); }}
            className="text-[9px] text-white/30 hover:text-white/50 px-1.5 py-0.5 rounded transition-colors"
          >
            Clear
          </button>
        </div>
      </div>

      <ScrollArea className="flex-1">
        <div className="p-2 font-mono text-[10px] leading-[16px] whitespace-pre-wrap break-all">
          {lines.length === 0 ? (
            <div className="text-white/20 text-center py-8">
              No output yet. Select an image and click Boot.
            </div>
          ) : (
            lines.map((line, i) => (
              <div key={i} className={highlightLine(line)}>
                {line}
              </div>
            ))
          )}
          <div ref={bottomRef} />
        </div>
      </ScrollArea>
    </div>
  );
}
