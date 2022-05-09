#ifndef TFTP_H__25_03_2020__18_36
#define TFTP_H__25_03_2020__18_36

#include "emac.h"
//#include "main.h"

extern bool TFTP_Idle();
extern bool RequestTFTP(EthBuf* mb);
inline bool TFTP_Connected() { extern u16 tftp_processed; return tftp_processed != 0; }  

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct FLWB
{
	u32		adr;
	u32 	dataLen;
	u32 	dataOffset;
	byte	data[0];
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // TFTP_H__25_03_2020__18_36


