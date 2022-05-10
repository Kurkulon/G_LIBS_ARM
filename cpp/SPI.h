#ifndef SPI_H__21_04_2022__11_18
#define SPI_H__21_04_2022__11_18

#include "types.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct DSCSPI
{
	DSCSPI*			next;

	void*			wdata;
	void*			rdata;
	u32				adr;
	u16				alen;
	u16				wlen;
	u16				rlen;
	volatile bool	ready;
	byte			csnum;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void SPI_Init();
extern bool SPI_AddRequest(DSCSPI *d);
extern bool SPI_Update();
//extern i32	Get_FRAM_SPI_SessionsAdr();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // SPI_H__21_04_2022__11_18
