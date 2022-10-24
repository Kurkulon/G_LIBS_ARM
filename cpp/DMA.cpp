#include "DMA.h"

#ifdef CPU_SAME53

	__align(16) T_HW::DMADESC DmaTable[32];
	__align(16) T_HW::DMADESC DmaWRB[32];
//__align(16) T_HW::DMADESC DMA_CH::_DmaTable[32];
//__align(16) T_HW::DMADESC DMA_CH::_DmaWRB[32];

	__align(16) T_HW::DMADESC DMA_CH::_wr_dmadsc[32];

DMA_CH		DMA_CH0		(0	);
DMA_CH		DMA_CH1		(1	);
DMA_CH		DMA_CH2		(2	);
DMA_CH		DMA_CH3		(3	);
DMA_CH		DMA_CH4		(4	);
DMA_CH		DMA_CH5		(5	);
DMA_CH		DMA_CH6		(6	);
DMA_CH		DMA_CH7		(7	);
DMA_CH		DMA_CH8		(8	);
DMA_CH		DMA_CH9		(9	);
DMA_CH		DMA_CH10	(10	);
DMA_CH		DMA_CH11	(11	);
DMA_CH		DMA_CH12	(12	);
DMA_CH		DMA_CH13	(13	);
DMA_CH		DMA_CH14	(14	);
DMA_CH		DMA_CH15	(15	);
DMA_CH		DMA_CH16	(16	);
DMA_CH		DMA_CH17	(17	);
DMA_CH		DMA_CH18	(18	);
DMA_CH		DMA_CH19	(19	);
DMA_CH		DMA_CH20	(20	);
DMA_CH		DMA_CH21	(21	);
DMA_CH		DMA_CH22	(22	);
DMA_CH		DMA_CH23	(23	);
DMA_CH		DMA_CH24	(24	);
DMA_CH		DMA_CH25	(25	);
DMA_CH		DMA_CH26	(26	);
DMA_CH		DMA_CH27	(27	);
DMA_CH		DMA_CH28	(28	);
DMA_CH		DMA_CH29	(29	);
DMA_CH		DMA_CH30	(30	);
DMA_CH		DMA_CH31	(31	);
	
#elif defined(CPU_XMC48)

DMA_CH		DMA_CH0		(HW::DMA0,	0	);
DMA_CH		DMA_CH1		(HW::DMA0,	1	);
DMA_CH		DMA_CH2		(HW::DMA0,	2	);
DMA_CH		DMA_CH3		(HW::DMA0,	3	);
DMA_CH		DMA_CH4		(HW::DMA0,	4	);
DMA_CH		DMA_CH5		(HW::DMA0,	5	);
DMA_CH		DMA_CH6		(HW::DMA0,	6	);
DMA_CH		DMA_CH7		(HW::DMA0,	7	);
																
