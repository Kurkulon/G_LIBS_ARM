#include "DMA.h"

DMA0_CH_0_1	DMA_CH0		(HW::GPDMA0, HW::GPDMA0_CH0,	1UL<<0,		0	);
DMA0_CH_0_1	DMA_CH1		(HW::GPDMA0, HW::GPDMA0_CH1,	1UL<<1,		1	);
DMA_CH		DMA_CH2		(HW::GPDMA0, HW::GPDMA0_CH2,	1UL<<2,		2	);
DMA_CH		DMA_CH3		(HW::GPDMA0, HW::GPDMA0_CH3,	1UL<<3,		3	);
DMA_CH		DMA_CH4		(HW::GPDMA0, HW::GPDMA0_CH4,	1UL<<4,		4	);
DMA_CH		DMA_CH5		(HW::GPDMA0, HW::GPDMA0_CH5,	1UL<<5,		5	);
DMA_CH		DMA_CH6		(HW::GPDMA0, HW::GPDMA0_CH6,	1UL<<6,		6	);
DMA_CH		DMA_CH7		(HW::GPDMA0, HW::GPDMA0_CH7,	1UL<<7,		7	);
																	
DMA_CH		DMA_CH8		(HW::GPDMA1, HW::GPDMA1_CH0,	1UL<<8,		0	);
DMA_CH		DMA_CH9		(HW::GPDMA1, HW::GPDMA1_CH1,	1UL<<9,		1	);
DMA_CH		DMA_CH10	(HW::GPDMA1, HW::GPDMA1_CH2,	1UL<<10,	2	);
DMA_CH		DMA_CH11	(HW::GPDMA1, HW::GPDMA1_CH3,	1UL<<11,	3	);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DMA_CH::MemCopy(volatile void *src, volatile void *dst, u16 len)
{
	using namespace HW;

	_GPDMA->DMACFGREG = 1;

	if (len > BLOCK_TS(~0))
	{
		//_dataSrc = ((byte*)src) + BLOCK_TS(~0);
		//_dataDst = ((byte*)dst) + BLOCK_TS(~0);
		_dataLen = len - BLOCK_TS(~0);
		len = BLOCK_TS(~0);
	}
	else
	{
		_dataLen = 0;
	};

	_dmach->CTLL = DST_INC|SRC_INC|TT_FC(0)|DEST_MSIZE(0)|SRC_MSIZE(0);
	_dmach->CTLH = BLOCK_TS(len);

	_dmach->SAR = (u32)src;
	_dmach->DAR = (u32)dst;
	_dmach->CFGL = 0;
	_dmach->CFGH = PROTCTL(1);

	_GPDMA->CHENREG = ((1<<GPDMA0_CHENREG_CH_Pos)|(1<<GPDMA0_CHENREG_WE_CH_Pos)) << _chnum;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool DMA_CH::CheckMemCopyComplete()
{
#ifdef CPU_SAME53

	return (HW::DMAC->CH[NAND_MEMCOPY_DMACH].CTRLA & DMCH_ENABLE) == 0 || (HW::DMAC->CH[NAND_MEMCOPY_DMACH].INTFLAG & DMCH_TCMPL);

#elif defined(CPU_XMC48)

	if ((_GPDMA->CHENREG & ((1<<GPDMA0_CHENREG_CH_Pos)<<_chnum)) == 0)
	{
		if (_dataLen > 0) 
		{
			if (_dataLen > BLOCK_TS(~0))
			{
				_dmach->CTLH = BLOCK_TS(~0);
				_dataLen -= BLOCK_TS(~0);
			}
			else
			{
				_dmach->CTLH = BLOCK_TS(_dataLen);
				_dataLen = 0;
			};

			//_dmach->SAR = (u32)_dataSrc;
			//_dmach->DAR = (u32)_dataDst;

			_GPDMA->CHENREG = ((1<<GPDMA0_CHENREG_CH_Pos)|(1<<GPDMA0_CHENREG_WE_CH_Pos)) << _chnum;
		}
		else
		{
			return true;
		};
	};

	return false;

#elif defined(WIN32)

	return true;

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

