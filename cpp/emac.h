#ifndef EMAC_H__02_03_2015__16_38
#define EMAC_H__02_03_2015__16_38

#include "types.h"
#include "list.h"
#include "EMAC_DEF.h"
#include "mem.h"

extern bool	InitEMAC();
extern void	UpdateEMAC();
extern void ResetPHY();	
extern void EnablePHY();

inline u16 GetIpID() {extern u16 txIpID; return ReverseWord(txIpID++); }

//extern void WritePHY(byte PhyReg, u16 Value);
//extern u16  ReadPHY(byte PhyReg);
//extern void ReqWritePHY(byte PhyReg, u16 Value);
//extern void ReqReadPHY(byte PhyReg);
//extern bool IsReadyPHY();
//extern u16 ResultPHY();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool EmacIsConnected()
{
	extern bool emacConnected;
	return emacConnected;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool EmacIsEnergyDetected()
{
	extern bool emacEnergyDetected;
	return emacEnergyDetected;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

inline bool EmacIsCableNormal()
{
	extern bool emacCableNormal;
	return emacCableNormal;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct MAC
{
	u32 B;
	u16 T;

	inline void operator=(const MAC &v) { B = v.B; T = v.T; }
	inline bool operator==(const MAC &v) { return (B == v.B) && (T == v.T); }
	inline bool operator!=(const MAC &v) { return (B != v.B) || (T != v.T); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	

	struct Receive_Desc
	{
		private:

			void*	addr1;
			u32		stat;

		public:

			bool 	CheckRcvFrame()				{ return ((u32)addr1 & ~(OWNERSHIP_BIT|WRAP_BIT)) != 0 && (stat & (RD_EOF|RD_SOF)) == (RD_EOF|RD_SOF); }
			bool 	CheckOwn()					{ return (u32)addr1 & OWNERSHIP_BIT; }
			void 	SetOwn()					{ *((u32*)&addr1) &= ~OWNERSHIP_BIT; }
			void 	ClrOwn()					{ *((u32*)&addr1) |= OWNERSHIP_BIT; }
			void 	SetWrap()					{ *((u32*)&addr1) |= WRAP_BIT; }
			//void 	ClrWrap()					{ *((u32*)&addr1) &= ~WRAP_BIT; }
			bool 	ChkWrap()					{ return *((u32*)&addr1) & WRAP_BIT; }
			byte*	GetAdr()					{ return (byte*)((u32)addr1 & ~(OWNERSHIP_BIT|WRAP_BIT)); }
			void	SetAdr(void *adr, u32 len)	{ addr1 = (void*)(((u32)addr1 & WRAP_BIT) | ((u32)adr & (~OWNERSHIP_BIT|WRAP_BIT))); }
			void	ClrAdr()					{ *((u32*)&addr1) &= OWNERSHIP_BIT|WRAP_BIT; ClrOwn(); }
			u32		GetStatus()					{ return stat; }
	};

	struct Transmit_Desc
	{
		private:

			void*	addr1;
			u32		stat;

		public:

			void	Init()						{ addr1 = 0; stat = TD_TRANSMIT_OK; }
			void	SetWrap()					{ stat |= TD_TRANSMIT_WRAP; }
			//void	ClrWrap()					{ stat &= ~TD_TRANSMIT_WRAP; }
			bool	ChkWrap()					{ return stat & TD_TRANSMIT_WRAP; }
			bool	ChkFree()					{ return (stat & TD_TRANSMIT_OK) && (stat & TD_LENGTH_MASK) == 0; }
			bool	ChkTransmit()				{ return (stat & TD_TRANSMIT_OK) && (stat & TD_LENGTH_MASK) != 0; }
			void	Free()						{ stat &= TD_TRANSMIT_OK|TD_TRANSMIT_WRAP; }

			void 	SetAdr(void *adr, u32 len)	{ addr1 = adr; stat &= TD_TRANSMIT_OK|TD_TRANSMIT_WRAP; stat |= TD_LAST_BUF | (len & TD_LENGTH_MASK); stat &= ~TD_TRANSMIT_OK; }
			byte*	GetAdr() 					{ return (byte*)addr1; }
			//void	ClrAdr()					{ addr1 = 0; stat = (stat & TD_TRANSMIT_WRAP)|TD_TRANSMIT_OK; }

	};

	//typedef Buf_Desc Receive_Desc;

#elif defined(CPU_XMC48)

	struct Receive_Desc
	{
		private:

			u32		stat;
			u32		ctrl;
			void*	addr1;
			void*	addr2;

		public:

			bool 	CheckRcvFrame()				{ return addr1 != 0 && (stat & (RD0_LS|RD0_FS|RD0_CE|RD0_FT)) == (RD0_LS|RD0_FS|RD0_FT); }
			bool 	CheckOwn()					{ return (stat & RD0_OWN) == 0; }
			void 	SetOwn() 					{ stat |= RD0_OWN; }
			void 	ClrOwn() 					{ stat &= ~RD0_OWN; }
			void	SetWrap()					{ ctrl |= RD1_RER; }
			bool	ChkWrap()					{ return ctrl & RD1_RER; }
			//void	ClrWrap()					{ ctrl &= ~RD1_RER; }
			byte*	GetAdr() 					{ return (byte*)addr1; }
			void 	SetAdr(void *adr, u32 len)	{ addr1 = adr; ctrl = (ctrl & RD1_RER) | (len & RD1_RBS1); addr2 = 0; SetOwn(); }
			void 	ClrAdr()					{ addr1 = 0;ClrOwn(); }
			u32		GetStatus()					{ return stat; }
	};

	struct Transmit_Desc
	{
		private:

			u32		stat;
			u32		ctrl;
			void*	addr1;
			void*	addr2;

		public:

			void	Init()						{ addr1 = 0; addr2 = 0; ctrl = 0; stat = TD0_IC; }
			void	SetWrap()					{ stat |= TD0_TER; }
			bool	ChkWrap()					{ return stat & TD0_TER; }
			bool	ChkFree()					{ return (stat & TD0_OWN) == 0 && (ctrl & TD1_TBS1) == 0; }
			bool	ChkTransmit()				{ return (stat & TD0_OWN) == 0 && (ctrl & TD1_TBS1) != 0; }
			void	Free()						{ ctrl = 0; }

			void 	SetAdr(void *adr, u32 len)	{ addr1 = adr; addr2 = 0; ctrl = TBS1(len); stat |= TD0_LS|TD0_FS; stat |= TD0_OWN; }
			byte*	GetAdr() 					{ return (byte*)addr1; }

	};

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct EthHdr
{
	MAC		dest;		/* Destination node		*/
	MAC		src;		/* Source node			*/
	u16		protlen;	/* Protocol or length		*/
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct ArpHdr
{
	u16		hrd;		/* Format of hardware address	*/
	u16		pro;		/* Format of protocol address	*/
	byte	hln;		/* Length of hardware address	*/
	byte	pln;		/* Length of protocol address	*/
	u16		op;			/* Operation			*/
	MAC		sha;		/* Sender hardware address	*/
	u32		spa;		/* Sender protocol address	*/
	MAC		tha;		/* Target hardware address	*/
	u32		tpa;		/* Target protocol address	*/
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//* IP Header structure

__packed struct IPheader
{
	byte	hl_v;		/* header length and version	*/
	byte	tos;		/* type of service		*/
	u16		len;		/* total length			*/
	u16		id;			/* identification		*/
	u16		off;		/* fragment offset field	*/
	byte	ttl;		/* time to live			*/
	byte	p;			/* protocol			*/
	u16		sum;		/* checksum			*/
	u32		src;		/* Source IP address		*/
	u32		dst;		/* Destination IP address	*/
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//* Preudo IP Header
__packed struct IPPseudoheader
{
	u32		srcAdr;	/* Source IP address		*/
	u32		dstAdr;	/* Destination IP address	*/
	byte   	zero;
	byte   	proto;
	u16		size;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//* ICMP echo header structure

__packed struct IcmpEchoHdr
{
	byte	type;       /* type of message */
	byte	code;       /* type subcode */
	u16		cksum;      /* ones complement cksum of struct */
	u16		id;         /* identifier */
	u16		seq;        /* sequence number */
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//* UDP header structure
__packed struct UdpHdr
{
	u16	src;	/* UDP source port		*/
	u16	dst;	/* UDP destination port		*/
	u16	len;	/* Length of UDP packet		*/
	u16	xsum;	/* Checksum			*/
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct EthArp
{
	EthHdr	eth;
	ArpHdr	arp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct EthIp
{
	EthHdr		eth;
	IPheader	iph;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct EthIcmp
{
	EthHdr		eth;
	IPheader	iph;
	IcmpEchoHdr	icmp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct EthUdp
{
	EthHdr		eth;
	IPheader	iph;
	UdpHdr		udp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct DhcpHdr
{
	byte	op;
	byte	htype;
	byte	hlen;
	byte	hops;

	u32		xid;
	u16		secs;
	u16		flags;

	u32		ciaddr;
	u32		yiaddr;
	u32		siaddr;
	u32		giaddr;

	MAC		chaddr;
	byte	pad_chaddr[10];

	char	sname[64];
	char	file[128];
	u32		magic;
	byte	options[230];
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

__packed struct EthDhcp
{
	EthHdr		eth; // 14
	IPheader	iph; // 20
	UdpHdr		udp; // 8
	DhcpHdr		dhcp;// 
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

union EthPtr
{
	EthHdr	*eth;
	EthArp	*earp;
	EthIp	*eip;
	EthIcmp *eicmp;
	EthUdp	*eudp;
	EthDhcp *edhcp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//__packed struct EthBuf
//{
//	EthBuf*		next;
//
//	u32			len;
//	//u16			z_align;
//
//	EthHdr		eth;
//
//	virtual	u32	MaxLen() { return sizeof(eth); }
//	virtual	void Free() {}
//
//	EthBuf() : next(0), len(0) {}
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//__packed struct EthArpBuf : public EthBuf
//{
//	ArpHdr	arp;
//};
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//__packed struct EthIpBuf : public EthBuf
//{
//	IPheader	iph;
//};
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//__packed struct EthIcmpBuf : public EthIpBuf
//{
//	IcmpEchoHdr	icmp;
//};
//
////+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//__packed struct EthUdpBuf : public EthIpBuf
//{
//	UdpHdr	udp;
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern bool TransmitEth(Ptr<MB> &mb);
extern bool TransmitIp(Ptr<MB> &mb);
extern bool TransmitFragIp(Ptr<MB> &mb);
//extern bool TransmitUdp(EthBuf *b);
extern bool TransmitFragUdp(Ptr<MB> &mb, u16 src, u16 dst);
//extern EthBuf* GetSysEthBuffer();
//extern EthBuf* GetHugeTxBuffer();
//extern const MAC& HW_EMAC_GetHwAdr();
//extern byte HW_EMAC_GetAdrPHY();
//extern u32  HW_EMAC_GetIpAdr();
//extern u32  HW_EMAC_GetIpMask();
//extern u32  HW_EMAC_GetDhcpIpAdr();
//extern u16  HW_EMAC_GetUdpInPort();
//extern u16  HW_EMAC_GetUdpOutPort();
//extern bool HW_EMAC_RequestUDP(Ptr<MB> &mb);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // EMAC_H__02_03_2015__16_38
