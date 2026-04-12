#ifndef ICON_M_H
#define ICON_M_H

#include <stddef.h>
#include <Arduino.h>

typedef struct {
  const char* code;
  const unsigned char* bmp;
} WeatherIconMapEntryM;

#define gImage_100 gImage_100_M
#include "7272/100.c"
#undef gImage_100
#define gImage_101 gImage_101_M
#include "7272/101.c"
#undef gImage_101
#define gImage_102 gImage_102_M
#include "7272/102.c"
#undef gImage_102
#define gImage_103 gImage_103_M
#include "7272/103.c"
#undef gImage_103
#define gImage_104 gImage_104_M
#include "7272/104.c"
#undef gImage_104
#define gImage_150 gImage_150_M
#include "7272/150.c"
#undef gImage_150
#define gImage_151 gImage_151_M
#include "7272/151.c"
#undef gImage_151
#define gImage_152 gImage_152_M
#include "7272/152.c"
#undef gImage_152
#define gImage_153 gImage_153_M
#include "7272/153.c"
#undef gImage_153
#define gImage_300 gImage_300_M
#include "7272/300.c"
#undef gImage_300
#define gImage_301 gImage_301_M
#include "7272/301.c"
#undef gImage_301
#define gImage_302 gImage_302_M
#include "7272/302.c"
#undef gImage_302
#define gImage_303 gImage_303_M
#include "7272/303.c"
#undef gImage_303
#define gImage_304 gImage_304_M
#include "7272/304.c"
#undef gImage_304
#define gImage_305 gImage_305_M
#include "7272/305.c"
#undef gImage_305
#define gImage_306 gImage_306_M
#include "7272/306.c"
#undef gImage_306
#define gImage_307 gImage_307_M
#include "7272/307.c"
#undef gImage_307
#define gImage_308 gImage_308_M
#include "7272/308.c"
#undef gImage_308
#define gImage_309 gImage_309_M
#include "7272/309.c"
#undef gImage_309
#define gImage_310 gImage_310_M
#include "7272/310.c"
#undef gImage_310
#define gImage_311 gImage_311_M
#include "7272/311.c"
#undef gImage_311
#define gImage_312 gImage_312_M
#include "7272/312.c"
#undef gImage_312
#define gImage_313 gImage_313_M
#include "7272/313.c"
#undef gImage_313
#define gImage_314 gImage_314_M
#include "7272/314.c"
#undef gImage_314
#define gImage_315 gImage_315_M
#include "7272/315.c"
#undef gImage_315
#define gImage_316 gImage_316_M
#include "7272/316.c"
#undef gImage_316
#define gImage_317 gImage_317_M
#include "7272/317.c"
#undef gImage_317
#define gImage_318 gImage_318_M
#include "7272/318.c"
#undef gImage_318
#define gImage_350 gImage_350_M
#include "7272/350.c"
#undef gImage_350
#define gImage_351 gImage_351_M
#include "7272/351.c"
#undef gImage_351
#define gImage_399 gImage_399_M
#include "7272/399.c"
#undef gImage_399
#define gImage_400 gImage_400_M
#include "7272/400.c"
#undef gImage_400
#define gImage_401 gImage_401_M
#include "7272/401.c"
#undef gImage_401
#define gImage_402 gImage_402_M
#include "7272/402.c"
#undef gImage_402
#define gImage_403 gImage_403_M
#include "7272/403.c"
#undef gImage_403
#define gImage_404 gImage_404_M
#include "7272/404.c"
#undef gImage_404
#define gImage_405 gImage_405_M
#include "7272/405.c"
#undef gImage_405
#define gImage_406 gImage_406_M
#include "7272/406.c"
#undef gImage_406
#define gImage_407 gImage_407_M
#include "7272/407.c"
#undef gImage_407
#define gImage_408 gImage_408_M
#include "7272/408.c"
#undef gImage_408
#define gImage_409 gImage_409_M
#include "7272/409.c"
#undef gImage_409
#define gImage_410 gImage_410_M
#include "7272/410.c"
#undef gImage_410
#define gImage_456 gImage_456_M
#include "7272/456.c"
#undef gImage_456
#define gImage_457 gImage_457_M
#include "7272/457.c"
#undef gImage_457
#define gImage_499 gImage_499_M
#include "7272/499.c"
#undef gImage_499
#define gImage_500 gImage_500_M
#include "7272/500.c"
#undef gImage_500
#define gImage_501 gImage_501_M
#include "7272/501.c"
#undef gImage_501
#define gImage_502 gImage_502_M
#include "7272/502.c"
#undef gImage_502
#define gImage_503 gImage_503_M
#include "7272/503.c"
#undef gImage_503
#define gImage_504 gImage_504_M
#include "7272/504.c"
#undef gImage_504
#define gImage_507 gImage_507_M
#include "7272/507.c"
#undef gImage_507
#define gImage_508 gImage_508_M
#include "7272/508.c"
#undef gImage_508
#define gImage_509 gImage_509_M
#include "7272/509.c"
#undef gImage_509
#define gImage_510 gImage_510_M
#include "7272/510.c"
#undef gImage_510
#define gImage_511 gImage_511_M
#include "7272/511.c"
#undef gImage_511
#define gImage_512 gImage_512_M
#include "7272/512.c"
#undef gImage_512
#define gImage_513 gImage_513_M
#include "7272/513.c"
#undef gImage_513
#define gImage_514 gImage_514_M
#include "7272/514.c"
#undef gImage_514
#define gImage_515 gImage_515_M
#include "7272/515.c"
#undef gImage_515
#define gImage_900 gImage_900_M
#include "7272/900.c"
#undef gImage_900
#define gImage_901 gImage_901_M
#include "7272/901.c"
#undef gImage_901

static const WeatherIconMapEntryM WEATHER_ICON_MAP_M[] = {
  { "100", gImage_100_M },
  { "101", gImage_101_M },
  { "102", gImage_102_M },
  { "103", gImage_103_M },
  { "104", gImage_104_M },
  { "150", gImage_150_M },
  { "151", gImage_151_M },
  { "152", gImage_152_M },
  { "153", gImage_153_M },
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
  { "500", gImage_500_M },
  { "501", gImage_501_M },
  { "502", gImage_502_M },
  { "503", gImage_503_M },
  { "504", gImage_504_M },
  { "507", gImage_507_M },
  { "508", gImage_508_M },
  { "509", gImage_509_M },
  { "510", gImage_510_M },
  { "511", gImage_511_M },
  { "512", gImage_512_M },
  { "513", gImage_513_M },
  { "514", gImage_514_M },
  { "515", gImage_515_M },
  { "900", gImage_900_M },
  { "901", gImage_901_M },
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
