#ifndef SPIS_IMP_H__08_06_2023__15_50
#define SPIS_IMP_H__08_06_2023__15_50

#ifndef CPU_LPC812

#include "spis.h"
#include "core.h"
#include "SEGGER_RTT.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define SPI__PCR	(SPI_MSLSEN | SPI_SELINV | SPI_TIWEN | SPI_MCLK | SPI_CTQSEL1(0) | SPI_PCTQ1(0) | SPI_DCTQ1(0))
#define SPI__TCSR	(USIC_TDEN(1) | USIC_HPCMD(0))
#define SPI__SCTR	(USIC_SDIR(1) | USIC_TRM(1) | USIC_FLE(0x3F) | USIC_WLE(7))
#define SPI__BRG	(USIC_SCLKCFG(2U) | USIC_CTQSEL(0) | USIC_DCTQ(1) | USIC_PCTQ(3) | USIC_CLKSEL(0))
#define SPI__CCR	(USIC_MODE_SPI)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIS::InitHW()
{
	#ifdef CPU_SAME53	

		HW::GCLK->PCHCTRL[_ugclk] = GCLK_GEN(_GEN_SRC)|GCLK_CHEN;	// 25 MHz

		HW::MCLK->ClockEnable(_upid);
		
		T_HW::S_SPI* spi = _uhw.spi;
		
		if (_PIO_SPCK != 0) _PIO_SPCK->SetWRCONFIG(	1UL<<_PIN_SPCK,	_PMUX_SPCK|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX);
		if (_PIO_MOSI != 0) _PIO_MOSI->SetWRCONFIG(	1UL<<_PIN_MOSI,	_PMUX_MOSI|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX);
		if (_PIO_MISO != 0) _PIO_MISO->SetWRCONFIG(	1UL<<_PIN_MISO,	_PMUX_MISO|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX);
		if (_PIO_SS != 0)	_PIO_SS->SetWRCONFIG(	1UL<<_PIN_SS,	_PMUX_SS  |PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX);

		spi->CTRLA = SPI_SWRST;

		while(spi->SYNCBUSY);

		spi->CTRLA = SERCOM_MODE_SPI_SLAVE;

		spi->CTRLA = SERCOM_MODE_SPI_SLAVE|_DIPO|_DOPO|__CTRLA;
		spi->CTRLB = 0;
		spi->CTRLC = 1;

		spi->DBGCTRL = 1;

		spi->CTRLA |= SPI_ENABLE;

		while(spi->SYNCBUSY);

		spi->STATUS = ~0;

	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIS::Disconnect()
{
	#ifdef CPU_SAME53	
		
		T_HW::S_SPI* spi = _uhw.spi;
		
		if (_PIO_SPCK != 0) _PIO_SPCK->SetWRCONFIG(1UL<<_PIN_SPCK, PORT_WRPMUX);
		if (_PIO_MOSI != 0) _PIO_MOSI->SetWRCONFIG(1UL<<_PIN_MOSI, PORT_WRPMUX);
		if (_PIO_MISO != 0) _PIO_MISO->SetWRCONFIG(1UL<<_PIN_MISO, PORT_WRPMUX);
		if (_PIO_SS != 0)	_PIO_SS->SetWRCONFIG(1UL<<_PIN_SS, PORT_WRPMUX);

		spi->CTRLA = SPI_SWRST;

		while(spi->SYNCBUSY);

		HW::MCLK->ClockDisable(_upid);

		Usic_Disconnect();

	#elif defined(CPU_XMC48)

	
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_SPIS::Connect(bool cpol, bool cpha, bool LSB)
{
#ifndef WIN32

	using namespace HW;

	SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_GREEN "SPI%u Slave Init ... ", _usic_num);

	#ifdef CPU_SAME53	
	if (!Usic_Connect())
	#elif defined(CPU_XMC48)
	if (!Usic_Connect())
	#endif
	{
		SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n");

		return false;
	};

	#ifdef CPU_SAME53	

		__CTRLA = 0;

		if (cpol)	__CTRLA |= SPI_CPOL;
		if (cpha)	__CTRLA |= SPI_CPHA;
		if (LSB)	__CTRLA |= SPI_DORD;

		InitHW();

	#elif defined(CPU_XMC48)

		__FDR = ((1024 - ((_GEN_CLK + baudrate/2) / baudrate + 1) / 2) | USIC_DM(1));

		InitHW();

	#endif

	SEGGER_RTT_WriteString(0, "OK\n");

#endif

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_SPIS::Update()
{
	bool result = true;

#ifdef CPU_SAME53

	T_HW::S_SPI* spi = _uhw.spi;

	switch (_state)
	{
		case WAIT:

			result = false;

			break;

		case WRITE:

			if (CheckWriteComplete())
			{
				_pwbuf->transmited = true;
				_state = WAIT;

				_DMATX->Disable();

				spi->INTFLAG = ~0;
				spi->INTENCLR = ~0;

				Usic_Unlock();

				result = false;
			};

			break;


		case READ:
		{
			u32 t = GetDmaCounter();

			if (spi->INTFLAG & SPI_DRE)
			{
				spi->DATA = 0x55;
				//spi->INTFLAG = SPI_DRE|SPI_TXC;

				_rtm.Reset();
				_readTimeout = _postReadTimeout;
			}
			else
			{
				if (_rtm.Timeout(_readTimeout))
				{
					_prbuf->len = GetRecievedLen();
					_prbuf->recieved = _prbuf->len > 0;

					_DMARX->Disable();

					spi->CTRLB &= ~SPI_RXEN;
					spi->INTFLAG = ~0;
					spi->INTENCLR = ~0;

					Usic_Unlock();

					_state = WAIT;

					result = false;
				};
			};

			break;
		};
	};

#elif defined(CPU_XMC48)


#endif

	return result;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_SPIS::Read(S_SPIS::RBUF *rbuf, u32 preTimeout, u32 postTimeout)
{
	if (_state != WAIT || rbuf == 0)
	{
		return false;
	};

	_readTimeout = preTimeout;
	_postReadTimeout = postTimeout;

	_prbuf = rbuf;
	_prbuf->recieved = false;
	_prbuf->len = 0;

	_state = READ;

	Usic_Lock();

	//_prevDmaCounter = 0;//_pReadBuffer->maxLen;

	volatile register byte t;

	#ifdef CPU_SAME53	

		_uhw.spi->INTFLAG = ~0;
		_uhw.spi->INTENCLR = ~0;
		_uhw.spi->CTRLB |= SPI_RXEN; while(_uhw.spi->SYNCBUSY);

		do t = _uhw.spi->DATA; while(_uhw.spi->INTFLAG & SPI_RXC); 

		while(_uhw.spi->INTFLAG & SPI_DRE) _uhw.spi->DATA = 0xAA;

		_prevCounter = _prbuf->maxLen;

		_DMARX->ReadPeripheral(&_uhw.spi->DATA, _prbuf->data, _prbuf->maxLen,	DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

	#elif defined(CPU_XMC48)

		USICHWT	&SPI = _uhw;

		SPI->RBCTR = 0;
		SPI->TBCTR = 0;

		SPI->CCR = SPI__CCR;
		SPI->PCR_SSCMode = SPI__PCR|SPI_SELO(1);

		while(SPI->PSR_SSCMode & (SPI_TBIF|SPI_RIF))
		{
			t = SPI->RBUF;
			SPI->PSCR = ~0;
		};

		_DMA->ReadPeripheralByte(&SPI->RBUF, data, count);

		SPI->INPR = USIC_RINP(Get_INPR_SR())|USIC_PINP(5)|USIC_TBINP(5);

		SPI->PSCR = ~0;
		
		SPI->CCR = SPI__CCR | USIC_RIEN;
		SPI->TCSR = SPI__TCSR|USIC_TDSSM(0);
	
		SPI->TBUF[0] = 0;
	
	#endif

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_SPIS::Write(S_SPIS::WBUF *wbuf, u32 timeout)
{
	if (_state != WAIT || wbuf == 0)
	{
		return false;
	};

	_pwbuf = wbuf;
	_pwbuf->transmited = false;

	_state = WRITE;

	Usic_Lock();

	#ifdef CPU_SAME53	

		_uhw.spi->INTFLAG = ~0;
		_uhw.spi->INTENCLR = ~0;
		_uhw.spi->CTRLB &= ~SPI_RXEN; while(_uhw.spi->SYNCBUSY);

		_DMATX->WritePeripheral(_pwbuf->data, &_uhw.spi->DATA, _pwbuf->len, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

	#elif defined(CPU_XMC48)

		USICHWT	&spi = _uhw;

		spi->TRBSCR = TRBSCR_FLUSHTB;
		spi->TBCTR = TBCTR_SIZE8|TBCTR_LIMIT(0);

		spi->TCSR = SPI__TCSR|USIC_TDSSM(1);

		spi->CCR = SPI__CCR;
		spi->PCR_SSCMode = SPI__PCR|SPI_SELO(1);

		spi->PSCR = ~0;

		while(spi->PSR_SSCMode & SPI_TBIF) spi->PSCR = ~0;

		_DMA->WritePeripheralByte(data, &spi->IN[4], count);

		spi->PSCR = ~0;
		spi->CCR = USIC_MODE_SPI|USIC_TBIEN;
		spi->INPR = USIC_TBINP(Get_INPR_SR())|USIC_RINP(5)|USIC_PINP(5);

		if ((spi->PSR_SSCMode & SPI_TBIF) == 0)
		{
			spi->FMR = USIC_CH_FMR_SIO0_Msk << Get_INPR_SR();
		};

	
	#endif

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // #ifndef CPU_LPC812

#endif // SPIS_IMP_H__08_06_2023__15_50
