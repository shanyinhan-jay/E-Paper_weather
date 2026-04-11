from pathlib import Path

icon_dir = Path('d:/ink/E-Paper_weather/e_weather/icon')
medium_dir = icon_dir / '7272'
codes = sorted([p.stem for p in medium_dir.glob('*.c') if p.stem.isdigit()], key=lambda s: int(s))

lines = []
lines += ['#ifndef ICON_M_H', '#define ICON_M_H', '', '#include <stddef.h>', '#include <Arduino.h>', '']
lines += ['typedef struct {', '  const char* code;', '  const unsigned char* bmp;', '} WeatherIconMapEntryM;', '']

for c in codes:
    lines += [f'#include "7272/{c}.c"']
lines += ['']

lines += ['static const WeatherIconMapEntryM WEATHER_ICON_MAP_M[] = {']
for c in codes:
    lines += [f'  {{ "{c}", gImage_{c}_M }},']
lines += ['};', '']

lines += ['static const size_t WEATHER_ICON_MAP_M_COUNT = sizeof(WEATHER_ICON_MAP_M) / sizeof(WEATHER_ICON_MAP_M[0]);', '']
lines += ['static inline const unsigned char* getMediumIconData(const String& code) {', '  for (size_t i = 0; i < WEATHER_ICON_MAP_M_COUNT; i++) {', '    if (code == WEATHER_ICON_MAP_M[i].code) {', '      return WEATHER_ICON_MAP_M[i].bmp;', '    }', '  }', '  return NULL;', '}', '', '#endif', '']

(icon_dir / 'icon_m.h').write_text('\n'.join(lines), encoding='utf-8')
