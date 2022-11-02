#ifndef DMA_H__26_05_2022__18_11
#define DMA_H__26_05_2022__18_11

#include "types.h"
#include "core.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class DMA_CH
{
protected:

#ifdef CPU_SAME53

	//static T_HW::DMADESC _DmaTable[32];
	//static T_HW::DMADESC _DmaWRB[32];

	static void SystemInit();
	
	T_HW::S_DMAC::S_DMAC_CH*	const _dmach;

	T_HW::DMADESC*				const _dmadsc;
	T_HW::DMADESC*				const _dmawrb;

	const u32					_act_mask;


	static T_HW::DMADESC		_wr_dmadsc[32];
	
#elif defined(CPU_XMC48)

	struct LLI
	{
		volatile void*	SAR;
		volatile void*	DAR;
		LLI*	LLP;
		u32		CTLL;
		u32		CTLH;
		u32		DSTAT;
	};

	LLI						_lli[4];

	T_HW::GPDMA_Type*		const _GPDMA; 
	T_HW::GPDMA_CH_Type*	const _dmach;

	u32		_dlr_lnen_mask;
	u32		_startSrcAdr;
	u32		_startDstAdr;
	u16		_dataLen;

	const	byte	_chnumabs;
	byte	_drl;
	
	void _InitLLI(const volatile void *src, volatile void *dst, u16 len, u32 ctrl);

#endif

	const byte		_chnum;

	void _MemCopy(const volatile void *src, volatile void *dst, u16 len, u32 ctrl);

public:

	void WritePeripheral(const volatile void *src, volatile void *dst, u16 len, u32 ctrl1, u32 ctrl2);
	void ReadPeripheral(const volatile void *src, volatile void *dst, u16 len, u32 ctrl1, u32 ctrl2);
	

#ifdef CPU_SAME53

	DMA_CH(byte chnum) : _dmach(&HW::DMAC->CH[chnum]), _dmadsc(&DmaTable[chnum]), _dmawrb(&DmaWRB[chnum]), _act_mask(0x8000|(chnum<<8)), _chnum(chnum) { _dmach->PRILVL = DMCH_PRILVL_LVL0; }

	//void Enable() {  }
	
	void Disable() { while (_dmach->CTRLA & DMCH_ENABLE) _dmach->CTRLA &= ~DMCH_ENABLE; _dmach->CTRLA = DMCH_SWRST; }
	void Reset() { _dmach->CTRLA = DMCH_SWRST; }

	bool CheckComplete() { if (_dmach->STATUS & DMCH_FERR) _dmach->CTRLB = DMCH_CMD_RESUME; return (_dmach->CTRLA & DMCH_ENABLE) == 0 /*|| (_dmach->INTFLAG & DMCH_TCMPL)*/; }
	
	void MemCopy(volatile void *src, volatile void *dst, u16 len)		{ _MemCopy((byte*)src+len, (byte*)dst+len, len, DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_DSTINC|DMDSC_SRCINC); }
	void MemCopySrcInc(volatile void *src, volatile void *dst, u16 len) { _MemCopy((byte*)src+len, dst, len, DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_SRCINC); }
	void MemCopyDstInc(volatile void *src, volatile void *dst, u16 len) { _MemCopy(src, (byte*)dst+len, len, DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_DSTINC); }
	bool CheckMemCopyComplete() { return CheckComplete(); }

	u32 GetSrcBytesReady()	{ return 0; }
	u32 GetDstBytesReady()	{ return 0; }
	u32 GetSrcBytesLeft()	{ return 0; }
	u32 GetDstBytesLeft()	{ return 0; }

	u32 GetBytesLeft()	{ u32 t = HW::DMAC->ACTIVE; return ((t & 0x9F00) == _act_mask) ? (t >> 16) : _dmawrb->BTCNT; }
	u32 GetBytesReady()	{ return _dmadsc->BTCNT - GetBytesLeft(); }

	void SoftwareTrigger() { HW::DMAC->SWTRIGCTRL = 1UL<<_chnum; }
	void SetEvCtrl(byte v) { _dmach->EVCTRL = v; }
	//void SetPriLvl(byte v) { _dmach->PRILVL = v; }

	void WritePeripheral(const volatile void *src, volatile void *dst, u16 len, const volatile void *src2, u16 len2, u32 ctrl1, u32 ctrl2);

	u16 Get_CRC_CCITT_Result() { return ReverseWord(HW::DMAC->CRCCHKSUM); }

	void CRC_CCITT(const void* data, u16 len, u16 init)
	{
		HW::DMAC->CRCCTRL = DMAC_CRCBEATSIZE_BYTE | DMAC_CRCPOLY_CRC16 | DMAC_CRCMODE_CRCGEN | DMAC_CRCSRC(0x20 + _chnum);
		WritePeripheral(data, (void*)init, len, DMCH_TRIGACT_TRANSACTION, DMDSC_BEATSIZE_BYTE);
		SoftwareTrigger(); //HW::DMAC->SWTRIGCTRL = 1UL << CRC_DMACH;
	}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#elif defined(CPU_XMC48)

	DMA_CH(T_HW::GPDMA_Type* gpdma, byte chnum) : _GPDMA(gpdma), _dmach(&gpdma->CH[chnum&7]), _chnumabs(chnum), _chnum(chnum&7) {}

	void Enable() { _GPDMA->CHENREG = ((1<<GPDMA0_CHENREG_CH_Pos)|(1<<GPDMA0_CHENREG_WE_CH_Pos)) << _chnum; }
	void Disable() { _GPDMA->CHENREG = (1<<GPDMA0_CHENREG_WE_CH_Pos) << _chnum; HW::DLR->LNEN &= ~_dlr_lnen_mask; }
	bool CheckComplete() { return (_GPDMA->CHENREG & ((1<<GPDMA0_CHENREG_CH_Pos)<<_chnum)) == 0; }
	void SetDlrLineNum(byte num) { _dlr_lnen_mask = 1UL << (_drl = num);  }
	void Enable_DLR() { HW::DLR->LNEN |= _dlr_lnen_mask; }
	void Disable_DLR() { HW::DLR->LNEN &= ~_dlr_lnen_mask; }
	
	void WritePeripheralByte(const volatile void *src, volatile void *dst, u16 len)	{ WritePeripheral(src, dst, len, DST_TR_WIDTH_8|SRC_TR_WIDTH_8|DST_NOCHANGE|SRC_INC|DEST_MSIZE_1|SRC_MSIZE_1|TT_FC_M2P_GPDMA|LLP_DST_EN, 0); }
	void ReadPeripheralByte(const volatile void *src, volatile void *dst, u16 len)	{ ReadPeripheral(src, dst, len, DST_TR_WIDTH_8|SRC_TR_WIDTH_8|DST_INC|SRC_NOCHANGE|DEST_MSIZE_1|SRC_MSIZE_1|TT_FC_P2M_GPDMA|LLP_SRC_EN, 0); }
	
	void MemCopy(const volatile void *src, volatile void *dst, u16 len)		{ _MemCopy(src, dst, len, DST_INC|SRC_INC|TT_FC_M2M_GPDMA|DEST_MSIZE_1|SRC_MSIZE_1|LLP_DST_EN|LLP_SRC_EN); }
	void MemCopySrcInc(const volatile void *src, volatile void *dst, u16 len) { _MemCopy(src, dst, len, DST_NOCHANGE|SRC_INC|TT_FC_M2M_GPDMA|DEST_MSIZE_1|SRC_MSIZE_1|LLP_SRC_EN); }
	void MemCopyDstInc(const volatile void *src, volatile void *dst, u16 len) { _MemCopy(src, dst, len, DST_INC|SRC_NOCHANGE|TT_FC_M2M_GPDMA|DEST_MSIZE_1|SRC_MSIZE_1|LLP_DST_EN); }
	bool CheckMemCopyComplete();

	u32 GetSrcCounter()	{ return _dmach->SAR - _startSrcAdr; }
	u32 GetDstCounter()	{ return _dmach->DAR - _startDstAdr; }

#endif

};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53

