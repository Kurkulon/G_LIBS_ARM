#ifndef NAND_IMP_H__06_05_2022__19_15
#define NAND_IMP_H__06_05_2022__19_15

#include "types.h"
#include "core.h"

#include "SEGGER_RTT.h"
#include "CRC16_8005.h"
#include "flash_def.h"
#include "DMA.h"

#ifdef WIN32

#include <windows.h>
#include <Share.h>
#include <conio.h>
#include <stdarg.h>
#include <stdio.h>
#include <intrin.h>
#include "CRC16_CCIT.h"
#include "list.h"

static HANDLE handleNandFile;
static const char nameNandFile[] = "NAND_FLASH_STORE.BIN";

static HANDLE handleWriteThread;
static HANDLE handleReadThread;

static byte nandChipSelected = 0;

static u64 curNandFilePos = 0;
//static u64 curNandFileBlockPos = 0;
static u32 curBlock = 0;
static u32 curRawBlock = 0;
static u16 curPage = 0;
static u16 curCol = 0;

static OVERLAPPED	_overlapped;
static u32			_ovlReadedBytes = 0;
static u32			_ovlWritenBytes = 0;

static void* nandEraseFillArray;
static u32 nandEraseFillArraySize = 0;
static byte nandReadStatus = 0x41;
static u32 lastError = 0;


//static byte fram_I2c_Mem[0x10000];
static byte fram_SPI_Mem[0x40000];

static bool fram_spi_WREN = false;

static u16 crc_ccit_result = 0;


struct BlockBuffer { BlockBuffer *next; u32 block; u32 prevBlock; u32 writed; u32 data[((NAND_PAGE_SIZE+NAND_SPARE_SIZE) << NAND_PAGE_BITS) >> 2]; };

static BlockBuffer _blockBuf[16];

static List<BlockBuffer> freeBlockBuffer;
static List<BlockBuffer> rdBlockBuffer;
static List<BlockBuffer> wrBlockBuffer;

static BlockBuffer *curNandBlockBuffer[4] = { 0 };

static volatile bool busyWriteThread = false;

#else

//#pragma O3
//#pragma Otime

#endif 

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static byte clog2(u32 v) {byte r = 0; while (v>>=1) {r++;}; return r;}
static void NAND_WriteDataDMA(volatile void *src, u16 len);
static void NAND_ReadDataDMA(volatile void *dst, u16 len);
//static void NAND_ReadDataDMA2(volatile void *dst, u16 len);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

const u16 testNandChipMask = 0xFFFF;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//static void InitVectorTable();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//#define NAND_BASE_PIO_DATA 		AT91C_BASE_PIOA
//#define NAND_PIO_DATA_OFFSET	10
//#define NAND_PIO_DATA_MASK		0xFF
//#define NAND_READ_PACK_BYTES	512
//#define NAND_WRITE_PACK_BYTES	256

