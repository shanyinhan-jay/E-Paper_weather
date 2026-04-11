#ifndef ICON_O_H
#define ICON_O_H

#include <Arduino.h>
#include <pgmspace.h>

#ifndef PROGMEM
#define PROGMEM
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char gImage_ink[] PROGMEM;
extern const unsigned char gImage_bell[] PROGMEM;
extern const unsigned char gImage_mon[] PROGMEM;
extern const unsigned char gImage_tue[] PROGMEM;
extern const unsigned char gImage_wed[] PROGMEM;
extern const unsigned char gImage_thu[] PROGMEM;
extern const unsigned char gImage_fri[] PROGMEM;
extern const unsigned char gImage_sat[] PROGMEM;
extern const unsigned char gImage_sun[] PROGMEM;
extern const unsigned char gImage_tem[] PROGMEM;

const unsigned char* getOtherIconData(const char* code);

#ifdef __cplusplus
}
#endif

#endif
