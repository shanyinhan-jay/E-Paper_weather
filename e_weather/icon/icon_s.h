#ifndef ICON_S_H
#define ICON_S_H

#include <stddef.h>
#include <Arduino.h>

typedef struct {
  const char* code;
  const unsigned char* bmp;
} WeatherIconMapEntryS;

#define gImage_100 gImage_100_S
#include "3636/100.c"
#undef gImage_100
#define gImage_101 gImage_101_S
#include "3636/101.c"
#undef gImage_101
#define gImage_102 gImage_102_S
#include "3636/102.c"
#undef gImage_102
#define gImage_103 gImage_103_S
#include "3636/103.c"
#undef gImage_103
#define gImage_104 gImage_104_S
#include "3636/104.c"
#undef gImage_104
#define gImage_150 gImage_150_S
#include "3636/150.c"
#undef gImage_150
#define gImage_151 gImage_151_S
#include "3636/151.c"
#undef gImage_151
#define gImage_152 gImage_152_S
#include "3636/152.c"
#undef gImage_152
#define gImage_153 gImage_153_S
#include "3636/153.c"
#undef gImage_153
#define gImage_300 gImage_300_S
#include "3636/300.c"
#undef gImage_300
#define gImage_301 gImage_301_S
#include "3636/301.c"
#undef gImage_301
#define gImage_302 gImage_302_S
#include "3636/302.c"
#undef gImage_302
#define gImage_303 gImage_303_S
#include "3636/303.c"
#undef gImage_303
#define gImage_304 gImage_304_S
#include "3636/304.c"
#undef gImage_304
#define gImage_305 gImage_305_S
#include "3636/305.c"
#undef gImage_305
#define gImage_306 gImage_306_S
#include "3636/306.c"
#undef gImage_306
#define gImage_307 gImage_307_S
#include "3636/307.c"
#undef gImage_307
#define gImage_308 gImage_308_S
#include "3636/308.c"
#undef gImage_308
#define gImage_309 gImage_309_S
#include "3636/309.c"
#undef gImage_309
#define gImage_310 gImage_310_S
#include "3636/310.c"
#undef gImage_310
#define gImage_311 gImage_311_S
#include "3636/311.c"
#undef gImage_311
#define gImage_312 gImage_312_S
#include "3636/312.c"
#undef gImage_312
#define gImage_313 gImage_313_S
#include "3636/313.c"
#undef gImage_313
#define gImage_314 gImage_314_S
#include "3636/314.c"
#undef gImage_314
#define gImage_315 gImage_315_S
#include "3636/315.c"
#undef gImage_315
#define gImage_316 gImage_316_S
#include "3636/316.c"
#undef gImage_316
#define gImage_317 gImage_317_S
#include "3636/317.c"
#undef gImage_317
#define gImage_318 gImage_318_S
#include "3636/318.c"
#undef gImage_318
#define gImage_350 gImage_350_S
#include "3636/350.c"
#undef gImage_350
#define gImage_351 gImage_351_S
#include "3636/351.c"
#undef gImage_351
#define gImage_399 gImage_399_S
#include "3636/399.c"
#undef gImage_399
#define gImage_400 gImage_400_S
#include "3636/400.c"
#undef gImage_400
#define gImage_401 gImage_401_S
#include "3636/401.c"
#undef gImage_401
#define gImage_402 gImage_402_S
#include "3636/402.c"
#undef gImage_402
#define gImage_403 gImage_403_S
#include "3636/403.c"
#undef gImage_403
#define gImage_404 gImage_404_S
#include "3636/404.c"
#undef gImage_404
#define gImage_405 gImage_405_S
#include "3636/405.c"
#undef gImage_405
#define gImage_406 gImage_406_S
#include "3636/406.c"
#undef gImage_406
#define gImage_407 gImage_407_S
#include "3636/407.c"
#undef gImage_407
#define gImage_408 gImage_408_S
#include "3636/408.c"
#undef gImage_408
#define gImage_409 gImage_409_S
#include "3636/409.c"
#undef gImage_409
#define gImage_410 gImage_410_S
#include "3636/410.c"
#undef gImage_410
#define gImage_456 gImage_456_S
#include "3636/456.c"
#undef gImage_456
#define gImage_457 gImage_457_S
#include "3636/457.c"
#undef gImage_457
#define gImage_499 gImage_499_S
#include "3636/499.c"
#undef gImage_499
#define gImage_500 gImage_500_S
#include "3636/500.c"
#undef gImage_500
#define gImage_501 gImage_501_S
#include "3636/501.c"
#undef gImage_501
#define gImage_502 gImage_502_S
#include "3636/502.c"
#undef gImage_502
#define gImage_503 gImage_503_S
#include "3636/503.c"
#undef gImage_503
#define gImage_504 gImage_504_S
#include "3636/504.c"
#undef gImage_504
#define gImage_507 gImage_507_S
#include "3636/507.c"
#undef gImage_507
#define gImage_508 gImage_508_S
#include "3636/508.c"
#undef gImage_508
#define gImage_509 gImage_509_S
#include "3636/509.c"
#undef gImage_509
#define gImage_510 gImage_510_S
#include "3636/510.c"
#undef gImage_510
#define gImage_511 gImage_511_S
#include "3636/511.c"
#undef gImage_511
#define gImage_512 gImage_512_S
#include "3636/512.c"
#undef gImage_512
#define gImage_513 gImage_513_S
#include "3636/513.c"
#undef gImage_513
#define gImage_514 gImage_514_S
#include "3636/514.c"
#undef gImage_514
#define gImage_515 gImage_515_S
#include "3636/515.c"
#undef gImage_515
#define gImage_900 gImage_900_S
#include "3636/900.c"
#undef gImage_900
#define gImage_901 gImage_901_S
#include "3636/901.c"
#undef gImage_901
#define gImage_999 gImage_999_S
#include "3636/999.c"
#undef gImage_999

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
  { "500", gImage_500_S },
  { "501", gImage_501_S },
  { "502", gImage_502_S },
  { "503", gImage_503_S },
  { "504", gImage_504_S },
  { "507", gImage_507_S },
  { "508", gImage_508_S },
  { "509", gImage_509_S },
  { "510", gImage_510_S },
  { "511", gImage_511_S },
  { "512", gImage_512_S },
  { "513", gImage_513_S },
  { "514", gImage_514_S },
  { "515", gImage_515_S },
  { "900", gImage_900_S },
  { "901", gImage_901_S },
  { "999", gImage_999_S },
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
