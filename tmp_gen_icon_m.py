from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
GEN = ROOT / "tools" / "icons" / "gen_all_icons.py"

if not GEN.exists():
    raise SystemExit(f"Generator not found: {GEN}")

cmd = [sys.executable, str(GEN), "--project-root", str(ROOT)]
raise SystemExit(subprocess.run(cmd).returncode)
