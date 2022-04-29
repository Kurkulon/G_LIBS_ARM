#ifndef I2C_H__21_04_2022__11_17
#define I2C_H__21_04_2022__11_17

#include "types.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct DSCI2C
{
	DSCI2C*			next;

	void*			wdata;
	void*			rdata;
	void*			wdata2;
	u16				wlen;
	u16				wlen2;
	u16				rlen;
	u16				readedLen;
	byte			adr;
	volatile bool	ready;
	volatile bool	ack;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void I2C_Init();
extern bool I2C_AddRequest(DSCI2C *d);
extern bool I2C_Update();
extern i32	Get_FRAM_I2C_SessionsAdr();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // I2C_H__21_04_2022__11_17
