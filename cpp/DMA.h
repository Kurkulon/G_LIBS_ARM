#ifndef DMA_H__26_05_2022__18_11
#define DMA_H__26_05_2022__18_11

#include "types.h"
#include "core.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class DMA_CH
{
protected:

#ifdef CPU_SAME53

	static T_HW::DMADESC _DmaTable[32];
	static T_HW::DMADESC _DmaWRB[32];

	static void SystemInit();
	
	T_HW::S_DMAC::S_DMAC_CH*	const _dmach;

	T_HW::DMADESC*				const _dmadsc;
	T_HW::DMADESC*				const _dmawrb;

	
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

	const u32		_dlr_lnen_mask;
	const byte		_chnumabs;

	u32		_startSrcAdr;
	u32		_startDstAdr;
	u16		_dataLen;
	
	void _InitLLI(const volatile void *src, volatile void *dst, u16 len, u32 ctrl);

#endif

	const byte		_chnum;

	void _MemCopy(const volatile void *src, volatile void *dst, u16 len, u32 ctrl);

public:

	void WritePeripheral(const volatile void *src, volatile void *dst, u16 len, u32 ctrl1, u32 ctrl2);
	void ReadPeripheral(const volatile void *src, volatile void *dst, u16 len, u32 ctrl1, u32 ctrl2);
	

#ifdef CPU_SAME53

	DMA_CH(byte chnum) : _dmach(&HW::DMAC->CH[chnum]), _dmadsc(&_DmaTable[chnum]), _dmawrb(&_DmaWRB[chnum]), _chnum(chnum) {}

	//void Enable() {  }
	
	void Disable() { _dmach->CTRLA = 0; while (_dmach->CTRLA & DMCH_ENABLE); _dmach->CTRLA = DMCH_SWRST; }

	bool CheckComplete() { return (_dmach->CTRLA & DMCH_ENABLE) == 0 || (_dmach->INTFLAG & DMCH_TCMPL); }
	
	void MemCopy(volatile void *src, volatile void *dst, u16 len)		{ _MemCopy((byte*)src+len, (byte*)dst+len, len, DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_DSTINC|DMDSC_SRCINC); }
	void MemCopySrcInc(volatile void *src, volatile void *dst, u16 len) { _MemCopy((byte*)src+len, dst, len, DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_SRCINC); }
	void MemCopyDstInc(volatile void *src, volatile void *dst, u16 len) { _MemCopy(src, (byte*)dst+len, len, DMDSC_VALID|DMDSC_BEATSIZE_BYTE|DMDSC_DSTINC); }
	bool CheckMemCopyComplete() { return CheckComplete(); }

	u32 GetSrcBytesReady()	{ return _dmach->SAR - _startSrcAdr; }
	u32 GetDstBytesReady()	{ return _dmach->DAR - _startDstAdr; }
	u32 GetSrcBytesLeft()	{ return u32 t = HW::DMAC->ACTIVE; return ((t & 0x9F00) == _dma_act_mask) ? (t >> 16) : _dmawrb->BTCNT; }
	u32 GetDstBytesLeft()	{ return _dmach->DAR - _startDstAdr; }

#elif defined(CPU_XMC48)

	DMA_CH(T_HW::GPDMA_Type* gpdma, byte chnum, u32 dlr_mask) : _GPDMA(gpdma), _dmach(&gpdma->CH[chnum&7]), _dlr_lnen_mask(dlr_mask), _chnum(chnum&7), _chnumabs(chnum) {}

	void Enable() { _GPDMA->CHENREG = ((1<<GPDMA0_CHENREG_CH_Pos)|(1<<GPDMA0_CHENREG_WE_CH_Pos)) << _chnum; }
	void Disable() { _GPDMA->CHENREG = (1<<GPDMA0_CHENREG_WE_CH_Pos) << _chnum; HW::DLR->LNEN &= ~_dlr_lnen_mask; }
	bool CheckComplete() { return (_GPDMA->CHENREG & ((1<<GPDMA0_CHENREG_CH_Pos)<<_chnum)) == 0; }
	
	void WritePeripheralByte(const volatile void *src, void *dst, u16 len)	{ WritePeripheral(src, dst, len, DST_TR_WIDTH_8|SRC_TR_WIDTH_8|DST_NOCHANGE|SRC_INC|DEST_MSIZE_1|SRC_MSIZE_1|TT_FC_M2P_GPDMA|LLP_DST_EN, 0); }
	void ReadPeripheralByte(const volatile void *src, void *dst, u16 len)	{ ReadPeripheral(src, dst, len, DST_TR_WIDTH_8|SRC_TR_WIDTH_8|DST_INC|SRC_NOCHANGE|DEST_MSIZE_1|SRC_MSIZE_1|TT_FC_P2M_GPDMA|LLP_SRC_EN, 0); }
	
	void MemCopy(const volatile void *src, volatile void *dst, u16 len)		{ _MemCopy(src, dst, len, DST_INC|SRC_INC|TT_FC_M2M_GPDMA|DEST_MSIZE_1|SRC_MSIZE_1|LLP_DST_EN|LLP_SRC_EN); }
	void MemCopySrcInc(const volatile void *src, volatile void *dst, u16 len) { _MemCopy(src, dst, len, DST_NOCHANGE|SRC_INC|TT_FC_M2M_GPDMA|DEST_MSIZE_1|SRC_MSIZE_1|LLP_SRC_EN); }
	void MemCopyDstInc(const volatile void *src, volatile void *dst, u16 len) { _MemCopy(src, dst, len, DST_INC|SRC_NOCHANGE|TT_FC_M2M_GPDMA|DEST_MSIZE_1|SRC_MSIZE_1|LLP_DST_EN); }
	bool CheckMemCopyComplete();

	u32 GetSrcCounter()	{ return _dmach->SAR - _startSrcAdr; }
	u32 GetDstCounter()	{ return _dmach->DAR - _startDstAdr; }

#endif

};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // DMA_H__26_05_2022__18_11
