#ifndef TFTP_DEF_H__25_03_2020__18_35
#define TFTP_DEF_H__25_03_2020__18_35

#include "mem.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53
	#define ISP_PAGESIZE 512
#elif defined(CPU_XMC48)
	#define ISP_PAGESIZE 256
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct FLWB
{
	u32		adr;
	u32 	dataLen;
	u32 	dataOffset;
	byte	data[0];
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern u16	ISP_GetBootloaderVersion();
extern void	ISP_InitFlashWrite();
extern bool ISP_RequestFlashWrite(Ptr<MB> &mb);
extern u32	ISP_Flash_Read(u32 addr, byte *data, u32 size);
extern u32	ISP_GetFlashWriteError();
extern u32	ISP_GetFlashWriteOK();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#endif // TFTP_DEF_H__25_03_2020__18_35


