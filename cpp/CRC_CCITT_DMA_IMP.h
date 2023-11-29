#ifndef CRC_CCITT_DMA_IMP_H__24_11_2023__15_50
#define CRC_CCITT_DMA_IMP_H__24_11_2023__15_50

#pragma once

#include "CRC_CCITT_DMA.h"
#include "DMA.h"

bool busy_CRC_CCITT_DMA = false;

#ifdef CPU_XMC48

u16 CRC_CCITT_PIO(const void *data, u32 len, u16 init)
{
	CRC_FCE->CRC = init;	//	DataCRC CRC = { init };

	__packed const byte *s = (__packed const byte*)data;

	for ( ; len > 0; len--)
	{
		CRC_FCE->IR = *(s++);
	};

	//if (len > 0)
	//{
	//	CRC_FCE->IR = *(s++)&0xFF;
	//}

	__dsb(15);

	return CRC_FCE->RES;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init)
{
	CRC_FCE->CRC = init;	//	DataCRC CRC = { init };

	CRC_DMA.MemCopySrcInc(data, &CRC_FCE->IR, len);

	while (!CRC_DMA.CheckMemCopyComplete());

	__dsb(15);

	return (byte)CRC_FCE->RES;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init)
{
	if (busy_CRC_CCITT_DMA) return false;

	busy_CRC_CCITT_DMA = true;

	CRC_FCE->CRC = init;	//	DataCRC CRC = { init };

	CRC_DMA.MemCopySrcInc(data, &CRC_FCE->IR, len);

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_CheckComplete(u16* crc)
{
	if (CRC_DMA.CheckMemCopyComplete())
	{
		__dsb(15);

		*crc = (byte)CRC_FCE->RES;

		busy_CRC_CCITT_DMA = false;

		return true;
	}
	else
	{
		return false;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void Init_CRC_CCITT_DMA()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Init_CRC_CCITT_DMA ... ");

	HW::Peripheral_Enable(PID_FCE);

	HW::FCE->CLC = 0;
	CRC_FCE->CFG = 0;

	SEGGER_RTT_WriteString(0, "OK\n");
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#elif defined(CPU_SAME53)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init)
{
	CRC_DMA.CRC_CCITT(data, len, init);

	while (!CRC_DMA.CheckComplete());

	__dsb(15);

	return CRC_DMA.Get_CRC_CCITT_Result();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init)
{
	if (busy_CRC_CCITT_DMA) return false;

	busy_CRC_CCITT_DMA = true;

	CRC_DMA.CRC_CCITT(data, len, init);

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_CheckComplete(u16* crc)
{
	if (CRC_DMA.CheckComplete())
	{
		__dsb(15);

		*crc = CRC_DMA.Get_CRC_CCITT_Result();

		busy_CRC_CCITT_DMA = false;

		return true;
	}
	else
	{
		return false;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#elif defined(WIN32)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 CRC_CCITT_DMA(const void *data, u32 len, u16 init)
{
	return 0;//GetCRC16_CCIT(data, len, init);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_Async(const void* data, u32 len, u16 init)
{
	crc_ccit_result = 0;//GetCRC16_CCIT(data, len, init);

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool CRC_CCITT_DMA_CheckComplete(u16* crc)
{
	if (crc != 0)
	{
		*crc = crc_ccit_result;

		return true;
	}
	else
	{
		return false;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#endif // MEM_IMP_H__12_10_2022__17_05
