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
#include "3636/150.c"
#include "3636/151.c"
#include "3636/152.c"
#include "3636/153.c"
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

static const WeatherIconMapEntryS WEATHER_ICON_MAP_S[] = {
  { "100", gImage_100_S },
  { "101", gImage_101_S },
  { "102", gImage_102_S },
  { "103", gImage_103_S },
  { "104", gImage_104_S },
  { "150", gImage_150_S },
  { "151", gImage_151_S },
  { "152", gImage_152_S },
  { "153", gImage_153_S },
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
