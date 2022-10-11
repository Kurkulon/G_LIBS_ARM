#ifndef EMAC_IMP_H__11_10_2022__18_02
#define EMAC_IMP_H__11_10_2022__18_02

#ifdef WIN32

#include <winsock2.h>
#include <WS2TCPIP.H>
#include <conio.h>
#include <stdio.h>

static SOCKET	lstnSocket;

static HANDLE handleTxThread;
static HANDLE handleRxThread;

DWORD txThreadCount = 0;

#else

#include "core.h"
#include "EMAC_DEF.h"

//#pragma O3
//#pragma Otime

#endif

#include "emac.h"
#include "xtrap.h"
#include "list.h"
#include "SEGGER_RTT.h"

//#pragma diag_suppress 546,550,177

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct SysEthBuf : public EthBuf
{
	byte data[((ETH_RX_BUF_SIZE+3) & ~3) - sizeof(EthBuf::eth)];

	static List<SysEthBuf> freeList;

	static	SysEthBuf*	Alloc()	{ return freeList.Get(); }
	virtual	u32			MaxLen() { return sizeof(eth) + sizeof(data); }
	virtual void		Free()	{ freeList.Add(this); }

						SysEthBuf() { freeList.Init(); freeList.Add(this); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct HugeTx : public EthBuf
{
	IPheader	iph; // 20
	UdpHdr		udp; // 8
	byte		data[IP_MTU - sizeof(UdpHdr) + (sizeof(EthBuf::eth) & 3)];

	static List<HugeTx> freeList;

	static	HugeTx*		Alloc()		{ return freeList.Get(); }
	virtual	u32			MaxLen()	{ return sizeof(eth) + sizeof(data); }
	virtual void		Free()		{ freeList.Add(this); }

						HugeTx()	{ freeList.Init(); freeList.Add(this); }

};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

List<SysEthBuf> SysEthBuf::freeList;
List<HugeTx>	HugeTx::freeList;

static SysEthBuf	sysTxBuf[NUM_SYS_TXBUF];
static HugeTx		hugeTxBuf[NUM_HUGE_TXBUF];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* Net_Config.c */

//#define OUR_IP_ADDR   	IP32(192, 168, 3, 234)
//#define DHCP_IP_ADDR   	IP32(192, 168, 3, 254)

//static const MAC hwAdr = HW_EMAC_GetHwAdr(); //{0x12345678, 0x9ABC};
static const MAC hwBroadCast = {0xFFFFFFFF, 0xFFFF};
static const u32 ipAdr = OUR_IP_ADDR;
static const u32 ipMask = OUR_IP_MASK;

//static const u16 udpInPort = ReverseWord(HW_EMAC_GetUdpInPort());
//static const u16 udpOutPort = ReverseWord(HW_EMAC_GetUdpOutPort());

bool emacConnected = false;
bool emacEnergyDetected = false;
bool emacCableNormal = false;

/* Local variables */
//static const byte PHYA = HW_EMAC_GetAdrPHY();
static byte RxBufIndex = 0;
static byte TxBufIndex = 0;
static byte TxFreeIndex = 0;

enum	StateEM { LINKING, CONNECTED };	

StateEM stateEMAC = LINKING;

static byte linkState = 0;

//static byte linkState = 0;

u32 reqArpCount = 0;
u32 reqIpCount = 0;
u32 reqUdpCount = 0;
u32 reqIcmpCount = 0;
u32 rxCount = 0;
//u32 countBNA = 0;
//u32 countREC = 0;
//u32 countRXOVR = 0;
//u32 countHNO = 0;

u32 trp[4] = {-1};

u16  txIpID = 0;


/* GMAC local IO buffers descriptors. */
#ifdef CPU_SAME53	
	Receive_Desc	Rx_Desc[NUM_RX_BUF];
	Transmit_Desc	Tx_Desc[NUM_TX_DSC];
#elif defined(CPU_XMC48)
	Receive_Desc	Rx_Desc[NUM_RX_BUF];
	Transmit_Desc	Tx_Desc[NUM_TX_DSC];
#endif

/* GMAC local buffers must be 8-byte aligned. */
//byte rx_buf[NUM_RX_BUF][ETH_RX_BUF_SIZE];
//byte tx_buf[NUM_TX_BUF][ETH_TX_BUF_SIZE];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//static byte			indSysTx = 0;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static List<EthBuf> txList;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef WIN32

static void rx_descr_init (void);
static void tx_descr_init (void);

#ifdef CPU_SAME53	
	inline void EnableMDI() { HW::GMAC->NCR |= GMAC_MPE; }
	inline void DisableMDI() { HW::GMAC->NCR &= ~GMAC_MPE; }
			bool IsReadyPHY() { return HW::GMAC->NSR & GMAC_IDLE; }
			u16 ResultPHY() { return HW::GMAC->MAN; }
	inline	bool CheckStatusIP(u32 stat) { return (stat & RD_IP_CHECK); }
	inline 	bool CheckStatusUDP(u32 stat) { return (stat & RD_IP_CHECK) == RD_IP_UDP_OK; }
	inline	void ResumeReceiveProcessing() {}

#elif defined(CPU_XMC48)
	inline void EnableMDI() { /*HW::GMAC->NCR |= GMAC_MPE;*/ }
	inline void DisableMDI() { /*HW::GMAC->NCR &= ~GMAC_MPE;*/ }
			bool IsReadyPHY() { return (HW::ETH0->GMII_ADDRESS & GMII_MB) == 0; }
			u16 ResultPHY() { return HW::ETH0->GMII_DATA; }
	inline	bool CheckStatusIP(u32 stat) { return (stat & RD_IP_ERR) == 0; }
	inline 	bool CheckStatusUDP(u32 stat) { return (stat & RD_UDP_ERR) == 0; }
	inline	void ResumeReceiveProcessing() { HW::ETH0->STATUS = ETH_STATUS_RU_Msk; HW::ETH0->RECEIVE_POLL_DEMAND = 0; }
#endif

	void HW_EMAC_StartLink() { linkState = 0; }
	inline bool IsBusyPHY() { return !IsReadyPHY(); }

#else

	inline bool CheckStatusUDP(u32 stat) { return true; }
//	static const u16 udpInPort = ReverseWord(HW_EMAC_GetUdpInPort());
//	static const u16 udpOutPort = ReverseWord(HW_EMAC_GetUdpOutPort());

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

EthBuf* GetSysEthBuffer()
{
	return SysEthBuf::Alloc();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

EthBuf* GetHugeTxBuffer()
{
	return HugeTx::Alloc();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef WIN32

static Transmit_Desc* GetTxDesc()
{
	Transmit_Desc *p = 0;
	Transmit_Desc &td = Tx_Desc[TxBufIndex];

	if (td.ChkFree())
	{
		p = &td;
		TxBufIndex = (td.ChkWrap()) ? 0 : TxBufIndex + 1;
	};

	return p;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void FreeTxDesc()
{
	Transmit_Desc &td = Tx_Desc[TxFreeIndex];

	if (td.ChkTransmit())
	{
		td.Free();

		if (__debug && !HW::RamCheck(td.GetAdr())) __breakpoint(0); 

		EthBuf* b; b = (EthBuf*)(td.GetAdr() - ((byte*)&b->eth - (byte*)b));

		b->Free();

		TxFreeIndex = (td.ChkWrap()) ? 0 : TxFreeIndex + 1;

		if (__debug && TxFreeIndex >= ArraySize(Tx_Desc)) __breakpoint(0);
	};
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//static void UpdateStatistic()
//{
//	using namespace HW;
//
//	stat.FR			+= 	GMAC->FR;			
//	stat.BCFR		+= 	GMAC->BCFR;		
//	stat.MFR		+= 	GMAC->MFR;		
//	stat.PFR		+= 	GMAC->PFR;		
//	stat.BFR64		+= 	GMAC->BFR64;	
//	stat.TBFR127	+= 	GMAC->TBFR127;	
//	stat.TBFR255 	+= 	GMAC->TBFR255;	
//	stat.TBFR511 	+= 	GMAC->TBFR511;	
//	stat.TBFR1023 	+= 	GMAC->TBFR1023;	
//	stat.TBFR1518 	+= 	GMAC->TBFR1518;	
//	stat.TMXBFR		+= 	GMAC->TMXBFR;		
//	stat.UFR 		+= 	GMAC->UFR;		
//	stat.OFR 		+= 	GMAC->OFR;		
//	stat.JR			+= 	GMAC->JR;			
//	stat.FCSE		+= 	GMAC->FCSE;		
//	stat.LFFE		+= 	GMAC->LFFE;		
//	stat.RSE		+= 	GMAC->RSE;		
//	stat.AE			+= 	GMAC->AE;			
//	stat.RRE		+= 	GMAC->RRE;		
//	stat.ROE		+= 	GMAC->ROE;		
//	stat.IHCE		+= 	GMAC->IHCE;		
//	stat.TCE		+= 	GMAC->TCE;		
//	stat.UCE		+= 	GMAC->UCE;		
//}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 IpChkSum(u16 *p, u16 size)
{
	register u32 sum = 0;
	register u32 t;

#ifndef WIN32

	__asm
	{
loop:
		LDRH	t, [p], #2 
		ADD		sum, t
		SUBS	size, size, #1
		BNE		loop

		LSR		t, sum, #16
		AND		sum, sum, #0xFFFF
		ADD		sum, sum, t
		ADD		sum, sum, sum, LSR#16 
	};
 
#endif

	return ~sum;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool TransmitEth(EthBuf *b)
{
	if (b == 0 || b->len < sizeof(EthHdr))
	{
		return false;
	};

#ifndef WIN32

	b->eth.src = hwAdr;

	txList.Add(b);

#else

	txList.Add(b);

	ResumeThread(handleTxThread);

	//EthPtr ep;

	//ep.eth = &b->eth;

	//sockaddr_in srvc;

	//srvc.sin_family = AF_INET;
	//srvc.sin_addr.s_addr = ep.eudp->iph.dst;
	//srvc.sin_port = ep.eudp->udp.dst;

	//in_addr srcip;
	//in_addr dstip;

	//srcip.S_un.S_addr = ep.eip->iph.src;
	//dstip.S_un.S_addr = ep.eip->iph.dst;

	//int len = sendto(lstnSocket, (char*)&ep.eip->iph, b->len - sizeof(b->eth), 0, (SOCKADDR*)&srvc, sizeof(srvc)); 

	//b->len = 0;


#endif

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool TransmitIp(EthBuf *mb)
{
	if (mb == 0 || mb->len < sizeof(EthIp)) return false;

	EthIp &b = (EthIp&)mb->eth;

	b.iph.off = 0;		

	b.eth.protlen = SWAP16(PROT_IP);

	b.iph.hl_v = 0x45;	
	b.iph.tos = 0;		
	b.iph.ttl = 64;		
	b.iph.sum = 0;		
	b.iph.src = ipAdr;		
	b.iph.len = ReverseWord(mb->len - sizeof(EthHdr));		

	b.iph.sum = IpChkSum((u16*)&b.iph, 10);

	return TransmitEth(mb);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool TransmitFragIp(EthBuf *mb)
{
	if (mb == 0 || mb->len < sizeof(EthIp))	return false;

	EthIp &b = (EthIp&)mb->eth;

	b.eth.protlen = SWAP16(PROT_IP);

	b.iph.hl_v = 0x45;	
	b.iph.tos = 0;		
	b.iph.ttl = 64;		
	b.iph.sum = 0;		
	b.iph.src = ipAdr;		
	b.iph.off = ReverseWord(b.iph.off);		
	b.iph.len = ReverseWord(mb->len - sizeof(EthHdr));		

	b.iph.sum = IpChkSum((u16*)&b.iph, 10);

	return TransmitEth(mb);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool TransmitUdp(EthBuf *mb)
{
	if (mb == 0 || mb->len < sizeof(EthUdp)) return false;

	EthUdp &b = (EthUdp&)mb->eth;

	b.iph.p = PROT_UDP;
	//b.udp.src = udpOutPort;
	b.udp.xsum = 0;
	b.udp.len = ReverseWord(mb->len - sizeof(EthIp));

	return TransmitIp(mb);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool TransmitFragUdp(EthBuf *mb, u16 src, u16 dst)
{
	if (mb == 0 || mb->len < sizeof(EthUdp)) return false;

	EthUdp &b = (EthUdp&)mb->eth;

	b.iph.p = PROT_UDP;

	if ((b.iph.off & 0x1FFF) == 0)
	{
		b.udp.dst = dst;
		b.udp.src = src;
		b.udp.xsum = 0;
		b.udp.len = ReverseWord((b.iph.off & 0x2000) ? b.udp.len : (mb->len - sizeof(EthIp)));
	};

	return TransmitFragIp(mb);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestARP(EthBuf *mb)
{
	EthArp *h = (EthArp*)&mb->eth;

	if (ReverseWord(h->arp.op) == ARP_REQUEST) // ARP REPLY operation
	{     
		if (h->arp.tpa == ipAdr)
		{
			reqArpCount++;

			EthBuf *buf = GetSysEthBuffer();

			if (buf == 0) return false;

			EthArp *t = (EthArp*)&buf->eth;

			t->eth.dest = h->eth.src;

			t->eth.protlen = SWAP16(PROT_ARP);

			t->arp.hrd = 0x100;	
			t->arp.pro = 8;	
			t->arp.hln = 6;	
			t->arp.pln = 4;	
			t->arp.op =  SWAP16(ARP_REPLY);				

			t->arp.tha = h->arp.sha;
			t->arp.sha = hwAdr;

			t->arp.tpa = h->arp.spa;
			t->arp.spa = ipAdr;

			buf->len = sizeof(EthArp);

			TransmitEth(buf);
		};	
	};

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestICMP(EthBuf* mb)
{
	EthIcmp *h = (EthIcmp*)&mb->eth;

	if(h->icmp.type == ICMP_ECHO_REQUEST)
	{
		reqIcmpCount++;

		EthIpBuf *buf = (EthIpBuf*)GetSysEthBuffer();

		if (buf == 0) return false;

		EthIcmp *t = (EthIcmp*)&buf->eth;

		t->eth.dest = h->eth.src;
		t->iph.len = h->iph.len;		
		t->iph.id = GetIpID(); //h->iph.id;		
		t->iph.p = PROT_ICMP;		
		t->iph.dst = h->iph.src;	

		u16 icmp_len = (ReverseWord(t->iph.len) - 20);	// Checksum of the ICMP Message

		if (icmp_len & 1)
		{
			*((byte*)&h->icmp + icmp_len) = 0;
			icmp_len ++;
		};

		icmp_len >>= 1;

		u16 *d = (u16*)&t->icmp;
		u16 *s = (u16*)&h->icmp;
		u16 c = icmp_len;

		while (c > 0)
		{
			*d++ = *s++; c--;
		};

		t->icmp.type = ICMP_ECHO_REPLY;
		t->icmp.code = 0;
		t->icmp.cksum = 0;

		t->icmp.cksum = IpChkSum((u16*)&t->icmp, icmp_len);

		buf->len = ReverseWord(t->iph.len) + sizeof(t->eth);

		TransmitIp(buf);
	};

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestDHCP(EthBuf *mb)
{
	EthDhcp *h = (EthDhcp*)&mb->eth;

	if (h->dhcp.op != 1) return false;

	i32 optLen = (i32)ReverseWord(h->iph.len) - sizeof(h->iph) - sizeof(h->udp) - 240;

	if (optLen < 3 || h->dhcp.magic != DHCPCOOKIE) return false;

	i32 i = 0; 
	bool c = false;

	while (i < optLen)
	{
		if (h->dhcp.options[i] == 53)
		{
			c = true; break;
		};
		
		i++;

		i += h->dhcp.options[i];
	};

	if (!c) return false;

	byte op = h->dhcp.options[i+2];

	if (op != DHCPDISCOVER && op != DHCPREQUEST) return false;

	EthIpBuf *buf = (EthIpBuf*)GetSysEthBuffer();

	if (buf == 0) return false;

	EthDhcp *t = (EthDhcp*)&buf->eth;



	t->dhcp.op = 2;
	t->dhcp.htype = 1;
	t->dhcp.hlen = 6;
	t->dhcp.hops = 0;
	t->dhcp.xid = h->dhcp.xid;
	t->dhcp.secs = 0;
	t->dhcp.flags = 0;
	t->dhcp.ciaddr = 0;
	t->dhcp.yiaddr = DHCP_IP_ADDR; // New client IP
	t->dhcp.siaddr = ipAdr;
	t->dhcp.giaddr = 0;
	t->dhcp.chaddr = h->dhcp.chaddr; //h->eth.src;
	t->dhcp.magic = DHCPCOOKIE;

	DataPointer p(t->dhcp.options);

	*p.b++ = 53;
	*p.b++ = 1;
	*p.b++ = (op == DHCPDISCOVER) ? DHCPOFFER : DHCPACK;

	*p.b++ = 1; // Sub-net Mask
	*p.b++ = 4;
	*p.d++ = ipMask;

	*p.b++ = 51; // IP Address Lease Time
	*p.b++ = 4;
	*p.d++ = SWAP32(3600*24);

	*p.b++ = 54; // Server IP
	*p.b++ = 4;
	*p.d++ = ipAdr;

	*p.b++ = 33;	// Static Route
	*p.b++ = 8;
	*p.d++ = ipAdr;	// Destination
	*p.d++ = t->dhcp.yiaddr; // Router

	*p.b++ = -1; // End option

	u16 ipLen = sizeof(EthDhcp) - sizeof(t->dhcp.options) + (p.b - (byte*)t->dhcp.options);

	t->eth.dest = hwBroadCast;
	t->iph.id = GetIpID(); //h->iph.id;		
	t->iph.p = PROT_UDP;		
	t->iph.dst = -1; //BroadCast	

	t->udp.src = BOOTPS;
	t->udp.dst = BOOTPC;
	t->udp.len = ReverseWord(ipLen - sizeof(t->iph));
	t->udp.xsum = 0;

	buf->len = ipLen + sizeof(t->eth);

	TransmitIp(buf);

	return false;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestUDP(EthBuf* mb)
{
	EthUdp *h = (EthUdp*)&mb->eth;

	bool c = false;

	switch (h->udp.dst)
	{
		case BOOTPS:	c = RequestDHCP(mb);		break;
		case udpInPort:	c = HW_EMAC_RequestUDP(mb);	break;
	};

	reqUdpCount++;

	return c;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static bool RequestIP(EthBuf* mb, u32 stat)
{
	EthIp *h = (EthIp*)&mb->eth;

	if (h->iph.hl_v != 0x45) return false;

	reqIpCount++;

	bool c = false;

	switch(h->iph.p)
	{
		case PROT_ICMP:	c = RequestICMP(mb);	break; 

		case PROT_UDP:	if (CheckStatusUDP(stat)) { c = RequestUDP(mb); };		break;
	};

	return c;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void RecieveFrame()
{
#ifndef WIN32

	Receive_Desc &buf = Rx_Desc[RxBufIndex];

	if(!buf.CheckOwn())
	{
	//		FreeTxDesc();
	}
	else
	{
		bool c = false;

		SysEthBuf *mb = 0;

		if (buf.CheckRcvFrame()) // buffer contains a whole frame
		{
			if (__debug && !HW::RamCheck(buf.GetAdr())) __breakpoint(0); 

			mb = (SysEthBuf*)(buf.GetAdr() - ((byte*)&mb->eth - (byte*)mb));

			switch (ReverseWord(mb->eth.protlen))
			{
				case PROT_ARP: // ARP Packet format

					c = RequestARP(mb); break; 

				case PROT_IP:	// IP protocol frame

					if (CheckStatusIP(buf.GetStatus()))
					{
						c = RequestIP(mb, buf.GetStatus());
					};

					break;
			};

			if (c)
			{
				buf.ClrAdr();
			};
		};

		if (buf.GetAdr() == 0)
		{
			EthBuf* nmb = GetSysEthBuffer();

			if (nmb != 0)
			{
				buf.SetAdr(&nmb->eth, nmb->MaxLen());
			};
		}
		else
		{
			buf.SetOwn();
		};

		++rxCount;

		ResumeReceiveProcessing();

		RxBufIndex = (buf.ChkWrap()) ? 0 : (RxBufIndex+1);

		if (__debug && RxBufIndex >= ArraySize(Rx_Desc)) __breakpoint(0);
	};

#else // #ifndef WIN32

	//static u32 pt = 0;

	//u32 t = GetMilliseconds();

	//if (t != pt)
	//{
	//	pt = t;

	//	EthBuf *buf = GetSysEthBuffer();

	//	if (buf != 0)
	//	{
	//		EthPtr ep;

	//		ep.eth = &buf->eth;

	//		while (1)
	//		{
	//			int len = recv(lstnSocket, (char*)&ep.eip->iph, buf->MaxLen()-sizeof(buf->eth), 0); 

	//			if (len > 0) //(len != SOCKET_ERROR/* && len >= sizeof(ep.eip->iph)*/)
	//			{
	//				//in_addr srcip;
	//				//in_addr dstip;

	//				//srcip.S_un.S_addr = ep.eip->iph.src;
	//				//dstip.S_un.S_addr = ep.eip->iph.dst;

	//				//printf("Recv prot: %hi, srcip %08lX, dstip %08lX, len:%i\n", ep.eip->iph.p, ReverseDword(ep.eip->iph.src), ReverseDword(ep.eip->iph.dst), len);

	//				RequestIP(buf, 0);
	//			}
	//			else
	//			{
	//				//int	error = WSAGetLastError(); //WSAEOPNOTSUPP

	//				break;
	//			};
	//		};

	//		//Printf(20, 6, 0xF0, "txLocksCount %-9lu", txList.locks_count);
	//	};
	//};

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void UpdateTransmit()
{
#ifndef WIN32

	static byte i = 0;

	static EthBuf *buf = 0;

	static Transmit_Desc *dsc = 0;

	switch (i)
	{
		case 0:

			if ((buf = txList.Get()) != 0)
			{
				i += 1;
			}
			else
			{
				FreeTxDesc();
			};

			break;

		case 1:

			if ((dsc = GetTxDesc()) != 0)
			{
				dsc->SetAdr(&buf->eth, buf->len);

				#ifdef CPU_SAME53	

					//dsc->addr1 = &buf->eth;
					//dsc->stat &= TD_TRANSMIT_OK|TD_TRANSMIT_WRAP;
					//dsc->stat |= TD_LAST_BUF | (buf->len & TD_LENGTH_MASK);
					//dsc->stat &= ~TD_TRANSMIT_OK;

					HW::GMAC->TSR = 0x17F;
					HW::GMAC->NCR |= GMAC_TXEN|GMAC_TSTART;

				#elif defined(CPU_XMC48)

					//dsc->addr1 = &buf->eth;
					//dsc->addr2 = 0;
					//dsc->ctrl = TBS1(buf->len);
					//dsc->stat |= TD0_LS|TD0_FS;
					//dsc->stat |= TD0_OWN;

					HW::ETH0->MAC_CONFIGURATION |= MAC_TE;
					HW::ETH0->OPERATION_MODE |= ETH_OPERATION_MODE_ST_Msk;

					//Clear TU flag to resume processing
					HW::ETH0->STATUS = ETH_STATUS_TU_Msk;

					//Instruct the DMA to poll the transmit descriptor list
					HW::ETH0->TRANSMIT_POLL_DEMAND = 0;

				#endif

				i += 1;
			}
			else
			{
				FreeTxDesc();
			};

			break;

		case 2:

			FreeTxDesc();

			i = 0;

			break;
	};
#else

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef WIN32

DWORD WINAPI TxThreadFunction(LPVOID lpParam) 
{
	EthBuf *b = 0;

	u32 count = 0;

	while(1)
	{
		b = txList.Get();

		if (b != 0)
		{
			EthPtr ep;

			ep.eth = &b->eth;

			sockaddr_in srvc;

			srvc.sin_family = AF_INET;
			srvc.sin_addr.s_addr = ep.eudp->iph.dst;
			srvc.sin_port = ep.eudp->udp.dst;

			in_addr srcip;
			in_addr dstip;

			srcip.S_un.S_addr = ep.eip->iph.src;
			dstip.S_un.S_addr = ep.eip->iph.dst;

			int len = sendto(lstnSocket, (char*)&ep.eip->iph, b->len - sizeof(b->eth), 0, (SOCKADDR*)&srvc, sizeof(srvc)); 

			b->len = 0;

			b->Free();

			count = 100;
		}
		else
		{
			//Sleep(0);

			if (count == 0) SuspendThread(handleTxThread); else count--;
		};

		txThreadCount++;
	};
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef WIN32

DWORD WINAPI RxThreadFunction(LPVOID lpParam) 
{
	EthBuf *buf = 0;

	u32 count = 0;

	while(1)
	{
		if (buf == 0) buf = GetSysEthBuffer();

		if (buf != 0)
		{
			EthPtr ep;

			ep.eth = &buf->eth;

			int len = recv(lstnSocket, (char*)&ep.eip->iph, buf->MaxLen()-sizeof(buf->eth), 0); 

			if (len > 0) //(len != SOCKET_ERROR/* && len >= sizeof(ep.eip->iph)*/)
			{
				if (RequestIP(buf, 0))
				{
					buf = 0;
				};
			};
		};
	};
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*--------------------------- init_ethernet ---------------------------------*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool InitEMAC()
{
#ifndef WIN32

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "Init EMAC ... ");

	using namespace HW;
	
	//PHYA = GetAdrPHY();

	if (sizeof(SysEthBuf) & 3) __breakpoint(0);
	if (sizeof(HugeTx) & 3) __breakpoint(0);

	HW_EMAC_Init();

	#ifdef CPU_SAME53	
		// Enable GMAC in RMII mode.
		HW::GMAC->UR = 0;//GMAC_MII;

		// Transmit and Receive disable. 
		HW::GMAC->NCR &= ~(GMAC_RXEN | GMAC_TXEN);

		/* Initialize Tx and Rx DMA Descriptors */
		rx_descr_init ();
		tx_descr_init ();

		HW::GMAC->DCFGR = GMAC_RXBMS_FULL|GMAC_DRBS(ETH_RX_DRBS)|GMAC_FBLDO_INCR4|GMAC_TXPBMS|GMAC_TXCOEN; // DMA Receive Buffer Size 512 bytes

		/* The sequence write GMAC_SA1L and write GMAC_SA1H must be respected. */
		HW::GMAC->SA[0].B = hwAdr.B;
		HW::GMAC->SA[0].T = hwAdr.T;

		/* Enable receiving of all Multicast packets. */
		HW::GMAC->HRB  = 0xFFFFFFFF;
		HW::GMAC->HRT  = 0xFFFFFFFF;

		/* Clear receive and transmit status registers. */
		HW::GMAC->RSR  = (RSR_RXOVR | RSR_REC | RSR_BNA | RSR_HNO);
		HW::GMAC->TSR  = (TSR_HRESP | TSR_UND | TSR_TXCOMP| TSR_TFC | TSR_RLE| TSR_COL | TSR_UBR);

		/* Configure GMAC operation mode, enable Multicast. */
		HW::GMAC->NCFGR |= GMAC_LFERD | GMAC_RXCOEN;// | GMAC_CAF ;
		//HW::GMAC->NCR   |= (GMAC_TXEN  | GMAC_RXEN | GMAC_WESTAT);

		/* Configure the GMAC Interrupts. */
		HW::GMAC->IDR  = ~0;

	#elif defined(CPU_XMC48)

		// Transmit and Receive disable. 
		HW::ETH0->MAC_CONFIGURATION = MAC_IPC|MAC_DM;

		//Set the MAC address
		HW::ETH0->MAC_ADDRESS0_LOW = hwAdr.B;
		HW::ETH0->MAC_ADDRESS0_HIGH = hwAdr.T;
	 
		//Initialize hash table
		HW::ETH0->HASH_TABLE_LOW = 0;
		HW::ETH0->HASH_TABLE_HIGH = 0;
	 
		//Configure the receive filter
		HW::ETH0->MAC_FRAME_FILTER = ETH_MAC_FRAME_FILTER_PR_Msk;//ETH_MAC_FRAME_FILTER_HPF_Msk | ETH_MAC_FRAME_FILTER_HMC_Msk;
		//Disable flow control
		HW::ETH0->FLOW_CONTROL = 0;
		//Enable store and forward mode
		//HW::ETH0->OPERATION_MODE = ETH_OPERATION_MODE_RSF_Msk | ETH_OPERATION_MODE_TSF_Msk;
	 
		//Configure DMA bus mode
		//HW::ETH0->BUS_MODE = BUS_AAL | BUS_FB | BUS_USP | BUS_RPBL(16) | BUS_PR(0) | BUS_PBL(16);


		/* Initialize Tx and Rx DMA Descriptors */
		rx_descr_init ();
		tx_descr_init ();

	   //Prevent interrupts from being generated when statistic counters reach
		//half their maximum value
		ETH0->MMC_TRANSMIT_INTERRUPT_MASK = ~0;
		ETH0->MMC_RECEIVE_INTERRUPT_MASK = ~0;
		ETH0->MMC_IPC_RECEIVE_INTERRUPT_MASK = ~0;
	 
		//Disable MAC interrupts
		HW::ETH0->INTERRUPT_MASK = 0x208; // ETH_INTERRUPT_MASK_TSIM_Msk | ETH_INTERRUPT_MASK_PMTIM_Msk;
	 
		//Enable the desired DMA interrupts
		HW::ETH0->INTERRUPT_ENABLE = 0; //ETH_INTERRUPT_ENABLE_NIE_Msk | ETH_INTERRUPT_ENABLE_RIE_Msk | ETH_INTERRUPT_ENABLE_TIE_Msk;
	 
	 
		//Enable MAC transmission and reception
		HW::ETH0->MAC_CONFIGURATION |= MAC_IPC|MAC_DM|MAC_RE; //ETH_MAC_CONFIGURATION_TE_Msk | ETH_MAC_CONFIGURATION_RE_Msk;

		//Enable DMA transmission and reception
		HW::ETH0->OPERATION_MODE |= ETH_OPERATION_MODE_SR_Msk; // | ETH_OPERATION_MODE_ST_Msk;

	#endif

	HW_EMAC_StartLink();

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");

#else

//	accptSocket = SOCKET_ERROR;

	int error = 0;

	WSADATA wsaData;
	
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
	{
		cputs("Error at WSAStartup()\n");
		return false;
	}
	else
	{
		cputs("WSAStartup() ... OK\n");
	};

	cputs("Creating socket ... ");

	lstnSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	
	if (lstnSocket == INVALID_SOCKET)
	{
		cputs("ERROR!!!\n");
	    WSACleanup();
		return false;
	}
	else
	{
		cputs("OK\n");
	};


	//int iMode = 0;

	//if (ioctlsocket(lstnSocket, FIONBIO, (u_long FAR*) &iMode) != 0)
	//{
	//	cputs("Set socket to blocking mode FAILED!!!\n");
	//};

	BOOL bOptVal = TRUE;
	int bOptLen = sizeof(BOOL);

	if(setsockopt(lstnSocket, IPPROTO_IP, IP_HDRINCL, (char*)&bOptVal, bOptLen) != SOCKET_ERROR)
	{
		cputs("Socket IP_HDRINCL enabled\n");
	};

	if(setsockopt(lstnSocket, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, bOptLen) != SOCKET_ERROR)
	{
		cputs("Socket SO_BROADCAST enabled\n");
	};

	sockaddr_in srvc;

	srvc.sin_family = AF_INET;
	srvc.sin_addr.s_addr = htonl(INADDR_ANY);
	srvc.sin_port = udpInPort;

	if (bind(lstnSocket, (SOCKADDR*)&srvc, sizeof(srvc)) == SOCKET_ERROR )
	{
		cputs("bind() socket failed\n" );
		closesocket(lstnSocket);
	    WSACleanup();
		return false;
	};

	cputs("Create thread 'txThread' ... ");

	handleTxThread = CreateThread(0, 0, TxThreadFunction, 0, 0, 0);

	cputs((handleTxThread == INVALID_HANDLE_VALUE) ? "!!! ERROR !!!\n" : "OK\n");

	cputs("Create thread 'rxThread' ... ");

	handleRxThread = CreateThread(0, 0, RxThreadFunction, 0, 0, 0);

	cputs((handleRxThread == INVALID_HANDLE_VALUE) ? "!!! ERROR !!!\n" : "OK\n");

	emacConnected = true;

#endif

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

static void rx_descr_init (void)
{
	RxBufIndex = 0;

	for (u32 i = 0; i < NUM_RX_BUF; i++)
	{
		EthBuf *b = GetSysEthBuffer();

		if (b != 0)
		{
			Rx_Desc[i].SetAdr(&b->eth, b->MaxLen());
		}
		else
		{
			Rx_Desc[i].ClrAdr();
		};
	};

	Rx_Desc[NUM_RX_BUF-1].SetWrap();

	#ifdef CPU_SAME53	

		HW::GMAC->RBQB = (u32)Rx_Desc; // Set Rx Queue pointer to descriptor list.

	#elif defined(CPU_XMC48)

		HW::ETH0->RECEIVE_DESCRIPTOR_LIST_ADDRESS = (u32)Rx_Desc; // Set Rx Queue pointer to descriptor list.

	#endif

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static void tx_descr_init (void)
{
	TxBufIndex = 0;

	for (u32 i = 0; i < NUM_TX_DSC; i++)
	{
		Tx_Desc[i].Init();
	};
  
	Tx_Desc[NUM_TX_DSC-1].SetWrap();// |= TD_TRANSMIT_WRAP; // Set the WRAP bit at the end of the list descriptor. 

	#ifdef CPU_SAME53	

		HW::GMAC->TBQB = (u32)&Tx_Desc[0]; // Set Tx Queue pointer to descriptor list. 

	#elif defined(CPU_XMC48)

		HW::ETH0->TRANSMIT_DESCRIPTOR_LIST_ADDRESS = (u32)&Tx_Desc[0]; // Set Tx Queue pointer to descriptor list. 

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void WritePHY (byte PhyReg, u16 Value)
{
	#ifdef CPU_SAME53	

		HW::GMAC->MAN = GMAC_MAN_CLTTO|GMAC_MAN_WTN(2)|GMAC_MAN_OP(1)|GMAC_MAN_PHYA(PHYA)|GMAC_MAN_REGA(PhyReg)|Value;

	#elif defined(CPU_XMC48)

		HW::ETH0->GMII_DATA = Value;
		HW::ETH0->GMII_ADDRESS = GMII_PA(PHYA)|GMII_CR(0)|GMII_MR(PhyReg)|GMII_MW|GMII_MB;

	#endif

	while (!IsReadyPHY());
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

u16 ReadPHY (byte PhyReg)
{
	#ifdef CPU_SAME53	

	  HW::GMAC->MAN = GMAC_MAN_CLTTO|GMAC_MAN_WTN(2)|GMAC_MAN_OP(2)|GMAC_MAN_PHYA(PHYA)|GMAC_MAN_REGA(PhyReg);

	#elif defined(CPU_XMC48)
	
		HW::ETH0->GMII_ADDRESS = GMII_PA(PHYA)|GMII_CR(0)|GMII_MR(PhyReg)|GMII_MB;

	#endif

  while (!IsReadyPHY());
  
  return ResultPHY();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ReqWritePHY(byte PhyReg, u16 Value)
{
	#ifdef CPU_SAME53	

		HW::GMAC->MAN = GMAC_MAN_CLTTO|GMAC_MAN_WTN(2)|GMAC_MAN_OP(1)|GMAC_MAN_PHYA(PHYA)|GMAC_MAN_REGA(PhyReg)|Value;

	#elif defined(CPU_XMC48)

		HW::ETH0->GMII_DATA = Value;
		HW::ETH0->GMII_ADDRESS = GMII_PA(PHYA)|GMII_CR(0)|GMII_MR(PhyReg)|GMII_MW|GMII_MB;

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ReqReadPHY(byte PhyReg)
{
	#ifdef CPU_SAME53	

		HW::GMAC->MAN = GMAC_MAN_CLTTO|GMAC_MAN_WTN(2)|GMAC_MAN_OP(2)|GMAC_MAN_PHYA(PHYA)|GMAC_MAN_REGA(PhyReg);

	#elif defined(CPU_XMC48)

		HW::ETH0->GMII_ADDRESS = GMII_PA(PHYA)|GMII_CR(0)|GMII_MR(PhyReg)|GMII_MB;

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

bool HW_EMAC_UpdateLink()
{
	bool result = false;

	switch(linkState)
	{
		case 0:		

			ReqReadPHY(PHY_REG_BMSR);

			linkState++;

			break;

		case 1:

			if (IsReadyPHY())
			{
				if (ResultPHY() & BMSR_LINKST)
				{
					linkState++;
				}
				else
				{
					linkState = 0;
				};
			};

			break;

		case 2:

			ReqWritePHY(PHY_REG_BMCR, BMCR_ANENABLE|BMCR_FULLDPLX);

			linkState++;

			break;

		case 3:

			if (IsReadyPHY())
			{
//				ReqReadPHY(PHY_REG_BMSR);
				ReqReadPHY(PHY_REG_PHYCON1);

				linkState++;
			};

			break;

		case 4:

			if (IsReadyPHY())
			{
				if (ResultPHY() & PHYCON1_OP_MODE_MASK /*BMSR_LINKST*/)
				{
					#ifdef CPU_SAME53	
						HW::GMAC->NCFGR &= ~(GMAC_SPD|GMAC_FD);
					#elif defined(CPU_XMC48)
						HW::ETH0->MAC_CONFIGURATION &= ~(MAC_DM|MAC_FES);
					#endif

					if (ResultPHY() & PHYCON1_OP_MODE_100BTX /*ANLPAR_100*/)	// Speed 100Mbit is enabled.
					{
						#ifdef CPU_SAME53	
							HW::GMAC->NCFGR |= GMAC_SPD;
						#elif defined(CPU_XMC48)
							HW::ETH0->MAC_CONFIGURATION |= MAC_FES;
						#endif
					};

					if (ResultPHY() & 4 /*ANLPAR_DUPLEX*/)	//  Full duplex is enabled.
					{
						#ifdef CPU_SAME53	
							HW::GMAC->NCFGR |= GMAC_FD;
						#elif defined(CPU_XMC48)
							HW::ETH0->MAC_CONFIGURATION |= MAC_DM;
						#endif
					};

					result = true;

					linkState++;
				}
				else
				{
					linkState = 3;
				};
			};

			break;
	};

	return result;
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef WIN32

static bool CheckLink() // Если нет связи, то результат false
{
	static byte state = 0;

	bool result = true;

	switch (state)
	{
		case 0:		

			ReqReadPHY(PHY_REG_BMSR);

			state++;

			break;

		case 1:

			if (IsReadyPHY())
			{
				result = ((ResultPHY() & BMSR_LINKST) != 0);

				if (!result) SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_YELLOW "Ethernet Link Down\n");

				state = 0;
			};

			break;
	};

	return result;
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void UpdateEMAC()
{
	static byte i = 0;

#ifndef WIN32

	switch(stateEMAC)
	{
		case LINKING:

			if (HW_EMAC_UpdateLink())
			{
				stateEMAC = CONNECTED;

				#ifdef CPU_SAME53	
					HW::GMAC->NCR |= GMAC_RXEN;
				#elif defined(CPU_XMC48)
					HW::ETH0->MAC_CONFIGURATION |= MAC_RE;
				#endif

				emacConnected = true;
			};
			
			break;

		case CONNECTED:

			if (!CheckLink())
			{
				#ifdef CPU_SAME53
					HW::GMAC->NCR &= ~GMAC_RXEN;
				#elif defined(CPU_XMC48)
					HW::ETH0->MAC_CONFIGURATION &= ~MAC_RE;
				#endif

				HW_EMAC_StartLink();
				stateEMAC = LINKING;
				emacConnected = false;
			}
			else
			{
				switch(i++)
				{
					case 0:	RecieveFrame();		break;
					case 1: UpdateTransmit();	break;
				};

				i &= 1;
			};

			break;
	};

#else

	RecieveFrame();

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // EMAC_IMP_H__11_10_2022__18_02




