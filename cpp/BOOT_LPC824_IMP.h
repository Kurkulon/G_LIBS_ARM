#ifndef BOOT_LPC824_IMP_H__01_12_2022__10_57
#define BOOT_LPC824_IMP_H__01_12_2022__10_57

#if defined(CPU_LPC824) || defined(CPU_LPC812)
  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <time.h>
#include <crc16.h>
#include <SEGGER_RTT.h>

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef BOOT_COM_MODE
#define BOOT_COM_MODE	ComPort::ASYNC
#endif

#define FRDY 1
#define FCMDE 2
#define FLOCKE 4
#define PAGESIZE 64
#define PAGEDWORDS (PAGESIZE>>2)
#define PAGES_IN_SECTOR 16
#define SECTORSIZE (PAGESIZE*PAGES_IN_SECTOR)
#define SECTORDWORDS (SECTORSIZE>>2)

u32 PLANESIZE = 0x1000;

#ifndef BOOT_START_SECTOR
	#ifdef CPU_LPC824
		#define BOOT_START_SECTOR 8
	#elif defined(CPU_LPC812)
		#define BOOT_START_SECTOR 4
	#endif
#endif

#define START_PAGE (BOOT_START_SECTOR*PAGES_IN_SECTOR)

#define BOOTSIZE (SECTORSIZE*BOOT_START_SECTOR)
#define FLASH0 0x000000
#define FLASH_START (FLASH0+BOOTSIZE)

//#define FLASH1 (FLASH0+PLANESIZE)

#define FLASH_END (FLASH0+PLANESIZE)
#define FLASH_SIZE (FLASH_END-FLASH_START)

#define IAP_LOCATION 0X1FFF1FF1
static u32 command_param[5];
static u32 status_result[4];
typedef void (*IAP)(unsigned int [],unsigned int[]);
#define iap_entry ((void(*)(u32[],u32[]))IAP_LOCATION)
//#define iap_entry ((IAP)IAP_LOCATION);

enum IAP_STATUS { CMD_SUCCESS = 0,  INVALID_COMMAND,  SRC_ADDR_ERROR,  DST_ADDR_ERROR,  SRC_ADDR_NOT_MAPPED,  DST_ADDR_NOT_MAPPED,  COUNT_ERROR,  INVALID_SECTOR,  SECTOR_NOT_BLANK, 
 SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION, COMPARE_ERROR, BUSY, ERR_ISP_IRC_NO_POWER , ERR_ISP_FLASH_NO_POWER,  ERR_ISP_FLASH_NO_CLOCK  };

//static ComPort com;

//static ComPort* actCom = 0;


struct FL
{
	u32 id;
	u32 size;
	u32 pageSize;
	u32 nbPlane;
	u32 planeSize;
};

//static FL flDscr;

static bool run = true;

//static u32 crcErrors = 0;
//static u32 lenErrors = 0;
//static u32 reqErrors = 0;