DMA_CH		DMA_CH8		(HW::DMA1,	8	);
DMA_CH		DMA_CH9		(HW::DMA1,	9	);
DMA_CH		DMA_CH10	(HW::DMA1,	10	);
DMA_CH		DMA_CH11	(HW::DMA1,	11	);

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DMA_CH::_MemCopy(const volatile void *src, volatile void *dst, u16 len, u32 ctrl)
{
	//using namespace HW;

#ifdef CPU_SAME53
	
	_dmadsc->SRCADDR = src;
	_dmadsc->DSTADDR = dst;
	_dmadsc->DESCADDR = 0;
	_dmadsc->BTCNT = len;
	_dmadsc->BTCTRL = ctrl;

	_dmach->INTENCLR = ~0;
	_dmach->INTFLAG = ~0;
	_dmach->CTRLA = DMCH_ENABLE|DMCH_TRIGACT_TRANSACTION;

	SoftwareTrigger();

#elif defined(CPU_XMC48)

	_GPDMA->DMACFGREG = 1;

	_InitLLI(src, dst, len, ctrl); //DST_INC|SRC_INC|TT_FC_M2M_GPDMA|DEST_MSIZE_1|SRC_MSIZE_1|LLP_DST_EN|LLP_SRC_EN
	
	_dmach->CFGL = 0;
	_dmach->CFGH = PROTCTL(1);

	Enable();

#endif

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_XMC48

bool DMA_CH::CheckMemCopyComplete()
{
	if (CheckComplete())
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

			Enable();
		}
		else
		{
			return true;
		};
	};

	return false;
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DMA_CH::WritePeripheral(const volatile void *src, volatile void *dst, u16 len, u32 ctrl1, u32 ctrl2)
{
	//using namespace HW;

#ifdef CPU_SAME53

	_dmach->CTRLA = 0;

	_dmadsc->SRCADDR = (byte*)src+len;
	_dmadsc->DSTADDR = dst;
	_dmadsc->DESCADDR = 0;
	_dmadsc->BTCNT = len;
	_dmadsc->BTCTRL = ctrl2 | DMDSC_VALID|DMDSC_SRCINC;//|DMDSC_BEATSIZE_BYTE;

	_dmach->INTENCLR = ~0;
	_dmach->INTFLAG = ~0;

	_dmach->CTRLA = ctrl1 | DMCH_ENABLE; //DMCH_TRIGACT_BURST|_dma_trgsrc_tx;
	
#elif defined(CPU_XMC48)

	HW::DLR->LNEN |= _dlr_lnen_mask;

	_GPDMA->DMACFGREG = 1;

	_InitLLI(src, dst, len, ctrl1);

	_dmach->CFGL = HS_SEL_SRC;
	_dmach->CFGH = PROTCTL(1)|DEST_PER(_drl&7);

	Enable();

#endif

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DMA_CH::ReadPeripheral(const volatile void *src, volatile void *dst, u16 len, u32 ctrl1, u32 ctrl2)
{
	//using namespace HW;

#ifdef CPU_SAME53

		_dmach->CTRLA = 0;

		_dmadsc->SRCADDR = src;
		_dmadsc->DSTADDR = (byte*)dst+len;
		_dmadsc->DESCADDR = 0;
		_dmadsc->BTCNT = _dmawrb->BTCNT = len;
		_dmadsc->BTCTRL = ctrl2 | DMDSC_VALID|DMDSC_DSTINC; //|DMDSC_BEATSIZE_BYTEC;

		_dmach->INTENCLR = ~0;
		_dmach->INTFLAG = ~0;

		_dmach->CTRLA = ctrl1 | DMCH_ENABLE; //|DMCH_TRIGACT_BURST|_dma_trgsrc_rx;
	
#elif defined(CPU_XMC48)

	_GPDMA->DMACFGREG = 1;

	_InitLLI(src, dst, len, ctrl1);

	_dmach->CFGL = HS_SEL_DST;
	_dmach->CFGH = PROTCTL(1)|SRC_PER(_drl&7);

	Enable();

	HW::DLR->LNEN |= _dlr_lnen_mask;

#endif

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DMA_CH::SystemInit()
{
	HW::MCLK->AHBMASK |= AHB_DMAC;
	HW::DMAC->CTRL = 0;
	HW::DMAC->CTRL = DMAC_SWRST;
	HW::DMAC->DBGCTRL = DMAC_DBGRUN;
	HW::DMAC->BASEADDR	= DmaTable;
	HW::DMAC->WRBADDR	= DmaWRB;
	HW::DMAC->CTRL = DMAC_DMAENABLE|DMAC_LVLEN0|DMAC_LVLEN1|DMAC_LVLEN2|DMAC_LVLEN3;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DMA_CH::WritePeripheral(const volatile void *src, volatile void *dst, u16 len, const volatile void *src2, u16 len2, u32 ctrl1, u32 ctrl2)
{
	using namespace HW;

	_dmach->CTRLA = 0;

	if (src2 != 0 && len2 > 0)
	{
		T_HW::DMADESC &wrdsc = _wr_dmadsc[_chnum];
		wrdsc.SRCADDR = (byte*)src2+len2;
		wrdsc.DSTADDR = dst;
		wrdsc.BTCNT = len2;
		wrdsc.BTCTRL = ctrl2 | DMDSC_VALID|DMDSC_SRCINC;//|DMDSC_BEATSIZE_BYTE;
		_dmadsc->DESCADDR = &wrdsc;
	}
	else
	{
		_dmadsc->DESCADDR = 0;
	};

	_dmadsc->SRCADDR = (byte*)src+len;
	_dmadsc->DSTADDR = dst;
	_dmadsc->BTCNT = _dmawrb->BTCNT = len;
	_dmadsc->BTCTRL = ctrl2 | DMDSC_VALID|DMDSC_SRCINC;//|DMDSC_BEATSIZE_BYTE;

	_dmach->INTENCLR = ~0;
	_dmach->INTFLAG = ~0;

	_dmach->CTRLA = ctrl1 | DMCH_ENABLE; //DMCH_TRIGACT_BURST|_dma_trgsrc_tx;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	
#elif defined(CPU_XMC48)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DMA_CH::_InitLLI(const volatile void *src, volatile void *dst, u16 len, u32 ctrl)
{
	using namespace HW;

	_dmach->SAR = _startSrcAdr = (u32)src;
	_dmach->DAR = _startDstAdr = (u32)dst;

	if (_chnumabs < 2)
	{
		if (len > BLOCK_TS(~0))
		{
			u32 i = 0;

			LLI *lli = &_lli[0];

			for ( ; i < ArraySize(_lli); i++)
			{
				lli = _lli+i;

				lli->SAR = (void*)src;
				lli->CTLL = ctrl;
				lli->DAR = dst;

				if (len > BLOCK_TS(~0))
				{
					lli->LLP = _lli+i+1;
					lli->CTLH = BLOCK_TS(~0);
					len -= BLOCK_TS(~0);
					*((byte**)&src) += BLOCK_TS(~0);
					*((byte**)&dst) += BLOCK_TS(~0);
				}
				else
				{
					lli->LLP = 0;
					lli->CTLH = len;
					len = 0;
					break;
				};
			};

			lli->LLP = 0;

			_dmach->CTLH = BLOCK_TS(~0);
			_dmach->CTLL = ctrl;
			((T_HW::GPDMA0_CH_Type*)_dmach)->LLP = (u32)&_lli[0];

			_dataLen = len;
		}
		else
		{
			_dmach->CTLH = len;
			_dmach->CTLL = ctrl;
			((T_HW::GPDMA0_CH_Type*)_dmach)->LLP = 0;
			_dataLen = 0;
		};
	}
	else
	{
		if (len > BLOCK_TS(~0))
		{
			_dataLen = len - BLOCK_TS(~0);
			len = BLOCK_TS(~0);
		}
		else
		{
			_dataLen = 0;
		};

		_dmach->CTLH = len;
		_dmach->CTLL = ctrl;
	};
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