#define NAND_CMD_RESET				0xFF
#define NAND_CMD_READ_ID			0x90
#define NAND_CMD_READ_1				0x00
#define NAND_CMD_READ_2				0x30
#define NAND_CMD_RANDREAD_1			0x05
#define NAND_CMD_RANDREAD_2			0xE0
#define NAND_CMD_PAGE_PROGRAM_1		0x80
#define NAND_CMD_PAGE_PROGRAM_2		0x10
#define NAND_CMD_READ_STATUS		0x70
#define NAND_CMD_BLOCK_ERASE_1		0x60
#define NAND_CMD_BLOCK_ERASE_2		0xD0
#define NAND_CMD_READ_PARAM			0xEC
#define NAND_CMD_CHANGE_WRCOL		0x85
#define NAND_CMD_COPYBACK_PROGRAM	0x85
#define NAND_CMD_SET_FEATURES		0xEF
#define NAND_CMD_GET_FEATURES		0xEE



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NandMemSize nandSize;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct NandParamPage
{
 	char	signature[4];
 	u16		revisionNumber;
 	u16		featuresSupported;
	u16		optionalCommandsSupported;
	byte	_rezerved1[22];
	char	deviceManufacturer[12];
	char	deviceModel[20];
	byte	JEDEC_manufacturer_ID;
	u16		dateCode;
	byte	_rezerved2[13];
	u32		numberDataBytesPerPage;
	u16		numberSpareBytesPerPage;
	byte	_rezerved3[6];
	u32		numberPagesPerBlock;
	u32		numberBlocksPerLUN;
	byte	numberLUNsPerChip;
	byte	numberAddressCycles;
	byte	numberBitsPerCell;
	u16		badBlocksMaximumPerLUN;
	u16		blockEndurans;
	byte	guaranteedValidBlocks;
	u16		blockEnduranceForGuaranteedValidBlocks;
	byte	numberProgramsPerPage;
	byte	_rezerved4;
	byte	number_Bits_ECC_correctability;
	byte	number_of_interleaved_address_bits;
	byte	interleaved_operation_attributes;
	byte	_rezerved5[13];
	byte	pin_capacitance_per_chip_enable;
	u16		timing_mode_support;

	byte	_rezerved6[123];
	u16		integrityCRC;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NandParamPage nandParamPage[NAND_MAX_CHIP];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline byte NAND_READ()
{
	#ifdef CPU_SAME53	
		PIO_WE_RE->CLR(RE); 
		NAND_DELAY_RP();
		byte v = PIO_NAND_DATA->IN; 
		PIO_WE_RE->SET(RE); 
		NAND_DELAY_REH();
		return v; 
	#elif defined(CPU_XMC48)
		return *FLD;
	#elif defined(WIN32)
		return 0;
	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_WRITE(byte data)
{ 
	#ifdef CPU_SAME53	

		PIO_NAND_DATA->OUT8(data); 
		PIO_WE_RE->CLR(WE); 
		NAND_DELAY_WP();
		PIO_WE_RE->SET(WE); 
		NAND_DELAY_WH();

	#elif defined(CPU_XMC48)

		*FLD = data;

	#endif
}       

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_CMD_LATCH(byte cmd)
{ 
	#ifdef CPU_SAME53	

		PIO_CLE->SET(CLE); 
		PIO_ALE->CLR(ALE); 
		//__nop(); __nop(); __nop(); __nop();__nop(); __nop(); __nop(); __nop(); 
		NAND_WRITE(cmd); 
		//__nop(); __nop();__nop(); __nop();__nop(); __nop(); __nop(); __nop(); 
		PIO_CLE->CLR(CLE|ALE); 

	#elif defined(CPU_XMC48)

		*FLC = cmd;

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_ADR_LATCH(byte cmd)
{
	#ifdef CPU_SAME53	

		PIO_ALE->SET(ALE); 
		PIO_CLE->CLR(CLE); 
		//__nop(); __nop();__nop(); __nop();__nop(); __nop(); __nop(); __nop(); 
		NAND_WRITE(cmd); 
		//__nop(); __nop();__nop(); __nop();__nop(); __nop(); __nop(); __nop(); 
		PIO_CLE->CLR(CLE|ALE); 

	#elif defined(CPU_XMC48)

		*FLA = cmd;

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline byte NAND_ADR_READ()
{
#ifdef CPU_SAME53	
	PIO_ALE->SET(ALE);
	PIO_CLE->CLR(CLE);
	__nop(); __nop();
	PIO_WE_RE->CLR(RE);
	__nop(); __nop();
	byte v = PIO_NAND_DATA->IN;
	PIO_WE_RE->SET(RE);
	PIO_CLE->CLR(CLE | ALE);
	return v;
#elif defined(CPU_XMC48)
	return *FLA;
#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_ADR_LATCH_COL(u16 col) 
{ 
	#ifdef CPU_SAME53	

		PIO_CLE->CLR(CLE); 
		PIO_ALE->SET(ALE); 
		NAND_WRITE(col); 
		NAND_WRITE(col>>8); 
		PIO_CLE->CLR(CLE|ALE); 

	#elif defined(CPU_XMC48)

		*FLA = col; *FLA = col >> 8;

	#endif	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_ADR_LATCH_ROW(u32 row) 
{ 
	#ifdef CPU_SAME53	

		PIO_CLE->CLR(CLE); 
		PIO_ALE->SET(ALE); 
		NAND_WRITE(row); 
		NAND_WRITE(row>>8); 
		NAND_WRITE(row>>16); 
		PIO_CLE->CLR(CLE|ALE); 

	#elif defined(CPU_XMC48)

		*FLA = row; *FLA = row >> 8; *FLA = row >> 16;

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_ADR_LATCH_COL_ROW(u16 col, u32 row)
{ 
	#ifdef CPU_SAME53	

		PIO_CLE->CLR(CLE); 
		PIO_ALE->SET(ALE); 
		NAND_WRITE(col); 
		NAND_WRITE(col>>8); 
		NAND_WRITE(row); 
		NAND_WRITE(row>>8); 
		NAND_WRITE(row>>16); 
		PIO_CLE->CLR(CLE|ALE);

	#elif defined(CPU_XMC48)

		*FLA = col; *FLA = col >> 8;
		*FLA = row; *FLA = row >> 8; *FLA = row >> 16;

	#endif
} 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//inline bool FlashReady()
//{
//	return (HW::PIOC->PDSR & (1UL<<31)) != 0;
//}
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//inline bool FlashBusy()
//{
//	return (HW::PIOC->PDSR & (1UL<<31)) == 0;
//}
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool NAND_BUSY() 
{
	#ifdef CPU_SAME53	
		return PIO_FLREADY->TBCLR(PIN_FLREADY); 
	#elif defined(CPU_XMC48)
		return PIO_FLREADY->TBCLR(PIN_FLREADY);
	#elif defined(WIN32)
		return !HasOverlappedIoCompleted(&_overlapped);
	#endif
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_EnableWriteProtect()
{
#ifndef WIN32
	PIO_WP->CLR(WP);
#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline void NAND_DisableWriteProtect()
{
#ifndef WIN32
	PIO_WP->SET(WP);
#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline byte NAND_CmdReadStatus()
{
#ifndef WIN32

	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_READ_STATUS);
	NAND_DIR_IN();
	return NAND_READ();

#else

	return (nandReadStatus & ~0x40) | ((NAND_BUSY()) ? 0 : 0x40);

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool NAND_CmdBusy()
{
	return NAND_BUSY() || ((NAND_CmdReadStatus() & NAND_SR_RDY) == 0);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_Chip_Select(byte chip) 
{    
#ifndef WIN32
	if(chip < NAND_MAX_CHIP)                   
	{ 				
	#ifdef CPU_SAME53	
		PIO_WE_RE->SET(RE|WE); 
	#endif
		PIO_FCS->SET(maskChipSelect ^ chipSelect[chip]);
		PIO_FCS->CLR(chipSelect[chip]);
	};
#else
	nandChipSelected = chip;
#endif
}                                                                              

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_Chip_Disable() 
{  
#ifndef WIN32
	PIO_FCS->SET(maskChipSelect);

	#ifdef CPU_SAME53	
		NAND_DIR_IN();
		PIO_WE_RE->SET(RE|WE); 
	#endif

#else
	nandChipSelected = ~0;
#endif
}                                                                              

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool NAND_Reset()
{
	while(NAND_BUSY());
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_RESET);
	NAND_CmdReadStatus();
	u32 count = 1000; while (!NAND_BUSY() && (count-- > 0));
	while(NAND_BUSY());
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_XMC48	

static byte *nandWriteDataSrc = 0;
static byte *nandReadDataDst = 0;
static u16	nandWriteDataLen = 0;
static u16	nandReadDataLen = 0;

#endif

static bool NAND_CheckDataComplete()
{
	#ifdef CPU_SAME53

		if (NAND_DMA.CheckComplete())
		{
			PIO_WE_RE->SET(WE|RE);
			PIO_WE_RE->DIRSET = WE|RE;
			PIO_WE_RE->PINCFG[PIN_RE] = PIN_RE_CFG;
			PIO_WE_RE->PINCFG[PIN_WE] = PIN_WE_CFG;

			#ifdef nandTCC
							nandTCC->CTRLA = 0;
							nandTCC->CTRLA = TC_SWRST;
			#else
							nandTC->CTRLA = 0;
							nandTC->CTRLA = TC_SWRST;
			#endif

			return true;
		}
		else
		{
			return false;
		};
	
	#elif defined(CPU_XMC48)

		return NAND_DMA.CheckMemCopyComplete();

	#elif defined(WIN32)

		return true; //HasOverlappedIoCompleted(&_overlapped);

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_Set_Features(byte adr, byte p1, byte p2, byte p3, byte p4)
{
#ifndef WIN32
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_SET_FEATURES);
	NAND_ADR_LATCH(adr);
	NAND_DELAY_ADL();
	NAND_WRITE(p1); 
	NAND_WRITE(p2); 
	NAND_WRITE(p3); 
	NAND_WRITE(p4); 
	while(!NAND_BUSY());
	while(NAND_BUSY());

	NAND_DIR_IN();
#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_Get_Features(byte adr, byte* p)
{
#ifndef WIN32
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_GET_FEATURES);
	NAND_ADR_LATCH(adr);

	while(!NAND_BUSY());

	NAND_DIR_IN();

	while(NAND_BUSY());

	NAND_DELAY_PR();

	p[0] = NAND_READ(); 
	p[1] = NAND_READ(); 
	p[2] = NAND_READ(); 
	p[3] = NAND_READ(); 
#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool NAND_Read_ID(NandID *id)
{
#ifndef WIN32
	__disable_irq();

	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_READ_ID);
	NAND_ADR_LATCH(0);
	NAND_DIR_IN();
	NAND_DELAY_WHR();

	byte *p = (byte*)id;

	*(p++) = NAND_READ(); 
	*(p++) = NAND_READ(); 
	*(p++) = NAND_READ(); 
	*(p++) = NAND_READ(); 
	*(p++) = NAND_READ(); 

	__enable_irq();

	//NAND_ReadDataDMA(id, sizeof(NandID));

	//while (!NAND_CheckDataComplete());

#endif

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_Read_PARAM(NandParamPage *pp)
{
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_READ_PARAM);
	NAND_ADR_LATCH(0);
	NAND_DIR_IN();

	while(!NAND_BUSY());
	while(NAND_BUSY());
	
	NAND_ReadDataDMA(pp, sizeof(NandParamPage)); while (!NAND_CheckDataComplete());

	//PIO_WE_RE->SET(RE); 
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline u32 NAND_ROW(u32 block, u16 page)
{
	return (block << FLADR::PAGE_BITS) + page;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

static BlockBuffer* NAND_AllocBlockBuffer()
{
	BlockBuffer *bb;

	while ((bb = freeBlockBuffer.Get()) == 0) { Sleep(0); };

	bb->block = ~0;
	bb->prevBlock = ~0;
	bb->writed = 0;

	return bb;
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

static void NAND_ReqReadBlock(BlockBuffer* bb)
{
	if (bb != 0)
	{
		u64 adr = bb->block;

		adr *= (NAND_PAGE_SIZE+NAND_SPARE_SIZE) << NAND_PAGE_BITS;

		_overlapped.Offset = (u32)adr;
		_overlapped.OffsetHigh = (u32)(adr>>32);
		_overlapped.hEvent = 0;
		_overlapped.Internal = 0;
		_overlapped.InternalHigh = 0;

		ReadFile(handleNandFile, bb->data, sizeof(bb->data), 0, &_overlapped);
	};
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

static void NAND_SetCurAdrFile(u16 col, u32 bl, u16 pg)
{
	curBlock = bl;
	curPage = pg;
	curCol = col;

	u64 adr = bl;
	
	curRawBlock = adr = (adr << NAND_CHIP_BITS) + nandChipSelected;

	adr *= (NAND_PAGE_SIZE + NAND_SPARE_SIZE) << NAND_PAGE_BITS;

	adr += (u32)(NAND_PAGE_SIZE + NAND_SPARE_SIZE) * pg;

	adr += col;// & ((1 << (NAND_COL_BITS+1))-1);

	curNandFilePos = adr;

	BlockBuffer* &bb = curNandBlockBuffer[curRawBlock&3];

	if (bb != 0 && bb->block != curRawBlock) 
	{
		wrBlockBuffer.Add(bb);
		ResumeThread(handleWriteThread);
		bb = 0;
	};

	if (bb == 0)
	{
		bb = NAND_AllocBlockBuffer();
	};

}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

void NAND_ReqFlushBlockBuffers()
{
	for (u32 i = 0; i < ArraySize(curNandBlockBuffer); i++)
	{
		BlockBuffer* &bb = curNandBlockBuffer[i];

		if (bb != 0) wrBlockBuffer.Add(bb), bb = 0;
	};

	ResumeThread(handleWriteThread);
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

void NAND_FlushBlockBuffers()
{
	NAND_ReqFlushBlockBuffers();

	while(busyWriteThread) Sleep(1);
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_CmdEraseBlock(u32 bl)
{
#ifndef WIN32

	bl = NAND_ROW(bl, 0);
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_BLOCK_ERASE_1);
	NAND_ADR_LATCH_ROW(bl);
	NAND_CMD_LATCH(NAND_CMD_BLOCK_ERASE_2);

#else

	NAND_SetCurAdrFile(0, bl, 0);

	BlockBuffer* &bb = curNandBlockBuffer[curRawBlock&3];

	bb->block = curRawBlock;

	u32 *d = bb->data;

	u32 count = ArraySize(bb->data);

	while (count != 0) *(d++) = ~0, count--;

	bb->writed = 1;

	//*((u32*)nandEraseFillArray) = (bl << NAND_CHIP_BITS) + nandChipSelected;

	//u64 adr = curNandFilePos;// + NAND_PAGE_SIZE;

	//_overlapped.Offset = (u32)adr;
	//_overlapped.OffsetHigh = (u32)(adr>>32);
	//_overlapped.hEvent = 0;
	//_overlapped.Internal = 0;
	//_overlapped.InternalHigh = 0;

	//WriteFile(handleNandFile, nandEraseFillArray, (NAND_PAGE_SIZE+NAND_SPARE_SIZE) << NAND_PAGE_BITS, 0, &_overlapped);

	nandReadStatus = 0;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_CmdRandomRead(u16 col)
{
#ifndef WIN32

	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_RANDREAD_1);
	NAND_ADR_LATCH_COL(col);
	NAND_CMD_LATCH(NAND_CMD_RANDREAD_2);

#else

	NAND_SetCurAdrFile(col, curBlock, curPage);

	BlockBuffer* &bb = curNandBlockBuffer[curRawBlock&3];

	if (bb != 0 && bb->block != curRawBlock)
	{
		bb->block = curRawBlock;

		NAND_ReqReadBlock(bb);
	};

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_CmdReadPage(u16 col, u32 bl, u16 pg)
{
#ifndef WIN32

	bl = NAND_ROW(bl, pg);
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_READ_1);
	NAND_ADR_LATCH_COL_ROW(col, bl);
	NAND_CMD_LATCH(NAND_CMD_READ_2);

#else

	NAND_SetCurAdrFile(col, bl, pg);

	BlockBuffer* &bb = curNandBlockBuffer[curRawBlock&3];

	if (bb != 0 && bb->block != curRawBlock)
	{
		bb->block = curRawBlock;

		NAND_ReqReadBlock(bb);
	};

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_CmdWritePage(u16 col, u32 bl, u16 pg)
{
#ifndef WIN32

	bl = NAND_ROW(bl, pg);
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_PAGE_PROGRAM_1);
	NAND_ADR_LATCH_COL_ROW(col, bl);

#else

	NAND_SetCurAdrFile(col, bl, pg);

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void NAND_CmdWritePage2()
{
#ifndef WIN32
	NAND_DIR_OUT();
	NAND_CMD_LATCH(NAND_CMD_PAGE_PROGRAM_2);
#else

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef WIN32

struct FLADR_old
{
	union
	{
		struct
		{
			u64		col		: NAND_COL_BITS;
			u64 	page	: NAND_PAGE_BITS;
			u64		chip	: NAND_CHIP_BITS;
			u64 	block	: NAND_BLOCK_BITS;

			u64		overflow : (64-(NAND_COL_BITS+NAND_PAGE_BITS+NAND_CHIP_BITS+NAND_BLOCK_BITS));
		};

		u64	raw;
	};

	enum { pg = (1<<NAND_COL_BITS) };
//	u32		rawpage;

//	const NandMemSize& sz;

	inline void operator=(const FLADR &a) { raw = a.raw; }

	FLADR_old() : raw(0) {}
	FLADR_old(u32 bl, u16 pg, u16 cl, byte ch) : block(bl), page(pg), col(cl), chip(ch) {}
	FLADR_old(u32 pg) : col(0) { SetRawPage(pg); }

	u32		GetRawPage() { return (raw & NAND_RAWADR_MASK) >> NAND_COL_BITS; }

	void	SetRawPage(u32 p) { raw = (u64)(p & NAND_RAWPAGE_MASK) << NAND_COL_BITS; };

	u32		GetRawBlock() { return (raw & NAND_RAWADR_MASK) >> (NAND_COL_BITS+NAND_PAGE_BITS); }

	void	SetRawBlock(u32 b) { raw = (u64)(b & NAND_RAWBLOCK_MASK) << (NAND_COL_BITS+NAND_PAGE_BITS); };

	u64		GetRawAdr()	{ return raw & NAND_RAWADR_MASK; };
	void	SetRawAdr(u64 a) { raw  = a & NAND_RAWADR_MASK; };

	void	NextPage()	{ col = 0; raw += 1 << NAND_COL_BITS; raw += FLADR::chipOffsetNext[chip]; }
	void	NextBlock()	{ col = 0;page = 0;raw += 1 << (NAND_COL_BITS + NAND_PAGE_BITS); raw += FLADR::chipOffsetNext[chip];}
	void	PrevPage()	{ raw -= 1 << NAND_COL_BITS; col = 0; raw -= FLADR::chipOffsetPrev[chip]; }
	void	PrevBlock()	{ raw -= 1 << (NAND_COL_BITS + NAND_PAGE_BITS);col = 0;page = 0; raw -= FLADR::chipOffsetPrev[chip];}

	void	AddRaw(u32 v) { raw += v; raw += FLADR::chipOffsetNext[chip]; }
	void	SubRaw(u32 v) { raw -= v; raw -= FLADR::chipOffsetPrev[chip]; }
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void NAND_Test_FLADR()
{
	FLADR_old	x;
	FLADR		y;

	u32 col = 0;
	u32 page = 0;
	u32 chip = 0;
	u32 block = 0;
	u32 rawpage = 0;
	u32 rawblock = 0;
	u64 rawadr = 0;

	for (u32 n = 1; n < (1<<NAND_MAX_CHIP); n++)
	{
		FLADR::InitVaildTables(n);

		for (u32 i = 0; i < 10000; i++)
		{
			x.col = col;				y.SetCol(col);				if (x.raw != y.raw)	__debugbreak();

			x.page = page;				y.SetPage(page);			if (x.raw != y.raw)	__debugbreak();

			x.chip = chip;				y.SetChip(chip);			if (x.raw != y.raw)	__debugbreak();

			x.block = block;			y.SetBlock(block);			if (x.raw != y.raw)	__debugbreak();

			x.SetRawPage(rawpage);		y.SetRawPage(rawpage);		if (x.raw != y.raw)	__debugbreak();

			x.SetRawBlock(rawblock);	y.SetRawBlock(rawblock);	if (x.raw != y.raw)	__debugbreak();

			x.SetRawAdr(rawadr);		y.SetRawAdr(rawadr);		if (x.raw != y.raw)	__debugbreak();

			x.NextPage();				y.NextPage();				if (x.raw != y.raw)	__debugbreak();
			x.NextBlock();				y.NextBlock();				if (x.raw != y.raw)	__debugbreak();
			x.PrevPage();				y.PrevPage();				if (x.raw != y.raw)	__debugbreak();
			x.PrevBlock();				y.PrevBlock();				if (x.raw != y.raw)	__debugbreak();


			if (x.GetRawPage()	!= y.GetRawPage())		__debugbreak();
			if (x.GetRawBlock() != y.GetRawBlock())		__debugbreak();
			if (x.GetRawAdr()	!= y.GetRawAdr())		__debugbreak();

			col		+= 9859;
			page	+= 1019;
			chip	+= 647;
			block		= (block+1019)*9871;
			rawpage		= (rawpage+1087)*98887;
			rawblock	= (rawblock+1153)*99901;
			rawadr		= (rawadr+1229)*99991;
		};
	};

}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

DWORD WINAPI NAND_WriteThread(LPVOID lpParam) 
{
	static OVERLAPPED	ovrl;
	static BlockBuffer *bb = 0; 
	static HANDLE ev;
	static u32 numBytesWriten;
	
	ev = CreateEventA(0, FALSE, FALSE, 0);

	while(1)
	{
		busyWriteThread = true;
		
		bb = wrBlockBuffer.Get();

		if (bb != 0)
		{
			if (bb->writed)
			{
				u64 adr = bb->block;// + NAND_PAGE_SIZE;

				adr *= (NAND_PAGE_SIZE+NAND_SPARE_SIZE) << NAND_PAGE_BITS;

				ovrl.Offset = (u32)adr;
				ovrl.OffsetHigh = (u32)(adr>>32);
				ovrl.hEvent = ev;
				ovrl.Internal = 0;
				ovrl.InternalHigh = 0;

				//SetFilePointer(handleNandFile, ovrl.Offset, (i32*)&ovrl.OffsetHigh, FILE_BEGIN);

				WriteFile(handleNandFile, bb->data, sizeof(bb->data), 0, &ovrl);

				WaitForSingleObject(ev, INFINITE);

				bb->writed = 0;
			};

			freeBlockBuffer.Add(bb);
		}
		else
		{
			busyWriteThread = false;

			SuspendThread(GetCurrentThread());
		};
	};
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

DWORD WINAPI NAND_ReadThread(LPVOID lpParam) 
{
	while(1)
	{
		SuspendThread(GetCurrentThread());
	};
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//#pragma push
//#pragma O0
//#pragma Otime

void NAND_Init()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "NAND Flash Init ... \n");

#ifndef WIN32

	using namespace HW;

	U32u p1, p2;

#ifdef CPU_SAME53

	NAND_ClockEnable();

	HW::GCLK->PCHCTRL[EVENT_NAND_1+GCLK_EVSYS0] = GCLK_GEN(GEN_MCK)|GCLK_CHEN;

	EVSYS->CH[EVENT_NAND_1].CHANNEL = NAND_EVENT_GEN|EVSYS_PATH_ASYNCHRONOUS|EVSYS_EDGSEL_RISING_EDGE;
	EVSYS->USER[NAND_EVSYS_USER] = EVENT_NAND_1+1;

	PIO_NAND_DATA->DIRSET = 0xFF;
	PIO_NAND_DATA->SetWRCONFIG(0xFF, PORT_INEN|PORT_DRVSTR|PORT_WRPINCFG);
	PIO_NAND_DATA->CTRL |= 0xFF;
	PIO_FCS->DIRSET = maskChipSelect; PIO_FCS->SET(maskChipSelect);
	PIO_CLE->DIRSET = CLE; PIO_CLE->CLR(CLE);
	PIO_ALE->DIRSET = ALE; PIO_ALE->CLR(ALE);
	PIO_CLE->PINCFG[PIN_CLE] = PIN_CLE_CFG;
	PIO_ALE->PINCFG[PIN_ALE] = PIN_ALE_CFG;

	PIO_WE_RE->DIRSET = WE; PIO_WE_RE->SET(WE); PIO_WE_RE->SetWRCONFIG(WE, WE_PORT_PMUX|PORT_WRPMUX);
	PIO_WE_RE->DIRSET = RE; PIO_WE_RE->SET(RE); PIO_WE_RE->SetWRCONFIG(RE, RE_PORT_PMUX|PORT_WRPMUX);
	PIO_WE_RE->PINCFG[PIN_WE] = PIN_WE_CFG;
	PIO_WE_RE->PINCFG[PIN_RE] = PIN_RE_CFG;

	PIO_FLREADY->DIRCLR = FLREADY; PIO_FLREADY->PINCFG[PIN_FLREADY] = PINGFG_INEN|PINGFG_PULLEN; PIO_FLREADY->CTRL |= FLREADY; PIO_FLREADY->SET(FLREADY);
	PIO_WP->DIRSET = WP; PIO_WP->SET(WP);

	#ifdef nandTCC
							nandTCC->CTRLA = 0;
							nandTCC->CTRLA = TCC_SWRST;
							while(nandTCC->SYNCBUSY & TCC_SWRST);
	#elif defined(nandTC)
							nandTC->CTRLA = 0;
							nandTC->CTRLA = TC_SWRST;
							while(nandTC->SYNCBUSY & TC_SWRST);
	#else
			#error  Must defined nandTCC or nandTC
	#endif

	NAND_DIR_OUT();

#elif defined(CPU_XMC48)

	PIO_D0->ModePin			( PIN_D0,		HWIO1	);
	PIO_D1->ModePin			( PIN_D1,		HWIO1	);
	PIO_D2->ModePin			( PIN_D2,		HWIO1	);
	PIO_D3->ModePin			( PIN_D3,		HWIO1	);
	PIO_D4->ModePin			( PIN_D4,		HWIO1	);
	PIO_D5->ModePin			( PIN_D5,		HWIO1	);
	PIO_D6->ModePin			( PIN_D6,		HWIO1	);
	PIO_D7->ModePin			( PIN_D7,		HWIO1	);
	PIO_NANDCLE->ModePin	( PIN_NANDCLE,	HWIO1	);
	PIO_NANDALE->ModePin	( PIN_NANDALE,	HWIO1	);
	PIO_NANDOE->ModePin		( PIN_NANDOE,	HWIO1	);
	PIO_NANDwE->ModePin		( PIN_NANDwE,	HWIO1	);
	PIO_WP->ModePin			( PIN_WP,		G_PP	);
	PIO_FLREADY->ModePin	( PIN_FLREADY,	I2DPU	);

	PIO_FCS->DIRSET(maskChipSelect);

	NAND_Chip_Disable();

	HW::EBU_Enable(__EBU_DIV);

	EBU->CLC = 0;
	EBU->MODCON = /*EBU_ARBSYNC|*/EBU_ARBMODE(3);
	EBU->USERCON = 0x3FF<<16;

	EBU->ADDRSEL0 = EBU_REGENAB/*|EBU_ALTENAB*/;

	EBU->BUSRCON0 = EBU_AGEN(4)|EBU_WAIT(0)|EBU_PORTW(1);
	EBU->BUSRAP0 = EBU_ADDRC(0)|EBU_CMDDELAY(0)|EBU_WAITRDC(NAND_WAITRDC)|EBU_DATAC(0)|EBU_RDRECOVC(NS2EBUCLK(0))|EBU_RDDTACS(0);

	EBU->BUSWCON0 = EBU_LOCKCS|EBU_AGEN(4)|EBU_WAIT(0)|EBU_PORTW(1);

//				 = |			|				 |		tWP		 |			   |			   |				;
	EBU->BUSWAP0 = EBU_ADDRC(0)|EBU_CMDDELAY(0)|EBU_WAITWRC(NAND_WAITWRC)|EBU_DATAC(0)|EBU_WRRECOVC(NS2EBUCLK(0))|EBU_WRDTACS(0);

#endif

	byte checkBuf[16];
	byte readBuf[16];

	for (byte i = 0; i < 8; i++)
	{
		checkBuf[i] = 1 << i;
		checkBuf[i+8] = ~(1 << i);
	};

	for(byte chip = 0; chip < NAND_MAX_CHIP; chip ++)
	{
		SEGGER_RTT_printf(0, RTT_CTRL_TEXT_WHITE "Chip %u - %s ... ", chip, chipRefDes[chip]);

		NAND_Chip_Select(chip);
		NAND_Reset();

		for (byte i = 0; i < ArraySize(checkBuf); i++)
		{
			NAND_DIR_OUT();
			NAND_ADR_LATCH(checkBuf[i]);
			NAND_DIR_IN();
			readBuf[i] = NAND_ADR_READ();
		};

		byte bitMask = 0;

		for (byte i = 0; i < 8; i++)
		{
			if (checkBuf[i] != readBuf[i] || checkBuf[i + 8] != readBuf[i + 8])
			{
				bitMask |= 1 << i;
			};
		};

		nandSize.chipDataBusMask[chip] = ~bitMask;
	
		NandID &id = nandSize.id[chip];

		NAND_Read_ID(&id);

		u32 chipSize = 0;

		if((id.maker == 0xEC) && (id.device == 0xD3))
		{
			byte col_bits = id.pageSize + 10;
			byte page_off = col_bits;
			byte chip_off = id.blockSize + 16;
			byte page_bits = chip_off - page_off;
			byte block_bits = (id.planeSize + 23 + id.planeNumber) - chip_off;

			chipSize = 1UL << (col_bits + page_bits + block_bits - 20);

			if (nandSize.ch == 0)
			{
				FLADR::COL_BITS 	= col_bits;
				FLADR::PAGE_OFF		= page_off;		
				FLADR::CHIP_OFF		= chip_off;
				FLADR::PAGE_BITS	= page_bits;
				FLADR::BLOCK_BITS	= block_bits;

				nandSize.ch = 1ULL << (FLADR::COL_BITS+FLADR::PAGE_BITS+FLADR::BLOCK_BITS);
			};
			
			nandSize.fl += nandSize.ch;
		
			if (bitMask == 0) { nandSize.mask |= (1 << chip); };

			nandSize.integrityCRC[chip] = 0;

			SEGGER_RTT_printf(0, "Samsung - %u MB - ", chipSize);
		}
		else if((id.maker == 0x2C) && (id.device == 0x68))
		{
			SEGGER_RTT_WriteString(0, "Micron - ");

			NAND_Reset();

			NAND_Set_Features(0x01, 0, 0, 0, 0);
			NAND_Set_Features(0x10, 0, 0, 0, 0);

			NAND_Get_Features(0x01, p1.b);
			NAND_Get_Features(0x10, p2.b);

			SEGGER_RTT_printf(0, "FE1:0x%02X FE2:0x%02X - ", p1.d, p2.d);

			NandParamPage &np = nandParamPage[chip];

			NAND_Read_PARAM(&np);

			SEGGER_RTT_printf(0, "mode:0x%02X - ", np.timing_mode_support);

			u16 crc = GetCRC16_8005_refl(&np, sizeof(np)-2, 0x4F4E, true);

			nandSize.integrityCRC[chip] = crc;

			SEGGER_RTT_printf(0, "CRC:0x%04X - ", crc);

			if (np.integrityCRC == crc/* || np.integrityCRC == 0xA61F*/)
			{
				byte col_bits = clog2(np.numberDataBytesPerPage);
				byte page_off = col_bits;
				byte page_bits = clog2(np.numberPagesPerBlock);
				byte chip_off = page_off + page_bits;
				byte block_bits = clog2(np.numberBlocksPerLUN * np.numberLUNsPerChip);

				chipSize = 1UL << (col_bits + page_bits + block_bits - 20);

				if (nandSize.mask == 0)
				{
					FLADR::COL_BITS 	= col_bits;
					FLADR::PAGE_OFF		= page_off;		
					FLADR::CHIP_OFF		= chip_off;
					FLADR::PAGE_BITS	= page_bits;
					FLADR::BLOCK_BITS	= block_bits;

					nandSize.ch = 1ULL << (FLADR::COL_BITS+FLADR::PAGE_BITS+FLADR::BLOCK_BITS);
				};
				
				nandSize.fl += nandSize.ch;
				
				if (bitMask == 0) { nandSize.mask |= (1 << chip); };

				SEGGER_RTT_printf(0, "%u MB - ", chipSize);
			};
		};
		
		SEGGER_RTT_WriteString(0, (nandSize.mask & (1 << chip)) ? (RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n") : (RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n"));
	};

	if (nandSize.ch != 0)
	{
		FLADR::BLOCK_OFF		= FLADR::CHIP_OFF + NAND_CHIP_BITS;
		FLADR::COL_MASK			= (1UL<<FLADR::COL_BITS)-1;
		FLADR::PAGE_MASK		= (1UL<<FLADR::PAGE_BITS)-1;
		FLADR::CHIP_MASK		= NAND_CHIP_MASK;
		FLADR::BLOCK_MASK		= (1UL<<FLADR::BLOCK_BITS)-1;
		FLADR::RAWPAGE_MASK		= (1UL<<(FLADR::PAGE_BITS+NAND_CHIP_BITS+FLADR::BLOCK_BITS))-1;
		FLADR::RAWBLOCK_MASK	= (1UL<<(NAND_CHIP_BITS+FLADR::BLOCK_BITS))-1;
		FLADR::RAWADR_MASK		= (1ULL<<(FLADR::COL_BITS+FLADR::PAGE_BITS+NAND_CHIP_BITS+FLADR::BLOCK_BITS))-1;
		FLADR::pg				= 1UL << FLADR::COL_BITS;
	};

	nandSize.mask &= testNandChipMask;

	FLADR::InitVaildTables(nandSize.mask);

	NAND_Chip_Disable();

	NAND_DisableWriteProtect();

#else

	printf("Open file %s ... ", nameNandFile);

	handleNandFile = CreateFile(nameNandFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, 0);
	//handleNandFile = CreateFile(nameNandFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0 , 0);

	cputs((handleNandFile == INVALID_HANDLE_VALUE) ? "!!! ERROR !!!\n" : "OK\n");

	FLADR::PAGE_OFF			= FLADR::COL_BITS = NAND_COL_BITS;
	FLADR::PAGE_BITS		= NAND_PAGE_BITS;
	FLADR::CHIP_OFF			= FLADR::PAGE_OFF + FLADR::PAGE_BITS;
	FLADR::BLOCK_BITS		= NAND_BLOCK_BITS;
	FLADR::BLOCK_OFF		= FLADR::CHIP_OFF + NAND_CHIP_BITS;
	FLADR::COL_MASK			= (1UL<<FLADR::COL_BITS)-1;
	FLADR::PAGE_MASK		= (1UL<<FLADR::PAGE_BITS)-1;
	FLADR::CHIP_MASK		= NAND_CHIP_MASK;
	FLADR::BLOCK_MASK		= (1UL<<FLADR::BLOCK_BITS)-1;
	FLADR::RAWPAGE_MASK		= (1UL<<(FLADR::PAGE_BITS+NAND_CHIP_BITS+FLADR::BLOCK_BITS))-1;
	FLADR::RAWBLOCK_MASK	= (1UL<<(NAND_CHIP_BITS+FLADR::BLOCK_BITS))-1;
	FLADR::RAWADR_MASK		= (1ULL<<(FLADR::COL_BITS+FLADR::PAGE_BITS+NAND_CHIP_BITS+FLADR::BLOCK_BITS))-1;
	FLADR::pg				= 1UL << FLADR::COL_BITS;

	//nandSize.pg = 1 << (nandSize.bitCol = NAND_COL_BITS);
	//nandSize.bl = 1 << (nandSize.shBl = NAND_COL_BITS+NAND_PAGE_BITS);
	nandSize.ch = 1ULL << (NAND_COL_BITS+NAND_PAGE_BITS+NAND_BLOCK_BITS);
	
	//nandSize.pagesInBlock = 1 << (nandSize.bitPage = nandSize.shBl - nandSize.shPg);

	//nandSize.maskPage = nandSize.pagesInBlock - 1;
	//nandSize.maskBlock = (1 << (nandSize.bitBlock = nandSize.shCh - nandSize.shBl)) - 1;
			
	nandSize.fl = nandSize.ch * NAND_MAX_CHIP;
			
	nandSize.mask = (1 << NAND_MAX_CHIP) - 1;

	cputs("Alloc nandEraseFillArray ... ");

	nandEraseFillArray = VirtualAlloc(0, nandEraseFillArraySize = 1UL << (NAND_COL_BITS+NAND_PAGE_BITS+1), MEM_COMMIT, PAGE_READWRITE);

	cputs((nandEraseFillArray == NULL) ? "!!! ERROR !!!\n" : "OK\n");

	if (nandEraseFillArray != 0) for (u32 i = nandEraseFillArraySize/4, *p = (u32*)nandEraseFillArray; i != 0; i--) *(p++) = ~0;

	NAND_Test_FLADR();

	FLADR::InitVaildTables(nandSize.mask);

	
	cputs("Create thread 'writeThread' ... ");

	handleWriteThread = CreateThread(0, 0, NAND_WriteThread, 0, 0, 0);

	cputs((handleWriteThread == INVALID_HANDLE_VALUE) ? "!!! ERROR !!!\n" : "OK\n");

	cputs("Create thread 'readThread' ... ");

	handleReadThread = CreateThread(0, 0, NAND_ReadThread, 0, 0, 0);

	cputs((handleReadThread == INVALID_HANDLE_VALUE) ? "!!! ERROR !!!\n" : "OK\n");

	for (u32 i = 0; i < ArraySize(_blockBuf); i++) freeBlockBuffer.Add(&(_blockBuf[i]));


#endif
}

//#pragma pop

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	

//void NAND_WriteDataDMA_old(volatile void *src, u16 len)
//{
//	using namespace HW;
//
//	nandTC->CTRLA = 0;
//	nandTC->CTRLA = TC_SWRST;
//
//	DmaTable[0].SRCADDR = (byte*)src+len;
//	DmaTable[0].DSTADDR = &HW::PIOA->OUT;
//	DmaTable[0].DESCADDR = 0;
//	DmaTable[0].BTCNT = len;
//	DmaTable[0].BTCTRL = DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_SRCINC|DMDSC_EVOSEL_BEAT;
//
//	DMAC->CH[0].EVCTRL = DMCH_EVOE;
//	DMAC->CH[0].PRILVL = DMCH_PRILVL_LVL3;
//	DMAC->CH[0].CTRLA = DMCH_ENABLE|DMCH_TRIGACT_BURST|DMCH_TRIGSRC_TC0_MC0;
//
//	PIO_WE_RE->EVCTRL.EV[0] = PIN_WE|PORT_PORTEI|PORT_EVACT_SET;
//	PIO_WE_RE->EVCTRL.EV[1] = PIN_WE|PORT_PORTEI|PORT_EVACT_CLR;
//	PIO_WE_RE->EVCTRL.EV[2] = PIN_WE|PORT_PORTEI|PORT_EVACT_SET;
//
//	while(nandTC->SYNCBUSY & TC_SWRST);
//
//	nandTC->CTRLA = TC_MODE_COUNT8;
//	nandTC->WAVE = TC_WAVEGEN_NPWM;
//	nandTC->PER8 = 250;
//	nandTC->CC8[0] = 1; 
//
//	NAND_DIR_OUT();
//
//	nandTC->EVCTRL = TC_OVFEO|TC_MCEO0|TC_TCEI|TC_EVACT_RETRIGGER;
//
//	nandTC->CTRLBSET = TC_ONESHOT;
//	nandTC->CTRLA = TC_MODE_COUNT8|TC_ENABLE;
//
//	DMAC->SWTRIGCTRL = 1;
//}

#elif defined(CPU_XMC48)
#endif


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void NAND_WriteDataDMA(volatile void *src, u16 len)
{
#ifndef WIN32

	using namespace HW;

	#ifdef CPU_SAME53	

		NAND_DMA.SetEvCtrl(DMCH_EVOE);
		NAND_DMA.WritePeripheral(src, &HW::PIOA->OUT, len, DMCH_TRIGACT_BURST|NAND_TRIGSRC_MC1, DMDSC_BEATSIZE_BYTE|DMDSC_EVOSEL_BEAT);

		#ifdef nandTCC
			nandTCC->WAVE = TCC_WAVEGEN_NPWM|TCC_POL0;
			nandTCC->DRVCTRL = TCC_NRE0|TCC_NRE1|TCC_NRV0|TCC_NRV1;
			nandTCC->PER	= NAND_WE_PER;
			nandTCC->CC[0]	= NAND_WE_CC0;
			nandTCC->CC[1]	= NAND_WE_CC1;

			nandTCC->EVCTRL = TCC_OVFEO|TCC_MCEO1|TCC_TCEI1|TCC_EVACT1_RETRIGGER;
		#else
			nandTC->CTRLA = TC_MODE_COUNT8;
			nandTC->WAVE = TC_WAVEGEN_NPWM;
			nandTC->DRVCTRL = TC_INVEN0|TC_INVEN1;
			nandTC->PER8 = 250;
			nandTC->CC8[0] = NS2CLK(25); 
			nandTC->CC8[1] = 1; 

			nandTC->EVCTRL = TC_OVFEO|TC_MCEO1|TC_TCEI|TC_EVACT_RETRIGGER;
		#endif

		NAND_DIR_OUT();
		PIO_WE_RE->SET(WE|RE); 
		PIO_WE_RE->DIRSET = WE|RE;
		
		#ifdef nandTCC
			PIO_WE_RE->PINCFG[PIN_WE] = PINGFG_PMUXEN|PIN_WE_CFG;

			nandTCC->CTRLBSET = TC_ONESHOT;
			nandTCC->CTRLA = TC_ENABLE;
		#else
			nandTC->CTRLBSET = TC_ONESHOT;
			nandTC->CTRLA = TC_MODE_COUNT8|TC_ENABLE;

			PIO_WE_RE->PINCFG[PIN_WE] = PINGFG_PMUXEN|PIN_WE_CFG;
		#endif

		NAND_DMA.SoftwareTrigger(); // DMAC->SWTRIGCTRL = 1UL<<NAND_DMACH;

	#elif defined(CPU_XMC48)

		NAND_DMA.MemCopySrcInc(src, FLD, len);

	#endif

#else

	BlockBuffer* &bb = curNandBlockBuffer[curRawBlock&3];

	if (bb != 0 && bb->block == curRawBlock)
	{
		DataPointer s((void*)src);
		DataPointer d(bb->data);

		d.b += (u32)(NAND_PAGE_SIZE + NAND_SPARE_SIZE) * curPage + curCol;

		curCol += len;

		while (len > 3) *d.d++ = *s.d++, len -= 4;
		while (len > 0) *d.b++ = *s.b++, len -= 1;

		bb->writed = 1;
	};

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void NAND_WriteDataPIO(volatile void *src, u16 len)
//{
//	//using namespace HW;
//
//	byte* p = (byte*)src;
//
//	#ifdef CPU_SAME53	
//
//		PIO_WE_RE->PINCFG[PIN_WE] &= ~PINGFG_PMUXEN;
//		PIO_WE_RE->PINCFG[PIN_RE] &= ~PINGFG_PMUXEN;
//
//		NAND_DIR_OUT();
//
//		while(len != 0) { NAND_WRITE(*(p++)); len--; };
//
//	#elif defined(CPU_XMC48)
//
//		while(len != 0) { *FLD = *(p++); len--; };
//
//	#endif
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CPU_SAME53	

//void NAND_ReadDataDMA_old(volatile void *dst, u16 len)
//{
//	using namespace HW;
//
//	nandTC->CTRLA = 0;
//	nandTC->CTRLA = TC_SWRST;
//
//	DmaTable[0].SRCADDR = &PIO_NAND_DATA->IN;
//	DmaTable[0].DSTADDR = (byte*)dst+len;
//	DmaTable[0].DESCADDR = 0;
//	DmaTable[0].BTCNT = len;
//	DmaTable[0].BTCTRL = DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_DSTINC|DMDSC_EVOSEL_BEAT;
//
//	DMAC->CH[0].EVCTRL = DMCH_EVOE;
//	DMAC->CH[0].PRILVL = DMCH_PRILVL_LVL3;
//	DMAC->CH[0].CTRLA = DMCH_ENABLE|DMCH_TRIGACT_BURST|DMCH_TRIGSRC_TC0_MC0;
//
//	PIO_WE_RE->EVCTRL.EV[0] = PIN_RE|PORT_PORTEI|PORT_EVACT_CLR;
//	PIO_WE_RE->EVCTRL.EV[1] = PIN_RE|PORT_PORTEI|PORT_EVACT_SET;
//	PIO_WE_RE->EVCTRL.EV[2] = PIN_RE|PORT_PORTEI|PORT_EVACT_SET;
//
//	while(nandTC->SYNCBUSY & TC_SWRST);
//
//	nandTC->CTRLA = TC_MODE_COUNT8;
//	nandTC->WAVE = TC_WAVEGEN_NPWM;
//	nandTC->PER8 = 250;
//	nandTC->CC8[0] = 1; 
//
//	NAND_DIR_IN();
//	PIO_WE_RE->CLR(RE); 
//
//	nandTC->EVCTRL = TC_OVFEO|TC_MCEO0|TC_TCEI|TC_EVACT_RETRIGGER;
//
//	nandTC->CTRLBSET = TC_ONESHOT;
//	nandTC->CTRLA = TC_MODE_COUNT8|TC_ENABLE;
//
//	DMAC->SWTRIGCTRL = 1;
//}

#elif defined(CPU_XMC48)
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CPU_XMC48	


#endif

void NAND_ReadDataDMA(volatile void *dst, u16 len)
{
#ifndef WIN32

	using namespace HW;

	#ifdef CPU_SAME53	

		NAND_DMA.SetEvCtrl(DMCH_EVOE);

		#ifdef nandTCC
						NAND_DMA.ReadPeripheral(&PIO_NAND_DATA->IN, dst, len, DMCH_TRIGACT_BURST|NAND_TRIGSRC_MC0, DMDSC_BEATSIZE_BYTE|DMDSC_EVOSEL_BLOCK);

						nandTCC->WAVE = TCC_WAVEGEN_NPWM|TCC_POL0;
						nandTCC->DRVCTRL = TCC_NRE0|TCC_NRE1|TCC_NRV0|TCC_NRV1;
						nandTCC->PER	= NAND_RE_PER;
						nandTCC->CC[0]	= NAND_RE_CC0;
						nandTCC->CC[1]	= NAND_RE_CC1;

						nandTCC->EVCTRL = TCC_OVFEO|TCC_MCEO0|TCC_TCEI1|TCC_EVACT1_STOP;
		#else
						NAND_DMA.ReadPeripheral(&PIO_NAND_DATA->IN, dst, len, DMCH_TRIGACT_BURST|NAND_TRIGSRC_MC0, DMDSC_BEATSIZE_BYTE|DMDSC_EVOSEL_BEAT);

						nandTC->CTRLA = TC_MODE_COUNT8;
						nandTC->WAVE = TC_WAVEGEN_NPWM;
						nandTC->DRVCTRL = TC_INVEN0|TC_INVEN1;
						nandTC->PER8	= NAND_RE_PER;
						nandTC->CC8[0]	= 1;//NS2CLK(5); 
						nandTC->CC8[1]	= NAND_RE_CC1; 

						nandTC->EVCTRL = TC_OVFEO|TC_MCEO0|TC_TCEI|TC_EVACT_RETRIGGER;
		#endif

		NAND_DIR_IN();
		PIO_WE_RE->SET(WE|RE); 
		PIO_WE_RE->DIRSET = WE|RE;

		#ifdef nandTCC
						PIO_WE_RE->PINCFG[PIN_RE] = PINGFG_PMUXEN|PIN_RE_CFG;

						nandTCC->CTRLA = TCC_ENABLE;
						nandTCC->CTRLBSET = TCC_CMD_RETRIGGER;
		#else
						nandTC->CTRLA = TC_MODE_COUNT8|TC_ENABLE;

						PIO_WE_RE->PINCFG[PIN_RE] = PINGFG_PMUXEN|PIN_RE_CFG;

						nandTC->CTRLBSET = TC_ONESHOT|TC_CMD_RETRIGGER;
		#endif

	#elif defined(CPU_XMC48)

		NAND_DMA.MemCopyDstInc(FLD, dst, len);

	#endif

#else

	BlockBuffer* &bb = curNandBlockBuffer[curRawBlock&3];

	if (bb != 0 && bb->block == curRawBlock)
	{
		DataPointer d((void*)dst);
		DataPointer s(bb->data);

		s.b += (u32)(NAND_PAGE_SIZE + NAND_SPARE_SIZE) * curPage + curCol;

		curCol += len;

		while (len > 3) *d.d++ = *s.d++, len -= 4;
		while (len > 0) *d.b++ = *s.b++, len -= 1;
	};

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void NAND_ReadDataPIO(volatile void *dst, u16 len)
//{
//	//using namespace HW;
//
//	byte* p = (byte*)dst;
//
//	#ifdef CPU_SAME53
//
//		PIO_WE_RE->PINCFG[PIN_WE] &= ~PINGFG_PMUXEN;
//		PIO_WE_RE->PINCFG[PIN_RE] &= ~PINGFG_PMUXEN;
//
//		NAND_DIR_IN();
//
//	#elif defined(CPU_XMC48)
//		
//		while(len != 0) { *(p++) = *FLD; len--; };
//
//	#endif
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CPU_XMC48	

static byte *nandCopyDataSrc = 0;
static byte *nandCopyDataDst = 0;
static u16	nandCopyDataLen = 0;

#endif

void NAND_CopyDataDMA(volatile void *src, volatile void *dst, u16 len)
{
#ifndef WIN32

	NAND_MEMCOPY_DMA.MemCopy(src, dst, len);

#else

	DataPointer s((void*)src);
	DataPointer d((void*)dst);

	while (len > 3) *d.d++ = *s.d++, len -= 4;
	while (len > 0) *d.b++ = *s.b++, len -= 1;

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool NAND_CheckCopyComplete()
{
#ifndef WIN32

	return NAND_MEMCOPY_DMA.CheckMemCopyComplete();

#elif defined(WIN32)

	return true;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // NAND_IMP_H__06_05_2022__19_15
