#ifndef COMPORT_IMP_H__24_06_13_40
#define COMPORT_IMP_H__24_06_13_40

//#pragma O3
//#pragma Otime

//#include <stdio.h>
//#include <conio.h>

#include "ComPort.h"
#include "time.h"

#ifdef _DEBUG_
//	static const bool _debug = true;
#else
//	static const bool _debug = false;
#endif

extern dword millisecondsCount;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#elif defined(CPU_XMC48)	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define __SCTR (PDL(1) | TRM(1) | FLE(7) | WLE(7))

	#define __CCR (MODE(2))

	#define __BRG_ASYN	(DCTQ(15))
	#define __BRG_SYNC	(DCTQ(0)|CLKSEL(3))

	//#define __DX0CR (DSEL(0) | INSW(0) | DFEN(0) | DSEN(0) | DPOL(0) | SFSEL(0) | CM(0) | DXS(0))
	//#define __DX1CR (DSEL(0) | INSW(0) | DFEN(0) | DSEN(0) | DPOL(0) | SFSEL(0) | CM(0) | DXS(0))
	//#define __DX2CR (DSEL(0) | INSW(0) | DFEN(0) | DSEN(0) | DPOL(0) | SFSEL(0) | CM(0) | DXS(0))
	//#define __DX3CR (DSEL(0) | INSW(0) | DFEN(0) | DSEN(0) | DPOL(0) | SFSEL(0) | CM(0) | DXS(0))

	#define __PCR_ASYN	(SMD(1) | SP(9) | RSTEN(1) | TSTEN(1))
	#define __PCR_SYNC	(SMD(0) | SP(0) | RSTEN(1) | TSTEN(1))

//	#define __FDR (STEP(0x3FF) | DM(1))

	#define __TCSR (TDEN(1)|TDSSM(1))

	ComPort::ComBase	ComPort::_bases[3] = { 
	//		used	dsel	USIC_CH			port_RTS	pin_RTS		USIC PID	INPR_SRx	GPDMA		dmaCh	DLR
		{	false, 	1, 		HW::USIC0_CH0,	HW::P1, 	1<<0,		PID_USIC0,  0,			HW::DMA0, 	0, 		0 	}, 
		{	false, 	2, 		HW::USIC0_CH1,	HW::P1, 	1<<13,		PID_USIC0, 	1,			HW::DMA0, 	1, 		2 	},
		{	false, 	4, 		HW::USIC2_CH1,	HW::P1, 	1<<8,		PID_USIC2,	1,			HW::DMA1, 	0, 		9 	}
	};

	static u32 parityMask[3] = { PM(0), PM(3), PM(2) };

	//#define READ_PIN_SET()	HW::P1->BSET(0)
	//#define READ_PIN_CLR()	HW::P1->BCLR(0)


