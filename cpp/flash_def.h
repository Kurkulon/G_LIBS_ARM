#ifndef FLASH_DEF_H__20_04_2022__10_55
#define FLASH_DEF_H__20_04_2022__10_55

#include "types.h"
//#include "core.h"
#include "time.h"
#include "CRC16.h"

//#define NAND_SAMSUNG
#define NAND_MICRON

//#define NAND_CHIP_BITS		3
#define NAND_MAX_CHIP		(1<<3)
//#define NAND_CHIP_MASK		(NAND_MAX_CHIP-1)

#define K9K8_CHIP_BITS		3
#define K9K8_MAX_CHIP		(1<<3)
#define K9K8_CHIP_MASK		(K9K8_MAX_CHIP-1)
#define K9K8_COL_BITS		11
#define K9K8_BLOCK_BITS		13
#define K9K8_PAGE_BITS		6
#define K9K8_SPARE_SIZE		64
#define K9K8_PAGE_SIZE		(1 << K9K8_COL_BITS)
#define K9K8_RAWPAGE_MASK	((1 << (K9K8_PAGE_BITS + K9K8_CHIP_BITS + K9K8_BLOCK_BITS)) - 1)
#define K9K8_RAWBLOCK_MASK	((1 << (K9K8_CHIP_BITS + K9K8_BLOCK_BITS)) - 1)
#define K9K8_RAWADR_MASK	(((u64)1 << (K9K8_COL_BITS + K9K8_PAGE_BITS + K9K8_CHIP_BITS + K9K8_BLOCK_BITS)) - 1)

#define MT29_CHIP_BITS		3
#define MT29_MAX_CHIP		(1<<3)
#define MT29_CHIP_MASK		(MT29_MAX_CHIP-1)
#define MT29_COL_BITS		13
#define MT29_BLOCK_BITS		12
#define MT29_PAGE_BITS		7
#define MT29_SPARE_SIZE		448
#define MT29_PAGE_SIZE		(1 << MT29_COL_BITS)
#define MT29_RAWPAGE_MASK	((1 << (MT29_PAGE_BITS + MT29_CHIP_BITS + MT29_BLOCK_BITS)) - 1)
#define MT29_RAWBLOCK_MASK	((1 << (MT29_CHIP_BITS + MT29_BLOCK_BITS)) - 1)
#define MT29_RAWADR_MASK	(((u64)1 << (MT29_COL_BITS + MT29_PAGE_BITS + MT29_CHIP_BITS + MT29_BLOCK_BITS)) - 1)

#ifdef NAND_SAMSUNG

//#define NAND_MAX_CHIP			K9K8_MAX_CHIP	
//#define NAND_CHIP_MASK		K9K8_CHIP_MASK	
//#define NAND_CHIP_BITS		K9K8_CHIP_BITS	
#define NAND_COL_BITS		K9K8_COL_BITS			
#define NAND_BLOCK_BITS		K9K8_BLOCK_BITS		
#define NAND_PAGE_BITS		K9K8_PAGE_BITS		
#define NAND_SPARE_SIZE		K9K8_SPARE_SIZE		
#define NAND_PAGE_SIZE		K9K8_PAGE_SIZE		
#define NAND_RAWPAGE_MASK	K9K8_RAWPAGE_MASK	
#define NAND_RAWBLOCK_MASK	K9K8_RAWBLOCK_MASK	
#define NAND_RAWADR_MASK	K9K8_RAWADR_MASK	
#define nandType			K9K8 

#elif defined(NAND_MICRON)

//#define NAND_MAX_CHIP			MT29_MAX_CHIP
//#define NAND_CHIP_MASK		MT29_CHIP_MASK
//#define NAND_CHIP_BITS		MT29_CHIP_BITS
#define NAND_COL_BITS		MT29_COL_BITS		
#define NAND_BLOCK_BITS		MT29_BLOCK_BITS		
#define NAND_PAGE_BITS		MT29_PAGE_BITS		
#define NAND_SPARE_SIZE		MT29_SPARE_SIZE		
#define NAND_PAGE_SIZE		MT29_PAGE_SIZE		
#define NAND_RAWPAGE_MASK	MT29_RAWPAGE_MASK	
#define NAND_RAWBLOCK_MASK	MT29_RAWBLOCK_MASK	
#define NAND_RAWADR_MASK	MT29_RAWADR_MASK	
#define nandType			MT29 

#endif


#define NAND_SR_FAIL				0x01	// 0 = Pass, 1 = Fail
#define NAND_SR_FAILC				0x02	// 0 = Pass, 1 = Fail
#define NAND_SR_ARDY				0x20	// 0 = Busy, 1 = Ready
#define NAND_SR_RDY					0x40	// 0 = Busy, 1 = Ready
#define NAND_SR_WP					0x80	// 0 = Protected, 1 = Not Protected

