"use client";

import { useState, useEffect } from "react";

function StatusBar() {
  const [time, setTime] = useState("");
  const [mounted, setMounted] = useState(false);
  useEffect(() => {
    setMounted(true);
    const u = () => setTime(new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", hour12: false }));
    u();
    const id = setInterval(u, 30000);
    return () => clearInterval(id);
  }, []);

  return (
    <div className="flex items-center justify-between px-5 h-[22px]">
      <span className="text-[10px] font-[500] text-white drop-shadow-sm">{mounted ? time : " "}</span>
      <div className="flex items-center gap-[4px]">
        <svg width="9" height="9" viewBox="0 0 24 24" fill="white" className="drop-shadow-sm">
          <rect x="1" y="17" width="4" height="5" rx="0.5" opacity="0.35" />
          <rect x="7" y="13" width="4" height="9" rx="0.5" opacity="0.55" />
          <rect x="13" y="8" width="4" height="14" rx="0.5" opacity="0.8" />
          <rect x="19" y="3" width="4" height="19" rx="0.5" />
        </svg>
        <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="white" strokeWidth="2.5" strokeLinecap="round" className="drop-shadow-sm">
          <path d="M5 12.55a11 11 0 0 1 14.08 0" /><path d="M8.53 16.11a6 6 0 0 1 6.95 0" /><circle cx="12" cy="20" r="1" fill="white" stroke="none" />
        </svg>
        <svg width="16" height="9" viewBox="0 0 28 14" className="drop-shadow-sm">
          <rect x="0.5" y="0.5" width="22" height="12" rx="2.5" fill="none" stroke="white" strokeWidth="1" opacity="0.5" />
          <rect x="23.5" y="4" width="2.5" height="5" rx="1" fill="white" opacity="0.3" />
          <rect x="2" y="2" width="15" height="9" rx="1.5" fill="white" />
        </svg>
      </div>
    </div>
  );
}

function DateWidget() {
  const [t, setT] = useState("");
  useEffect(() => { setT(new Date().toLocaleDateString("en-US", { weekday: "long", month: "long", day: "numeric" })); }, []);
  return <>{t || " "}</>;
}

const APPS = [
  { n: "Phone", c: "#0b57d0", i: "phone" },
  { n: "Messages", c: "#0b57d0", i: "chat" },
  { n: "Chrome", c: "#fff", i: "chrome" },
  { n: "Camera", c: "#1a1a1a", i: "camera" },
  { n: "Photos", c: "#fff", i: "photos" },
  { n: "Maps", c: "#fff", i: "maps" },
  { n: "Gmail", c: "#fff", i: "mail" },
  { n: "Calendar", c: "#fff", i: "cal" },
  { n: "Drive", c: "#fff", i: "drive" },
  { n: "YouTube", c: "#fff", i: "yt" },
  { n: "Settings", c: "#e3e3e8", i: "settings" },
  { n: "Play Store", c: "#fff", i: "play" },
  { n: "Clock", c: "#1a1a2e", i: "clock" },
  { n: "Files", c: "#0b57d0", i: "folder" },
  { n: "Calc", c: "#1a1a2e", i: "calc" },
  { n: "Contacts", c: "#0b57d0", i: "contacts" },
];

const DOCK = [
  { n: "Phone", c: "#0b57d0", i: "phone" },
  { n: "Messages", c: "#0b57d0", i: "chat" },
  { n: "Chrome", c: "#fff", i: "chrome" },
  { n: "Camera", c: "#1a1a1a", i: "camera" },
];

function Icon({ i, s }: { i: string; s: number }) {
  const p: Record<string, JSX.Element> = {
    phone: <svg width={s} height={s} viewBox="0 0 24 24" fill="white"><path d="M6.62 10.79c1.44 2.83 3.76 5.14 6.59 6.59l2.2-2.2c.27-.27.67-.36 1.02-.24 1.12.37 2.33.57 3.57.57.55 0 1 .45 1 1V20c0 .55-.45 1-1 1-9.39 0-17-7.61-17-17 0-.55.45-1 1-1h3.5c.55 0 1 .45 1 1 0 1.25.2 2.45.57 3.57.11.35.03.74-.25 1.02l-2.2 2.2z"/></svg>,
    chat: <svg width={s} height={s} viewBox="0 0 24 24" fill="white"><path d="M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2z"/></svg>,
    chrome: <svg width={s} height={s} viewBox="0 0 48 48"><circle cx="24" cy="24" r="8" fill="#4285f4"/><path d="M24 4a20 20 0 0 0-17.32 10h13.66z" fill="#ea4335"/><path d="M6.68 14A20 20 0 0 0 14 39.32l6.93-12z" fill="#fbbc05"/><path d="M14 39.32A20 20 0 0 0 44 24H30.68z" fill="#34a853"/></svg>,
    camera: <svg width={s} height={s} viewBox="0 0 24 24" fill="white"><circle cx="12" cy="13" r="3.2"/><path d="M9 2L7.17 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2h-3.17L15 2H9zm3 15c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5z"/></svg>,
    photos: <svg width={s} height={s} viewBox="0 0 24 24"><path d="M21 5v6.59l-3-3.01-4 4.01-4-4-4 4-3-3.01V5c0-1.1.9-2 2-2h14c1.1 0 2 .9 2 2z" fill="#4285f4"/><path d="M18 11.58l3 3.01V19c0 1.1-.9 2-2 2H5c-1.1 0-2-.9-2-2v-6.42l3 3.01 4-4.01 4 4.01z" fill="#ea4335"/></svg>,
    maps: <svg width={s} height={s} viewBox="0 0 24 24"><path d="M15 3l-6 2.3L3 3v18l6 2.3 6-2.3 6 2.3V5z" fill="#34a853"/><path d="M15 3v18l6 2.3V5z" fill="#4285f4"/><path d="M9 5.3V23l6-2.3V3z" fill="#fbbc05"/><path d="M3 3v18l6 2.3V5.3z" fill="#ea4335"/></svg>,
    mail: <svg width={s} height={s} viewBox="0 0 24 24"><rect x="2" y="4" width="20" height="16" rx="2" fill="#ea4335"/><path d="M22 6l-10 7L2 6" fill="none" stroke="white" strokeWidth="1.5"/></svg>,
    cal: <svg width={s} height={s} viewBox="0 0 24 24"><rect x="3" y="4" width="18" height="18" rx="2" fill="#fff"/><rect x="3" y="4" width="18" height="5" fill="#4285f4" rx="2"/><text x="12" y="18.5" textAnchor="middle" fontSize="9" fontWeight="700" fill="#202124">14</text></svg>,
    drive: <svg width={s} height={s} viewBox="0 0 24 24"><path d="M8 2L2.5 11.5h5L13 2z" fill="#0066da"/><path d="M16 2L7.5 17h5L21 7.5z" fill="#00ac47"/><path d="M2.5 11.5L7.5 21h9l5-9.5z" fill="#ffba00"/></svg>,
    yt: <svg width={s} height={s} viewBox="0 0 24 24"><rect x="2" y="4.5" width="20" height="15" rx="3" fill="#ff0000"/><path d="M10 8.5v7l6-3.5z" fill="#fff"/></svg>,
    settings: <svg width={s} height={s} viewBox="0 0 24 24" fill="#5f6368"><path d="M19.14 12.94c.04-.3.06-.61.06-.94s-.02-.64-.07-.94l2.03-1.58a.49.49 0 0 0 .12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.07.62-.07.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61zM12 15.6A3.6 3.6 0 1 1 12 8.4a3.6 3.6 0 0 1 0 7.2z"/></svg>,
    play: <svg width={s} height={s} viewBox="0 0 24 24"><path d="M4 3l16 9-16 9z" fill="#34a853"/><path d="M4 3l10 6.2L4 14z" fill="#4285f4"/><path d="M4 14l10-4.8L20 12 4 21z" fill="#ea4335"/><path d="M14 9.2L20 12l-6 3.8z" fill="#fbbc05"/></svg>,
    clock: <svg width={s} height={s} viewBox="0 0 24 24" fill="none" stroke="white" strokeWidth="2"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg>,
    folder: <svg width={s} height={s} viewBox="0 0 24 24" fill="white"><path d="M10 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z"/></svg>,
    calc: <svg width={s} height={s} viewBox="0 0 24 24" fill="none" stroke="white" strokeWidth="1.5"><rect x="4" y="2" width="16" height="20" rx="2"/><line x1="8" y1="10" x2="16" y2="10"/><line x1="12" y1="6" x2="12" y2="10"/><line x1="8" y1="14" x2="16" y2="14"/></svg>,
    contacts: <svg width={s} height={s} viewBox="0 0 24 24" fill="white"><path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"/></svg>,
  };
  return p[i] || null;
}

export function AndroidHome() {
  return (
    <div className="relative flex flex-col h-full overflow-hidden">
      {/* Swirling Petals wallpaper — Obsidian dark variant */}
      <div className="absolute inset-0" style={{
        background: "linear-gradient(170deg, #0d1117 0%, #131820 20%, #1a1f2e 50%, #0f1419 80%, #0a0e13 100%)",
      }}>
        {/* Petal blobs */}
        <div className="absolute w-[200px] h-[200px] rounded-full top-[5%] left-[-20%] opacity-[0.12]"
          style={{ background: "radial-gradient(ellipse, #4a6fa5 0%, transparent 70%)", filter: "blur(30px)" }} />
        <div className="absolute w-[250px] h-[180px] rounded-full top-[25%] right-[-15%] opacity-[0.1]"
          style={{ background: "radial-gradient(ellipse, #6b5b8a 0%, transparent 70%)", filter: "blur(35px)" }} />
        <div className="absolute w-[180px] h-[220px] rounded-full bottom-[20%] left-[-10%] opacity-[0.08]"
          style={{ background: "radial-gradient(ellipse, #3d6b7e 0%, transparent 70%)", filter: "blur(25px)" }} />
        <div className="absolute w-[160px] h-[160px] rounded-full top-[55%] right-[10%] opacity-[0.06]"
          style={{ background: "radial-gradient(ellipse, #8b6f9e 0%, transparent 70%)", filter: "blur(28px)" }} />
        <div className="absolute w-[120px] h-[140px] rounded-full top-[10%] right-[20%] opacity-[0.09]"
          style={{ background: "radial-gradient(ellipse, #4a7fa5 0%, transparent 70%)", filter: "blur(22px)" }} />
      </div>

      {/* Content */}
      <div className="relative z-10 flex flex-col h-full">
        <StatusBar />

        {/* At a Glance */}
        <div className="px-5 pt-2 pb-1">
          <span className="text-[11px] text-white/75 font-[400]"><DateWidget /></span>
        </div>

        {/* Google search pill */}
        <div className="px-4 pb-3">
          <div className="flex items-center h-[36px] rounded-full px-3 gap-2 backdrop-blur-md"
            style={{ background: "rgba(255,255,255,0.08)", border: "1px solid rgba(255,255,255,0.06)" }}>
            <svg width="16" height="16" viewBox="0 0 24 24">
              <path d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92a5.06 5.06 0 0 1-2.2 3.32v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.1z" fill="#4285F4"/>
              <path d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z" fill="#34A853"/>
              <path d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93z" fill="#FBBC05"/>
              <path d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z" fill="#EA4335"/>
            </svg>
            <span className="text-white/30 text-[12px] flex-1">Search</span>
            <svg width="14" height="14" viewBox="0 0 24 24"><path d="M12 14c1.66 0 3-1.34 3-3V5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3z" fill="#4285F4"/><path d="M5 11c0 3.53 2.61 6.43 6 6.92V21h2v-3.08c3.39-.49 6-3.39 6-6.92h-2a5 5 0 0 1-10 0H5z" fill="#34A853"/></svg>
          </div>
        </div>

        {/* App grid */}
        <div className="flex-1 px-2">
          <div className="grid grid-cols-4 gap-y-[10px] justify-items-center">
            {APPS.map((a) => (
              <div key={a.n} className="flex flex-col items-center gap-[2px] w-[60px]">
                <div className="w-[40px] h-[40px] rounded-[12px] flex items-center justify-center shadow-sm"
                  style={{ backgroundColor: a.c }}>
                  <Icon i={a.i} s={20} />
                </div>
                <span className="text-[8px] text-white/55 truncate max-w-full">{a.n}</span>
              </div>
            ))}
          </div>
        </div>

        {/* Page indicator */}
        <div className="flex justify-center gap-[3px] py-1">
          <div className="w-[4px] h-[4px] rounded-full bg-white/50" />
          <div className="w-[4px] h-[4px] rounded-full bg-white/20" />
        </div>

        {/* Dock */}
        <div className="mx-3 mb-[4px] px-3 py-[4px] rounded-[20px] backdrop-blur-xl"
          style={{ background: "rgba(255,255,255,0.06)", border: "1px solid rgba(255,255,255,0.04)" }}>
          <div className="flex justify-around">
            {DOCK.map((a) => (
              <div key={`d-${a.n}`} className="w-[38px] h-[38px] rounded-[11px] flex items-center justify-center"
                style={{ backgroundColor: a.c }}>
                <Icon i={a.i} s={19} />
              </div>
            ))}
          </div>
        </div>

        {/* Gesture bar */}
        <div className="flex justify-center pb-[5px] pt-[2px]">
          <div className="w-[72px] h-[3px] rounded-full bg-white/30" />
        </div>
      </div>
    </div>
  );
}
