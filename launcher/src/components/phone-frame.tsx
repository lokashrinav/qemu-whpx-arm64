"use client";

interface PhoneFrameProps {
  children?: React.ReactNode;
}

export function PhoneFrame({ children }: PhoneFrameProps) {
  return (
    <div className="relative flex-shrink-0">
      {/* Shadow layer */}
      <div
        className="absolute inset-0 rounded-[36px]"
        style={{
          boxShadow: "0 25px 60px rgba(0,0,0,0.55), 0 8px 20px rgba(0,0,0,0.35)",
        }}
      />

      {/* Metallic rim — the chamfered aluminum edge */}
      <div
        className="relative rounded-[36px] p-[2px]"
        style={{
          background: "linear-gradient(160deg, #5a5a5e 0%, #3a3a3e 20%, #48484c 40%, #2a2a2e 60%, #3e3e42 80%, #4a4a4e 100%)",
        }}
      >
        {/* Inner phone body — matte black */}
        <div className="rounded-[34px] bg-[#161618] p-[7px]">

          {/* Power button */}
          <div
            className="absolute -right-[1px] top-[85px] w-[3px] h-[40px] rounded-r-[1.5px]"
            style={{ background: "linear-gradient(90deg, #3a3a3c, #58585c, #3a3a3c)" }}
          />
          {/* Volume up */}
          <div
            className="absolute -left-[1px] top-[70px] w-[3px] h-[28px] rounded-l-[1.5px]"
            style={{ background: "linear-gradient(270deg, #3a3a3c, #58585c, #3a3a3c)" }}
          />
          {/* Volume down */}
          <div
            className="absolute -left-[1px] top-[106px] w-[3px] h-[28px] rounded-l-[1.5px]"
            style={{ background: "linear-gradient(270deg, #3a3a3c, #58585c, #3a3a3c)" }}
          />

          {/* Screen glass */}
          <div className="relative rounded-[27px] overflow-hidden bg-black">
            {/* Camera sensor */}
            <div className="absolute top-[7px] left-1/2 -translate-x-1/2 z-30">
              <div className="w-[7px] h-[7px] rounded-full bg-[#111] ring-[1px] ring-[#222]">
                <div className="absolute inset-[2px] rounded-full bg-[#1a1a2e] opacity-60" />
              </div>
            </div>

            {/* Screen content */}
            <div className="w-[280px] h-[590px] relative">
              {children}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
