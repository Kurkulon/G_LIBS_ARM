#include "flash.h"
#include "vector.h"
#include "list.h"
#include "trap_def.h"
#include "trap.h"
#include "PointerCRC.h"
#include "xtrap.h"
#include "SEGGER_RTT.h"

#include "flash_def.h"
#include "extern_def.h"

#include "i2c.h"
#include "spi.h"

#pragma diag_suppress 550,177

//#pragma O3
//#pragma Otime


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u32 FRAM_SPI_SESSIONS_ADR = 0;
static u32 FRAM_I2C_SESSIONS_ADR = 0;

static bool FRAM_SPI_PRESENT = false;
static bool FRAM_I2C_PRESENT = false;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#pragma push
#pragma O3
#pragma Otime

static bool __memcmp(ConstDataPointer s, ConstDataPointer d, u32 len)
{
	while (len > 3)
	{
		if (*s.d++ != *d.d++) return false;
		len -= 4;
	};

	while (len > 0)
	{
		if (*s.b++ != *d.b++) return false;
		len -= 1;
	};

	return true;
}

#pragma pop

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//#define FLASH_SAVE_BUFFER_SIZE		8400
//#define FLASH_READ_BUFFER_SIZE		8400

//static PtrFLWB flashPtrFLWB_Buffer[16];


List<ListItem> ListItem::freeItemList;
static ListItem listItems[128];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void UNIBUF::_FreeCallBack() {  }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//List<UNIBUF> UNIBUF::freeBufList;
static UNIBUF flashWriteBuffer[8];
static FLRB flashReadBuffer[8];

//List<PtrFLWB> PtrFLWB::freePtrList;

//static List<FLWB> freeFlWrBuf;
static ListRef<UNIBUF> writeFlBuf;

static List<FLRB> freeFlRdBuf;
static List<FLRB> readFlBuf;

static FLRB *curRdBuf = 0;

struct PageBuffer { PageBuffer *next; u32 page; u32 prevPage; byte data[NAND_PAGE_SIZE]; SpareArea spare; };

static PageBuffer _pageBuf[2];

static List<PageBuffer> freePageBuffer;
static List<PageBuffer> readyPageBuffer;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static SpareArea spareRead;
//static SpareArea spareWrite;
//static SpareArea spareErase;

static u32 pagesRead = 0;
static u32 pagesReadOK = 0;
static u32 pagesReadErr = 0;

static u64 adrLastVector = -1;
static u32 lastSessionBlock = -1;
static u32 lastSessionPage = -1;

static bool cmdCreateNextFile = false;
static bool cmdFullErase = false;
static bool cmdSendSession = false;

static bool writeFlashEnabled = false;
static bool flashFull = false;
static bool flashEmpty = false;

static bool testWriteFlash = false;

static const bool verifyWritePage = true; // �������� ��������� ��������, ���� ������ �������� � ��������� � �������

#ifndef WIN32
static const bool forceEraseWrite = false;
#else
static const bool forceEraseWrite = true;
#endif

static SessionInfo lastSessionInfo;

u16 deviceID = 0;

