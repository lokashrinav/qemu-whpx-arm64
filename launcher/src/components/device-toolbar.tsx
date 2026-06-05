"use client";

import { useState } from "react";
import { Tooltip, TooltipContent, TooltipTrigger } from "@/components/ui/tooltip";

interface ToolButton {
  id: string;
  label: string;
  shortcut?: string;
  icon: React.ReactNode;
  toggleable?: boolean;
}

const BUTTONS: (ToolButton | "sep")[] = [
  {
    id: "power",
    label: "Power",
    shortcut: "Ctrl+P",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
        <path d="M18.36 6.64a9 9 0 1 1-12.73 0" />
        <line x1="12" y1="2" x2="12" y2="12" />
      </svg>
    ),
  },
  {
    id: "volup",
    label: "Volume up",
    shortcut: "Ctrl+=",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z" />
      </svg>
    ),
  },
  {
    id: "voldown",
    label: "Volume down",
    shortcut: "Ctrl+-",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M18.5 12c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM5 9v6h4l5 5V4L9 9H5z" />
      </svg>
    ),
  },
  "sep",
  {
    id: "rotate_left",
    label: "Rotate left",
    shortcut: "Ctrl+Left",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M7.11 8.53L5.7 7.11C4.8 8.27 4.24 9.61 4.07 11h2.02c.14-.87.49-1.72 1.02-2.47zM6.09 13H4.07c.17 1.39.72 2.73 1.62 3.89l1.41-1.42c-.52-.75-.87-1.59-1.01-2.47zm1.01 5.32c1.16.9 2.51 1.44 3.9 1.61V17.9c-.87-.15-1.71-.49-2.46-1.03L7.1 18.32zM13 4.07V1L8.45 5.55 13 10V6.09c2.84.48 5 2.94 5 5.91s-2.16 5.43-5 5.91v2.02c3.95-.49 7-3.85 7-7.93s-3.05-7.44-7-7.93z" />
      </svg>
    ),
  },
  {
    id: "rotate_right",
    label: "Rotate right",
    shortcut: "Ctrl+Right",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor" style={{ transform: "scaleX(-1)" }}>
        <path d="M7.11 8.53L5.7 7.11C4.8 8.27 4.24 9.61 4.07 11h2.02c.14-.87.49-1.72 1.02-2.47zM6.09 13H4.07c.17 1.39.72 2.73 1.62 3.89l1.41-1.42c-.52-.75-.87-1.59-1.01-2.47zm1.01 5.32c1.16.9 2.51 1.44 3.9 1.61V17.9c-.87-.15-1.71-.49-2.46-1.03L7.1 18.32zM13 4.07V1L8.45 5.55 13 10V6.09c2.84.48 5 2.94 5 5.91s-2.16 5.43-5 5.91v2.02c3.95-.49 7-3.85 7-7.93s-3.05-7.44-7-7.93z" />
      </svg>
    ),
  },
  "sep",
  {
    id: "back",
    label: "Back",
    shortcut: "Ctrl+Backspace",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z" />
      </svg>
    ),
  },
  {
    id: "home",
    label: "Home",
    shortcut: "Ctrl+H",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z" />
      </svg>
    ),
  },
  {
    id: "overview",
    label: "Overview",
    shortcut: "Ctrl+O",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M3 3h8v8H3V3zm0 10h8v8H3v-8zm10-10h8v8h-8V3zm0 10h8v8h-8v-8z" />
      </svg>
    ),
  },
  "sep",
  {
    id: "screenshot",
    label: "Take screenshot",
    shortcut: "Ctrl+S",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z" />
      </svg>
    ),
  },
  {
    id: "zoom",
    label: "Zoom",
    shortcut: "Ctrl+Z",
    toggleable: true,
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" />
        <path d="M12 10h-2v2H9v-2H7V9h2V7h1v2h2v1z" />
      </svg>
    ),
  },
  "sep",
  {
    id: "more",
    label: "Extended controls",
    icon: (
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M12 8c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm0 2c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2zm0 6c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2z" />
      </svg>
    ),
  },
];

interface DeviceToolbarProps {
  onMore?: () => void;
}

export function DeviceToolbar({ onMore }: DeviceToolbarProps) {
  const [activeToggle, setActiveToggle] = useState<string | null>(null);

  return (
    <div
      className="flex flex-col items-center w-[46px] py-[6px] rounded-[8px] flex-shrink-0 ml-[10px]"
      style={{ backgroundColor: "#2c3239", boxShadow: "0 4px 16px rgba(0,0,0,0.3)" }}
    >
      {BUTTONS.map((btn, i) => {
        if (btn === "sep") {
          return (
            <div key={`sep-${i}`} className="w-[22px] h-[1px] bg-white/[0.07] my-[3px]" />
          );
        }

        const isActive = btn.toggleable && activeToggle === btn.id;

        return (
          <Tooltip key={btn.id}>
            <TooltipTrigger
              className={`
                w-[34px] h-[34px] flex items-center justify-center rounded-[6px] cursor-pointer transition-colors
                ${isActive ? "bg-white/20 text-white" : "text-[#9aa0a6] hover:text-[#e8eaed] hover:bg-white/[0.08]"}
              `}
              onClick={() => {
                if (btn.toggleable) {
                  setActiveToggle(activeToggle === btn.id ? null : btn.id);
                }
                if (btn.id === "more") { onMore?.(); return; }
                if (["power","volup","voldown","home","back","overview"].includes(btn.id)) {
                  fetch("/api/command", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ command: btn.id }),
                  }).catch(() => {});
                }
              }}
            >
              {btn.icon}
            </TooltipTrigger>
            <TooltipContent side="left" sideOffset={8} className="text-xs flex items-center gap-2">
              <span>{btn.label}</span>
              {btn.shortcut && (
                <kbd className="text-[10px] opacity-60 font-mono">{btn.shortcut}</kbd>
              )}
            </TooltipContent>
          </Tooltip>
        );
      })}
    </div>
  );
}
