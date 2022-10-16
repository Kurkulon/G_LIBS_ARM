#include "xtrap.h"
#include "trap.h"
#include "trap_def.h"
#include "list.h"
#include "flash.h"
#include "vector.h"
#include "extern_def.h"

#pragma diag_suppress 550,177

#pragma O3
#pragma Otime

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct TrapReq
{
	TrapReq	*next;
	
	MAC		srcHA;
	u32		srcIP;
	u16		srcPort;
	u16		dlen;

	Trap	trap;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static TrapReq  traps[10];

//static List<TrapReq> freeTrapList;
static ListPtr<MB> reqTrapList;

static ListPtr<MB> txList;
//static List<SmallTx> txFree;

static MAC ComputerEmacAddr = {0,0};	// Our Ethernet MAC address and IP address
static u32 ComputerIpAddr	= IP32(192,168,3,254);
static u16 ComputerUDPPort	= 0;
static u16 srcUDPPort	= 0;
static u32 ComputerOldIpAddr	= 0;
static u16 ComputerOldUDPPort	= 0;
bool ComputerFind = false;




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void HandleRxPacket(EthUdp *h, u32 stat)
//{
////	if((h->iph.hl_v & 0x0F) != 5) { return; }	// Если IP пакет имеет поля `OPTION`, явно не наш пакет
//
//	u16 l = (unsigned short)(ReverseWord(h->udp.len) - sizeof(UdpHdr));
//
//	bool recapture = false;
//
//	unsigned int i;
//
//	if (ComputerEmacAddr != h->eth.src || ComputerIpAddr != h->iph.src || ComputerUDPPort != ReverseWord(h->udp.src)) 
//	{
//		recapture = true;
//	};
//
//	if (recapture || !ComputerFind)
//	{
//		u32 new_ip = h->iph.src;
//		u32 old_ip = ComputerIpAddr;
//		u16 new_port = ReverseWord(h->udp.src);
//		u16 old_port = ComputerUDPPort;
//		
//		if (ComputerFind) 
//		{
//			TRAP_INFO_SendLostIP(new_ip, new_port);
//		}
//		else 
//		{
//			ComputerFind = true;
//		};
//		
//		ComputerEmacAddr = h->eth.src;
//		ComputerIpAddr  = h->iph.src;
//		ComputerUDPPort = h->udp.src;
//
//		TRAP_INFO_SendCaptureIP(old_ip, old_port);
//	}
//     
//	TRAP_HandleRxData((char*)(h) + sizeof(EthUdp), l);
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool RequestTrap(Ptr<MB> &mb)
{
	EthUdp *h = ((EthUdp*)mb->GetDataPtr());

	if ((h->udp.len = ReverseWord(h->udp.len)) < 19) return false;

	u16 len = h->udp.len - sizeof(h->udp);

	if (len > sizeof(Trap)) return false;

	reqTrapList.Add(mb);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void FreeSmallTxBuffer(EthBuf* b)
//{
//	txFree.Add(b);
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//void FreeHugeTxBuffer(EthBuf* b)
//{
//	txFree.Add(b);
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SendTrap(Ptr<MB> &mb)
{
	((EthIp*)mb->GetDataPtr())->iph.off = 0;

	txList.Add(mb);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void SendFragTrap(Ptr<MB> &mb)
{
	txList.Add(mb);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateRequestTraps()
{
	static byte i = 0;
	static Ptr<MB> mb;
	static EthPtr et = {0};

	switch(i)
	{
		case 0:

			mb = reqTrapList.Get();

			if (mb.Valid())
			{
				et.eth = (EthHdr*)mb->GetDataPtr();
				i++;
			};

			break;

		case 1:

			if (!ComputerFind)
			{
				i = 3;
			}
			else if (ComputerEmacAddr != et.eth->src || ComputerIpAddr != et.eip->iph.src || ComputerUDPPort != et.eudp->udp.src)
			{
				i = 2;
			}
			else
			{
				i = 5;
			};

			break;

		case 2:

			if (TRAP_INFO_SendLostIP(ReverseDword(et.eip->iph.src), ReverseWord(et.eudp->udp.src)))
			{
				i++;
			};

			break;

		case 3:

			ComputerOldIpAddr	= ComputerIpAddr;
			ComputerOldUDPPort	= ComputerUDPPort;

			ComputerEmacAddr	= et.eth->src;
			ComputerIpAddr		= et.eip->iph.src;
			ComputerUDPPort		= et.eudp->udp.src;
			srcUDPPort			= et.eudp->udp.dst;

			ComputerFind = true;

			i++;

		case 4:

			i += (TRAP_INFO_SendCaptureIP(ReverseDword(ComputerOldIpAddr), ReverseWord(ComputerOldUDPPort))) ? 1 : 0;

			break;

		case 5:

			TRAP_HandleRxData(mb);

			mb.Free();

			i = 0;

			break;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateSendTraps()
{
	static TM32 tm;

	if (!EmacIsConnected())
	{
		return;
	};

	Ptr<MB> mb = txList.Get();

	if (mb.Valid())
	{
		EthUdp &et = *((EthUdp*)mb->GetDataPtr());

		et.eth.dest = ComputerEmacAddr;

		et.iph.dst = ComputerIpAddr;	

		if (et.iph.off == 0) { et.iph.id = GetIpID(); };

		TransmitFragUdp(mb, srcUDPPort, ComputerUDPPort);
	};
	
	if (ComputerFind && tm.Check(500))
	{ 
		TRAP_CLOCK_SendMain();
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateTraps()
{
	static byte i = 0;

	#define CALL(p) case (__LINE__-S): p; break;

	enum C { S = (__LINE__+3) };
	switch(i++)
	{
		CALL( UpdateRequestTraps()		);
		CALL( UpdateSendTraps()			);
		CALL( TRAP_Idle()				);
	};

	i = (i > (__LINE__-S-3)) ? 0 : i;

	#undef CALL
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
