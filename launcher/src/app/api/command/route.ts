import { NextResponse } from "next/server";
import { sendAdbCommand } from "@/lib/emulator";

const KEY_MAP: Record<string, string> = {
  power: "shell input keyevent KEYCODE_POWER",
  volup: "shell input keyevent KEYCODE_VOLUME_UP",
  voldown: "shell input keyevent KEYCODE_VOLUME_DOWN",
  home: "shell input keyevent KEYCODE_HOME",
  back: "shell input keyevent KEYCODE_BACK",
  overview: "shell input keyevent KEYCODE_APP_SWITCH",
  screenshot: "exec-out screencap -p",
};

export async function POST(req: Request) {
  const { command } = await req.json();
  if (!command) return NextResponse.json({ error: "No command" }, { status: 400 });

  const adbCmd = KEY_MAP[command];
  if (!adbCmd) return NextResponse.json({ error: `Unknown command: ${command}` }, { status: 400 });

  const result = await sendAdbCommand(adbCmd);
  return NextResponse.json({ ok: true, output: result.slice(0, 500) });
}
