#ifndef WIN32

#include "time.h"
#include <string.h>
#include "boot_isp.h"
#include "tftp.h"
#include "emac.h"
#include "list.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**************************************************************/
/**************************************************************/
//#define TFTP_DATA_BUFFER_SIZE	TFTP_DATA_CHUNK_SIZE
//#define TFTP_TX_BUFFER_SIZE	TFTP_DATA_BUFFER_SIZE + 16
/**************************************************************/
#define TFTP_OPCODE_RRQ		0x1
#define TFTP_OPCODE_WRQ		0x2
#define TFTP_OPCODE_DATA	0x3
#define TFTP_OPCODE_ACK		0x4
#define TFTP_OPCODE_ERROR	0x5

//#define TFTP_ERRORCODE_NOT_DEFINED			0x0
#define TFTP_ERRORCODE_FILE_NOT_FOUND			0x1
#define TFTP_ERRORCODE_ACCESS_VIOLATION			0x2
#define TFTP_ERRORCODE_DISK_FULL				0x3
#define TFTP_ERRORCODE_ILLEGAL_OPERATION		0x4
#define TFTP_ERRORCODE_UNKNOWN_ID				0x5
//#define TFTP_ERRORCODE_FILE_ALREADY_EXISTS	0x6
//#define TFTP_ERRORCODE_NO_SUCH_USER			0x7

//#define TFTP_MODE_NETASCII	"netascii"
//#define TFTP_MODE_OCTET		"octet"
//#define TFTP_MODE_MAIL		"mail"
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

__packed struct	TFTP_PACKET_type
{
	u16 opcode;
};	

__packed struct	TFTP_PACKET_PRQ_type
{
	u16 opcode;
};	

__packed struct	TFTP_PACKET_WRQ_type
{
	u16 opcode;
};	

__packed struct	TFTP_PACKET_DATA_type
{
	u16 opcode;
	u16 block;
};	

__packed struct	TFTP_PACKET_ACK_type
{
	u16 opcode;
	u16 block;
};	

__packed struct	TFTP_PACKET_ERROR_type
{
	u16 opcode;
	u16 error;
};


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

enum 
{
	TFTP_WAIT = 0, TFTP_READ_START, TFTP_READ_UPDATE, TFTP_WRITE_START,  TFTP_WRITE_UPDATE,
};


//unsigned char tftp_tx_buffer[TFTP_TX_BUFFER_SIZE];
//unsigned char tftp_data_buffer[TFTP_DATA_BUFFER_SIZE];

u8	tftp_read_command = TFTP_COMMAND_NONE;
u16 tftp_read_block = 0;
u16 tftp_read_block_ready = 0;
u16 tftp_read_block_size = 0;
u8	tftp_write_command = TFTP_COMMAND_NONE;
u16 tftp_write_block = 0;
u16 tftp_write_block_req = 0;
u16 tftp_write_block_ready = 0;
u16 tftp_write_block_size = 0;

u16 tftp_request = 0;
u16 tftp_processed = 0;
u16 tftp_err_opcode = 0;

//static byte tftp_state = 0;
static MAC ComputerEmacAddr = {0,0};	// Our Ethernet MAC address and IP address
static u32 ComputerIpAddr	= IP32(192,168,3,254);
static u16 ComputerUDPPort	= 0;
static u16 srcUDPPort	= 0;
//static u32 ComputerOldIpAddr	= 0;
//static u16 ComputerOldUDPPort	= 0;
//static bool ComputerFind = false;

//static TrapReq  traps[10];

//static List<Receive_Desc> freeTrapList;
static ListPtr<MB> reqTrapList;