static u64 flashUsedSize = 0;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct NVV // NonVolatileVars  
{
	u32 timeStamp;

	FileDsc f;

	u16 index;

	u32 prevFilePage;

	u32 badBlocks[8];
	u32 pageError[8];
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct NVSI // NonVolatileSessionInfo  
{
	u32 timeStamp;

	FileDsc f;

	u16 crc;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static NVV nvv;

static NVSI nvsi[128];

static byte buf[sizeof(nvv)*2+4];

byte savesCount = 0;

byte savesSessionsCount = 0;

byte eraseSessionsCount = 0;

//static TWI	twi;

static void SaveVars();

static bool loadVarsOk = false;
static bool loadSessionsOk = false;

bool fram_I2C_flashVarsOk = false;
bool fram_SPI_flashVarsOk = false;
bool fram_I2C_loadSessionsOk = false;
bool fram_SPI_loadSessionsOk = false;

u32 verifyFlashErrors = 0;

inline void SaveParams() { savesCount = 1; }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


const SessionInfo* GetLastSessionInfo()
{
	return &lastSessionInfo;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

enum flash_status_type
{
	FLASH_STATUS_WAIT = 0,
	FLASH_STATUS_WRITE,
	FLASH_STATUS_READ,
//	FLASH_STATUS_ERASE
};

static byte flashStatus = FLASH_STATUS_WAIT;

enum NandState
{
	NAND_STATE_WAIT = 0,
	NAND_STATE_WRITE_START,
	NAND_STATE_READ_START,
	NAND_STATE_FULL_ERASE_START,
	NAND_STATE_FULL_ERASE_0,
	NAND_STATE_CREATE_FILE,
	NAND_STATE_SEND_SESSION,
	NAND_STATE_SEND_BAD_BLOCKS,
	NAND_STATE_SEND_PAGE_ERRORS,
	NAND_STATE_SEND_DATABUS_MASK
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static NandState nandState = NAND_STATE_WAIT;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static byte lastFlashStatus = FLASH_STATUS_NONE;
static u32 lastFlashProgress = -1;
static u32 lastFlashTime = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static const NandMemSize *nandSZ;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static u32 invalidBlocks = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static ComPort com1;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

byte	FLADR::chipValidNext[FLASH_MAX_CHIP]; // ���� ��� �����, �� �� ������� ��������� ��������� ������� ���
byte	FLADR::chipValidPrev[FLASH_MAX_CHIP]; // ���� ��� �����, �� �� ������� ��������� ���������� ������� ���
u32		FLADR::chipOffsetNext[FLASH_MAX_CHIP]; // ���� ��� �����, �� �� ������� ��������� �������� ������ �� ��������� ������� ���
u32		FLADR::chipOffsetPrev[FLASH_MAX_CHIP]; // ���� ��� �����, �� �� ������� ��������� �������� ������ �� ���������� ������� ���
byte 	FLADR::COL_BITS;
byte 	FLADR::PAGE_BITS;		
byte 	FLADR::BLOCK_BITS;		
//byte 	FLADR::COL_OFF;		// = 0
byte 	FLADR::PAGE_OFF;	// = COL_BITS;
byte 	FLADR::CHIP_OFF;	// = PAGE_OFF + PAGE_BITS
byte 	FLADR::BLOCK_OFF;	// = CHIP_OFF + NAND_CHIP_BITS 		
u32 	FLADR::COL_MASK;
u32 	FLADR::PAGE_MASK;		
u32 	FLADR::CHIP_MASK;		
u32 	FLADR::BLOCK_MASK;		
u32 	FLADR::RAWPAGE_MASK;	
u32 	FLADR::RAWBLOCK_MASK;	
u64 	FLADR::RAWADR_MASK;
u32 	FLADR::pg; //enum { pg = (1<<NAND_COL_BITS) };

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void	FLADR::InitVaildTables(u16 mask)
{
	u32 blocksize = 1UL << CHIP_OFF;

	for(byte chip = 0; chip < FLASH_MAX_CHIP; chip++)
	{
		chipValidNext[chip] = 0;
		chipValidPrev[chip] = 0;

		u32 offset = 0;

		for (byte i = 0; i < FLASH_MAX_CHIP; i++)
		{
			byte cn = chip+i; if (cn >= FLASH_MAX_CHIP) cn = 0;

			if (mask & (1<<cn))
			{
				chipValidNext[chip] = cn;
				chipOffsetNext[chip] = offset;
				break;
			};

			offset += blocksize;
		};

		offset = 0;

		for (byte i = 0; i < FLASH_MAX_CHIP; i++)
		{
			byte cp = chip-i; if (cp >= FLASH_MAX_CHIP) cp = FLASH_MAX_CHIP - 1;

			if (mask & (1<<cp))
			{
				chipValidPrev[chip] = cp;
				chipOffsetPrev[chip] = offset;
				break;
			};
			
			offset += blocksize;
		};
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool FLASH_SendStatus(u32 progress, byte status)
{
	lastFlashProgress = progress;
	lastFlashStatus = status;
	lastFlashTime = GetMilliseconds();
	
	return TRAP_MEMORY_SendStatus(lastFlashProgress, lastFlashStatus);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void FLASH_UpdateStatus()
{
	u32 t = GetMilliseconds();
	u32 dt = t - lastFlashTime;

	if (dt >= 500)
	{
		TRAP_MEMORY_SendStatus(lastFlashProgress, lastFlashStatus);

		lastFlashProgress = ~0;
		lastFlashStatus = FLASH_STATUS_NONE;
		lastFlashTime = t;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitFlashBuffer()
{
	//for (byte i = 0; i < ArraySize(flashWriteBuffer); i++)
	//{
	//	freeFlWrBuf.Add(&flashWriteBuffer[i]);
	//};

	for (byte i = 0; i < ArraySize(flashReadBuffer); i++)
	{
		freeFlRdBuf.Add(&flashReadBuffer[i]);
	};

	for (byte i = 0; i < ArraySize(_pageBuf); i++)
	{
		freePageBuffer.Add(&_pageBuf[i]);
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//FLWB* AllocFlashWriteBuffer()
//{
//	return FLWB::freeBufList.Get();
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void FreeFlashWriteBuffer(FLWB* b)
//{
//	b->Free();
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

FLRB* AllocFlashReadBuffer()
{
	return freeFlRdBuf.Get();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void FreeFlashReadBuffer(FLRB* b)
{
	freeFlRdBuf.Add(b);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool RequestFlashRead(FLRB* b)
{
	if ((b != 0) && (b->data != 0) && (b->maxLen > 0))
	{
		b->ready = false;
		b->len = 0;

		readFlBuf.Add(b);

		return true;
	}
	else
	{
		return false;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void NAND_FullErase()
{
	cmdFullErase = true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void NAND_NextSession()
{
	cmdCreateNextFile = true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ResetNandState()
{
	nandState = NAND_STATE_WAIT;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

NandState GetNandState()
{
	return nandState;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct EraseBlock
{
	enum {	WAIT = 0,ERASE_START,ERASE_0,ERASE_1,ERASE_2,ERASE_3,ERASE_4,ERASE_5 };

	SpareArea spare;
	
	FLADR er;

	byte state;
	bool force;		// ������� ��������
	bool check;		// ��������� ��������� ��������
	u16	errBlocks;

	EraseBlock() : state(WAIT), force(false), check(true), errBlocks(0), er(~0) {}

	void Start(const FLADR &rd, bool frc, bool chk);
	bool Update();
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void EraseBlock::Start(const FLADR &rd, bool frc, bool chk)
{
	er = rd;

	force = frc;
	check = chk;

	errBlocks = 0;

	state = ERASE_START;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool EraseBlock::Update()
{
	switch(state)
	{
		case WAIT:
			
			return false;

		case ERASE_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++		
																																
			NAND_Chip_Select(er.GetChip());

			if (force)
			{
				NAND_CmdEraseBlock(er.GetBlock());																						
																															
				state = ERASE_2;																				
			}
			else
			{
				NAND_CmdReadPage(er.pg, er.GetBlock(), 0);																					
																																	
				state = ERASE_0;																						
			};
																																
			break;																												
																																
		case ERASE_0:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++				
																																
			if(!NAND_BUSY())																									
			{																													
				NAND_ReadDataDMA(&spare, spare.CRC_SKIP);	

				state = ERASE_1;																					
			};																													
																																
			break;																												
																																
		case ERASE_1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++				
																																
			if (NAND_CheckDataComplete())																							
			{																													
				if (spare.validPage != 0xFFFF && spare.validBlock != 0xFFFF)									
				{																												
					errBlocks += 1;	
					nvv.badBlocks[er.GetChip()] += 1;
																																
					er.NextBlock();	

					state = ERASE_START; 
				}	
				else if (spare.badPages != 0xFFFF)
				{
					errBlocks += 1;	
																																
					NAND_CmdWritePage(er.pg, er.GetBlock(), 0);																			
																																
					NAND_WRITE(0); NAND_WRITE(0); NAND_WRITE(0); NAND_WRITE(0); //*(u32*)FLD = 0;		// spareErase.validPage = 0; spareErase.validBlock = 0;																
																																
					NAND_CmdWritePage2();																							
																																
					state = ERASE_3;																				
				}
				else																											
				{																												
					NAND_CmdEraseBlock(er.GetBlock());																						
																																
					state = ERASE_2;																				
				};																												
			};																													
																																
			break;																												
																																
																																
		case ERASE_2:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++				
																																
			if(!NAND_BUSY())																									
			{					
				byte status = NAND_CmdReadStatus();

				if ((status & (NAND_SR_FAIL|NAND_SR_RDY)) != NAND_SR_RDY && check) // erase error																	
				{																												
					errBlocks += 1;	
					nvv.badBlocks[er.GetChip()] += 1;

//					__breakpoint(0);																							
																																
					NAND_CmdWritePage(er.pg, er.GetBlock(), 0);																			
																																
					NAND_WRITE(0);NAND_WRITE(0);NAND_WRITE(0);NAND_WRITE(0);//*(u32*)FLD = 0;		// spareErase.validPage = 0; spareErase.validBlock = 0;																
																																
					NAND_CmdWritePage2();																							
																																
					state = ERASE_3;																				
				}																												
				else																											
				{																												
					//er_block = wr.block;														
					//er_chip = wr.chip;																							
																																
					state = WAIT;		

					return false;
				};																												
			};																													
																																
			break;																												
																																
		case ERASE_3:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++				
																																
			if(!NAND_BUSY())																									
			{																													
				er.NextBlock();	

				state = ERASE_START; 
			};

			break;
	};

	return true;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct Write
{
	enum {	WAIT = 0,				WRITE_START,			WRITE_BUFFER,			WRITE_PAGE,				WRITE_PAGE_0,	WRITE_PAGE_1,
			WRITE_PAGE_2,			WRITE_PAGE_3,			WRITE_PAGE_4,			WRITE_PAGE_5,			WRITE_PAGE_6,	WRITE_PAGE_7,	WRITE_PAGE_8,			
			ERASE,			
			WRITE_CREATE_FILE_0,	WRITE_CREATE_FILE_1,	WRITE_CREATE_FILE_2,	WRITE_CREATE_FILE_3,	WRITE_CREATE_FILE_4 };

	FLADR wr;

	//u16		wr_cur_col;
	//u32 	wr_cur_pg;

	byte	wr_pg_error;
	u16		wr_count;
	VecData *vector;
	byte*	wr_data	;
	void*	wr_ptr	;

	u64		prWrAdr;
	u32		rcvVec ;
	u32		rejVec ;
	u32		errVec ;

	Ptr<UNIBUF> curWrBuf;

	SpareArea spare;

	SpareArea rspare;

	byte state;

	bool createFile;

	EraseBlock eraseBlock;

	u32	wrBuf[NAND_PAGE_SIZE/4];
	//u32	rdBuf[(NAND_PAGE_SIZE+NAND_SPARE_SIZE)/4];

	bool Start();
	bool Update();
	void Vector_Make(VecData *vd, u16 size);
	void Finish();
	void Init();
	void Init(u32 bl, u32 file, u32 prfile);

	void SaveSession();

//	void BufWriteData(void *data, u16 len) { NAND_CopyDataDMA(data, (byte*)wrBuf+wr.col, len); }

	void CreateNextFile() { state = WRITE_CREATE_FILE_0; }


};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static Write write;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Write::Init()
{
	wr.SetRawPage(nvv.f.lastPage);

	spare.v1.file = nvv.f.session;  

	spare.v1.prev = nvv.prevFilePage;		

	spare.v1.start = nvv.f.startPage;		
	spare.v1.fpn = 0;	

	spare.v1.vectorCount = 0;

	spare.v1.vecFstOff = -1;
	spare.v1.vecFstLen = 0;

	spare.v1.vecLstOff = -1;
	spare.v1.vecLstLen = 0;

	spare.v1.fbb = 0;		
	spare.v1.fbp = 0;		

	spare.v1.chipMask = nandSZ->mask;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Write::Init(u32 bl, u32 file, u32 prfile)
{
	wr.SetRawBlock(bl);

	spare.v1.file = file;  
//	spare.v1.lpn = wr.GetRawPage();

	spare.v1.prev = prfile;		

	spare.v1.start = wr.GetRawPage();		
	spare.v1.fpn = 0;	

	spare.v1.vectorCount = 0;

	spare.v1.vecFstOff = -1;
	spare.v1.vecFstLen = 0;

	spare.v1.vecLstOff = -1;
	spare.v1.vecLstLen = 0;

	spare.v1.fbb = 0;		
	spare.v1.fbp = 0;		

	spare.v1.chipMask = nandSZ->mask;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Write::SaveSession()
{
	nvv.index = (nvv.index+1) & 127;

	FileDsc &s = nvv.f;
	NVSI &d = nvsi[nvv.index];

	d.f = nvv.f;

	d.crc = GetCRC16(&d.f, sizeof(d.f));

	s.session = spare.v1.file;
	s.startPage = s.lastPage = wr.GetRawPage();
	s.size = 0;
	GetTime(&s.start_rtc);
	s.stop_rtc = s.start_rtc;
	s.flags = 0;

	SaveParams();
	savesSessionsCount = 1;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Write::Vector_Make(VecData *vd, u16 size)
{
	vd->h.session = spare.v1.file;
//	vd->h.device = 0;
	GetTime(&vd->h.rtc);

	vd->h.prVecAdr = prWrAdr; 
	vd->h.flags = 0;
	vd->h.dataLen = size;
	vd->h.crc = GetCRC16(&vd->h, sizeof(vd->h) - sizeof(vd->h.crc));
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Write::Start()
{
#ifdef WIN32

	static TM32 tm;

	if (tm.Check(100))
	{
		Printf(20, 0, 0xF0, "Session num  %-5hu",	nvv.f.session				);
		Printf(20, 1, 0xF0, "Session size %09llX",	nvv.f.size					);
		Printf(20, 2, 0xF0, "Raw adr      %09llX",	wr.GetRawAdr()				);
		Printf(20, 3, 0xF0, "rcvVec       %lu",		rcvVec						);
		Printf(20, 4, 0xF0, "Writed Vec   %lu",		write.spare.v1.vectorCount	);
		Printf(20, 5, 0xF0, "GetTickCount %hu",		GetMillisecondsLow()		);
	};

#endif

	curWrBuf = writeFlBuf.Get();

	if (curWrBuf.Valid())
	{
		rcvVec += 1;

		if (!writeFlashEnabled || flashFull || curWrBuf->dataOffset < sizeof(VecData::Hdr))
		{
//			rejVec += 1;

//			curWrBuf->ready[0] = true;
			curWrBuf.Free();
			return false;
		};

		vector = (VecData*)(curWrBuf->data + curWrBuf->dataOffset - sizeof(VecData::Hdr));

		Vector_Make(vector, curWrBuf->dataLen);
	
		prWrAdr = wr.GetRawAdr();

		wr_data = (byte*)vector;
		wr_count = vector->h.dataLen + sizeof(vector->h);

		if (spare.v1.vecFstOff == 0xFFFF)
		{
			spare.v1.vecFstOff = wr.GetCol();
			spare.v1.vecFstLen = wr_count;
		};

		spare.v1.vecLstOff = wr.GetCol();
		spare.v1.vecLstLen = wr_count;

		spare.v1.vectorCount += 1;

		state = WRITE_START;

		return true;
	}
	else
	{
		return false;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Write::Finish()
{
	if (curWrBuf.Valid())
	{
		nvv.f.size += vector->h.dataLen + sizeof(vector->h);
		nvv.f.stop_rtc = vector->h.rtc;
		nvv.f.lastPage = spare.v1.rawPage;

		SaveParams();

//		curWrBuf->ready[0] = true;

		curWrBuf.Free();

		//curWrBuf = 0;

		//wr_prev_col = wr_cur_col;
		//wr_prev_pg = wr_cur_pg;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Write::Update()
{
//	u32 t;

	switch(state)
	{
		case WAIT:	return false;

		case WRITE_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	
			
//			NAND_Chip_Select(wr.chip);

			{
				register u16 c = wr.pg - wr.GetCol();


				if (wr.GetCol() == 0 && wr_count >= wr.pg) // ������ ����� �� ����
				{
					wr_ptr = wr_data;
					wr_count -= wr.pg;
					wr_data += wr.pg;

					state = WRITE_PAGE;
				}
				else // ������ � �����
				{
					if (wr_count < c ) c = wr_count;

					NAND_CopyDataDMA(wr_data, (byte*)wrBuf+wr.GetCol(), c);	// BufWriteData(wr_data, c);

					wr.SetCol(wr.GetCol() + c);
					wr_data += c;
					wr_count -= c;

					state = WRITE_BUFFER;
				};
			};

			break;
	
		case WRITE_BUFFER:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (NAND_CheckCopyComplete())
			{
				if (wr.GetCol() == 0)
				{
					wr_ptr = wrBuf;
//					wr.col = 0;

					state = WRITE_PAGE;
				}
				else if (wr_count > 0)
				{
					state = WRITE_START;
				}
				else
				{
					Finish();

					state = WAIT;

					return false;
				};
			};

			break;

		case WRITE_PAGE:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if(eraseBlock.er.GetRawBlock() != wr.GetRawBlock())	// ����� ����
			{
				wr_pg_error = 0;

				eraseBlock.Start(wr, forceEraseWrite, true);

	            state = ERASE;

				break;
			};

		case WRITE_PAGE_0:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			NAND_Chip_Select(wr.GetChip());

			NAND_CmdWritePage(0, wr.GetBlock(), wr.GetPage());

			NAND_WriteDataDMA(wr_ptr, wr.pg);
			
			state = WRITE_PAGE_1;

			break;

		case WRITE_PAGE_1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (NAND_CheckDataComplete())
			{
				spare.validPage = ~0;
				spare.validBlock = ~0;
				spare.badPages = ~0;
				spare.v1.rawPage = wr.GetRawPage();
				spare.v1.chipMask = nandSZ->mask;

				spare.v1.UpdateCRC();

				NAND_WriteDataDMA(&spare, sizeof(spare));

				state = WRITE_PAGE_2;
			};

			break;

		case WRITE_PAGE_2:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (NAND_CheckDataComplete())
			{
				NAND_CmdWritePage2();

				state = WRITE_PAGE_3;
			};

			break;

		case WRITE_PAGE_3:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if(!NAND_BUSY())
			{
				byte status = NAND_CmdReadStatus();

				if ((status & (NAND_SR_FAIL|NAND_SR_RDY)) != NAND_SR_RDY) // program error
				{
					spare.v1.fbp += 1;
					nvv.pageError[wr.GetChip()] += 1;

					NAND_CmdWritePage(wr.pg, wr.GetBlock(), wr.GetPage());

					NAND_WRITE(0);NAND_WRITE(0);//*(u16*)FLD = 0; // spareErase.validPage = 0;

					NAND_CmdWritePage2();

					state = WRITE_PAGE_4;
				}
				else if (verifyWritePage)
				{
					NAND_CmdReadPage(wr.pg, wr.GetBlock(), wr.GetPage());
					
					state = WRITE_PAGE_6;
				}
				else
				{
					state = WRITE_PAGE_8;
				};
			};

			break;

		case WRITE_PAGE_4:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++			

			if(!NAND_BUSY())																									
			{																													
				if (wr_pg_error == 2) // �������� ���� ��� ������� ������� ��������
				{
					NAND_CmdWritePage(wr.pg + sizeof(spare.validPage) + sizeof(spare.validBlock), wr.GetBlock(), 0);

					NAND_WRITE(0);NAND_WRITE(0);//*(u16*)FLD = 0; // spare.badPages = 0;

					NAND_CmdWritePage2();

					state = WRITE_PAGE_5;
				}
				else
				{
					state = WRITE_PAGE;																			
				};

				wr_pg_error += 1;

				wr.NextPage();		
			};																													

			break;

		case WRITE_PAGE_5:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++			
																																
			if(!NAND_BUSY())																									
			{																													
				state = WRITE_PAGE;																			
			};
																																
			break;	

		case WRITE_PAGE_6:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if(!NAND_BUSY())
			{
				NAND_ReadDataDMA(&rspare, sizeof(rspare));

				state = WRITE_PAGE_7;
			};

			break;	

		case WRITE_PAGE_7:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++			

			if (NAND_CheckDataComplete())
			{
				if (!__memcmp(&spare, &rspare, sizeof(spare)))
				{
					verifyFlashErrors += 1;

					spare.v1.fbp += 1;

					NAND_CmdWritePage(wr.pg, wr.GetBlock(), wr.GetPage());

					NAND_WRITE(0);NAND_WRITE(0);//*(u16*)FLD = 0; // spareErase.validPage = 0;

					NAND_CmdWritePage2();

					state = WRITE_PAGE_4;
				}
				else
				{
					state = WRITE_PAGE_8;
				};
			};

			break;

		case WRITE_PAGE_8:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (wr_count == 0)
			{
				Finish();

				state = (createFile) ? WRITE_CREATE_FILE_1 : WAIT;
			}
			else
			{
				state = WRITE_START;
			};

			wr.NextPage();

			spare.v1.fpn += 1;

			spare.v1.vecFstOff = -1;
			spare.v1.vecFstLen = 0;

			spare.v1.vecLstOff = -1;
			spare.v1.vecLstLen = 0;

			break;	

		case ERASE:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++			
					
			if (!eraseBlock.Update())
			{
				wr = eraseBlock.er;

				spare.v1.fbb += eraseBlock.errBlocks;

				state = WRITE_PAGE_0;																			
			};
																																
			break;																												

		case WRITE_CREATE_FILE_0:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (wr.GetCol() > 0)
			{
				wr_ptr = wrBuf;
				wr_count = 0;

				byte *p = (byte*)wrBuf;

				p += wr.GetCol();

				for (u16 i = wr.GetCol()&3; i > 0; i--)
				{
					*(p++) = 0xff;
				};

				u32 *d = wrBuf + (wr.GetCol()+3)/4;

				for (u16 i = (sizeof(wrBuf)-wr.GetCol())/4; i > 0; i--)
				{
					*(d++) = -1;
				};

				createFile = true;

				state = WRITE_PAGE_0;
				
				break;
			};


		case WRITE_CREATE_FILE_1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			spare.v1.file += 1;  		

			spare.v1.prev = spare.v1.start;		

			if (wr.GetPage() > 0)
			{
				wr.NextBlock();

				//if (wr.overflow != 0)
				//{
				//	flashFull = true;
				//};
			};

			spare.v1.start = wr.GetRawPage();		
			spare.v1.fpn = 0;
			spare.v1.vectorCount = 0;

			//wr_prev_pg = -1;
			//wr_prev_col = 0;

			spare.v1.fbb = 0;		
			spare.v1.fbp = 0;		

			spare.v1.chipMask = nandSZ->mask;	

			SaveSession();

			createFile = false;

			state = WAIT;

			return false;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct ReadSpare
{
	enum { WAIT = 0, START, READ_1, READ_2, READ_3 };

	SpareArea	*spare;
	FLADR		*rd;

	u32			blockTryCount;
	u32			pageTryCount;

	byte state;

	ReadSpare() : spare(0), rd(0) {}

	bool Start(SpareArea *sp, FLADR *frd);
	bool Update();
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ReadSpare::Start(SpareArea *sp, FLADR *frd)
{
	if (sp == 0 || frd == 0)
	{
		return false;
	};

	spare = sp;
	rd = frd;

	blockTryCount = 32;
	pageTryCount = 128;


	state = START;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ReadSpare::Update()
{
	switch(state)
	{
		case WAIT:

			return false;

		case START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			NAND_Chip_Select(rd->GetChip());

			NAND_CmdReadPage(rd->pg, rd->GetBlock(), rd->GetPage());

			state = READ_1;

			break;

		case READ_1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if(!NAND_BUSY())
			{
				NAND_ReadDataDMA(spare, sizeof(*spare));

				state = READ_2;
			};

			break;

		case READ_2:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (NAND_CheckDataComplete())
			{
				if (spare->validBlock != 0xFFFF)
				{
					if (blockTryCount > 0)
					{
						blockTryCount--;

						rd->NextBlock();

						NAND_Chip_Select(rd->GetChip());
						NAND_CmdReadPage(rd->pg, rd->GetBlock(), rd->GetPage());

						state = READ_1;
					}
					else
					{
						state = WAIT;
					};
				}				
				else if (spare->validPage != 0xFFFF)
				{
					if (pageTryCount > 0)
					{
						pageTryCount--;

						rd->NextPage();

						NAND_Chip_Select(rd->GetChip());
						NAND_CmdReadPage(rd->pg, rd->GetBlock(), rd->GetPage());

						state = READ_1;
					}
					else
					{
						state = WAIT;
					}
				}
				else
				{
					spare->v1.CheckCRC();

					state = WAIT;

					return false;
				};
			};

			break;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct Read
{
	enum {	WAIT = 0,READ_START,READ_1, /*READ_2,*/ READ_PAGE,READ_PAGE_1,FIND_START,FIND_1,/*FIND_2,*/FIND_3/*,FIND_4*/};

	FLADR	rd;
	byte*	rd_data;
	u16		rd_count;
	u16		findTryCount;

	u32 	sparePage;

	SpareArea spare;	

	ReadSpare readSpare;

	bool vecStart;

	byte state;

	Read() : sparePage(~0), rd_data(0), rd_count(0), vecStart(false), state(WAIT) {}

	bool Start();
//	static bool Start(FLRB *flrb, FLADR *adr);
	bool Update();
	void End() { curRdBuf->ready = true; curRdBuf = 0; state = WAIT; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct Read2
{
	enum {	WAIT = 0, READ_START, /*READ_1, READ_2, READ_3,*/ READ_PAGE, /*READ_PAGE_1,*/ /*FIND_START,FIND_1,*//*FIND_2,*/FIND_3, FLUSH_PAGES};

	FLADR	rd;
	byte*	rd_data;
	u16		rd_count;
	u16		findTryCount;

	u32 	sparePage;
	u32 	prevSparePage;

	//SpareArea spare;

	ReadSpare readSpare;

	bool	vecStart;
	bool	cmdFlushPageBuffer;
	u32 	initFlushSparePage;


	void FlushPageBuffer(u32 page) { cmdFlushPageBuffer = true; initFlushSparePage = page; }

	byte state;
	byte statePage;

	PageBuffer *pagebuf;

	Read2() : sparePage(~0), prevSparePage(~0), rd_data(0), rd_count(0), vecStart(false), state(WAIT), statePage(0), pagebuf(0) {}
	
	bool Start();
//	bool Start(FLRB *flrb, FLADR *adr);
	bool Update();
	void End() { curRdBuf->ready = true; curRdBuf = 0; state = WAIT; }
	void UpdatePage();
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static Read2 read;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Read::Start()
{
	if ((curRdBuf = readFlBuf.Get()) != 0)
	{
		if (curRdBuf->useAdr) { rd.SetRawAdr(curRdBuf->adr); };

		vecStart = curRdBuf->vecStart;

		if (vecStart)
		{
			rd_data = (byte*)&curRdBuf->hdr;
			rd_count = sizeof(curRdBuf->hdr);
			curRdBuf->len = 0;	
		}
		else
		{
			rd_data = curRdBuf->data;
			rd_count = curRdBuf->maxLen;
			curRdBuf->len = 0;	
		};

		state = READ_START;

		return true;
	};

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Read::Update()
{
	switch(state)
	{
		case WAIT:	return false;

		case READ_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			NAND_Chip_Select(rd.GetChip());

			if (rd.GetRawPage() != sparePage)
			{
				readSpare.Start(&spare, &rd);

				state = READ_1;
			}
			else
			{
				NAND_CmdRandomRead(rd.GetCol());

				state = READ_PAGE;
			};

			break;

		case READ_1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (!readSpare.Update())
			{
				sparePage = rd.GetRawPage();

				NAND_CmdRandomRead(rd.GetCol());

				state = READ_PAGE;
			};

			break;

		//case READ_2:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	


		//	break;

		case READ_PAGE:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if(!NAND_BUSY())
			{
				register u16 c = rd.pg - rd.GetCol();

				if (rd_count < c) c = rd_count;

				NAND_ReadDataDMA(rd_data, c);

				rd_count -= c;
				rd.AddRaw(c);
				rd_data += c;
				curRdBuf->len += c;

				state = READ_PAGE_1;
			};

			break;

		case READ_PAGE_1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (NAND_CheckDataComplete())
			{
				if (rd_count == 0)
				{
					if (vecStart)
					{
						curRdBuf->hdr.crc = GetCRC16(&curRdBuf->hdr, sizeof(curRdBuf->hdr));

						if (curRdBuf->hdr.crc == 0)
						{
							rd_data = curRdBuf->data;
							rd_count = (curRdBuf->hdr.dataLen > curRdBuf->maxLen) ? curRdBuf->maxLen : curRdBuf->hdr.dataLen;
							curRdBuf->len = 0;	
							vecStart = false;

							if (rd_data == 0 || rd_count == 0)
							{
								End();

								return false;
							}
							else
							{
								state = READ_START;
							};
						}
						else
						{
							// ������ ������

							findTryCount = 1024;

							state = FIND_START;
						};
					}
					else
					{
						End();

						return false;
					};
				}
				else
				{
					state = READ_START;
				};
			};

			break;

		case FIND_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (spare.v1.start == -1 || spare.v1.fpn == -1)
			{
				if (findTryCount == 0)
				{
					// ������� ���������
					state = FIND_3;
				}
				else
				{
					findTryCount -= 1;

					rd.NextPage();

					readSpare.Start(&spare, &rd);

					state = FIND_1;
				};
			}
			else if (spare.v1.crc != 0 || spare.v1.vecFstOff == 0xFFFF || spare.v1.vecLstOff == 0xFFFF || rd.GetCol() > spare.v1.vecLstOff)
			{
				rd.NextPage();

				readSpare.Start(&spare, &rd);

				state = FIND_1;
			}
			else 
			{
				if (rd.GetCol() <= spare.v1.vecFstOff)
				{
					rd.SetCol(spare.v1.vecFstOff);
				}
				else if (rd.GetCol() <= (spare.v1.vecFstOff+spare.v1.vecFstLen))
				{
					rd.SetCol(spare.v1.vecFstOff+spare.v1.vecFstLen);
				}
				else if (rd.GetCol() <= spare.v1.vecLstOff)
				{
					rd.SetCol(spare.v1.vecLstOff);
				};

				rd_data = (byte*)&curRdBuf->hdr;
				rd_count = sizeof(curRdBuf->hdr);
				curRdBuf->len = 0;	

				state = READ_START;
			};

			break;

		case FIND_1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (!readSpare.Update())	//(!NAND_BUSY())
			{
				sparePage = rd.GetRawPage();

				state = FIND_START;
			};

			break;

		case FIND_3:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			curRdBuf->len = 0;
			curRdBuf->hdr.dataLen = 0;

			End();

			break;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Read2::Start()
{
	if ((curRdBuf = readFlBuf.Get()) != 0)
	{
		if (curRdBuf->useAdr) 
		{ 
			rd.SetRawAdr(curRdBuf->adr); 
			FlushPageBuffer(rd.GetRawPage()); 

			state = FLUSH_PAGES;
		}
		else
		{
			state = READ_START;
		};

		vecStart = curRdBuf->vecStart;

		if (vecStart)
		{
			rd_data = (byte*)&curRdBuf->hdr;
			rd_count = sizeof(curRdBuf->hdr);
			curRdBuf->len = 0;	
		}
		else
		{
			rd_data = curRdBuf->data;
			rd_count = curRdBuf->maxLen;
			curRdBuf->len = 0;	
		};

		findTryCount = 1024;


		return true;
	};

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Read2::UpdatePage()
{
	static PageBuffer *pb = 0;
	static FLADR padr;

	switch(statePage)
	{
		case 0:	

			if (cmdFlushPageBuffer)
			{
				statePage = 5;

				break;
			};

			if (sparePage != ~0ul)
			{
				pb = freePageBuffer.Get();

				if (pb != 0)
				{
					padr.SetRawPage(sparePage);

					//NAND_Chip_Select(padr.chip);

					readSpare.Start(&pb->spare, &padr);	

					statePage++;
				};
			};
			
			break;

		case 1:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!readSpare.Update())
			{
				pb->prevPage = prevSparePage;
				pb->page = sparePage = padr.GetRawPage();

				if (pb->spare.v1.crc == 0)
				{
					NAND_CmdRandomRead(0);
					statePage++;
				}
				else
				{
					statePage = 4;
				};
			};

			break;

		case 2:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			
			if(!NAND_BUSY())
			{
				NAND_ReadDataDMA(pb->data, padr.pg);

				statePage++;
			};

			break;

		case 3:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			
			if(NAND_CheckDataComplete())
			{
				statePage++;
			};

			break;

		case 4:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			
			readyPageBuffer.Add(pb);

			prevSparePage = sparePage;

			sparePage++;

			if (cmdFlushPageBuffer)
			{
				cmdFlushPageBuffer = false;
				
				statePage++;
			}
			else
			{
				statePage = 0;	
			};

			break;

		case 5:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			
			if (pagebuf != 0)
			{
				freePageBuffer.Add(pagebuf);
			};
				
			while ((pagebuf = readyPageBuffer.Get()) != 0)
			{
				freePageBuffer.Add(pagebuf);
			};

			sparePage = initFlushSparePage;
			prevSparePage = ~0;

			cmdFlushPageBuffer = false;

			statePage = 0;	

			break;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool Read2::Update()
{
	UpdatePage();

	switch(state)
	{
		case WAIT:	return false;

		case READ_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (pagebuf == 0)
			{
				pagebuf = readyPageBuffer.Get();

				if (pagebuf != 0)
				{
					if (pagebuf->spare.v1.crc == 0)
					{
						u16 col = rd.GetCol();

						rd.SetRawPage(pagebuf->page);
						rd.SetCol(col);
					}
					else 
					{
						freePageBuffer.Add(pagebuf);
						pagebuf = 0;

						if (findTryCount > 0)
						{
							findTryCount--;
						}
						else
						{
							// ������� ���������
							state = FIND_3;
						};
					};
				};
			};

			if (pagebuf != 0)
			{
				u16 c = rd.pg - rd.GetCol();

				if (rd_count < c) c = rd_count;

				NAND_CopyDataDMA(pagebuf->data+rd.GetCol(), rd_data, c);

				rd_count -= c;
				rd.raw += c;
				rd_data += c;
				curRdBuf->len += c;

				state = READ_PAGE;
			};

			break;

		case READ_PAGE:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (NAND_CheckCopyComplete())
			{
				if (rd.GetRawPage() != pagebuf->page)
				{ 
					freePageBuffer.Add(pagebuf);

					pagebuf = 0;
				};

				if (rd_count == 0)
				{
					if (vecStart)
					{
						curRdBuf->hdr.crc = GetCRC16(&curRdBuf->hdr, sizeof(curRdBuf->hdr));

						if (curRdBuf->hdr.crc == 0)
						{
							rd_data = curRdBuf->data;
							rd_count = (curRdBuf->hdr.dataLen > curRdBuf->maxLen) ? curRdBuf->maxLen : curRdBuf->hdr.dataLen;
							curRdBuf->len = 0;	
							vecStart = false;

							if (rd_data == 0 || rd_count == 0)
							{
								End();

								return false;
							}
							else
							{
								state = READ_START;
							};
						}
						else if (findTryCount == 0)
						{
							// ������� ���������
							state = FIND_3;
						}
						else
						{
							// ������ ������

							findTryCount -= 1;

							if (pagebuf != 0)
							{
								SpareArea &spare = pagebuf->spare;

								if (spare.v1.vecFstOff == 0xFFFF || spare.v1.vecLstOff == 0xFFFF || rd.GetCol() > spare.v1.vecLstOff)
								{
									rd.NextPage();

									freePageBuffer.Add(pagebuf); pagebuf = 0;
								}
								else if (rd.GetCol() <= spare.v1.vecFstOff)
								{
									rd.SetCol(spare.v1.vecFstOff);
								}
								else if (rd.GetCol() <= (spare.v1.vecFstOff+spare.v1.vecFstLen))
								{
									rd.SetCol(spare.v1.vecFstOff+spare.v1.vecFstLen);
								}
								else if (rd.GetCol() <= spare.v1.vecLstOff)
								{
									rd.SetCol(spare.v1.vecLstOff);
								};
							};

							rd_data = (byte*)&curRdBuf->hdr;
							rd_count = sizeof(curRdBuf->hdr);
							curRdBuf->len = 0;	

							state = READ_START;
						};
					}
					else
					{
						End();

						return false;
					};
				}
				else
				{
					state = READ_START;
				};
			};

			break;

		case FIND_3:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			curRdBuf->len = 0;
			curRdBuf->hdr.dataLen = 0;

			End();

			break;
		
		case FLUSH_PAGES:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	

			if (!cmdFlushPageBuffer)
			{
				state = READ_START;
			};

			break;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static void ReadSpareNow(SpareArea *spare, FLADR *rd, bool crc)
//{
//	//SpareArea *spare;
//	//FLADR *rd;
//
//	NAND_Chip_Select(rd->chip);
//
//	NAND_CmdReadPage(rd->pg, rd->block, rd->page);
//
//	while(!NAND_BUSY());
//	while(NAND_BUSY());
//
//	NAND_ReadDataDMA(spare, sizeof(*spare));
//
//	while (!NAND_CheckDataComplete());
//
//	if (crc)
//	{
//		spare->crc = GetCRC16((void*)&spare->file, sizeof(*spare) - spare->CRC_SKIP);
//	};
//}
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//static void ReadDataNow(void *dst, u16 len, FLADR *rd)
//{
//	NAND_Chip_Select(rd->chip);
//
//	NAND_CmdReadPage(rd->col, rd->block, rd->page);
//
//	while(!NAND_BUSY());
//	while(NAND_BUSY());
//
//	NAND_ReadDataDMA(dst, len);
//
//	while (!NAND_CheckDataComplete());
//}
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//static void ReadVecHdrNow(VecData::Hdr *h, FLADR *rd)
//{
//	ReadDataNow(h, sizeof(*h), rd);
//
//	h->crc = GetCRC16(h, sizeof(*h));
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitSessions()
{
//	__breakpoint(0);

	write.Init();

	if (nvv.f.size > 0)
	{
//		NAND_NextSession();

		write.CreateNextFile();

		while (write.Update()) ;
	};

	u32 ms = 0, me = 0, ls = -1;
	u32 sp = 0;

	bool bm = false, bl = false;

	//FLADR sb(0), eb(-1); // free start and end block

	for (u16 i = 128, ind = nvv.index; i > 0; i--, ind = (ind-1)&127)
	{
		FileDsc &f = nvsi[ind].f;

		if (f.size == 0) continue;

		if (bl)
		{
			if (f.lastPage < f.startPage)
			{
				f.size = 0;
			}
			else
			{
				if (f.lastPage < ls)
				{
					ls = f.startPage;

					if (bm)
					{
						if (f.lastPage <= me)
						{
							f.size = 0;
						}
						else if (f.startPage < me)
						{
							f.startPage = me+1;
						};
					};
				}
				else
				{
					f.size = 0;
				};
			};
		}
		else
		{
			if (f.lastPage < f.startPage)
			{
				bl = true;
				ls = f.startPage;
				sp = 0;
			}
			else
			{
				sp = f.startPage;
			};

			if (!bm)
			{
				bm = true;
				ms = sp;
				me = f.lastPage;
			}
			else if (f.lastPage < ms)
			{
				ms = sp;
			}
			else if (f.lastPage > me)
			{
				f.startPage = me+1;
				me = f.lastPage;
			}
			else
			{
				f.size = 0;
			};
		};



	}; // 	for (u16 i = 128, ind = nvv.index; i > 0; i--, ind = (ind-1)&127)
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void InitSessionsNew()
{
	//__breakpoint(0);

	SpareArea spare;
	ReadSpare rdspr;

	write.Init();

	if (nvv.f.size > 0)
	{
		write.CreateNextFile();

		while (write.Update()) ;
	};

	FLADR firstSessionAdr = write.wr;

	u16 firstSessionNum = 0;
	bool firstSessionValid = false;

	u16 count = 100;

	while (count--)
	{
		rdspr.Start(&spare, &firstSessionAdr);	while (rdspr.Update());

		if (spare.v1.crc == 0)
		{ 
			firstSessionNum = spare.v1.file; 
			firstSessionValid = true; 
			break; 
		};
	};

	bool c = false;

	flashUsedSize = 0;

	for (u16 i = 128, ind = nvv.index; i > 0; i--, ind = (ind-1)&127)
	{
		FileDsc &f = nvsi[ind].f;

		if (firstSessionValid && f.session == firstSessionNum)
		{
			f.startPage = firstSessionAdr.GetRawPage();
			//f.size = ???;

			firstSessionValid = false;
			c = true;
		}
		else if (c)
		{
			f.size = 0;
		}
		else if (f.size != 0)
		{
			FLADR adr(f.startPage);

			count = 100;

			while (count--)
			{
				rdspr.Start(&spare, &adr);	while (rdspr.Update());

				if (spare.v1.crc == 0 ) break;

				adr.NextPage();
			};

			if (spare.v1.crc != 0 || f.session != spare.v1.file)
			{
				f.size = 0;
			};
		};

		flashUsedSize += f.size;
	}; 

	u64 fullSize = FLASH_Full_Size_Get();

	if (flashUsedSize > fullSize) flashUsedSize = fullSize;

	NAND_Chip_Disable();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//bool GetSessionNow(SessionInfo *si)
//{
//	static SpareArea spare;
//
//	static VecData::Hdr hdr;
//
//	FLADR rd(0,0,0,0);
//
//	rd.raw = adrLastVector;
//
//	ReadSpareNow(&spare, &rd, true);
//
//	si->size = (u64)spare.fpn << NAND_COL_BITS;
//
//	si->last_adress = adrLastVector;
//	si->session = spare.file;
//
//	ReadVecHdrNow(&hdr, &rd);
//
//	if (hdr.crc == 0)
//	{
//		si->stop_rtc = hdr.rtc;
//	}
//	else
//	{
//
//	};
//
//	rd.SetRawPage(spare.start);
//
//	ReadVecHdrNow(&hdr, &rd);
//
//	if (hdr.crc == 0)
//	{
//		si->start_rtc = hdr.rtc;
//	}
//	else
//	{
//
//	};
//
//	return true;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void StartSendSession()
{
	cmdSendSession = true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool UpdateSendSession()
{
	enum {	WAIT = 0, READ_START, READ_1, READ_2, READ_PAGE,READ_PAGE_1,FIND_START,FIND_1,FIND_2,FIND_3,FIND_4, READ_END};

	static byte i = WAIT;
	
	static u16 ind = 0;
	static u32 prgrss = 0;
	static u16 count = 0;
	static u32 lp = 0;
	static u32 sum = 0;
	static FLADR a(0);

	FileDsc &s = nvsi[ind].f;

	switch (i)
	{
		case 0:

			if (cmdSendSession)
			{
				ind = nvv.index;
				prgrss = 0;
				count = 128;

				i++;
			}
			else
			{
				return false;
			};

			break;

		case 1:

			if (s.size > 0)
			{
				if (!TRAP_MEMORY_SendSession(s.session, s.size, (u64)s.startPage * FLADR::pg, s.start_rtc, s.stop_rtc, s.flags))
				{
					break;
				};
			};

			ind = (ind-1)&127;

			prgrss += 0x100000000/128;

			count--;

			i++;

			break;

		case 2:

			if (FLASH_SendStatus(prgrss, FLASH_STATUS_READ_SESSION_IDLE))
			{
				if (count > 0)
				{
					i = 1;
				}
				else
				{
					i++;
				};
			};

			break;

		case 3:

			if (FLASH_SendStatus(-1, FLASH_STATUS_READ_SESSION_READY))
			{
				cmdSendSession = false;

				i = 0;
			};

			break;
	};

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

FileDsc* GetSessionInfo(u16 session, u64 adr)
{
	u16 ind = nvv.index;

	FileDsc *s = 0;

	for (u16 i = 128; i > 0; i--)
	{
		s = &nvsi[ind].f;

		if (s->session == session)
		{
			return s;
		};

		ind = (ind-1)&127;
	};

	return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void NAND_Idle()
{
//	register u32 t;

	static i32 t = 0;
	static i32 eb = 0;
	static TM32 tm;
	static FLADR er(0);
	static EraseBlock eraseBlock;

	switch (nandState)
	{
		case NAND_STATE_WAIT: 

			if (cmdCreateNextFile)
			{
				write.CreateNextFile();
				cmdCreateNextFile = false;
				nandState = NAND_STATE_CREATE_FILE;

				break;
			};

			if (cmdFullErase)
			{
				cmdFullErase = false;
				eraseSessionsCount = 1;

#ifdef WIN32
				nandState = NAND_STATE_FULL_ERASE_START;
#endif

				break;
			};

			if (cmdSendSession)
			{
				nandState = NAND_STATE_SEND_SESSION;

				break;
			};

			if (write.Start())
			{
				nandState = NAND_STATE_WRITE_START;
			}
			else if (read.Start())
			{
				nandState = NAND_STATE_READ_START;
			};

			FLASH_UpdateStatus();

			break;

		case NAND_STATE_WRITE_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!write.Update())
			{
				nandState = NAND_STATE_WAIT;
			};

			break;

		case NAND_STATE_READ_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!read.Update())
			{
				nandState = NAND_STATE_WAIT;
			};

			break;

		case NAND_STATE_FULL_ERASE_START:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			t = eb = nandSZ->fl >> er.CHIP_OFF; // blocks count

			er.SetRawAdr(0);

			eraseBlock.Start(er, true, true);

			nandState = NAND_STATE_FULL_ERASE_0;

			break;

		case NAND_STATE_FULL_ERASE_0:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!eraseBlock.Update())
			{
				invalidBlocks += eraseBlock.errBlocks;

				t -= eraseBlock.errBlocks;
				t -= 1;

				write.errVec = t;

				if (t > 0)
				{
					er.NextBlock();

					eraseBlock.Start(er, true, true);

					if (tm.Check(500)) 
					{ 
						FLASH_SendStatus((eb-t)*((u64)0x100000000)/eb, FLASH_STATUS_BUSY); 
#ifdef WIN32
						Printf(0, 3, 0xF0, "%9li", t);
#endif
					};
				}
				else
				{
					flashEmpty = true;

					write.Init(0, 1, ~0);

					adrLastVector = ~0;

					FLASH_SendStatus(~0, FLASH_STATUS_ERASE);
					nandState = NAND_STATE_WAIT;
				};
			};

			//if (tm.Check(200) && IsComputerFind() && EmacIsConnected()) TRAP_MEMORY_SendStatus(lastFlashProgress, lastFlashStatus);

			break;

		case NAND_STATE_CREATE_FILE:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!write.Update())
			{
				nandState = NAND_STATE_WAIT;
			};

			break;

		case NAND_STATE_SEND_SESSION:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (!UpdateSendSession())
			{
				nandState = NAND_STATE_SEND_BAD_BLOCKS;
			};
	
			//if (tm.Check(200) && IsComputerFind() && EmacIsConnected()) TRAP_MEMORY_SendStatus(lastFlashProgress, lastFlashStatus);

			break;

		case NAND_STATE_SEND_BAD_BLOCKS:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (TRAP_TRACE_PrintString("NAND chip mask: 0x%02hX; Bad Blocks: %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu", 
				NAND_GetMemSize()->mask, nvv.badBlocks[0], nvv.badBlocks[1], nvv.badBlocks[2], nvv.badBlocks[3], nvv.badBlocks[4], nvv.badBlocks[5], nvv.badBlocks[6], nvv.badBlocks[7]))
			{
				nandState = NAND_STATE_SEND_PAGE_ERRORS;
			};

			break;

		case NAND_STATE_SEND_PAGE_ERRORS:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			if (TRAP_TRACE_PrintString("NAND Page Error: %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu", 
				nvv.pageError[0], nvv.pageError[1], nvv.pageError[2], nvv.pageError[3], nvv.pageError[4], nvv.pageError[5], nvv.pageError[6], nvv.pageError[7]))
			{
				nandState = NAND_STATE_SEND_DATABUS_MASK;
			};

			break;

		case NAND_STATE_SEND_DATABUS_MASK:	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			const byte* p = NAND_GetMemSize()->chipDataBusMask;

			if (TRAP_TRACE_PrintString("NAND DataBus Mask: 0x%02hX, 0x%02hX, 0x%02hX, 0x%02hX, 0x%02hX, 0x%02hX, 0x%02hX, 0x%02hX", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]))
			{
				nandState = NAND_STATE_WAIT;
			};

			break;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u32 FLASH_Vectors_Errors_Get()
{
	return write.errVec;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u32 FLASH_Vectors_Saved_Get()
{
	return write.spare.v1.vectorCount;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u32 FLASH_Vectors_Recieved_Get()
{
	return write.rcvVec;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u32 FLASH_Vectors_Rejected_Get()
{
	return write.rejVec;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u32 FLASH_Session_Get()
{
	return write.spare.v1.file;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u64 FLASH_Current_Adress_Get()
{
	return write.wr.GetRawAdr();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u64 FLASH_Full_Size_Get()
{
	return nandSZ->fl;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u64 FLASH_Used_Size_Get()
{
	return flashUsedSize;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

i64 FLASH_Empty_Size_Get()
{
	return FLASH_Full_Size_Get() - FLASH_Used_Size_Get();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 FLASH_Chip_Mask_Get()
{
	return nandSZ->mask;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ������� 
bool FLASH_Erase_Full()
{
	//if(flash_status_operation != FLASH_STATUS_OPERATION_WAIT) return false;
	//FRAM_Memory_Start_Adress_Set(FRAM_Memory_Current_Adress_Get());
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ������������
bool FLASH_UnErase_Full()
{
	//if(flash_status_operation != FLASH_STATUS_OPERATION_WAIT) return false;
	//FRAM_Memory_Start_Adress_Set(-1);
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool RequestFlashWrite(Ptr<UNIBUF> &fwb, u16 devID)
{
	bool result = false;

	if (!fwb.Valid())
	{
		//		freeReqList.Add(req);
	}
	else
	{
		if (fwb->dataLen > 0 && fwb->dataOffset >= sizeof(VecData::Hdr))
		{
			VecData* vd = (VecData*)(fwb->data + fwb->dataOffset - sizeof(VecData::Hdr));

			DataPointer p(vd->data);

			p.b += fwb->dataLen;

			*p.w = CRC_CCITT_DMA(vd->data, fwb->dataLen, 0xFFFF);

			fwb->dataLen += 2;

			vd->h.device = deviceID = devID;

			result = writeFlBuf.Add(fwb);
		};
	};

	return result;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool FLASH_WriteEnable()
{
	bool result = false;

	if (!writeFlashEnabled)
	{
		nvv.f.size = 0;
		GetTime(&nvv.f.start_rtc);

		result = true;
	};

	writeFlashEnabled = true;
	flashStatus = FLASH_STATUS_WRITE;

	return result;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void FLASH_WriteDisable()
{
	if (writeFlashEnabled && nvv.f.size > 0)
	{
		NAND_NextSession();
	};

	writeFlashEnabled = false;

	flashStatus = FLASH_STATUS_WAIT;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

byte FLASH_Status()
{
	return flashStatus;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void LoadVars()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "Flash Load Vars ... ");

	PointerCRC p(buf);

	static DSCI2C dsc;
	static DSCSPI spi;
	static u16 romAdr = 0;
	static TM32 tm;

	NVV nv1, nv2;
	bool c1 = false;
	bool c2 = false;

	spi.adr = (ReverseDword(FRAM_SPI_SESSIONS_ADR) & ~0xFF) | 3;
	spi.alen = 4;
	spi.csnum = 1;
	spi.wdata = 0;
	spi.wlen = 0;
	spi.rdata = buf;
	spi.rlen = sizeof(buf);

	if (SPI_AddRequest(&spi))
	{
		while (!(spi.ready || tm.Check(200))) { SPI_Update(); };
	};

	for (byte i = 0; i < 2; i++)
	{
		p.CRC.w = 0xFFFF;
		p.ReadArrayB(&nv1, sizeof(nv1));
		p.ReadW();

		if (p.CRC.w == 0) { c1 = true; break; };
	};

	romAdr = ReverseWord(FRAM_I2C_SESSIONS_ADR);

	dsc.wdata = &romAdr;
	dsc.wlen = sizeof(romAdr);
	dsc.wdata2 = 0;
	dsc.wlen2 = 0;
	dsc.rdata = buf;
	dsc.rlen = sizeof(buf);
	dsc.adr = 0x50;

	if (I2C_AddRequest(&dsc))
	{
		while (!(dsc.ready || tm.Check(100))) { I2C_Update(); };
	};

	loadVarsOk = false;

	p.b = buf;

	for (byte i = 0; i < 2; i++)
	{
		p.CRC.w = 0xFFFF;
		p.ReadArrayB(&nv2, sizeof(nv2));
		p.ReadW();

		if (p.CRC.w == 0) { c2 = true; break; };
	};

	fram_SPI_flashVarsOk = c1;
	fram_I2C_flashVarsOk = c2;

	SEGGER_RTT_WriteString(0, "FRAM SPI - "); SEGGER_RTT_WriteString(0, (c1) ? (RTT_CTRL_TEXT_BRIGHT_GREEN "OK") : (RTT_CTRL_TEXT_BRIGHT_RED "ERROR"));

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE " ... FRAM I2C - "); SEGGER_RTT_WriteString(0, (c2) ? (RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n") : (RTT_CTRL_TEXT_BRIGHT_RED "ERROR\n"));

	if (c1 && c2)
	{
		if (nv1.timeStamp > nv2.timeStamp)
		{
			c2 = false;
		}
		else if (nv1.timeStamp < nv2.timeStamp)
		{
			c1 = false;
		};
	};

	if (c1)	{ nvv = nv1; } else if (c2) { nvv = nv2; };

	loadVarsOk = c1 || c2;

	if (!c1 || !c2)
	{
		if (!loadVarsOk)
		{
			nvv.timeStamp = 0;
			nvv.index = 0;
			nvv.prevFilePage = ~0;

			nvv.f.session = 0;
			nvv.f.size = 0;
			nvv.f.startPage = 0;
			nvv.f.lastPage = 0;
			GetTime(&nvv.f.start_rtc);
			GetTime(&nvv.f.stop_rtc);
			nvv.f.flags = 0;

			for (u32 i = 0; i < ArraySize(nvv.badBlocks); i++)
			{
				nvv.badBlocks[i] = 0;
				nvv.pageError[i] = 0;
			};
		};

		savesCount = 2;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void SaveVars()
{
	const u16 sa = 0x100;

	PointerCRC p(buf);

	static DSCI2C dsc;
	static DSCSPI spi, spi2;
	static u16 romAdr = 0;

	static byte i = 0;
	static TM32 tm;

	switch (i)
	{
		case 0:

			if (tm.Check(200))
			{
				if (savesCount > 0)
				{
					savesCount--;
					i++;
				}
				else if (savesSessionsCount > 0)
				{
					savesSessionsCount--;
					i = 5;
				}
				else if (eraseSessionsCount > 0)
				{
					eraseSessionsCount--;

					for (u16 n = 0; n < ArraySize(nvsi); n++)
					{
						nvsi[n].timeStamp = 0;
						nvsi[n].f.size = 0;
						nvsi[n].crc = 0;
					};

					nvv.f.session += 1;
					nvv.f.size = 0;
					//nvv.f.startPage = 0;
					//nvv.f.lastPage = 0;
					nvv.index = 0;
					nvv.timeStamp = 0;

					savesCount = 1;

					i = 6;
				};
			};

			break;

		case 1:

			nvv.timeStamp = GetMilliseconds();

			for (byte j = 0; j < 2; j++)
			{
				p.CRC.w = 0xFFFF;
				p.WriteArrayB(&nvv, sizeof(nvv));
				p.WriteW(p.CRC.w);
			};

			romAdr = ReverseWord(FRAM_I2C_SESSIONS_ADR);

			dsc.wdata = &romAdr;
			dsc.wlen = sizeof(romAdr);
			dsc.wdata2 = buf;
			dsc.wlen2 = sizeof(buf);
			dsc.rdata = 0;
			dsc.rlen = 0;
			dsc.adr = 0x50;

			spi.adr = (ReverseDword(FRAM_SPI_SESSIONS_ADR) & ~0xFF)|2;
			spi.alen = 4;
			spi.csnum = 1;
			spi.wdata = buf;
			spi.wlen = sizeof(buf);
			spi.rdata = 0;
			spi.rlen = 0;

			spi2.adr = 6;
			spi2.alen = 1;
			spi2.csnum = 1;
			spi2.wdata = 0;
			spi2.wlen = 0;
			spi2.rdata = 0;
			spi2.rlen = 0;

			tm.Reset();

			i = (I2C_AddRequest(&dsc)) ? (i+1) : 0;

			break;

		case 2:

			if (dsc.ready || tm.Check(100))
			{
				tm.Reset();

				SPI_AddRequest(&spi2);

				i++;
			};

			break;

		case 3:

			if (spi2.ready || tm.Check(200))
			{
				SPI_AddRequest(&spi);

				i++;
			};

			break;

		case 4:

			if (spi.ready || tm.Check(200))
			{
				i = 0;
			};

			break;

		case 5:

			{
				NVSI &si = nvsi[nvv.index];

				si.timeStamp = GetMilliseconds();

				//u32 adr = sa+sizeof(si)*nvv.index;

				romAdr = ReverseWord(FRAM_I2C_SESSIONS_ADR+sa+sizeof(si)*nvv.index);

				dsc.wdata = &romAdr;
				dsc.wlen = sizeof(romAdr);
				dsc.wdata2 = buf;
				dsc.wlen2 = sizeof(si);
				dsc.rdata = 0;
				dsc.rlen = 0;
				dsc.adr = 0x50;

				spi.adr = (ReverseDword(FRAM_SPI_SESSIONS_ADR+sa+sizeof(si)*nvv.index) & ~0xFF) | 2;
				spi.alen = 4;
				spi.csnum = 1;
				spi.wdata = buf;
				spi.wlen = sizeof(si);
				spi.rdata = 0;
				spi.rlen = 0;

				spi2.adr = 6;
				spi2.alen = 1;
				spi2.csnum = 1;
				spi2.wdata = 0;
				spi2.wlen = 0;
				spi2.rdata = 0;
				spi2.rlen = 0;

				p.CRC.w = 0xFFFF;
				p.WriteArrayB(&si, sizeof(si)-2);
				p.WriteW(p.CRC.w);

				tm.Reset();

				i = (I2C_AddRequest(&dsc)) ? 2 : 0;
			};

			break;

		case 6:

			romAdr = ReverseWord(FRAM_I2C_SESSIONS_ADR+sa);

			dsc.wdata = &romAdr;
			dsc.wlen = sizeof(romAdr);
			dsc.wdata2 = nvsi;
			dsc.wlen2 = sizeof(nvsi);
			dsc.rdata = 0;
			dsc.rlen = 0;
			dsc.adr = 0x50;

			spi.adr = (ReverseDword(FRAM_SPI_SESSIONS_ADR+sa) & ~0xFF) | 2;
			spi.alen = 4;
			spi.csnum = 1;
			spi.wdata = nvsi;
			spi.wlen = sizeof(nvsi);
			spi.rdata = 0;
			spi.rlen = 0;

			spi2.adr = 6;
			spi2.alen = 1;
			spi2.csnum = 1;
			spi2.wdata = 0;
			spi2.wlen = 0;
			spi2.rdata = 0;
			spi2.rlen = 0;

			i = (I2C_AddRequest(&dsc)) ? 2 : 0;

			break;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void LoadSessions()
{
	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Flash Load Sessions ... ");

	static DSCI2C dsc;
	static DSCSPI spi, spi2;
	static u16 romAdr = 0;
	static u32 spiAdr = 0;
	static TM32 tm;

	const u16 sa = 0x100;

	loadSessionsOk = true;
	fram_SPI_loadSessionsOk = true;
	fram_I2C_loadSessionsOk = true;

	NVSI nv1, nv2;

	for (u16 i = 0; i < ArraySize(nvsi); i++)
	{
		NVSI &si = nvsi[i];

		spiAdr = (ReverseDword(FRAM_SPI_SESSIONS_ADR + sa + sizeof(nv1) * i) & ~0xFF);

		spi.adr = spiAdr | 3;
		spi.alen = sizeof(spiAdr);
		spi.csnum = 1;
		spi.wdata = 0;
		spi.wlen = 0;
		spi.rdata = &nv1;
		spi.rlen = sizeof(nv1);

		tm.Reset();

		SPI_AddRequest(&spi);

		while (!(spi.ready || tm.Check(200))) { SPI_Update(); };

		romAdr = ReverseWord(FRAM_I2C_SESSIONS_ADR+sa+sizeof(nv2)*i);

		dsc.wdata = &romAdr;
		dsc.wlen = sizeof(romAdr);
		dsc.wdata2 = 0;
		dsc.wlen2 = 0;
		dsc.rdata = &nv2;
		dsc.rlen = sizeof(nv2);
		dsc.adr = 0x50;

		tm.Reset();

		if (I2C_AddRequest(&dsc))
		{
			while (!(dsc.ready || tm.Check(100))) { I2C_Update(); };
		};

		bool c1 = GetCRC16(&nv1, sizeof(nv1)) == 0;
		bool c2 = GetCRC16(&nv2, sizeof(nv2)) == 0;

		if (!c1)
		{
			fram_SPI_loadSessionsOk = false;
		};

		if (!c2)
		{
			fram_I2C_loadSessionsOk = false;
		};

		if (c1 && c2)
		{
			if (nv1.timeStamp > nv2.timeStamp)
			{
				c2 = false;
			}
			else
			{
				c1 = false;
			};
		};

		if (c1)	{ si = nv1; } else if (c2) { si = nv2; };

		if (!c1 || !c2)
		{
			if (!c1 && !c2)
			{
				loadSessionsOk = false;

				si.f.session = 0;
				si.f.size = 0;
				si.f.startPage = 0;
				si.f.lastPage = 0;
				si.f.flags = 0;
				si.f.start_rtc.date = 0;
				si.f.start_rtc.time = 0;
				si.f.stop_rtc.date = 0;
				si.f.stop_rtc.time = 0;
				si.timeStamp = 0;
				si.crc = GetCRC16(&si, sizeof(si) - 2);

				dsc.wdata2 = &si;
				dsc.wlen2 = sizeof(si);

				spi.wdata = &si;
				spi.wlen = sizeof(si);
			}
			else if (c1)
			{
				dsc.wdata2 = &nv1;
				dsc.wlen2 = sizeof(nv1);
			}
			else if (c2)
			{
				spi.wdata = &nv2;
				spi.wlen = sizeof(nv2);
			};

			if (!c1)
			{
				spi.adr = spiAdr | 2;
				spi.alen = sizeof(spiAdr);
				spi.csnum = 1;
				spi.rdata = 0;
				spi.rlen = 0;

				spi2.adr = 6;
				spi2.alen = 1;
				spi2.csnum = 1;
				spi2.wdata = 0;
				spi2.wlen = 0;
				spi2.rdata = 0;
				spi2.rlen = 0;

				tm.Reset();

				SPI_AddRequest(&spi2);
				SPI_AddRequest(&spi);

				while (!((spi.ready && spi2.ready) || tm.Check(200))) { SPI_Update(); };
			};

			if (!c2)
			{
				dsc.wdata = &romAdr;
				dsc.wlen = sizeof(romAdr);
				dsc.rdata = 0;
				dsc.rlen = 0;
				dsc.adr = 0x50;

				tm.Reset();

				I2C_AddRequest(&dsc);

				while (!(dsc.ready || tm.Check(100))) { I2C_Update(); };
			};
		};
	};

	SEGGER_RTT_WriteString(0, "FRAM SPI - "); SEGGER_RTT_WriteString(0, (fram_SPI_loadSessionsOk) ? (RTT_CTRL_TEXT_BRIGHT_GREEN "OK") : (RTT_CTRL_TEXT_BRIGHT_RED "ERROR"));

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_WHITE " ... FRAM I2C - "); SEGGER_RTT_WriteString(0, (fram_I2C_loadSessionsOk) ? (RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n") : (RTT_CTRL_TEXT_BRIGHT_RED "ERROR\n"));
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void FLASH_Init()
{
	nandSZ = NAND_GetMemSize();

	FRAM_SPI_PRESENT = (Get_FRAM_SPI_SessionsAdr() >= 0);
	FRAM_I2C_PRESENT = (Get_FRAM_I2C_SessionsAdr() >= 0);
	FRAM_SPI_SESSIONS_ADR = Get_FRAM_SPI_SessionsAdr();
	FRAM_I2C_SESSIONS_ADR = Get_FRAM_I2C_SessionsAdr();

	LoadVars();

	LoadSessions();

	InitFlashBuffer();

	InitSessionsNew();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void FLASH_Update()
{
	SaveVars();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
