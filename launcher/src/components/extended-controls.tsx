"use client";

import { useState } from "react";
import { Slider } from "@/components/ui/slider";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Label } from "@/components/ui/label";
import { Input } from "@/components/ui/input";
import { Switch } from "@/components/ui/switch";
import { Separator } from "@/components/ui/separator";
import { ScrollArea } from "@/components/ui/scroll-area";

const TABS = [
  { id: "location", label: "Location", icon: "M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z" },
  { id: "cellular", label: "Cellular", icon: "M2 20h.01M7 20v-4M12 20v-8M17 20V8M22 20V4" },
  { id: "battery", label: "Battery", icon: "M17 6H7a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2zM23 11v2" },
  { id: "phone", label: "Phone", icon: "M22 16.92v3a2 2 0 0 1-2.18 2 19.79 19.79 0 0 1-8.63-3.07 19.5 19.5 0 0 1-6-6 19.79 19.79 0 0 1-3.07-8.67A2 2 0 0 1 4.11 2h3a2 2 0 0 1 2 1.72c.12.96.36 1.9.7 2.81a2 2 0 0 1-.45 2.11L8.09 9.91a16 16 0 0 0 6 6l1.27-1.27a2 2 0 0 1 2.11-.45c.91.34 1.85.58 2.81.7A2 2 0 0 1 22 16.92z" },
  { id: "sensors", label: "Sensors", icon: "M12 2v20M2 12h20" },
  { id: "fingerprint", label: "Fingerprint", icon: "M2 12C2 6.5 6.5 2 12 2a10 10 0 0 1 8 4M5 19.5C5.5 18 6 15 6 12c0-.7.12-1.37.34-2" },
  { id: "settings", label: "Settings", icon: "" },
  { id: "help", label: "Help", icon: "" },
];

export function ExtendedControls({ onClose }: { onClose: () => void }) {
  const [activeTab, setActiveTab] = useState("location");

  return (
    <div className="flex h-[580px] w-[720px] bg-[#394249] rounded-lg overflow-hidden shadow-2xl border border-white/10">
      {/* Sidebar tabs */}
      <div className="w-[180px] flex flex-col border-r border-white/10">
        <div className="flex items-center justify-between px-3 py-2 border-b border-white/10">
          <span className="text-[11px] font-semibold text-white/80 uppercase tracking-wider">
            Extended Controls
          </span>
          <button onClick={onClose} className="text-white/50 hover:text-white transition-colors">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round">
              <line x1="18" y1="6" x2="6" y2="18" />
              <line x1="6" y1="6" x2="18" y2="18" />
            </svg>
          </button>
        </div>
        <ScrollArea className="flex-1">
          <div className="py-1">
            {TABS.map((tab) => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                className={`w-full text-left px-4 py-2 text-[13px] transition-colors ${
                  activeTab === tab.id
                    ? "bg-white/15 text-white font-medium"
                    : "text-white/60 hover:text-white/90 hover:bg-white/5"
                }`}
              >
                {tab.label}
              </button>
            ))}
          </div>
        </ScrollArea>
      </div>

      {/* Content */}
      <div className="flex-1 bg-[#2d3238]">
        <ScrollArea className="h-full">
          <div className="p-5">
            {activeTab === "location" && <LocationPanel />}
            {activeTab === "cellular" && <CellularPanel />}
            {activeTab === "battery" && <BatteryPanel />}
            {activeTab === "phone" && <PhonePanel />}
            {activeTab === "sensors" && <SensorsPanel />}
            {activeTab === "fingerprint" && <FingerprintPanel />}
            {activeTab === "settings" && <SettingsPanel />}
            {activeTab === "help" && <HelpPanel />}
          </div>
        </ScrollArea>
      </div>
    </div>
  );
}

function SectionTitle({ children }: { children: React.ReactNode }) {
  return <h3 className="text-[13px] font-semibold text-white/90 mb-3">{children}</h3>;
}

function FieldRow({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <div className="flex items-center justify-between gap-4 py-1.5">
      <Label className="text-[12px] text-white/60 shrink-0">{label}</Label>
      <div className="flex-1 max-w-[280px]">{children}</div>
    </div>
  );
}

