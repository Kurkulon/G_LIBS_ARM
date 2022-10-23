#include "types.h"
#include "CRC16.h"

#ifndef WIN32
#pragma O3
#pragma Otime
#endif

word GetCRC16(const void *data, u32 len)
{
	DataCRC CRC = { 0xFFFF };

	const byte *s = (const byte*)data;

	for ( ; len > 0; len--)
	{
		CRC.w = tableCRC[CRC.b[0] ^ *(s++)] ^ CRC.b[1];
	};

	return CRC.w;
}

