#ifndef CLOCK_H__20_11_2023__16_38
#define CLOCK_H__20_11_2023__16_38

#pragma once

#include "time.h"

extern void SetClock(const RTC &t);
extern bool	ClockReqTemp(DSCI2C &dsc, byte buf[8]);
extern i16	ClockGetTemp(DSCI2C &dsc);
extern bool	ClockStartConvTemp(DSCI2C &dsc, byte buf[8]);

#endif // CLOCK_H__20_11_2023__16_38
