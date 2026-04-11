from pathlib import Path
import re

small_dir = Path('d:/ink/E-Paper_weather/e_weather/icon/3636')
for p in small_dir.glob('*.c'):
    txt = p.read_text(encoding='utf-8', errors='ignore')
    # Only rewrite variable declaration symbol, keep data unchanged
    txt_new = re.sub(
        r'(const\s+unsigned\s+char\s+)gImage_(\d+)(?:_S)?(\s*\[)',
        lambda m: f"{m.group(1)}gImage_{m.group(2)}_S{m.group(3)}",
        txt,
        count=1
    )
    p.write_text(txt_new, encoding='utf-8')
print('renamed symbols in 3636/*.c to *_S')
