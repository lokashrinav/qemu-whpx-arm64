import { NextResponse } from "next/server";
import { launchEmulator } from "@/lib/emulator";

export async function POST(req: Request) {
  const { avd } = await req.json();
  if (!avd) return NextResponse.json({ error: "No AVD specified" }, { status: 400 });

  const result = launchEmulator(avd);
  if ("error" in result) return NextResponse.json(result, { status: 500 });
  return NextResponse.json(result);
}
