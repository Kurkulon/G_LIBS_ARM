#ifndef FLASH_H__09_08_2019__08_17
#define FLASH_H__09_08_2019__08_17

//#include "hardware.h"

//#include "rtc.h"
//#include "vector.h"
#include "trap_def.h"
#include "list.h"
#include <stddef.h>
#include "CRC16.h"


//#define FLWB_LEN (3072)
//#define FLRB_LEN 1536

#define UNIBUF_LEN (3072)


enum  
{
	FLASH_STATUS_NONE = 0,
	FLASH_STATUS_BUSY,				// занят неизвестной для компьютера операцией (например записью в память)
	FLASH_STATUS_STOP,
	FLASH_STATUS_PAUSE,
	FLASH_STATUS_RESUME,
	FLASH_STATUS_ERASE,
	FLASH_STATUS_UNERASE,
	FLASH_STATUS_READ_SESSION_START,
	FLASH_STATUS_READ_SESSION_IDLE,
	FLASH_STATUS_READ_SESSION_FIND, // поиск вектора по несвязанной области
	FLASH_STATUS_READ_SESSION_ERROR,
	FLASH_STATUS_READ_SESSION_READY,
	FLASH_STATUS_READ_VECTOR_START,
	FLASH_STATUS_READ_VECTOR_IDLE,
	FLASH_STATUS_READ_VECTOR_FIND, // поиск вектора по несвязанной области
	FLASH_STATUS_READ_VECTOR_ERROR,
	FLASH_STATUS_READ_VECTOR_READY
};


enum flash_save_repeat_type
{
	FLASH_SAVE_REPEAT_NONE = 0,
	FLASH_SAVE_REPEAT_NORMAL = 2,	// одна перезапись, вторая - в новый блок
	FLASH_SAVE_REPEAT_HIGH = 4,	// .... 3 в новый блок
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct VecData
{
	__packed struct Hdr
	{
		u16 		session;
		u16 		device;
		RTC_type	rtc;
		byte		flags;		// флажки
		u64			prVecAdr;	// адрес предыдущего блядовектора
		u16			dataLen;
		u16			crc;		// CRC16 всей этой хрени
	} h;

	byte		data[/*FLWB_LEN*/]; // Последние 2 байта CRC16
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct UNIBUF : public PtrItem<UNIBUF>
{
	UNIBUF() : dataOffset(sizeof(VecData::Hdr)), dataLen(0)  { /*freeBufList.Add(this);*/ }

	u16		dataOffset;
	u16 	dataLen;

	byte	data[UNIBUF_LEN]; // Последние 2 байта CRC16

	void*	GetDataPtr() { return data+dataOffset; }
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct FLRB
{
	FLRB *next;

	bool	ready;
	bool	vecStart; // Найти начало вектора и считать hdr

	bool	useAdr;
	u64		adr;

	VecData::Hdr hdr;

	u16		maxLen;
	u16		len;
	u16		crc;

	byte	*data;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct FileDsc
{
	u16			session;	// file number
	i64			size;		//если 0 то сессия немного порченная
	RTC_type	start_rtc;	//если 0 то сессия немного порченная
	RTC_type	stop_rtc;  
	u32			startPage; 
	u32			lastPage;
	byte		flags;
};	

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//extern FLWB*	AllocFlashWriteBuffer();
extern FLRB*	AllocFlashReadBuffer();
//extern void		FreeFlashWriteBuffer(FLWB* b);
extern void		FreeFlashReadBuffer(FLRB* b);
extern bool		RequestFlashRead(FLRB* b);
extern bool		RequestFlashWrite(Ptr<UNIBUF> &b, u16 devID);

extern void NAND_Idle();
extern void NAND_FullErase();
//extern void NAND_NextSession();

inline u16 GetDeviceID() { extern u16 deviceID; return deviceID; }

extern void StartSendSession();
extern FileDsc* GetSessionInfo(u16 session, u64 adr);

//extern const SessionInfo* GetLastSessionInfo();


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void FLASH_Init();
extern bool FLASH_Reset();
extern void FLASH_Update();
extern bool FLASH_Busy();

extern bool FLASH_WriteEnable();
extern void FLASH_WriteDisable();
extern byte FLASH_Status();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//extern bool FLASH_Erase_Full();
//extern bool FLASH_UnErase_Full();
//extern bool FLASH_Write_Vector(u16 session, u16 device, RTC_type rtc, byte flags, byte *data, u16 size, flash_save_repeat_type repeat);
//extern bool FLASH_Read_Vector(u64 adr, u16 *size, bool *ready, byte **vector);

extern u32 FLASH_Vectors_Errors_Get();
extern u32 FLASH_Vectors_Saved_Get();
extern u32 FLASH_Vectors_Recieved_Get();
extern u32 FLASH_Vectors_Rejected_Get();
extern u32 FLASH_Session_Get();


//extern void FLASH_Vectors_Errors_Reset();
//extern void FLASH_Vectors_Saved_Reset();

extern u64 FLASH_Current_Adress_Get();
extern u64 FLASH_Full_Size_Get();
extern u16 FLASH_Chip_Mask_Get();
extern u64 FLASH_Used_Size_Get();

extern bool FLASH_SendStatus(u32 progress, byte status);


//extern i64 FLASH_Empty_Size_Get();


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // FLASH_H__09_08_2019__08_17