//static u32 lens[300] = {0};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct ReqMes
{
	u32 len;
	//u32 func;

	union
	{
		struct { u32 func; u32 len;										u16 align; u16 crc; }	F1; // Get CRC
		struct { u32 func;												u16 align; u16 crc; }	F2; // Exit boot loader
		struct { u32 func; u32 padr; u32 plen; u32 pdata[PAGEDWORDS];	u16 align; u16 crc; }	F3; // Programm page

		//struct { u32 flashLen;  u16 align; u16 crc; } F01; // Get Flash CRC
		//struct { u32 padr; u32 page[PAGEDWORDS]; u16 align; u16 crc; } F02; // Write page
		//struct { u16 align; u16 crc; } F03; // Exit boot loader
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct RspMes
{
	u32 len;
	//u32 func;

	union
	{
		struct { u32 func; u32 pageLen;	u32 len;	u16 sCRC;	u16 crc; }	F1; // Get CRC
		struct { u32 func;							u16 align;	u16 crc; } 	F2; // Exit boot loader
		struct { u32 func; u32 padr;	u32 status; u16 align;	u16 crc; } 	F3; // Programm page

		//struct { u32 flashLen; u16 flashCRC; u16 crc; } F01;
		//struct { u32 padr; u32 status; u16 align; u16 crc; } F02;
		//struct { u16 align; u16 crc; } F03;							// Exit boot loader
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitPlaneSize()
{
	byte id = (HW::SYSCON->DEVICE_ID >> 4) & 0xF;

	u32 planesize = 0x1000;

	switch (id)
	{
		case 0:	planesize = 0x1000; break;
		case 1:	planesize = 0x2000; break;
		case 2:	planesize = 0x4000; break;
		case 4:	planesize = 0x8000; break;
	};

	PLANESIZE = planesize;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool IAP_PrepareSector(u32 sector)
{
	sector += BOOT_START_SECTOR;

	command_param[0] = 50;
	command_param[1] = sector;
	command_param[2] = sector;
	command_param[3] = 0;

	__disable_irq(); iap_entry(command_param, status_result); __enable_irq();

	return status_result[0] == 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool IAP_WritePage(u32 pagenum, u32 *pbuf)
{
	command_param[0] = 51;
	command_param[1] = FLASH_START + pagenum*PAGESIZE;
	command_param[2] = (u32)pbuf;
	command_param[3] = PAGESIZE;
	command_param[4] = MCK/1000;

	__disable_irq(); iap_entry(command_param, status_result); __enable_irq();

	return status_result[0] == 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static bool IAP_WriteSector(u32 sector, u32 *pbuf)
//{
//	command_param[0] = 51;
//	command_param[1] = FLASH_START + sector*SECTORSIZE;
//	command_param[2] = (u32)pbuf;
//	command_param[3] = SECTORSIZE;
//	command_param[4] = 0;
//
//	iap_entry(command_param, status_result);
//
//	return status_result[0] == 0;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool IAP_EraseSector(u32 sector)
{
	sector += BOOT_START_SECTOR;

	command_param[0] = 52;
	command_param[1] = sector;
	command_param[2] = sector;
	command_param[3] = MCK/1000;

	__disable_irq(); iap_entry(command_param, status_result); __enable_irq();

	return status_result[0] == 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static bool IAP_ErasePage(u32 pagenum)
//{
//	pagenum += START_PAGE;
//
//	command_param[0] = 59;
//	command_param[1] = pagenum;
//	command_param[2] = pagenum;
//	command_param[3] = 0;
//
//	iap_entry(command_param, status_result);
//
//	return status_result[0] == 0;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool VerifyPage(u32 pagenum, u32 *pbuf)
{
	bool c = true;

	u32 *p = (u32*)(FLASH_START + pagenum*PAGESIZE);

	for (u32 i = 0; i < PAGEDWORDS; i++)
	{
		if (p[i] != pbuf[i])
		{
			c = false;
			break;
		};
	};

	return c;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool WritePage(u32 pagenum, u32 *pbuf)
{
	u32 sector = pagenum/PAGES_IN_SECTOR;

	if ((pagenum & (PAGES_IN_SECTOR-1)) == 0)
	{
		if (!IAP_PrepareSector(sector))
		{
			return false;
		};
		if (!IAP_EraseSector(sector))
		{
			return false;
		};
	};

	if (!IAP_PrepareSector(sector))
	{
		return false;
	};

	if (!IAP_WritePage(pagenum, pbuf))
	{
		return false;
	};

	if (!VerifyPage(pagenum, pbuf))
	{
		return false;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static bool VerifySector(u32 sector, u32 *pbuf)
//{
//	bool c = true;
//
//	u32 *p = (u32*)(FLASH_START + sector*SECTORSIZE);
//
//	for (u32 i = 0; i < SECTORDWORDS; i++)
//	{
//		if (p[i] != pbuf[i])
//		{
//			c = false;
//			break;
//		};
//	};
//
//	return c;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static bool WriteSector(u32 sector, u32 *pbuf)
//{
//	if (!VerifySector(sector, pbuf))
//	{
//		if (!IAP_PrepareSector(sector))
//		{
//			return false;
//		};
//		if (!IAP_EraseSector(sector))
//		{
//			return false;
//		};
//
//		if (!IAP_PrepareSector(sector))
//		{
//			return false;
//		};
//
//		if (!IAP_WriteSector(sector, pbuf))
//		{
//			return false;
//		};
//
//		if (!VerifySector(sector, pbuf))
//		{
//			return false;
//		};
//	};
//
//	return true;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static u32 bufSector[SECTORDWORDS];
//static u32 pageMask = 0;
//static u32 curSector = ~0;

//static bool WritePageBuf(u32 pagenum, u32 *pbuf)
//{
//	u32 sector = pagenum/PAGES_IN_SECTOR;
//
//	if (curSector != sector)
//	{
//		pageMask = 0;
//		curSector = sector;
//		
//		u32 *p = bufSector;
//
//		for (u32 i = ArraySize(bufSector); i > 0; i--)
//		{
//			*p++ = ~0;
//		};
//	};
//
//	pagenum &= (PAGES_IN_SECTOR-1);
//
//	if ((pageMask & (1<<pagenum)) == 0)
//	{
//		u32 *p = bufSector + pagenum*PAGEDWORDS;
//
//		for (u32 i = PAGEDWORDS; i > 0; i--)
//		{
//			*p++ = *pbuf++;
//		};
//
//		pageMask |= 1<<pagenum;
//	};
//
//	if ((pageMask & (PAGES_IN_SECTOR-1)) == (PAGES_IN_SECTOR-1))
//	{
//		if (!WriteSector(sector, bufSector))
//		{
//			return false;
//		};
//	};
//
//	return true;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef BOOT_HANDSHAKE

const unsigned __int64 masterGUID	= BOOT_MGUID;
const unsigned __int64 slaveGUID	= BOOT_SGUID;

__packed struct ReqHS { unsigned __int64 guid; u16 crc; };
__packed struct RspHS { unsigned __int64 guid; u16 crc; };

bool HandShake()
{
	static ReqHS req;
	static RspHS rsp;

	static ComPort::WriteBuffer wb = { false, sizeof(req), &req };

	static ComPort::ReadBuffer rb = { false, 0, sizeof(rsp), &rsp };

	req.guid = slaveGUID;
	req.crc = GetCRC16(&req, sizeof(req)-2);

	bool c = false;

	for (byte i = 0; i < 2; i++)
	{
		com.Read(&rb, BOOT_HANDSHAKE_PRETIMEOUT, BOOT_HANDSHAKE_POSTTIMEOUT);

		Pin_MainLoop_Set();

		while(com.Update())
		{
			HW::ResetWDT();
		};

		Pin_MainLoop_Clr();

		c = (rb.recieved && rb.len == sizeof(RspHS) && GetCRC16(rb.data, rb.len) == 0 && rsp.guid == masterGUID);

		if (c)
		{
			com.Write(&wb);

			while(com.Update()) { HW::ResetWDT() ; };

			break;
		};
	};

	return c;
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool Request_01_GetFlashCRC(ReqMes &req, RspMes &rsp)
{

	//if (req.F01.flashLen > FLASH_SIZE) { req.F01.flashLen = FLASH_SIZE; };

	u32 len = MIN(req.F1.len, FLASH_SIZE);

	rsp.F1.func		= req.F1.func;
	rsp.F1.pageLen	= PAGESIZE;
	rsp.F1.len		= len;
	rsp.F1.sCRC		= GetCRC16((void*)FLASH_START, len);
	rsp.F1.crc		= GetCRC16(&rsp.F1, sizeof(rsp.F1) - 2);
	rsp.len			= sizeof(rsp.F1);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool Request_03_WritePage(ReqMes &req, RspMes &rsp)
{
	bool c = false;

	if (req.len == sizeof(req.F3))
	{
		c = WritePage(req.F3.padr/PAGESIZE, req.F3.pdata);
	};

	rsp.F3.func		= req.F3.func;
	rsp.F3.padr		= req.F3.padr;
	rsp.F3.status	= (c) ? 1 : 0;
	rsp.F3.align	= ~req.F3.padr;
	rsp.F3.crc		= GetCRC16(&rsp.F3, sizeof(rsp.F3) - 2);
	rsp.len			= sizeof(rsp.F3);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool Request_02_ExitBootLoader(ReqMes &req, RspMes &rsp)
{
	rsp.F2.func		= req.F2.func;
	rsp.F2.align	= 0x5555;
	rsp.F2.crc		= GetCRC16(&rsp.F2, sizeof(rsp.F2) - sizeof(rsp.F2.crc));
	rsp.len			= sizeof(rsp.F2);

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static bool Request_Default(ReqMes &req, RspMes &rsp)
//{
//	rsp.func = req.func;
//	rsp.F03.align = 0xAAAA;
//	rsp.F03.crc = GetCRC16(&rsp.func, sizeof(rsp.F03) - sizeof(rsp.F03.crc) + sizeof(rsp.func));
//	rsp.len = sizeof(rsp.F03) + sizeof(rsp.func);
//
//	return true;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool Request_Handler(ReqMes &req, RspMes &rsp)
{
	bool c = false;

	switch (req.F1.func)
	{
		case 1:		c = Request_01_GetFlashCRC(req, rsp);		break;
		case 2:  	c = Request_02_ExitBootLoader(req, rsp);	break;
		case 3:  	c = Request_03_WritePage(req, rsp);			break;
		//default: 	c = Request_Default(req, rsp);				break;
	};

	return c;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


static void UpdateCom()
{
	static ComPort::WriteBuffer wb;
	static ComPort::ReadBuffer rb;

	static ReqMes req;
	static RspMes rsp;

	static byte i = 0;

	static bool c = true;

	//static TM32 tm;

	switch (i)
	{
		case 0:

			rb.data = &req.F1.func;
			rb.maxLen = sizeof(req)-sizeof(req.len);
			
			com.Read(&rb, BOOT_COM_PRETIMEOUT, BOOT_COM_POSTTIMEOUT);

			i++;

			break;

		case 1:

			if (!com.Update())
			{
				i++;
			};

			break;

		case 2:

			if (rb.recieved && rb.len > 0 && GetCRC16(rb.data, rb.len) == 0)
			{
				req.len = rb.len;

				c = Request_Handler(req, rsp);

				i++;
			}
			else
			{
				i = 0;
			};

			break;

		case 3:

			//while(!tm.Check(2)) ;

			wb.data = &rsp.F1.func;
			wb.len = rsp.len;

			com.Write(&wb);

			i++;

			break;

		case 4:

			if (!com.Update())
			{
				i = 0;

				run = c;
			};

			break;

	};

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern "C" void _MainAppStart(u32 adr);

int main()
{
	#ifdef BOOT_START_BREAKPOINT
		__breakpoint(0);
	#endif

	SEGGER_RTT_Init();
	SEGGER_RTT_WriteString(0, RTT_CTRL_CLEAR);
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Bootloader Start ...\n");

	Pin_MainLoop_Clr();

	InitPlaneSize();

	InitHardware();

	com.Connect(BOOT_COM_MODE, BOOT_COM_SPEED, BOOT_COM_PARITY, BOOT_COM_STOPBITS);

	#ifdef BOOT_HANDSHAKE

		run = HandShake();

	#endif

	CTM32	tm;

	while(run)
	{
		UpdateCom();

		HW::ResetWDT();

		if(tm.Check(MS2CTM(50))) Pin_MainLoop_Tgl();
	};

	#ifdef BOOT_EXIT_BREAKPOINT
		__breakpoint(0);
	#endif

	__disable_irq();

	SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_GREEN "Main App Start ... %u ms\n", GetMilliseconds());

	_MainAppStart(FLASH0+BOOTSIZE);

	return FLASH0+BOOTSIZE;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // #if defined(CPU_LPC824) || defined(CPU_LPC812)

#endif // BOOT_LPC824_IMP_H__01_12_2022__10_57
