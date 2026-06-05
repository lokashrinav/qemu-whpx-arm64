"use client";

import { useState } from "react";
import { PhoneFrame } from "@/components/phone-frame";
import { DeviceToolbar } from "@/components/device-toolbar";
import { AndroidHome } from "@/components/android-home";
import { ExtendedControls } from "@/components/extended-controls";
import { ControlPanel } from "@/components/control-panel";

export default function Home() {
  const [showExtended, setShowExtended] = useState(false);
  const [showPanel, setShowPanel] = useState(false);

  return (
    <div className="flex flex-col h-screen overflow-hidden bg-[#292a2d] select-none">
      {/* Title bar */}
      <div className="h-[30px] min-h-[30px] bg-[#202124] border-b border-black/40 flex items-center px-3 z-50">
        <div className="flex items-center gap-2">
          <button
            onClick={() => setShowPanel(!showPanel)}
            className="w-[14px] h-[14px] rounded-full bg-[#4285f4] flex items-center justify-center hover:ring-2 hover:ring-blue-400/30 transition-all"
          >
            <span className="text-[8px] font-bold text-white">Q</span>
          </button>
          <span className="text-[11px] text-white/70 font-medium">
            Android Emulator — arm64_atd:5554
          </span>
        </div>
        <div className="flex-1" />
        <div className="flex items-center gap-1">
          <button className="w-[28px] h-[20px] flex items-center justify-center text-white/40 hover:text-white/70 hover:bg-white/10 rounded-sm transition-colors">
            <svg width="10" height="10" viewBox="0 0 10 10"><line x1="1" y1="5" x2="9" y2="5" stroke="currentColor" strokeWidth="1.5"/></svg>
          </button>
          <button className="w-[28px] h-[20px] flex items-center justify-center text-white/40 hover:text-white/70 hover:bg-white/10 rounded-sm transition-colors">
            <svg width="10" height="10" viewBox="0 0 10 10"><rect x="1" y="1" width="8" height="8" fill="none" stroke="currentColor" strokeWidth="1.2"/></svg>
          </button>
          <button className="w-[28px] h-[20px] flex items-center justify-center text-white/40 hover:text-white hover:bg-red-500/80 rounded-sm transition-colors">
            <svg width="10" height="10" viewBox="0 0 10 10"><line x1="1" y1="1" x2="9" y2="9" stroke="currentColor" strokeWidth="1.5"/><line x1="9" y1="1" x2="1" y2="9" stroke="currentColor" strokeWidth="1.5"/></svg>
          </button>
        </div>
      </div>

      {/* Main area */}
      <div className="flex-1 flex overflow-hidden">
        {/* Device panel (slide-out) */}
        {showPanel && (
          <aside className="w-[260px] border-r border-white/[0.06] bg-[#1e1f22] flex-shrink-0">
            <ControlPanel />
          </aside>
        )}

        {/* Center */}
        <div className="flex-1 flex items-center justify-center py-2">
          <div className="flex items-start gap-0">
            <PhoneFrame>
              <AndroidHome />
            </PhoneFrame>
            <DeviceToolbar onMore={() => setShowExtended(!showExtended)} />
          </div>

          {showExtended && (
            <div className="ml-4 flex-shrink-0">
              <ExtendedControls onClose={() => setShowExtended(false)} />
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
