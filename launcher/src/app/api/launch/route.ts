import { NextResponse } from "next/server";
import { startBoot, stopEmulator } from "@/lib/emulator";

export async function POST(req: Request) {
  const { imageDir, action } = await req.json();

  if (action === "stop") {
    const stopped = stopEmulator();
    return NextResponse.json({ stopped });
  }

  if (!imageDir) {
    return NextResponse.json({ error: "No image directory specified" }, { status: 400 });
  }

  const result = startBoot(imageDir);
  if (!result.ok) {
    return NextResponse.json({ error: result.error }, { status: 500 });
  }
  return NextResponse.json({ ok: true, message: "Boot sequence started" });
}
