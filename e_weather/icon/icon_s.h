#ifndef ICON_S_H
#define ICON_S_H

#include <stddef.h>
#include <Arduino.h>

typedef struct {
  const char* code;
  const unsigned char* bmp;
} WeatherIconMapEntryS;

#include "3636/100.c"
#include "3636/101.c"
#include "3636/102.c"
#include "3636/103.c"
#include "3636/104.c"
#include "3636/131.c"
#include "3636/150.c"
#include "3636/151.c"
#include "3636/152.c"
#include "3636/153.c"
#include "3636/154.c"
#include "3636/300.c"
#include "3636/301.c"
#include "3636/302.c"
#include "3636/303.c"
#include "3636/304.c"
#include "3636/305.c"
#include "3636/306.c"
#include "3636/307.c"
#include "3636/308.c"
#include "3636/309.c"
#include "3636/310.c"
#include "3636/311.c"
#include "3636/312.c"
#include "3636/313.c"
#include "3636/314.c"
#include "3636/315.c"
#include "3636/316.c"
#include "3636/317.c"
#include "3636/318.c"
#include "3636/350.c"
#include "3636/351.c"
#include "3636/399.c"
#include "3636/400.c"
#include "3636/401.c"
#include "3636/402.c"
#include "3636/403.c"
#include "3636/404.c"
#include "3636/405.c"
#include "3636/406.c"
#include "3636/407.c"
#include "3636/408.c"
#include "3636/409.c"
#include "3636/410.c"
#include "3636/456.c"
#include "3636/457.c"
#include "3636/499.c"

static const WeatherIconMapEntryS WEATHER_ICON_MAP_S[] = {
  { "100", gImage_100_S },
  { "101", gImage_101_S },
  { "102", gImage_102_S },
  { "103", gImage_103_S },
  { "104", gImage_104_S },
  { "131", gImage_131_S },
  { "150", gImage_150_S },
  { "151", gImage_151_S },
  { "152", gImage_152_S },
  { "153", gImage_153_S },
  { "154", gImage_154_S },
  { "300", gImage_300_S },
  { "301", gImage_301_S },
  { "302", gImage_302_S },
  { "303", gImage_303_S },
  { "304", gImage_304_S },
  { "305", gImage_305_S },
  { "306", gImage_306_S },
  { "307", gImage_307_S },
  { "308", gImage_308_S },
  { "309", gImage_309_S },
  { "310", gImage_310_S },
  { "311", gImage_311_S },
  { "312", gImage_312_S },
  { "313", gImage_313_S },
  { "314", gImage_314_S },
  { "315", gImage_315_S },
  { "316", gImage_316_S },
  { "317", gImage_317_S },
  { "318", gImage_318_S },
  { "350", gImage_350_S },
  { "351", gImage_351_S },
  { "399", gImage_399_S },
  { "400", gImage_400_S },
  { "401", gImage_401_S },
  { "402", gImage_402_S },
  { "403", gImage_403_S },
  { "404", gImage_404_S },
  { "405", gImage_405_S },
  { "406", gImage_406_S },
  { "407", gImage_407_S },
  { "408", gImage_408_S },
  { "409", gImage_409_S },
  { "410", gImage_410_S },
  { "456", gImage_456_S },
  { "457", gImage_457_S },
  { "499", gImage_499_S },
};

static const size_t WEATHER_ICON_MAP_S_COUNT = sizeof(WEATHER_ICON_MAP_S) / sizeof(WEATHER_ICON_MAP_S[0]);

static inline const unsigned char* getSmallIconData(const String& code) {
  for (size_t i = 0; i < WEATHER_ICON_MAP_S_COUNT; i++) {
    if (code == WEATHER_ICON_MAP_S[i].code) {
      return WEATHER_ICON_MAP_S[i].bmp;
    }
  }
  return NULL;
}

#endif
