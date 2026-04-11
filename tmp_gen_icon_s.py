from pathlib import Path

icon_dir = Path('d:/ink/E-Paper_weather/e_weather/icon')
small_dir = icon_dir / '3636'
codes = sorted([p.stem for p in small_dir.glob('*.c') if p.stem.isdigit()], key=lambda s: int(s))

lines = []
lines += ['#ifndef ICON_S_H', '#define ICON_S_H', '', '#include <stddef.h>', '#include <Arduino.h>', '']
lines += ['typedef struct {', '  const char* code;', '  const unsigned char* bmp;', '} WeatherIconMapEntryS;', '']

for c in codes:
    lines += [f'#include "3636/{c}.c"']
lines += ['']

lines += ['static const WeatherIconMapEntryS WEATHER_ICON_MAP_S[] = {']
for c in codes:
    lines += [f'  {{ "{c}", gImage_{c}_S }},']
lines += ['};', '']

lines += ['static const size_t WEATHER_ICON_MAP_S_COUNT = sizeof(WEATHER_ICON_MAP_S) / sizeof(WEATHER_ICON_MAP_S[0]);', '']
lines += ['static inline const unsigned char* getSmallIconData(const String& code) {', '  for (size_t i = 0; i < WEATHER_ICON_MAP_S_COUNT; i++) {', '    if (code == WEATHER_ICON_MAP_S[i].code) {', '      return WEATHER_ICON_MAP_S[i].bmp;', '    }', '  }', '  return NULL;', '}', '', '#endif', '']

(icon_dir / 'icon_s.h').write_text('\n'.join(lines), encoding='utf-8')
print('icon_s.h generated with', len(codes), 'entries')