function LocationPanel() {
  return (
    <div className="space-y-4">
      <SectionTitle>Location point</SectionTitle>
      <div className="grid grid-cols-2 gap-3">
        <div>
          <Label className="text-[11px] text-white/50 mb-1 block">Longitude</Label>
          <Input defaultValue="-122.0840" className="bg-[#394249] border-white/10 text-white text-xs h-8" />
        </div>
        <div>
          <Label className="text-[11px] text-white/50 mb-1 block">Latitude</Label>
          <Input defaultValue="37.4220" className="bg-[#394249] border-white/10 text-white text-xs h-8" />
        </div>
        <div>
          <Label className="text-[11px] text-white/50 mb-1 block">Altitude</Label>
          <Input defaultValue="0" className="bg-[#394249] border-white/10 text-white text-xs h-8" />
        </div>
        <div>
          <Label className="text-[11px] text-white/50 mb-1 block">Speed</Label>
          <Input defaultValue="0" className="bg-[#394249] border-white/10 text-white text-xs h-8" />
        </div>
      </div>
      <button className="px-4 py-1.5 bg-[#4285f4] hover:bg-[#5a95f5] text-white text-xs rounded transition-colors">
        Send
      </button>
    </div>
  );
}

function CellularPanel() {
  return (
    <div className="space-y-4">
      <SectionTitle>Network type</SectionTitle>
      <FieldRow label="Network">
        <Select defaultValue="lte">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="lte">LTE</SelectItem>
            <SelectItem value="5g">5G</SelectItem>
            <SelectItem value="3g">HSDPA</SelectItem>
            <SelectItem value="edge">EDGE</SelectItem>
            <SelectItem value="gsm">GSM</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
      <FieldRow label="Signal strength">
        <Select defaultValue="great">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="great">Great</SelectItem>
            <SelectItem value="good">Good</SelectItem>
            <SelectItem value="moderate">Moderate</SelectItem>
            <SelectItem value="poor">Poor</SelectItem>
            <SelectItem value="none">None</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
    </div>
  );
}

function BatteryPanel() {
  const [charge, setCharge] = useState(85);

  return (
    <div className="space-y-4">
      <SectionTitle>Battery</SectionTitle>
      <FieldRow label="Charge level">
        <div className="flex items-center gap-3">
          <Slider value={[charge]} onValueChange={(v) => setCharge(Array.isArray(v) ? v[0] : v)} min={0} max={100} step={1} className="flex-1" />
          <span className="text-xs text-white/70 font-mono w-8 text-right">{charge}%</span>
        </div>
      </FieldRow>
      <FieldRow label="Charger">
        <Select defaultValue="ac">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="ac">AC charger</SelectItem>
            <SelectItem value="usb">USB</SelectItem>
            <SelectItem value="wireless">Wireless</SelectItem>
            <SelectItem value="none">None</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
      <FieldRow label="Battery health">
        <Select defaultValue="good">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="good">Good</SelectItem>
            <SelectItem value="overheat">Overheat</SelectItem>
            <SelectItem value="dead">Dead</SelectItem>
            <SelectItem value="unknown">Unknown</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
      <FieldRow label="Battery status">
        <Select defaultValue="charging">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="charging">Charging</SelectItem>
            <SelectItem value="discharging">Discharging</SelectItem>
            <SelectItem value="full">Full</SelectItem>
            <SelectItem value="not_charging">Not charging</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
    </div>
  );
}

function PhonePanel() {
  return (
    <div className="space-y-4">
      <SectionTitle>Telephony</SectionTitle>
      <div>
        <Label className="text-[11px] text-white/50 mb-1 block">From number</Label>
        <Input defaultValue="+1 (650) 555-1234" className="bg-[#394249] border-white/10 text-white text-xs h-8" />
      </div>
      <div className="flex gap-2">
        <button className="flex-1 px-4 py-1.5 bg-[#34a853] hover:bg-[#3dbd5d] text-white text-xs rounded transition-colors">
          Call device
        </button>
        <button className="flex-1 px-4 py-1.5 bg-[#ea4335] hover:bg-[#f05545] text-white text-xs rounded transition-colors">
          End call
        </button>
      </div>
      <Separator className="bg-white/10" />
      <SectionTitle>SMS</SectionTitle>
      <div>
        <Label className="text-[11px] text-white/50 mb-1 block">Message</Label>
        <textarea
          defaultValue="Hello from the emulator!"
          className="w-full bg-[#394249] border border-white/10 text-white text-xs rounded px-3 py-2 h-20 resize-none outline-none focus:border-[#4285f4]"
        />
      </div>
      <button className="px-4 py-1.5 bg-[#4285f4] hover:bg-[#5a95f5] text-white text-xs rounded transition-colors">
        Send message
      </button>
    </div>
  );
}

