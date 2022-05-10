#ifndef WIN32

#include "time.h"
#include <string.h>
#include "tftp_def.h"
//#include "flash.h"
#include "tftp.h"
#include "emac.h"
#include "list.h"

enum 
{
	TFTP_WAIT = 0, TFTP_READ_START, TFTP_READ_UPDATE, TFTP_WRITE_START,  TFTP_WRITE_UPDATE,
};


//unsigned char tftp_tx_buffer[TFTP_TX_BUFFER_SIZE];
//unsigned char tftp_data_buffer[TFTP_DATA_BUFFER_SIZE];

unsigned char tftp_read_command = TFTP_COMMAND_NONE;
unsigned short tftp_read_block = 0;
unsigned short tftp_read_block_ready = 0;
unsigned short tftp_read_block_size = 0;
unsigned char tftp_write_command = TFTP_COMMAND_NONE;
unsigned short tftp_write_block = 0;
unsigned short tftp_write_block_req = 0;
unsigned short tftp_write_block_ready = 0;
unsigned short tftp_write_block_size = 0;

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
static List<EthBuf> reqTrapList;

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

static void SendFragTFTP(EthBuf *p)
{
	if (!EmacIsConnected())
	{
		return;
	};

	EthUdp &et = (EthUdp&)p->eth;

	et.eth.dest = ComputerEmacAddr;

	et.iph.dst = ComputerIpAddr;	

	if (et.iph.off == 0) { et.iph.id = GetIpID(); };

	TransmitFragUdp(p, srcUDPPort, ComputerUDPPort);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void SendTFTP(EthBuf *p)
{
	EthUdp &et = (EthUdp&)p->eth;

	et.iph.off = 0;

	SendFragTFTP(p);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool RequestTFTP(EthBuf* mb)
{
	EthUdp *h = (EthUdp*)&mb->eth;

	h->udp.len = ReverseWord(h->udp.len);

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
static void TFTP_SendData(EthBuf *mb, u16 block, u16 size)
{
	if (mb == 0) return;

	EthTftp *h = (EthTftp*)&mb->eth;

	h->tftp.th.opcode = SWAP16(TFTP_OPCODE_DATA);
	h->tftp.th.DATA.block = SWAP16(block);
	mb->len = sizeof(EthUdp) + sizeof(h->tftp.th)+size;

	SendTFTP(mb);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendAck(u16 block)
{
	EthBuf *buf = GetSysEthBuffer();

	if (buf == 0) return;

	EthTftp *h = (EthTftp*)&buf->eth;

	h->tftp.th.opcode = SWAP16(TFTP_OPCODE_ACK);
	h->tftp.th.ACK.block = SWAP16(block);
	buf->len = sizeof(EthUdp) + sizeof(h->tftp.th);

	SendTFTP(buf);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendError(u16 error, char *message)
{
	EthBuf *buf = GetSysEthBuffer();

	if (buf == 0) return;

	EthTftp *h = (EthTftp*)&buf->eth;

	h->tftp.th.opcode = SWAP16(TFTP_OPCODE_ERROR);
	h->tftp.th.ERROR.error = SWAP16(error);

	u32 l = strlcpy(h->tftp.th.ERROR.data, message, sizeof(h->tftp.data));

	buf->len = sizeof(EthUdp) + sizeof(h->tftp.th) + l;

	SendTFTP(buf);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendVersion(u16 version)
{
	EthBuf *buf = GetSysEthBuffer();

	if (buf == 0) return;

	EthTftp *h = (EthTftp*)&buf->eth;

	h->tftp.th.opcode = SWAP16(TFTP_OPCODE_DATA);
	h->tftp.th.DATA.block = SWAP16(1);
	
	DataPointer p(h->tftp.th.DATA.data);

	p.w[0] = version;

	buf->len = sizeof(EthUdp) + sizeof(h->tftp.th) + 2;

	SendTFTP(buf);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void TFTP_SendMode(byte mode)
{
	EthBuf *buf = GetSysEthBuffer();

	if (buf == 0) return;

	EthTftp *h = (EthTftp*)&buf->eth;

	h->tftp.th.opcode = SWAP16(TFTP_OPCODE_DATA);
	h->tftp.th.DATA.block = SWAP16(1);
	
	h->tftp.th.DATA.data[0] = mode;

	buf->len = sizeof(EthUdp) + sizeof(h->tftp.th) - sizeof(h->tftp.th.DATA.data) + 1;

	SendTFTP(buf);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool TFTP_HandleRxData(EthBuf *mb)
{
	if (mb == 0) return false;

	tftp_processed++;

	EthTftp *h = (EthTftp*)&mb->eth;
	
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
				TFTP_SendVersion(TFTP_GetBootloaderVersion());
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

					FLWB& flwb = *((FLWB*)&mb->eth);

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
	static EthBuf *mb = 0;

	EthUdp* h = (EthUdp*)&mb->eth;

	switch(i)
	{
		case 0:

			mb = reqTrapList.Get();

			if (mb != 0)
			{
				i++;
			};

			break;

		case 1:

			ComputerEmacAddr	= h->eth.src;
			ComputerIpAddr		= h->iph.src;
			ComputerUDPPort		= h->udp.src;
			srcUDPPort			= h->udp.dst;

			if (!TFTP_HandleRxData(mb))
			{
				mb->Free();
			};

			i = 0;

			break;
	};
} 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool TFTP_Idle()
{
	UpdateRequestTFTP();

	EthBuf* buf = 0;

	bool c = true;

    if(tftp_read_block > tftp_read_block_ready) // запрос на чтение
	{
		switch (tftp_read_command)
		{
			case TFTP_COMMAND_PROGRAMM:

				buf = GetHugeTxBuffer();

				if (buf != 0)
				{
					tftp_read_block_size = ISP_Flash_Read((tftp_read_block - 1) * TFTP_DATA_CHUNK_SIZE, ((EthTftp*)&buf->eth)->tftp.th.DATA.data, TFTP_DATA_CHUNK_SIZE);

					TFTP_SendData(buf, tftp_read_block, tftp_read_block_size);
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
