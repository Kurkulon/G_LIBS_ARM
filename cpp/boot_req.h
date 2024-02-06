#ifndef BOOT_REQ_H__06_02_2024__11_48
#define BOOT_REQ_H__06_02_2024__11_48

#pragma once

#include "types.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct BootReqHS { u64 guid; u16 crc; };
__packed struct BootRspHS { u64 guid; u16 crc; };

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct BootReqMes
{
	u32 len;
	
	union
	{
		struct { u32 func; u32 len;								u16 align; u16 crc;	}	F1; // Get CRC
		struct { u32 func;										u16 align; u16 crc;	}	F2; // Exit boot loader
		struct { u32 func; u32 padr; u32 plen; u32 pdata[1];						}	F3; // Programm page
	};

};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct BootRspMes
{
	u32 len;

	union
	{
		struct { u32 func; u32 pageLen;	u32 len;	u16 sCRC;	u16 crc; }	F1; // Get CRC
		struct { u32 func;							u16 align;	u16 crc; } 	F2; // Exit boot loader
		struct { u32 func; u32 padr;	u32 status; u16 align;	u16 crc; } 	F3; // Programm page
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // BOOT_REQ_H__06_02_2024__11_48


