import { NextResponse } from "next/server";
import { getQemuPath, discoverSystemImages, getState } from "@/lib/emulator";

export const dynamic = "force-dynamic";

export async function GET() {
  return NextResponse.json({
    qemuPath: getQemuPath(),
    images: discoverSystemImages(),
    ...getState(),
  });
}
