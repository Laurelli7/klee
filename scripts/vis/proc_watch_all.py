#!/usr/bin/env python3
"""
proc_watch_all.py — log all running processes at an interval.

- One CSV row per process per sample.
- Columns include: timestamp, pid, ppid, uid, user, state, threads, VmRSS_kb, VmHWM_kb, VmSize_kb,
  RssAnon_kb, RssFile_kb, RssShmem_kb, VmData_kb, VmSwap_kb, heap_Rss_kb, rollup_Rss_kb, rollup_Anonymous_kb,
  exe, cmdline.
- Defaults: interval 60s, output "proc_watch.csv".
- Optional: --filter REGEX (match cmdline or exe), --pids "123,456" (only those PIDs), --light (skip smaps/heap),
  --owner self (only current user's processes).
"""
import argparse, datetime, os, re, time
from pathlib import Path
from typing import Dict, Optional, Tuple

def list_pids() -> list:
    out = []
    for d in os.listdir("/proc"):
        if d.isdigit():
            out.append(int(d))
    return out

def read_status(pid: int) -> Dict[str,str]:
    d = {}
    try:
        with open(f"/proc/{pid}/status","r") as f:
            for line in f:
                if ":" in line:
                    k,v = line.split(":",1)
                    d[k.strip()] = v.strip()
    except (FileNotFoundError, PermissionError):
        return {}
    return d

def parse_kb(d: Dict[str,str], key: str) -> Optional[int]:
    v = d.get(key, "")
    if not v: return None
    toks = v.split()
    for t in toks:
        if t.isdigit():
            return int(t)
    return None

def read_cmdline(pid: int) -> str:
    try:
        with open(f"/proc/{pid}/cmdline","rb") as f:
            data = f.read()
        if not data:
            return ""
        return data.replace(b"\x00", b" ").decode("utf-8", "replace").strip()
    except (FileNotFoundError, PermissionError):
        return ""

def read_exe(pid:int) -> str:
    try:
        return os.readlink(f"/proc/{pid}/exe")
    except (FileNotFoundError, PermissionError, OSError):
        return ""

def read_rollup(pid:int):
    rss = None
    anon = None
    try:
        with open(f"/proc/{pid}/smaps_rollup","r") as f:
            for line in f:
                if line.startswith("Rss:"):
                    rss = int(line.split()[1])
                elif line.startswith("Anonymous:"):
                    anon = int(line.split()[1])
    except (FileNotFoundError, PermissionError):
        pass
    return rss, anon

def heap_rss_from_smaps(pid:int):
    rss = 0
    in_heap = False
    try:
        with open(f"/proc/{pid}/smaps","r") as f:
            for line in f:
                if re.match(r"^[0-9A-Fa-f]+-[0-9A-Fa-f]+ ", line):
                    in_heap = line.rstrip().endswith("[heap]")
                    continue
                if in_heap and line.startswith("Rss:"):
                    rss += int(line.split()[1])
        return rss
    except (FileNotFoundError, PermissionError):
        return None

def parse_uid(d: Dict[str,str]) -> Optional[int]:
    v = d.get("Uid","")
    if not v: return None
    try:
        return int(v.split()[0])
    except Exception:
        return None

def username_from_uid(uid: Optional[int]) -> str:
    if uid is None: return ""
    try:
        import pwd
        return pwd.getpwuid(uid).pw_name
    except Exception:
        return str(uid)

def main():
    ap = argparse.ArgumentParser(description="Watch all processes with memory stats and command lines; write CSV.")
    ap.add_argument("--interval", type=float, default=60.0, help="Sample interval seconds (default 60)")
    ap.add_argument("--out", type=str, default="proc_watch.csv", help="CSV output path")
    ap.add_argument("--filter", type=str, default="", help="Regex to include only processes whose cmdline or exe matches")
    ap.add_argument("--pids", type=str, default="", help="Comma-separated PIDs to include (overrides filter if set)")
    ap.add_argument("--light", action="store_true", help="Skip smaps/heap and rollup to reduce overhead")
    ap.add_argument("--owner", choices=["all","self"], default="all", help="Limit to current user only")
    ap.add_argument("--once", action="store_true", help="Take one sample and exit")
    args = ap.parse_args()

    pid_set = set()
    if args.pids:
        for tok in re.split(r"[,\s]+", args.pids.strip()):
            if tok.isdigit():
                pid_set.add(int(tok))

    filt = re.compile(args.filter) if args.filter else None
    my_uid = os.getuid()
    header = ["timestamp_iso","pid","ppid","uid","user","state","threads",
              "VmRSS_kb","VmHWM_kb","VmSize_kb","RssAnon_kb","RssFile_kb","RssShmem_kb","VmData_kb","VmSwap_kb",
              "heap_Rss_kb","rollup_Rss_kb","rollup_Anonymous_kb","exe","cmdline"]
    mode = "w"
    with open(args.out, mode) as out:
        out.write(",".join(header) + "\n")
        start = time.time()
        while True:
            ts = datetime.datetime.now().isoformat(timespec="seconds")
            pids = list_pids()
            for pid in sorted(pids):
                if pid_set and pid not in pid_set:
                    continue
                st = read_status(pid)
                if not st:
                    continue
                uid = parse_uid(st)
                if args.owner == "self" and uid is not None and uid != my_uid:
                    continue

                exe = read_exe(pid)
                cmd = read_cmdline(pid)
                if filt and not (filt.search(cmd) or filt.search(exe)):
                    continue

                ppid = st.get("PPid","").split()[0] if st.get("PPid") else ""
                state = st.get("State","").split()[0]
                threads = st.get("Threads","0")

                VmRSS    = parse_kb(st, "VmRSS") or 0
                VmHWM    = parse_kb(st, "VmHWM") or 0
                VmSize   = parse_kb(st, "VmSize") or 0
                RssAnon  = parse_kb(st, "RssAnon") or 0
                RssFile  = parse_kb(st, "RssFile") or 0
                RssShmem = parse_kb(st, "RssShmem") or 0
                VmData   = parse_kb(st, "VmData") or 0
                VmSwap   = parse_kb(st, "VmSwap") or 0

                heap_rss = None
                roll_rss = None
                roll_anon = None
                if not args.light:
                    roll_rss, roll_anon = read_rollup(pid)
                    heap_rss = heap_rss_from_smaps(pid)

                row = [ts, str(pid), ppid, "" if uid is None else str(uid),
                       username_from_uid(uid), state, threads,
                       str(VmRSS), str(VmHWM), str(VmSize), str(RssAnon), str(RssFile), str(RssShmem), str(VmData), str(VmSwap),
                       "" if heap_rss is None else str(heap_rss),
                       "" if roll_rss is None else str(roll_rss),
                       "" if roll_anon is None else str(roll_anon),
                       exe.replace(",", " "), cmd.replace(",", " ")]
                out.write(",".join(row) + "\n")
            out.flush()
            if args.once:
                break
            time.sleep(args.interval)

if __name__ == "__main__":
    main()
