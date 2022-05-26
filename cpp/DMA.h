#ifndef DMA_H__26_05_2022__18_11
#define DMA_H__26_05_2022__18_11

#include "types.h"
#include "core.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class DMA0_CH_0_1
{
protected:

	T_HW::GPDMA0_GLOBAL_Type*	const _GPDMA; 
	T_HW::GPDMA0_CH_Type*		const _dmach;

	const u32		_dlr_lnen_mask;
	const byte		_chnum;

public:

	DMA0_CH_0_1(T_HW::GPDMA0_GLOBAL_Type* gpdma, T_HW::GPDMA0_CH_Type* dmach, u32 dlr_mask, byte chnum) : _GPDMA(gpdma), _dmach(dmach), _dlr_lnen_mask(dlr_mask), _chnum(chnum) {}

};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class DMA_CH
{
protected:

	T_HW::GPDMA0_GLOBAL_Type*	const _GPDMA; 
	T_HW::GPDMA1_CH_Type*		const _dmach;

	const u32		_dlr_lnen_mask;
	const byte		_chnum;

	byte*	_dataSrc;
	byte*	_dataDst;

	u16		_dataLen;

public:

	DMA_CH(T_HW::GPDMA0_GLOBAL_Type* gpdma, T_HW::GPDMA1_CH_Type* dmach, u32 dlr_mask, byte chnum) : _GPDMA(gpdma), _dmach(dmach), _dlr_lnen_mask(dlr_mask), _chnum(chnum) {}

	void MemCopy(volatile void *src, volatile void *dst, u16 len);
	bool CheckMemCopyComplete();

};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern DMA0_CH_0_1	DMA_CH0;
extern DMA0_CH_0_1	DMA_CH1;
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