extern DMA_CH		DMA_CH0	;
extern DMA_CH		DMA_CH1	;
extern DMA_CH		DMA_CH2	;
extern DMA_CH		DMA_CH3	;
extern DMA_CH		DMA_CH4	;
extern DMA_CH		DMA_CH5	;
extern DMA_CH		DMA_CH6	;
extern DMA_CH		DMA_CH7	;
extern DMA_CH		DMA_CH8	;
extern DMA_CH		DMA_CH9	;
extern DMA_CH		DMA_CH10;
extern DMA_CH		DMA_CH11;
extern DMA_CH		DMA_CH12;
extern DMA_CH		DMA_CH13;
extern DMA_CH		DMA_CH14;
extern DMA_CH		DMA_CH15;
extern DMA_CH		DMA_CH16;
extern DMA_CH		DMA_CH17;
extern DMA_CH		DMA_CH18;
extern DMA_CH		DMA_CH19;
extern DMA_CH		DMA_CH20;
extern DMA_CH		DMA_CH21;
extern DMA_CH		DMA_CH22;
extern DMA_CH		DMA_CH23;
extern DMA_CH		DMA_CH24;
extern DMA_CH		DMA_CH25;
extern DMA_CH		DMA_CH26;
extern DMA_CH		DMA_CH27;
extern DMA_CH		DMA_CH28;
extern DMA_CH		DMA_CH29;
extern DMA_CH		DMA_CH30;
extern DMA_CH		DMA_CH31;

#elif defined(CPU_XMC48)

extern DMA_CH		DMA_CH0;
extern DMA_CH		DMA_CH1;
extern DMA_CH		DMA_CH2;
extern DMA_CH		DMA_CH3;
extern DMA_CH		DMA_CH4;
extern DMA_CH		DMA_CH5;
extern DMA_CH		DMA_CH6;
extern DMA_CH		DMA_CH7;
extern DMA_CH		DMA_CH8;
extern DMA_CH		DMA_CH9;
extern DMA_CH		DMA_CH10;
extern DMA_CH		DMA_CH11;

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // DMA_H__26_05_2022__18_11
