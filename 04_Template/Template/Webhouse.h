#ifndef WEBHOUSE_H_
#define WEBHOUSE_H_

//-----Header-Files----------------------------------------------------------------
#include <stdint.h>

//-----Macros----------------------------------------------------------------------

//-----Data types------------------------------------------------------------------

//-----Function prototypes---------------------------------------------------------
extern void initWebhouse(void);
extern void closeWebhouse(void);

extern void turnTVOn(void);
extern void turnTVOff(void);
extern int  getTVState(void);

extern void dimRLamp(uint16_t Duty_cycle);
extern void dimSLamp(uint16_t Duty_cycle);

extern void turnLED1On(void);
extern void turnLED1Off(void);
extern int  getLED1State(void);

extern void turnLED2On(void);
extern void turnLED2Off(void);
extern int  getLED2State(void);

extern void turnHeatOn(void);
extern void turnHeatOff(void);
extern int  getHeatState(void);
extern float getTemp(void);

extern int getAlarmState(void);

#endif
