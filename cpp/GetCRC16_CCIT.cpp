#include "types.h"
#include "CRC16_CCIT.h"

#pragma O3
#pragma Otime

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

word GetCRC16_CCIT(const void *data, u32 len, word init)
{
	DataCRC CRC = { init };

	const byte *s = (const byte*)data;

	for ( ; len > 0; len--)
	{
		CRC.w = tableCRC_CCIT[CRC.b[0] ^ *(s++)] ^ CRC.b[1];
	};

	return CRC.w;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

word GetCRC16_CCIT_refl(const void *data, u32 len, word init)
{
	DataCRC CRC = { init };

	const byte *s = (const byte*)data;

	for ( ; len > 0; len--)
	{
		CRC.w = tableCRC_CCIT[CRC.b[1] ^ *(s++)] ^ (CRC.w<<8);
	};

	return CRC.w;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
