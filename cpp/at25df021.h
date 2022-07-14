/*******************************************************************************/
/*                                                                             */
/*   (C) Copyright 2008 - Analog Devices, Inc.  All rights reserved.           */
/*                                                                             */
/*    FILE:     at25df021.h                                                       */
/*																			   */
/*    PURPOSE:  This header file defines items specific to the M25P16 flash.   */
/*                                                                             */
/*******************************************************************************/

#ifndef AT25DF021_H__14_09_2016__08_29
#define AT25DF021_H__14_09_2016__08_29

#include "types.h"

enum ERROR_CODE
{
	NO_ERR,					/* No Error */
	POLL_TIMEOUT,			/* Polling toggle bit failed */
	VERIFY_WRITE,			/* Verifying write to flash failed */
	INVALID_SECTOR,			/* Invalid Sector */
	INVALID_BLOCK,			/* Invalid Block */
	UNKNOWN_COMMAND,		/* Unknown Command */
	PROCESS_COMMAND_ERR,	/* Processing command */
	NOT_READ_ERROR,			/* Could not read memory from target */
	DRV_NOTAT_BREAK,		/* The drive was not at AFP_BreakReady */
	BUFFER_IS_NULL,			/* Could not allocate storage for the buffer */
	NO_ACCESS_SECTOR,		/* Cannot access the sector( could be locked or something is stored there that should not be touched ) */
	NUM_ERROR_CODES
};

struct ADI_BOOT_HEADER
{
    u32   dBlockCode;
    void* pTargetAddress;
    u32   dByteCount;
    u32   dArgument;
    
}; 

extern bool BlackFin_CheckFlash(u16 *crc, u16 *len);
extern bool at25df021_Init(u32 baudrate);
extern void at25df021_Destroy();


extern ERROR_CODE at25df021_Read(void *data, u32 stAdr, u16 count );
//extern ERROR_CODE at25df021_Read_DMA(byte *data, u32 stAdr, u16 count, bool *ready);
//extern ERROR_CODE at25df021_Read_IRQ(byte *data, u32 stAdr, u16 count, bool *ready);
//
extern ERROR_CODE at25df021_Write(const byte *data, u32 stAdr, u32 count, bool verify);

//
//extern ERROR_CODE at25df021_GetCRC16_IRQ(u32 stAdr, u16 count, bool *ready, u16 *crc);
//extern u16 at25df021_GetCRC16(u32 stAdr, u16 count);
//
//
//extern ERROR_CODE GetCodes(int *pnManCode, int *pnDevCode);
//extern ERROR_CODE GetSectorStartEnd( unsigned long *ulStartOff, unsigned long *ulEndOff, int nSector );
//extern ERROR_CODE GetSectorNumber( unsigned long ulAddr, int *pnSector );
//extern ERROR_CODE EraseFlash();
//extern ERROR_CODE EraseBlock(u32 adr);
//extern ERROR_CODE ResetFlash();
//extern u32 GetNumSectors();
//extern u32 GetSectorSize();
//
//byte ReadStatusRegister(void);


#endif // AT25DF021_H__14_09_2016__08_29 