static TM32 tm;
static TM32 reboot;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct	TFTPHdr
{
	u16		opcode;

	__packed union
	{
		__packed struct { 				byte data[2]; } RRQ;
		__packed struct { 				byte data[2]; } WRQ;
		__packed struct { u16 block; 	byte data[0]; } DATA;
		__packed struct { u16 block; 	byte data[0]; } ACK;
		__packed struct { u16 error; 	char data[0]; } ERROR;
	};
};	

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct TFTP
{
	TFTPHdr th;	
	byte data[128];
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct	EthTftp
{
	EthUdp	eudp;
	TFTP	tftp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//__packed struct SmallTx : public EthUdpBuf
//{
//	TFTPHdr th;		// 4
//
//	byte data[128];
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//__packed struct HugeTx : public SmallTx
//{
//	byte exdata[IP_MTU - sizeof(UdpHdr) - sizeof(TFTPHdr) - sizeof(data)];
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void SendFragTFTP(Ptr<MB> &mb)
{
	if (!EmacIsConnected())
	{
		return;
	};

	EthUdp &et = *((EthUdp*)mb->GetDataPtr());

	et.eth.dest = ComputerEmacAddr;

	et.iph.dst = ComputerIpAddr;	

	if (et.iph.off == 0) { et.iph.id = GetIpID(); };

	TransmitFragUdp(mb, srcUDPPort, ComputerUDPPort);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void SendTFTP(Ptr<MB> &mb)
{
	EthUdp &et = *((EthUdp*)mb->GetDataPtr());

	et.iph.off = 0;

	SendFragTFTP(mb);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool RequestTFTP(Ptr<MB> &mb)
{
	EthUdp &h = *((EthUdp*)mb->GetDataPtr());

	h.udp.len = ReverseWord(h.udp.len);

	reqTrapList.Add(mb);

	tftp_request++;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//SmallTx* GetSmallTxBuffer()
//{
//	static byte		indSmallTx = 0;
//	static SmallTx	smallTxBuf[16];
//
//	SmallTx *p = &smallTxBuf[indSmallTx];
//
//	if (p->len == 0)
//	{
//		p->len = 1;
//	}
//	else
//	{
//		p = 0;
//	};
//
//	indSmallTx += 1;
//
//	if (indSmallTx >= ArraySize(smallTxBuf)) indSmallTx = 0;
//
//	return p;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//HugeTx* GetHugeTxBuffer()
//{
//	static byte		indHugeTx = 0;
//	static HugeTx	hugeTxBuf[8];
//
//	HugeTx *p = &hugeTxBuf[indHugeTx];
//
//	if (p->len == 0)
//	{
//		p->len = 1;
//	}
//	else
//	{
//		p = 0;
//	};
//
//	indHugeTx += 1;
//
//	if (indHugeTx >= ArraySize(hugeTxBuf)) indHugeTx = 0;
//
//	return p;
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


/*********************TX****************************************************/
static void TFTP_SendData(Ptr<MB> &mb, u16 block, u16 size)
{
	if (!mb.Valid()) return;

	EthTftp &h = *((EthTftp*)mb->GetDataPtr());

	h.tftp.th.opcode = SWAP16(TFTP_OPCODE_DATA);
	h.tftp.th.DATA.block = SWAP16(block);
	mb->len = sizeof(EthUdp) + sizeof(h.tftp.th)+size;

	SendTFTP(mb);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendAck(u16 block)
{
	Ptr<MB> mb(AllocMemBuffer(sizeof(EthTftp)));

	if (!mb.Valid()) return;

	EthTftp &h = *((EthTftp*)mb->GetDataPtr());

	h.tftp.th.opcode = SWAP16(TFTP_OPCODE_ACK);
	h.tftp.th.ACK.block = SWAP16(block);
	mb->len = sizeof(EthUdp) + sizeof(h.tftp.th);

	SendTFTP(mb);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendError(u16 error, char *message)
{
	Ptr<MB> mb(AllocMemBuffer(sizeof(EthTftp)));

	if (!mb.Valid()) return;

	EthTftp &h = *((EthTftp*)mb->GetDataPtr());

	h.tftp.th.opcode = SWAP16(TFTP_OPCODE_ERROR);
	h.tftp.th.ERROR.error = SWAP16(error);

	u32 l = strlcpy(h.tftp.th.ERROR.data, message, sizeof(h.tftp.data));

	mb->len = sizeof(EthUdp) + sizeof(h.tftp.th) + l;

	SendTFTP(mb);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendVersion(u16 version)
{
	Ptr<MB> mb(AllocMemBuffer(sizeof(EthTftp)));

	if (!mb.Valid()) return;

	EthTftp &h = *((EthTftp*)mb->GetDataPtr());

	h.tftp.th.opcode = SWAP16(TFTP_OPCODE_DATA);
	h.tftp.th.DATA.block = SWAP16(1);
	
	DataPointer p(h.tftp.th.DATA.data);

	p.w[0] = version;

	mb->len = sizeof(EthUdp) + sizeof(h.tftp.th) + 2;

	SendTFTP(mb);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendMode(byte mode)
{
	Ptr<MB> mb(AllocMemBuffer(sizeof(EthTftp)));

	if (!mb.Valid()) return;

	EthTftp &h = *((EthTftp*)mb->GetDataPtr());

	h.tftp.th.opcode = SWAP16(TFTP_OPCODE_DATA);
	h.tftp.th.DATA.block = SWAP16(1);
	
	h.tftp.th.DATA.data[0] = mode;

	mb->len = sizeof(EthUdp) + sizeof(h.tftp.th) - sizeof(h.tftp.th.DATA.data) + 1;

	SendTFTP(mb);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool TFTP_HandleRxData(Ptr<MB> &mb)
{
	if (!mb.Valid()) return false;

	tftp_processed++;

	EthTftp *h = ((EthTftp*)mb->GetDataPtr());
	
	u16 len = h->eudp.udp.len;

	if(len < sizeof(TFTPHdr)) 
	{ 
		TFTP_SendError(TFTP_ERRORCODE_UNKNOWN_ID, "");
		return false; 
	};

	bool c = false;

	TFTPHdr &tftp = h->tftp.th;

	switch (SWAP16(tftp.opcode))
	{
     	case TFTP_OPCODE_RRQ:

			if(strncmp((char*)tftp.RRQ.data, TFTP_FILE_VERSION, len) == 0)
			{
				TFTP_SendVersion(ISP_GetBootloaderVersion());
			}
			else if(strncmp((char*)tftp.RRQ.data, TFTP_FILE_MODE, len) == 0)
			{
				TFTP_SendMode(1/*FLASH_Get_Mode()*/);
			}
			else if(strncmp((char*)tftp.RRQ.data, TFTP_FILE_PROGRAMM, len) == 0)
			{
				tftp_read_command = TFTP_COMMAND_PROGRAMM;
				tftp_read_block = 1;
				tftp_read_block_ready = 0;
			}	
			else 
			{
				tftp_read_command = TFTP_COMMAND_NONE;
				TFTP_SendError(TFTP_ERRORCODE_FILE_NOT_FOUND, "");
			};

			break;

     	case TFTP_OPCODE_WRQ:

			if (strncmp((char*)tftp.WRQ.data, TFTP_FILE_VERSION, len) == 0)
			{
				TFTP_SendError(TFTP_ERRORCODE_ACCESS_VIOLATION, "");
			}
			else if (strncmp((char*)tftp.WRQ.data, TFTP_FILE_MODE, len) == 0)
			{
				tftp_write_command = TFTP_COMMAND_MODE;
				TFTP_SendAck(0);
			}
			else if(strncmp((char*)tftp.WRQ.data, TFTP_FILE_PROGRAMM, len) == 0)
			{
				tftp_write_command = TFTP_COMMAND_PROGRAMM;
				tftp_write_block = 0;
				tftp_write_block_ready = 0;
				tftp_write_block_req = 0;
				TFTP_SendAck(0);
				ISP_InitFlashWrite();
			}	
			else 
			{
				tftp_write_command = TFTP_COMMAND_NONE;
				TFTP_SendError(TFTP_ERRORCODE_FILE_NOT_FOUND, "");
			};

			break;

     	case TFTP_OPCODE_ACK:

			if((SWAP16(tftp.ACK.block) == tftp_read_block_ready) && tftp_read_block)
			{
				tftp_read_block ++;
			}
			else
			{
   				tftp_read_block = 0;
				tftp_read_block_ready = 0;
			};

			break;

     	case TFTP_OPCODE_DATA:

			if (tftp_write_command == TFTP_COMMAND_PROGRAMM)
			{
				if(SWAP16(tftp.DATA.block) == (tftp_write_block_req + 1))
				{
					tftp_write_block = tftp_write_block_req + 1;
					tftp_write_block_req++;
					tftp_write_block_size = h->eudp.udp.len - sizeof(h->eudp.udp) - sizeof(tftp) + sizeof(tftp.DATA.data);

					FLWB& flwb = *((FLWB*)mb->GetDataPtr());

					flwb.adr = (tftp_write_block - 1) * TFTP_DATA_CHUNK_SIZE;
					flwb.dataLen = tftp_write_block_size; //TFTP_DATA_CHUNK_SIZE;
					flwb.dataOffset = tftp.DATA.data - flwb.data;
					
					c = ISP_RequestFlashWrite(mb);

					tm.Reset();
				};
			}
			else if (tftp_write_command == TFTP_COMMAND_MODE)
			{
				if(SWAP16(tftp.DATA.block) == 1 && tftp.DATA.data[0] == 0)
				{
					TFTP_SendAck(1);
					tftp_write_command = TFTP_COMMAND_REBOOT;
					reboot.Reset();
				};
			}
			else
			{
				tftp_write_block = 0;
				tftp_write_block_ready = 0;
				TFTP_SendError(TFTP_ERRORCODE_ILLEGAL_OPERATION, "");
			};

			break;

     	case TFTP_OPCODE_ERROR:

			break;

		default:

			tftp_err_opcode++;

			break;
	};

	return c;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateRequestTFTP()
{
	static byte i = 0;
	static Ptr<MB> mb;

	switch(i)
	{
		case 0:

			mb = reqTrapList.Get();

			if (mb.Valid())
			{
				i++;
			};

			break;

		case 1:
		{
			EthUdp* h = (EthUdp*)mb->GetDataPtr();

			ComputerEmacAddr	= h->eth.src;
			ComputerIpAddr		= h->iph.src;
			ComputerUDPPort		= h->udp.src;
			srcUDPPort			= h->udp.dst;

			if (!TFTP_HandleRxData(mb))
			{
				mb.Free();
			};

			i = 0;

			break;
		};
	};
} 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool TFTP_Idle()
{
	UpdateRequestTFTP();

	Ptr<MB> mb;

	bool c = true;

    if(tftp_read_block > tftp_read_block_ready) // запрос на чтение
	{
		switch (tftp_read_command)
		{
			case TFTP_COMMAND_PROGRAMM:

				mb = AllocMemBuffer(sizeof(EthTftp) + TFTP_DATA_CHUNK_SIZE);

				if (mb.Valid())
				{
					tftp_read_block_size = ISP_Flash_Read((tftp_read_block - 1) * TFTP_DATA_CHUNK_SIZE, ((EthTftp*)mb->GetDataPtr())->tftp.th.DATA.data, TFTP_DATA_CHUNK_SIZE);

					TFTP_SendData(mb, tftp_read_block, tftp_read_block_size);
					tftp_read_block_ready = tftp_read_block;

					if (tftp_read_block_size < TFTP_DATA_CHUNK_SIZE) tftp_read_block = 0;
				};

				break;

			default:

				TFTP_SendError(TFTP_ERRORCODE_ILLEGAL_OPERATION, "");
				tftp_read_block = 0;
				break;
		};
	};

    if(tftp_write_block > tftp_write_block_ready) // запрос на запись
	{
		switch (tftp_write_command)
		{
			case TFTP_COMMAND_PROGRAMM:

				if (ISP_GetFlashWriteOK() > tftp_write_block_ready || tm.Check(500))
				{
					tftp_write_block_ready++;// = tftp_write_block;
		 			TFTP_SendAck(tftp_write_block_ready);
				}
				else if (ISP_GetFlashWriteError() != 0)
				{
					TFTP_SendError(TFTP_ERRORCODE_DISK_FULL, "");
					tftp_write_block = 0;
				};

				break;

			default:

				TFTP_SendError(TFTP_ERRORCODE_ILLEGAL_OPERATION, "");
				tftp_read_block = 0;

				break;
		};



	};

	if (tftp_write_command == TFTP_COMMAND_REBOOT && reboot.Timeout(100))
	{
		c = false;
	};

	return c;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // #ifndef WIN32
