import { NextResponse } from "next/server";
import { getEmulatorPath, discoverAVDs, getRunningPid } from "@/lib/emulator";

export const dynamic = "force-dynamic";

export async function GET() {
  return NextResponse.json({
    emulatorPath: getEmulatorPath(),
    avds: discoverAVDs(),
    runningPid: getRunningPid(),
  });
}