#endif	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ComPort::InitHW()
{
	#ifdef CPU_SAME53	

		HW::GCLK->PCHCTRL[_ugclk] = GCLK_GEN(_GEN_SRC)|GCLK_CHEN;	// 25 MHz

		HW::MCLK->ClockEnable(_upid);
		
		T_HW::S_USART* uhw = _uhw.usart;
		
		_PIO_TX->SetWRCONFIG(_MASK_TX, _PMUX_TX|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_PULLEN);
		_PIO_RX->SetWRCONFIG(_MASK_RX, _PMUX_RX|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX|PORT_PULLEN);

		_PIO_RTS->SetWRCONFIG(_MASK_RTS, PORT_WRPINCFG|PORT_WRPMUX);
		_PIO_RTS->DIRSET = _MASK_RTS; 
		_PIO_RTS->SET(_MASK_RTS); 

		uhw->CTRLA = USART_SWRST;

		while(uhw->SYNCBUSY);

		uhw->CTRLA = _CTRLA;
		uhw->CTRLB = _CTRLB;
		uhw->CTRLC = _CTRLC;
		uhw->BAUD = _BaudRateRegister;

		uhw->CTRLA |= USART_ENABLE;

		while(uhw->SYNCBUSY);

	#elif defined(CPU_XMC48)

		HW::Peripheral_Enable(_cb->usic_pid);

		_SU->KSCFG = MODEN|BPMODEN|BPNOM|NOMCFG(0);

		_SU->BRG = _brg;		
		_SU->PCR_ASCMode = _pcr;

		_SU->FDR			= _BaudRateRegister;
		_SU->SCTR			= __SCTR;
		_SU->DX0CR			= DSEL(_cb->dsel);//__DX0CR;
		_SU->DX1CR			= DPOL(1);
		_SU->TCSR			= __TCSR;
		_SU->PSCR			= ~0;
		_SU->CCR			= _ModeRegister;

	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ComPort::Connect(CONNECT_TYPE ct, dword speed, byte parity, byte stopBits)
{
#ifndef WIN32

	if(!Usic_Connect())
	{
		return false;
	};

	#ifdef CPU_SAME53	

		switch (ct)
		{
			case ASYNC:

				_CTRLA = USART_MODE_INT_CLK|_TXPO|_RXPO|USART_DORD;
				_BaudRateRegister = BoudToPresc(speed);

				break;

			case SYNC_M:

				_CTRLA = USART_MODE_INT_CLK|_TXPO|_RXPO|USART_CMODE|USART_DORD;
				_BaudRateRegister = (_GEN_CLK+speed) / (2*speed) - 1;

				break;

			case SYNC_S:

				_CTRLA = USART_MODE_EXT_CLK|_TXPO|_RXPO|USART_CMODE|USART_DORD;
				_BaudRateRegister = ~0;

				break;

		};

		_CTRLB = ((stopBits == 2) ? USART_SBMODE : 0);
		_CTRLC = 0;

		switch (parity)
		{
			case 0:		// ��� ��������

				break;

			case 1:
				_CTRLA |= USART_FORM_USART_PARITY;
				_CTRLB |= USART_PMODE;
				break;

			case 2:
				_CTRLA |= USART_FORM_USART_PARITY;
				_CTRLB &= ~USART_PMODE;
				break;
		};

		//_CTRLB |= USART_RXEN;

		InitHW();

	#elif defined(CPU_XMC48)

		_dma = cb.dma;
		_dmaChMask = 1<<cb.dmaCh;
		_chdma = &(_dma->CH[cb.dmaCh]);
		_dlr = cb.dlr;
		_inpr_sr = cb.inpr_sr;

		_ModeRegister = __CCR | ((parity < 3) ? parityMask[parity] : parityMask[0]);


		switch (ct)
		{
			case ASYNC:

				_brg				= __BRG_ASYN;
				_pcr				= __PCR_ASYN | ((stopBits == 2) ? STPB(1) : 0);;
				_BaudRateRegister	= BoudToPresc(speed) | DM(1);

				break;

			case SYNC_M:

				_brg				= __BRG_SYNC;
				_pcr				= __PCR_SYNC | ((stopBits == 2) ? STPB(1) : 0);
				_BaudRateRegister	= BoudToPresc(speed) | DM(1);

				break;

			case SYNC_S:

				_brg				= __BRG_SYNC;
				_pcr				= __PCR_SYNC | ((stopBits == 2) ? STPB(1) : 0);
				_BaudRateRegister	= 0;

				break;

		};

		InitHW();

	#endif

	_status485 = READ_END;

	return _connected = true;

#else

	return true;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ComPort::Disconnect()
{
#ifndef WIN32

	if (!_connected) return false;

	DisableReceive();
	DisableTransmit();

	_status485 = READ_END;

	_connected = false;

	Usic_Disconnect();

#endif

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

word ComPort::BoudToPresc(dword speed)
{
	if (speed == 0) return 0;

	word presc;

	#ifdef CPU_SAME53	

		//presc = (((MCK+8)/16) + speed/2) / speed;
		presc = 65536ULL*(_GEN_CLK - 16*speed)/_GEN_CLK;
		//presc = 65536 * (1 - 16.0f*speed/MCK);

		return presc;

	#elif defined(CPU_XMC48)

		presc = ((MCK + speed/2) / speed + 8) / 16;

		if (presc > 1024) presc = 1024;

		return 1024 - presc;

	#elif defined(WIN32)
		
		return 0;

	#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ComPort::TransmitByte(byte v)
{
#ifndef WIN32

	_PIO_RTS->SET(_MASK_RTS);

	#ifdef CPU_SAME53	

		_uhw.usart->CTRLB = _CTRLB|USART_TXEN;

		while ((_uhw.usart->INTFLAG & USART_DRE) == 0);

		_uhw.usart->DATA = v;

		while ((_uhw.usart->INTFLAG & USART_TXC) == 0);

		_uhw.usart->CTRLB = _CTRLB;	// Disable transmit and receive

	#elif defined(CPU_XMC48)

		_SU->CCR = _ModeRegister|TBIEN;
		_SU->INPR = _inpr_sr<<4;

		_SU->TBUF[0] = v;

		while ((_SU->PSR & TSIF) == 0);

		_SU->CCR = _ModeRegister;	// Disable transmit and receive

	#endif

	_PIO_RTS->CLR(_MASK_RTS);

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ComPort::EnableTransmit(void* src, word count)
{
#ifndef WIN32

	if (count == 0) return;

	_PIO_RTS->SET(_MASK_RTS);

	#ifdef CPU_SAME53	

		_status = 0;

		_uhw.usart->CTRLA = _CTRLA;
		_uhw.usart->CTRLB = _CTRLB;
		_uhw.usart->CTRLC = _CTRLC;
		_uhw.usart->BAUD = _BaudRateRegister;

		_uhw.usart->CTRLA |= USART_ENABLE;

		while(_uhw.usart->SYNCBUSY);

		_DMA->WritePeripheral(src, &_uhw.usart->DATA, count, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE); 

		//while (_uhw.usart->SYNCBUSY & USART_CTRLB);

		_uhw.usart->CTRLB = _CTRLB|USART_TXEN;
		_uhw.usart->INTFLAG = ~0;

	#elif defined(CPU_XMC48)

		if (_portNum > 1 && count > 0xFFF) { count = 0xFFF; };

		__disable_irq();

		HW::DLR->LNEN |= (1<<_dlr);

		_dma->DMACFGREG = 1;

		if (count > 0xFFF)
		{
			byte *p = (byte*)src;

			u32 i = 0;

			LLI *lli = &_lli[0];

			for ( ; i < ArraySize(_lli); i++)
			{
				lli = _lli+i;

				lli->SAR = src;
				lli->CTLL = DINC(2)|SINC(0)|TT_FC(1)|DEST_MSIZE(0)|SRC_MSIZE(0);
				lli->DAR = &_SU->TBUF[0];

				if (count > 0xFFF)
				{
					lli->LLP = _lli+i+1;
					lli->CTLH = 0xFFF;
					lli->CTLL |= LLP_DST_EN;
					count -= 0xFFF;
					p += 0xFFF;
				}
				else
				{
					lli->LLP = 0;
					lli->CTLH = count;
					count = 0;

					break;
				};
			};

			lli->LLP = 0;

			_dma->DMACFGREG = 1;

			_chdma->CTLH = _lli[0].CTLH;
			_chdma->CTLL = _lli[0].CTLL;
			_chdma->LLP = (u32)&_lli[0];
		}
		else
		{
			_chdma->CTLL = DINC(2)|SINC(0)|TT_FC(1)|DEST_MSIZE(0)|SRC_MSIZE(0);
			_chdma->CTLH = BLOCK_TS(count);
		};

		_chdma->SAR = (u32)src;
		_chdma->DAR = (u32)&_SU->TBUF[0];
		_chdma->CFGL = HS_SEL_SRC;
		_chdma->CFGH = PROTCTL(1)|DEST_PER(_dlr&7);

		_dma->CHENREG = _dmaChMask|(_dmaChMask<<8);

		_SU->PSCR = ~0;
		_SU->CCR = _ModeRegister|TBIEN;
		_SU->INPR = _inpr_sr<<4;

		if ((_SU->PSR & TBIF) == 0)
		{
			_SU->FMR = USIC_CH_FMR_SIO0_Msk<<_inpr_sr;
		};

		__enable_irq();

	#endif

	_status485 = WRITEING;

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ComPort::DisableTransmit()
{
#ifndef WIN32

	#ifdef CPU_SAME53	

		//while (_uhw.usart->SYNCBUSY & USART_CTRLB);

		_uhw.usart->CTRLA = USART_SWRST;

		//_uhw.usart->CTRLB = _CTRLB;	// Disable transmit and receive

		_DMA->Disable(); 

	#elif defined(CPU_XMC48)

		_dma->CHENREG = _dmaChMask<<8;
		_SU->CCR = _ModeRegister;
		HW::DLR->LNEN &= ~(1<<_dlr);

	#endif

	_PIO_RTS->CLR(_MASK_RTS);

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ComPort::EnableReceive(void* dst, word count)
{
#ifndef WIN32

	_PIO_RTS->CLR(_MASK_RTS);

	#ifdef CPU_SAME53	

		_status = 0;

		_uhw.usart->CTRLA = _CTRLA;
		_uhw.usart->CTRLB = _CTRLB;
		_uhw.usart->CTRLC = _CTRLC;
		_uhw.usart->BAUD = _BaudRateRegister;

		_uhw.usart->CTRLA |= USART_ENABLE;

		while(_uhw.usart->SYNCBUSY);

		_DMA->ReadPeripheral(&_uhw.usart->DATA, dst, _prevDmaCounter = count, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

		//while (_uhw.usart->SYNCBUSY & USART_CTRLB);

		_uhw.usart->CTRLB = _CTRLB|USART_RXEN;
		_uhw.usart->INTFLAG = ~0;

	#elif defined(CPU_XMC48)

		volatile u32 t;

		if (_portNum > 1 && count > 0xFFF) { count = 0xFFF; };

		__disable_irq();

		_dma->DMACFGREG = 1;

		if (count > 0xFFF)
		{
			byte *p = (byte*)dst;

			u32 i = 0;

			LLI *lli = &_lli[0];

			for ( ; i < ArraySize(_lli); i++)
			{
				lli = _lli+i;

				lli->SAR = &_SU->RBUF;
				lli->CTLL = DINC(0)|SINC(2)|TT_FC(2)|DEST_MSIZE(0)|SRC_MSIZE(0);
				lli->DAR = p;

				if (count > 0xFFF)
				{
					lli->LLP = _lli+i+1;
					lli->CTLH = 0xFFF;
					lli->CTLL |= LLP_SRC_EN;
					count -= 0xFFF;
					p += 0xFFF;
				}
				else
				{
					lli->LLP = 0;
					lli->CTLH = count;
					count = 0;

					break;
				};
			};

			lli->LLP = 0;

			_dma->DMACFGREG = 1;

			_chdma->CTLH = _lli[0].CTLH;
			_chdma->CTLL = _lli[0].CTLL;
			_chdma->LLP = (u32)&_lli[0];
		}
		else
		{
			_chdma->CTLH = BLOCK_TS(count);
			_chdma->CTLL = DINC(0)|SINC(2)|TT_FC(2)|DEST_MSIZE(0)|SRC_MSIZE(0);
		};

		_chdma->SAR = (u32)&_SU->RBUF;
		_chdma->DAR = _startDmaCounter = _prevDmaCounter = (u32)dst;
		_chdma->CFGL = HS_SEL_DST;
		_chdma->CFGH = PROTCTL(1)|SRC_PER(_dlr&7);
		_dma->CHENREG = _dmaChMask|(_dmaChMask<<8);

		t = _SU->RBUF;
		t = _SU->RBUF;
		_SU->PSCR = ~0;

		HW::DLR->LNEN |= (1<<_dlr);

		_SU->CCR = _ModeRegister|RIEN;
		_SU->INPR = _inpr_sr<<8;

		__enable_irq();

	#endif

#endif

	_rtm.Reset();

	_status485 = WAIT_READ;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void ComPort::DisableReceive()
{
#ifndef WIN32

	_PIO_RTS->CLR(_MASK_RTS);

	#ifdef CPU_SAME53	

		//while (_uhw.usart->SYNCBUSY & USART_CTRLB);

		_uhw.usart->CTRLA = USART_SWRST;

		//_uhw.usart->CTRLB = _CTRLB;

		_DMA->Disable();

	#elif defined(CPU_XMC48)

		_dma->CHENREG = _dmaChMask<<8;
		_SU->CCR = _ModeRegister;
		HW::DLR->LNEN &= ~(1<<_dlr);

	#endif

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	#ifdef CPU_SAME53	
	#elif defined(CPU_XMC48)
	#endif

bool ComPort::Update()
{
//	static u32 stamp = 0;

	bool r = true;

	if (!_connected) 
	{
		_status485 = READ_END;
	};

//	stamp = GetRTT();

	switch (_status485)
	{
		case WRITEING: //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			
			if (IsTransmited())
			{
				_pWriteBuffer->transmited = true;
				_status485 = READ_END;

				DisableTransmit();

				r = false;
			};

			break;

		case WAIT_READ: //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		{
			u32 t = GetDmaCounter();
			u32 s = _uhw.usart->INTFLAG & USART_ERROR;

			if (_prevDmaCounter != t || s)
			{
				//if(s) __breakpoint(0);

				//_status |= s;
				_uhw.usart->INTFLAG = s;

				_prevDmaCounter = t;
				_rtm.Reset();
				_readTimeout = _postReadTimeout;
			}
			else
			{
				if (_rtm.Timeout(_readTimeout))
				{
					DisableReceive();
					_pReadBuffer->len = GetRecievedLen();
					_pReadBuffer->recieved = _pReadBuffer->len > 0;
					_status485 = READ_END;
					r = false;
				};
			};

		};

		break;

		case READ_END: //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

			r = false;

			break;
	};

	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ComPort::Read(ComPort::ReadBuffer *readBuffer, dword preTimeout, dword postTimeout)
{
	if (_status485 != READ_END || readBuffer == 0)
	{
		return false;
	};

	_readTimeout = preTimeout;
	_postReadTimeout = postTimeout;

	_pReadBuffer = readBuffer;
	_pReadBuffer->recieved = false;
	_pReadBuffer->len = 0;

	//_prevDmaCounter = 0;//_pReadBuffer->maxLen;

	EnableReceive(_pReadBuffer->data, _pReadBuffer->maxLen);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool ComPort::Write(ComPort::WriteBuffer *writeBuffer)
{
	if (_status485 != READ_END || writeBuffer == 0)
	{
		return false;
	};

	_pWriteBuffer = writeBuffer;
	_pWriteBuffer->transmited = false;

	EnableTransmit(_pWriteBuffer->data, _pWriteBuffer->len);

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // COMPORT_IMP_H__24_06_13_40