function SensorsPanel() {
  return (
    <div className="space-y-4">
      <SectionTitle>Accelerometer</SectionTitle>
      <div className="grid grid-cols-3 gap-3">
        <div>
          <Label className="text-[11px] text-white/50 mb-1 block">X</Label>
          <Input defaultValue="0.0" className="bg-[#394249] border-white/10 text-white text-xs h-8 font-mono" />
        </div>
        <div>
          <Label className="text-[11px] text-white/50 mb-1 block">Y</Label>
          <Input defaultValue="9.8" className="bg-[#394249] border-white/10 text-white text-xs h-8 font-mono" />
        </div>
        <div>
          <Label className="text-[11px] text-white/50 mb-1 block">Z</Label>
          <Input defaultValue="0.0" className="bg-[#394249] border-white/10 text-white text-xs h-8 font-mono" />
        </div>
      </div>
      <Separator className="bg-white/10" />
      <SectionTitle>Temperature</SectionTitle>
      <FieldRow label="Ambient temp">
        <Input defaultValue="25.0" className="bg-[#394249] border-white/10 text-white text-xs h-8 font-mono" />
      </FieldRow>
      <FieldRow label="Proximity">
        <Input defaultValue="1.0" className="bg-[#394249] border-white/10 text-white text-xs h-8 font-mono" />
      </FieldRow>
      <FieldRow label="Light">
        <Input defaultValue="40.0" className="bg-[#394249] border-white/10 text-white text-xs h-8 font-mono" />
      </FieldRow>
    </div>
  );
}

function FingerprintPanel() {
  return (
    <div className="space-y-4">
      <SectionTitle>Fingerprint</SectionTitle>
      <FieldRow label="Finger">
        <Select defaultValue="1">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="1">Finger 1</SelectItem>
            <SelectItem value="2">Finger 2</SelectItem>
            <SelectItem value="3">Finger 3</SelectItem>
            <SelectItem value="4">Finger 4</SelectItem>
            <SelectItem value="5">Finger 5</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
      <button className="px-4 py-1.5 bg-[#4285f4] hover:bg-[#5a95f5] text-white text-xs rounded transition-colors">
        Touch sensor
      </button>
    </div>
  );
}

function SettingsPanel() {
  return (
    <div className="space-y-4">
      <SectionTitle>Emulator Settings</SectionTitle>
      <FieldRow label="Accelerator">
        <Select defaultValue="whpx">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="whpx">WHPX (Hardware)</SelectItem>
            <SelectItem value="tcg">TCG (Software)</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
      <FieldRow label="GPU mode">
        <Select defaultValue="auto">
          <SelectTrigger className="bg-[#394249] border-white/10 text-white text-xs h-8">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="auto">Auto</SelectItem>
            <SelectItem value="host">Host GPU</SelectItem>
            <SelectItem value="swiftshader">SwiftShader</SelectItem>
            <SelectItem value="off">Off</SelectItem>
          </SelectContent>
        </Select>
      </FieldRow>
      <Separator className="bg-white/10" />
      <FieldRow label="Verbose logging">
        <Switch />
      </FieldRow>
      <FieldRow label="Show perf stats">
        <Switch />
      </FieldRow>
    </div>
  );
}

function HelpPanel() {
  return (
    <div className="space-y-4">
      <SectionTitle>About</SectionTitle>
      <div className="space-y-2 text-xs text-white/60">
        <p><span className="text-white/80">QEMU WHPX ARM64</span> — Android Emulator</p>
        <p>Hardware-accelerated ARM64 emulation on Windows via WHPX</p>
        <Separator className="bg-white/10" />
        <p className="font-mono text-[11px]">Hypervisor: Windows Hypervisor Platform</p>
        <p className="font-mono text-[11px]">Target: ARM64 (aarch64)</p>
        <p className="font-mono text-[11px]">Board: Ranchu (Android Virtual Device)</p>
        <p className="font-mono text-[11px]">Interrupt Controller: GICv3</p>
      </div>
      <Separator className="bg-white/10" />
      <SectionTitle>Keyboard Shortcuts</SectionTitle>
      <div className="space-y-1 text-[11px] font-mono text-white/50">
        <div className="flex justify-between"><span>Power</span><span>Ctrl+P</span></div>
        <div className="flex justify-between"><span>Home</span><span>Ctrl+H</span></div>
        <div className="flex justify-between"><span>Back</span><span>Ctrl+Backspace</span></div>
        <div className="flex justify-between"><span>Overview</span><span>Ctrl+O</span></div>
        <div className="flex justify-between"><span>Volume up</span><span>Ctrl+=</span></div>
        <div className="flex justify-between"><span>Volume down</span><span>Ctrl+-</span></div>
        <div className="flex justify-between"><span>Rotate left</span><span>Ctrl+Left</span></div>
        <div className="flex justify-between"><span>Rotate right</span><span>Ctrl+Right</span></div>
        <div className="flex justify-between"><span>Screenshot</span><span>Ctrl+S</span></div>
        <div className="flex justify-between"><span>Zoom</span><span>Ctrl+Z</span></div>
      </div>
    </div>
  );
}