//#define FRAM_SPI_MAINVARS_ADR 0
//#define FRAM_SPI_SESSIONS_ADR 0x200
//
//#define FRAM_I2C_MAINVARS_ADR 0
//#define FRAM_I2C_SESSIONS_ADR 0x200

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct NandID
{
 	byte maker;
 	byte device;
	byte data0;

	__packed union
	{
		__packed struct 
		{
			u16 pageSize			: 2;
			u16 z_reserved1			: 2;
			u16 blockSize			: 2;
			u16 z_reserved2			: 4;
			u16 planeNumber			: 2;
			u16 planeSize			: 3;
		};

	 	byte data[2];
	};
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct NandMemSize
{
 	u64 ch;	// chip
 	u64 fl;	// full
	u16 mask;

	byte	chipDataBusMask[NAND_MAX_CHIP]; // ≈сли проблема по линии данных, то соответствующи бит равен 0

	NandID	id[NAND_MAX_CHIP];
	u16		integrityCRC[NAND_MAX_CHIP];
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct FLADR
{
	union
	{
		struct K9K8
		{
			u64		col		: K9K8_COL_BITS;
			u64 	page	: K9K8_PAGE_BITS;
			u64		chip	: K9K8_CHIP_BITS;
			u64 	block	: K9K8_BLOCK_BITS;
			//u64		overflow : (64-(K9K8_COL_BITS+K9K8_PAGE_BITS+NAND_CHIP_BITS+K9K8_BLOCK_BITS));
		} K9K8;

		struct MT29
		{
			u64		col		: MT29_COL_BITS;
			u64 	page	: MT29_PAGE_BITS;
			u64		chip	: MT29_CHIP_BITS;
			u64 	block	: MT29_BLOCK_BITS;
			//u64		overflow : (64-(MT29_COL_BITS+MT29_PAGE_BITS+NAND_CHIP_BITS+MT29_BLOCK_BITS));
		} MT29;

		u32 DW[2];
		u64	raw;
	};

	static byte COL_BITS;
	static byte PAGE_BITS;		
	static byte BLOCK_BITS;		

	//static byte COL_OFF;	// = 0
	static byte PAGE_OFF;	// = COL_BITS;
	static byte CHIP_OFF;	// = PAGE_OFF + PAGE_BITS
	static byte BLOCK_OFF;	// = CHIP_OFF + NAND_CHIP_BITS 		

	static u32 COL_MASK;
	static u32 PAGE_MASK;		
	static u32 CHIP_MASK;		
	static u32 BLOCK_MASK;		

	static u32 RAWPAGE_MASK;	
	static u32 RAWBLOCK_MASK;	

	static u64 RAWADR_MASK;

	static u32 pg; //enum { pg = (1<<NAND_COL_BITS) };
//	u32		rawpage;

//	const NandMemSize& sz;

	static byte	chipValidNext[NAND_MAX_CHIP]; // ≈сли чип битый, то по индексу находитс€ следующий хороший чип
	static byte	chipValidPrev[NAND_MAX_CHIP]; // ≈сли чип битый, то по индексу находитс€ предыдущий хороший чип

	static u32	chipOffsetNext[NAND_MAX_CHIP]; // ≈сли чип битый, то по индексу находитс€ смещение адреса на следующий хороший чип
	static u32	chipOffsetPrev[NAND_MAX_CHIP]; // ≈сли чип битый, то по индексу находитс€ смещение адреса на предыдущий хороший чип

	inline void operator=(const FLADR &a) { raw = a.raw; }

	void SetCol(u32 c)		{ DW[0] &= ~COL_MASK;						DW[0] |= c & COL_MASK; }
	void SetPage(u32 p)		{ DW[0] &= ~(PAGE_MASK << PAGE_OFF);		DW[0] |= (p & PAGE_MASK) << PAGE_OFF; }
	void SetChip(u32 c)		{ DW[0] &= ~(CHIP_MASK << CHIP_OFF);		DW[0] |= (c & CHIP_MASK) << CHIP_OFF; }
	void SetBlock(u32 b)	{ raw   &= ~((u64)BLOCK_MASK << BLOCK_OFF);	raw   |= (u64)(b & BLOCK_MASK) << BLOCK_OFF; }

	u16 GetCol()	{ return DW[0] & COL_MASK; }
	u16 GetPage()	{ return (DW[0] >> PAGE_OFF) & PAGE_MASK; }
	u16 GetChip()	{ return (DW[0] >> CHIP_OFF) & CHIP_MASK; }
	u16 GetBlock()	{ return (raw >> BLOCK_OFF) & BLOCK_MASK; }
	

	FLADR() : raw(0) {}
	//FLADR(u32 bl, u16 pg, u16 cl, byte ch) : block(bl), page(pg), col(cl), chip(ch) {}
	FLADR(u32 pg) { SetRawPage(pg); }

	static void	InitVaildTables(u16 mask);

	u32		GetRawPage() { return (raw & RAWADR_MASK) >> PAGE_OFF; }

	void	SetRawPage(u32 p) { raw = (u64)(p & RAWPAGE_MASK) << PAGE_OFF; };

	u32		GetRawBlock() { return (raw & RAWADR_MASK) >> CHIP_OFF; }

	void	SetRawBlock(u32 b) { raw = (u64)(b & RAWBLOCK_MASK) << CHIP_OFF; };

	u64		GetRawAdr()	{ return raw & RAWADR_MASK; };
	void	SetRawAdr(u64 a) { raw  = a & RAWADR_MASK; };

	void	NextPage()	{ DW[0] &= ~COL_MASK; raw += 1UL << COL_BITS; raw += chipOffsetNext[GetChip()]; }
	void	NextBlock()	{ DW[0] &= ~((PAGE_MASK << PAGE_OFF)|COL_MASK); raw += 1UL << CHIP_OFF; raw += chipOffsetNext[GetChip()];}
	void	PrevPage()	{ raw -= 1UL << COL_BITS; DW[0] &= ~COL_MASK; raw -= chipOffsetPrev[GetChip()]; }
	void	PrevBlock()	{ raw -= 1UL << CHIP_OFF; DW[0] &= ~((PAGE_MASK << PAGE_OFF)|COL_MASK); raw -= chipOffsetPrev[GetChip()];}

	void	AddRaw(u32 v) { raw += v; raw += chipOffsetNext[GetChip()]; }
	void	SubRaw(u32 v) { raw -= v; raw -= chipOffsetPrev[GetChip()]; }
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct SpareArea
{
	u16		validPage;	// 0xFFFF - good page
	u16		validBlock;	// 0xFFFF - good block
	u16		badPages;	// 0xFFFF - all pages good

	enum { CRC_SKIP = 6 };

	__packed union
	{
		__packed struct //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		{
			u16		file;  			// file number

			u32		start;			// start page of file
			u32		fpn;			// file page number
			u32		prev;			// start page of previos file
			u32		rawPage;		// raw page num

			u32		vectorCount;	// vector count in file

			u16		vecFstOff;		// first vector header offset. 0xFFFF - no vector header 
			u16		vecFstLen;		// first vector lenght. 0 - no vector

			u16		vecLstOff;		// last vector header offset. 0xFFFF - no vector header 
			u16		vecLstLen;		// last vector lenght. 0 - no vector

			u16		fbb;			// file bad blocks count
			u16		fbp;			// file bad pages count

			byte	chipMask;	

			u16		crc;

			void CheckCRC() { crc = GetCRC16(this, sizeof(*this)); }
			void UpdateCRC() { crc = GetCRC16(this, sizeof(*this) - sizeof(crc)); }

		} v1;

		__packed struct //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		{
			u16		len;			// struct lenght
			u16		version;		// struct version

			u16		file;  			// file number

			u32		start;			// start page of file
			u32		fpn;			// file page number
			u32		prev;			// start page of previos file
			u32		rawPage;		// raw page num

			u32		vectorCount;	// vector count in file

			u16		vecFstOff;		// first vector header offset. 0xFFFF - no vector header 
			u16		vecFstLen;		// first vector lenght. 0 - no vector

			u16		vecLstOff;		// last vector header offset. 0xFFFF - no vector header 
			u16		vecLstLen;		// last vector lenght. 0 - no vector

			//u32		vecPrPg;	// previos vector raw page
			//u16		vecPrOff;	// previos vector offset

			u16		fbb;			// file bad blocks count
			u16		fbp;			// file bad pages count

			byte	chipMask;	

			u16		crc;

			void CheckCRC() { crc = GetCRC16(this, sizeof(*this)); }
			void UpdateCRC() { crc = GetCRC16(this, sizeof(*this) - sizeof(crc)); }

		} v2;
	};
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern bool NAND_BUSY(); 
extern bool NAND_CmdBusy();
extern void NAND_WriteDataDMA(volatile void *src, u16 len);
extern void NAND_WriteDataPIO(volatile void *src, u16 len);
extern void NAND_ReadDataDMA(volatile void *dst, u16 len);
extern void NAND_ReadDataDMA2(volatile void *dst, u16 len);
extern void NAND_ReadDataPIO(volatile void *dst, u16 len);
extern bool NAND_CheckDataComplete();
extern void NAND_Chip_Select(byte chip);
extern void NAND_Chip_Disable();
extern void NAND_WRITE(byte data);
extern void NAND_CopyDataDMA(volatile void *src, volatile void *dst, u16 len);
extern bool NAND_CheckCopyComplete();

extern void NAND_CmdEraseBlock(u32 bl);
extern void NAND_CmdRandomRead(u16 col);
extern void NAND_CmdReadPage(u16 col, u32 bl, u16 pg);
extern void NAND_CmdWritePage(u16 col, u32 bl, u16 pg);
extern void NAND_CmdWritePage2();
extern byte NAND_CmdReadStatus();

inline const NandMemSize* NAND_GetMemSize() { extern NandMemSize nandSize; return &nandSize; } 
inline u32 NAND_GetGoodChipMask() { extern NandMemSize nandSize; return nandSize.mask; } 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef WIN32

extern int PutString(u32 x, u32 y, byte c, const char *str);
extern int Printf(u32 x, u32 y, byte c, const char *format, ... );

//extern void NAND_FlushBlockBuffers();
//extern void NAND_ReqFlushBlockBuffers();

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // FLASH_DEF_H__20_04_2022__10_55
