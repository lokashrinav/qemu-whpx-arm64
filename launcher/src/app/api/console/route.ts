import { NextResponse } from "next/server";
import { getConsoleOutput } from "@/lib/emulator";

export const dynamic = "force-dynamic";

export async function GET(req: Request) {
  const url = new URL(req.url);
  const since = parseInt(url.searchParams.get("since") || "0", 10);
  return NextResponse.json(getConsoleOutput(since));
}
