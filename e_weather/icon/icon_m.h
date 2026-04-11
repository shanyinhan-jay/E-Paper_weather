#ifndef ICON_M_H
#define ICON_M_H

#include <stddef.h>
#include <Arduino.h>

typedef struct {
  const char* code;
  const unsigned char* bmp;
} WeatherIconMapEntryM;

#include "7272/100.c"
#include "7272/101.c"
#include "7272/102.c"
#include "7272/103.c"
#include "7272/104.c"
#include "7272/150.c"
#include "7272/153.c"
#include "7272/154.c"
#include "7272/300.c"
#include "7272/301.c"
#include "7272/302.c"
#include "7272/303.c"
#include "7272/304.c"
#include "7272/305.c"
#include "7272/306.c"
#include "7272/307.c"
#include "7272/308.c"
#include "7272/309.c"
#include "7272/310.c"
#include "7272/311.c"
#include "7272/312.c"
#include "7272/313.c"
#include "7272/314.c"
#include "7272/315.c"
#include "7272/316.c"
#include "7272/317.c"
#include "7272/318.c"
#include "7272/350.c"
#include "7272/351.c"
#include "7272/399.c"
#include "7272/400.c"
#include "7272/401.c"
#include "7272/402.c"
#include "7272/403.c"
#include "7272/404.c"
#include "7272/405.c"
#include "7272/406.c"
#include "7272/407.c"
#include "7272/408.c"
#include "7272/409.c"
#include "7272/410.c"
#include "7272/456.c"
#include "7272/457.c"
#include "7272/499.c"

static const WeatherIconMapEntryM WEATHER_ICON_MAP_M[] = {
  { "100", gImage_100_M },
  { "101", gImage_101_M },
  { "102", gImage_102_M },
  { "103", gImage_103_M },
  { "104", gImage_104_M },
  { "150", gImage_150_M },
  { "153", gImage_153_M },
  { "154", gImage_154_M },
  { "300", gImage_300_M },
  { "301", gImage_301_M },
  { "302", gImage_302_M },
  { "303", gImage_303_M },
  { "304", gImage_304_M },
  { "305", gImage_305_M },
  { "306", gImage_306_M },
  { "307", gImage_307_M },
  { "308", gImage_308_M },
  { "309", gImage_309_M },
  { "310", gImage_310_M },
  { "311", gImage_311_M },
  { "312", gImage_312_M },
  { "313", gImage_313_M },
  { "314", gImage_314_M },
  { "315", gImage_315_M },
  { "316", gImage_316_M },
  { "317", gImage_317_M },
  { "318", gImage_318_M },
  { "350", gImage_350_M },
  { "351", gImage_351_M },
  { "399", gImage_399_M },
  { "400", gImage_400_M },
  { "401", gImage_401_M },
  { "402", gImage_402_M },
  { "403", gImage_403_M },
  { "404", gImage_404_M },
  { "405", gImage_405_M },
  { "406", gImage_406_M },
  { "407", gImage_407_M },
  { "408", gImage_408_M },
  { "409", gImage_409_M },
  { "410", gImage_410_M },
  { "456", gImage_456_M },
  { "457", gImage_457_M },
  { "499", gImage_499_M },
};

static const size_t WEATHER_ICON_MAP_M_COUNT = sizeof(WEATHER_ICON_MAP_M) / sizeof(WEATHER_ICON_MAP_M[0]);

static inline const unsigned char* getMediumIconData(const String& code) {
  for (size_t i = 0; i < WEATHER_ICON_MAP_M_COUNT; i++) {
    if (code == WEATHER_ICON_MAP_M[i].code) {
      return WEATHER_ICON_MAP_M[i].bmp;
    }
  }
  return NULL;
}

#endif
