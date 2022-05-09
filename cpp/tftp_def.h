#ifndef TFTP_DEF_H__25_03_2020__18_35
#define TFTP_DEF_H__25_03_2020__18_35

#include "emac.h"

/************ Common ******************************************/
//extern void TFTP_HandleRxData(char *data, unsigned int size);
//extern void TFTP_HandleRxError();
//extern void TFTP_Init();
//extern bool TFTP_Idle();
/**************************************************************/
#define TFTP_DATA_CHUNK_SIZE	512 // ������ 256 (�������� ������)
/**************************************************************/
#define TFTP_DATA_BUFFER_SIZE	TFTP_DATA_CHUNK_SIZE
#define TFTP_TX_BUFFER_SIZE	TFTP_DATA_BUFFER_SIZE + 16
/**************************************************************/
#define TFTP_OPCODE_RRQ		0x1
#define TFTP_OPCODE_WRQ		0x2
#define TFTP_OPCODE_DATA	0x3
#define TFTP_OPCODE_ACK		0x4
#define TFTP_OPCODE_ERROR	0x5

#define TFTP_ERRORCODE_NOT_DEFINED			0x0
#define TFTP_ERRORCODE_FILE_NOT_FOUND		0x1
#define TFTP_ERRORCODE_ACCESS_VIOLATION		0x2
#define TFTP_ERRORCODE_DISK_FULL			0x3
#define TFTP_ERRORCODE_ILLEGAL_OPERATION	0x4
#define TFTP_ERRORCODE_UNKNOWN_ID			0x5
#define TFTP_ERRORCODE_FILE_ALREADY_EXISTS	0x6
#define TFTP_ERRORCODE_NO_SUCH_USER			0x7

#define TFTP_MODE_NETASCII	"netascii"
#define TFTP_MODE_OCTET		"octet"
#define TFTP_MODE_MAIL		"mail"
/******************************************************/
enum 
{
	TFTP_COMMAND_NONE,
	TFTP_COMMAND_VERSION,
	TFTP_COMMAND_MODE,
	TFTP_COMMAND_PROGRAMM,
	TFTP_COMMAND_REBOOT
};

#define TFTP_FILE_VERSION	"bootloader_version"
#define TFTP_FILE_MODE		"bootloader_mode"
#define TFTP_FILE_PROGRAMM	"system.img"
/******************************************************/
typedef struct	__attribute__ ((packed))
{
	unsigned short opcode;
} TFTP_PACKET_type;	

typedef struct	__attribute__ ((packed))
{
	unsigned short opcode;
} TFTP_PACKET_PRQ_type;	

typedef struct	__attribute__ ((packed))
{
	unsigned short opcode;
} TFTP_PACKET_WRQ_type;	

typedef struct	__attribute__ ((packed))
{
	unsigned short opcode;
	unsigned short block;
} TFTP_PACKET_DATA_type;	

typedef struct	__attribute__ ((packed))
{
	unsigned short opcode;
	unsigned short block;
} TFTP_PACKET_ACK_type;	

typedef struct	__attribute__ ((packed))
{
	unsigned short opcode;
	unsigned short error;
} TFTP_PACKET_ERROR_type;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern u16	TFTP_GetBootloaderVersion();
extern void	ISP_InitFlashWrite();
extern bool ISP_RequestFlashWrite(EthBuf *mb);
extern u32	ISP_Flash_Read(u32 addr, byte *data, u32 size);
extern u32	ISP_GetFlashWriteError();
extern u32	ISP_GetFlashWriteOK();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#endif // TFTP_DEF_H__25_03_2020__18_35


