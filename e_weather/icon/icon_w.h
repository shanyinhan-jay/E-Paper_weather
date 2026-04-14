#ifndef ICON_W_H
#define ICON_W_H

#include <stddef.h>
#include <Arduino.h>

typedef struct {
  const char* code;
  const unsigned char* bmp;
} WeekIconMapEntry;

#define gImage_mon gImage_mon_W
#include "week/mon.c"
#undef gImage_mon

#define gImage_tue gImage_tue_W
#include "week/tue.c"
#undef gImage_tue

#define gImage_wed gImage_wed_W
#include "week/wed.c"
#undef gImage_wed

#define gImage_thu gImage_thu_W
#include "week/thu.c"
#undef gImage_thu

#define gImage_fri gImage_fri_W
#include "week/fri.c"
#undef gImage_fri

#define gImage_sat gImage_sat_W
#include "week/sat.c"
#undef gImage_sat

#define gImage_sun gImage_sun_W
#include "week/sun.c"
#undef gImage_sun

static const WeekIconMapEntry WEEK_ICON_MAP[] = {
  { "mon", gImage_mon_W },
  { "tue", gImage_tue_W },
  { "wed", gImage_wed_W },
  { "thu", gImage_thu_W },
  { "fri", gImage_fri_W },
  { "sat", gImage_sat_W },
  { "sun", gImage_sun_W },
};

static const size_t WEEK_ICON_MAP_COUNT = sizeof(WEEK_ICON_MAP) / sizeof(WEEK_ICON_MAP[0]);

static inline const unsigned char* getWeekIconData(const String& code) {
  for (size_t i = 0; i < WEEK_ICON_MAP_COUNT; i++) {
    if (code == WEEK_ICON_MAP[i].code) {
      return WEEK_ICON_MAP[i].bmp;
    }
  }
  return NULL;
}

#endif
