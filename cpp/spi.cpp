#include "spi.h"
#include "core.h"
#include "SEGGER_RTT.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::InitHW()
{
	#ifdef CPU_SAME53	

		HW::GCLK->PCHCTRL[_ugclk] = GCLK_GEN(_GEN_SRC)|GCLK_CHEN;	// 25 MHz

		HW::MCLK->ClockEnable(_upid);
		
		T_HW::S_SPI* spi = _uhw.spi;
		
		_PIO_SPCK->SetWRCONFIG(_MASK_SPCK, _PMUX_SPCK|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX);
		_PIO_MOSI->SetWRCONFIG(_MASK_MOSI, _PMUX_MOSI|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX);
		_PIO_MISO->SetWRCONFIG(_MASK_MISO, _PMUX_MISO|PORT_WRPINCFG|PORT_PMUXEN|PORT_WRPMUX);
		_PIO_CS->DIRSET = _MASK_CS_ALL; 
		_PIO_CS->SetWRCONFIG(_MASK_CS_ALL, PORT_WRPINCFG|PORT_WRPMUX);
		_PIO_CS->SET(_MASK_CS_ALL); 

		spi->CTRLA = SPI_SWRST;

		while(spi->SYNCBUSY);

		spi->CTRLA = SERCOM_MODE_SPI_MASTER;

		spi->CTRLA = SERCOM_MODE_SPI_MASTER|_DIPO|_DOPO;
		spi->CTRLB = 0;
		spi->CTRLC = 1;
		spi->BAUD = _baud;

		spi->DBGCTRL = 1;

		spi->CTRLA |= SPI_ENABLE;

		while(spi->SYNCBUSY);

		spi->STATUS = ~0;

	#elif defined(CPU_XMC48)
	
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::Disconnect()
{
	#ifdef CPU_SAME53	
		
		T_HW::S_SPI* spi = _uhw.spi;
		
		_PIO_SPCK->SetWRCONFIG(_MASK_SPCK, PORT_WRPMUX);
		_PIO_MOSI->SetWRCONFIG(_MASK_MOSI, PORT_WRPMUX);
		_PIO_MISO->SetWRCONFIG(_MASK_MISO, PORT_WRPMUX);
		_PIO_CS->SET(_MASK_CS_ALL); 
		_PIO_CS->DIRCLR = _MASK_CS_ALL; 
		_PIO_CS->SetWRCONFIG(_MASK_CS_ALL, PORT_PULLEN|PORT_WRPINCFG|PORT_WRPMUX);

		spi->CTRLA = SPI_SWRST;

		while(spi->SYNCBUSY);

		HW::MCLK->ClockDisable(_upid);

		Usic_Disconnect();

	#elif defined(CPU_XMC48)
	
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_SPIM::Connect(u32 baudrate)
{
#ifndef WIN32

	using namespace HW;

	SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_GREEN "SPI%u Init ... ", _usic_num);

	if (!Usic_Connect() || _MASK_CS == 0 || _MASK_CS_LEN == 0)
	{
		SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n");

		return false;
	};

	#ifdef CPU_SAME53	

		_MASK_CS_ALL = 0;

		for (u32 i = 0; i < _MASK_CS_LEN; i++)
		{
			_MASK_CS_ALL |= _MASK_CS[i];
		};

		if (baudrate == 0) baudrate = 1;

		u32 baud = (_GEN_CLK + baudrate/2) / baudrate;

		baud = (baud + 1) / 2;

		if (baud > 0) baud -= 1;

		if (baud > 0xFF) baud = 0xFF;

		_baud = baud;

		InitHW();

	#elif defined(CPU_XMC48)

		HW::Peripheral_Enable(SPI_PID);

		SPI->KSCFG = MODEN|BPMODEN|BPNOM|NOMCFG(0);

		SPI->CCR = 0;

		SPI->FDR = SPI__FDR;
		SPI->BRG = SPI__BRG;
	    
		SPI->SCTR = SPI__SCTR;
		SPI->TCSR = SPI__TCSR;

		SPI->PCR_SSCMode = SPI__PCR;

		SPI->PSCR = ~0;

		SPI->CCR = 0;

		SPI->DX0CR = SPI__DX0CR;
		SPI->DX1CR = SPI__DX1CR;

		SPI->TBCTR = 0;// TBCTR_SIZE8|TBCTR_LIMIT(0);
		SPI->RBCTR = 0;//RBCTR_SIZE8|RBCTR_LIMIT(0);

		SPI->CCR = SPI__CCR;

		PIO_SPCK->ModePin(PIN_SPCK, A2PP);
		PIO_MOSI->ModePin(PIN_MOSI, A2PP);
 		PIO_MISO->ModePin(PIN_MISO, I0DNP);
		PIO_CS->ModePin(PIN_CS0, G_PP);
		PIO_CS->ModePin(PIN_CS1, G_PP);
		PIO_CS->SET(CS0|CS1);

		VectorTableExt[SPI_IRQ] = SPI_Handler_Read;
		CM4::NVIC->CLR_PR(SPI_IRQ);
		CM4::NVIC->SET_ER(SPI_IRQ);


		//SPI->PCR_SSCMode = SPI__PCR|SELO(1);
		
	//	SPI->PSCR |= TBIF;

	//	SPI->CCR = SPI__CCR|TBIEN;
	//	SPI->INPR = 0;

		//SPI->IN[0] = 0x55;
		//SPI->IN[0] = 0x55;

		//while ((SPI->PSR & TSIF) == 0);

	//	SPI->CCR = SPI__CCR;

	#endif

	SEGGER_RTT_WriteString(0, "OK\n");

#endif

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::WritePIO(void *data, u16 count)
{
	byte *p = (byte*)data;

	#ifdef CPU_SAME53	

		while (count != 0)
		{
			_uhw.spi->DATA = *(p++); count--;

			while((_uhw.spi->INTFLAG & SPI_DRE) == 0);
		};

		while((_uhw.spi->INTFLAG & SPI_TXC) == 0);

	#elif defined(CPU_XMC48)
		
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::WriteAsyncDMA(void *data, u16 count)
{
	#ifdef CPU_SAME53	

		_uhw.spi->INTFLAG = ~0;
		_uhw.spi->INTENCLR = ~0;
		_uhw.spi->CTRLB &= ~SPI_RXEN; while(_uhw.spi->SYNCBUSY);

		_DMATX->WritePeripheral(data, &_uhw.spi->DATA, count, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

	#elif defined(CPU_XMC48)
	
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::WriteSyncDMA(void *data, u16 count)
{
	WriteAsyncDMA(data, count);

	while (!CheckWriteComplete());

	_DMATX->Disable();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::ReadPIO(void *data, u16 count)
{
	volatile register byte t;
	byte *p = (byte*)data;

	#ifdef CPU_SAME53	

		_uhw.spi->CTRLB |= SPI_RXEN; while(_uhw.spi->SYNCBUSY);

		do t = _uhw.spi->DATA; while(_uhw.spi->INTFLAG & SPI_RXC); 

		while (count != 0)
		{
			_uhw.spi->DATA = 0;

			while((_uhw.spi->INTFLAG & SPI_RXC) == 0);

			*(p++) = _uhw.spi->DATA; count--;
		};

	#elif defined(CPU_XMC48)
		
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::ReadAsyncDMA(void *data, u16 count)
{
	volatile register byte t;

	#ifdef CPU_SAME53	

		_uhw.spi->INTFLAG = ~0;
		_uhw.spi->INTENCLR = ~0;
		_uhw.spi->CTRLB |= SPI_RXEN; while(_uhw.spi->SYNCBUSY);

		do t = _uhw.spi->DATA; while(_uhw.spi->INTFLAG & SPI_RXC); 

		_DMARX->ReadPeripheral(&_uhw.spi->DATA, data, count,	DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);
		_DMATX->WritePeripheral(data, &_uhw.spi->DATA, count, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

	#elif defined(CPU_XMC48)
	
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_SPIM::ReadSyncDMA(void *data, u16 count)
{
	ReadAsyncDMA(data, count);

	while (!CheckReadComplete());

	_uhw.spi->CTRLB &= ~SPI_RXEN;

	_DMATX->Disable();
	_DMARX->Disable();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

byte S_SPIM::WriteReadByte(byte v)
{
	volatile register byte t;

	#ifdef CPU_SAME53	

		_uhw.spi->CTRLB |= SPI_RXEN; while(_uhw.spi->SYNCBUSY);

		do t = _uhw.spi->DATA; while(_uhw.spi->INTFLAG & SPI_RXC); 

		_uhw.spi->DATA = v;

		while((_uhw.spi->INTFLAG & SPI_RXC) == 0);

		return _uhw.spi->DATA; 

	#elif defined(CPU_XMC48)
		
	#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_SPIM::AddRequest(DSCSPI *d)
{
#ifndef WIN32

	if (d == 0) { return false; };
	if (d->csnum >= _MASK_CS_LEN) return false;

	d->next = 0;
	d->ready = false;

	_reqList.Add(d);

#endif

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_SPIM::Update()
{
	bool result = false;

	T_HW::S_SPI* spi = _uhw.spi;

#ifdef CPU_SAME53

	switch (_state)
	{
		case WAIT:

			if (CheckReset())
			{
				Usic_Update();
			}
			else
			{
				_dsc = _reqList.Get();

				if (_dsc != 0)
				{
					Usic_Lock();

					ChipSelect(_dsc->csnum);  //_PIO_CS->CLR(_MASK_CS[_dsc->csnum]);

					DSCSPI &dsc = *_dsc;

					dsc.ready = false;

					if (dsc.alen == 0)
					{
						if (dsc.wdata != 0 && dsc.wlen > 0)
						{
							WriteAsyncDMA(dsc.wdata, dsc.wlen);
							//_DMATX->WritePeripheral(dsc.wdata, &spi->DATA, dsc.wlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

							_state = WRITE; 
						}
						else if (dsc.rdata != 0 && dsc.rlen > 0)
						{
							ReadAsyncDMA(dsc.rdata, dsc.rlen);
							//spi->CTRLB |= SPI_RXEN;
							//_DMARX->ReadPeripheral(&spi->DATA, dsc.rdata, dsc.rlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);
							//_DMATX->WritePeripheral(dsc.rdata, &spi->DATA, dsc.rlen+1, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

							_state = STOP; 
						};
					}
					else
					{
						spi->INTFLAG = ~0;
						spi->INTENCLR = ~0;
						spi->CTRLB &= ~SPI_RXEN;

						_DMATX->WritePeripheral(&dsc.adr, &spi->DATA, dsc.alen, dsc.wdata, dsc.wlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

						_state = WRITE; 
					};
				};
			};

			break;

		case WRITE:
		{
			DSCSPI &dsc = *_dsc;

			if (CheckWriteComplete())
			{
				_DMATX->Disable();

				if (dsc.rdata != 0 && dsc.rlen > 0)
				{
					ReadAsyncDMA(dsc.rdata, dsc.rlen);

					//spi->CTRLB |= SPI_RXEN;

					//_DMARX->ReadPeripheral(&spi->DATA, dsc.rdata, dsc.rlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);
					//_DMATX->WritePeripheral(dsc.rdata, &spi->DATA, dsc.rlen+1, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);
				};

				_state = STOP; 
			};

			break;
		};

		case STOP:
		{
			if (CheckReadComplete())
			{
				_dsc->ready = true;
				
				_dsc = 0;
				
				ChipDisable();//_PIO_CS->SET(_MASK_CS_ALL);

				_DMARX->Disable();
				_DMATX->Disable();

				spi->CTRLB &= ~SPI_RXEN;
				spi->INTFLAG = ~0;
				spi->INTENCLR = ~0;

				_state = WAIT; 

				Usic_Unlock();
			};

			break;
		};
	};

#elif defined(CPU_XMC48)

	using namespace HW;

	static TM32 tm;

	__disable_irq();

	if (spi_dsc != 0)
	{
		if (!spi_dsc->ready && (GetMilliseconds() - spi_timestamp) > 100)
		{
			result = true;

			HW::Peripheral_Disable(SPI_PID);

			DSCSPI *dsc = spi_dsc;

			spi_dsc = 0;

			SPI_Init();

			SPI_WriteRead(dsc);
		};
	};
	
	__enable_irq();

#endif

	return result;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
