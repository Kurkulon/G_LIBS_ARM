#include "i2c.h"
#include "core.h"
#include "SEGGER_RTT.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void S_I2C::InitHW()
{
#ifdef CPU_SAME53	

	HW::GCLK->PCHCTRL[_ugclk] = GCLK_GEN(_GEN_SRC)|GCLK_CHEN;	// 25 MHz

	HW::MCLK->ClockEnable(_upid);

	_PIO_SCL->SetWRCONFIG(_MASK_SCL, _PMUX_SCL | PORT_PMUXEN | PORT_WRPMUX | PORT_PULLEN | PORT_WRPINCFG);
	_PIO_SDA->SetWRCONFIG(_MASK_SDA, _PMUX_SDA | PORT_PMUXEN | PORT_WRPMUX | PORT_PULLEN | PORT_WRPINCFG);
	_PIO_SCL->SET(_MASK_SCL);
	_PIO_SDA->SET(_MASK_SDA);

	T_HW::S_I2C* i2c = _uhw.i2c;

	i2c->CTRLA = I2C_SWRST;

	while(i2c->SYNCBUSY);

	i2c->CTRLA = SERCOM_MODE_I2C_MASTER;

	i2c->CTRLA = SERCOM_MODE_I2C_MASTER|I2C_INACTOUT_205US|I2C_SPEED_SM;
	i2c->CTRLB = I2C_SMEN;

	i2c->BAUD = _baud;

	i2c->CTRLA |= I2C_ENABLE;

	while(i2c->SYNCBUSY);

	i2c->STATUS = 0;
	i2c->STATUS.BUSSTATE = BUSSTATE_IDLE;
	
#elif defined(CPU_XMC48)

#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_I2C::Connect(u32 baudrate)
{
#ifndef WIN32

	using namespace HW;

	SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_CYAN "I2C%u Init ... ", _usic_num);

	if (!Usic_Connect())
	{
		SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_RED "!!! ERROR !!!\n");

		return false;
	};

	#ifdef CPU_SAME53	

		if (baudrate == 0) baudrate = 1;

		u32 baud = (_GEN_CLK + baudrate/2) / baudrate;

		u32 trim = 10 + (_GEN_CLK+1500000)/3000000;

		baud = (baud > trim) ? ((baud - trim + 1) / 2) : 1;

		if (baud > 0xFF) baud = 0xFF;

		_baud = baud;

		InitHW();

	#elif defined(CPU_XMC48)

		HW::Peripheral_Enable(I2C_PID);

 		P5->ModePin0(A1OD);
		P5->ModePin2(A1PP);

		i2c->KSCFG = MODEN|BPMODEN|BPNOM|NOMCFG(0);

		i2c->SCTR = I2C__SCTR;

		i2c->FDR = I2C__FDR;
		i2c->BRG = I2C__BRG;
	    
		i2c->TCSR = I2C__TCSR;

		i2c->PSCR = ~0;

		i2c->CCR = 0;

		i2c->DX0CR = I2C__DX0CR;
		i2c->DX1CR = I2C__DX1CR;

		i2c->CCR = I2C__CCR;


		i2c->PCR_IICMode = I2C__PCR;

		VectorTableExt[I2C_IRQ] = I2C_Handler;
		CM4::NVIC->CLR_PR(I2C_IRQ);
		CM4::NVIC->SET_ER(I2C_IRQ);

	#endif

#else

	//HANDLE h;

	//h = CreateFile("FRAM_I2C_STORE.BIN", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

	//if (h == INVALID_HANDLE_VALUE)
	//{
	//	return false;
	//};

	//dword bytes;

	//ReadFile(h, fram_I2c_Mem, sizeof(fram_I2c_Mem), &bytes, 0);
	//CloseHandle(h);

#endif

	SEGGER_RTT_WriteString(0, RTT_CTRL_TEXT_BRIGHT_GREEN "OK\n");

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_I2C::AddRequest(DSCI2C *d)
{
#ifndef WIN32

	if (d == 0) { return false; };
	if ((d->wdata == 0 || d->wlen == 0) && (d->rdata == 0 || d->rlen == 0)) { return false; }

	d->next = 0;
	d->ready = false;

	if (d->wdata2 == 0 || d->wlen2 == 0) d->wdata2 = 0, d->wlen2 = 0;

	_reqList.Add(d);

#endif

	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool S_I2C::Update()
{
	bool result = false;

#ifdef CPU_SAME53	

	//static byte *wrPtr = 0;
	//static byte *rdPtr = 0;
	//static u16 	wrCount = 0;
	//static u16 	rdCount = 0;
	//static byte *wrPtr2 = 0;
	//static u16	wrCount2 = 0;
	//static byte adr = 0;

	T_HW::S_I2C* i2c = _uhw.i2c;

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

					DSCI2C &dsc = *_dsc;

					dsc.ready = false;
					dsc.ack = false;
					dsc.readedLen = 0;

					i2c->CTRLB = I2C_SMEN;
					i2c->STATUS.BUSSTATE = BUSSTATE_IDLE;

					i2c->INTFLAG = ~0;

					if (_dsc->wlen == 0)
					{
						_DMA->ReadPeripheral(&i2c->DATA, _dsc->rdata, _dsc->rlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

						i2c->ADDR = ((_dsc->rlen <= 255) ? (I2C_LEN(_dsc->rlen)|I2C_LENEN) : 0) | (_dsc->adr << 1) | 1;
						_state = READ; 
					}
					else
					{
						_DMA->WritePeripheral(_dsc->wdata, &i2c->DATA, _dsc->wlen, _dsc->wdata2, _dsc->wlen2, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

						i2c->ADDR = (_dsc->adr << 1);
						_state = WRITE; 
					};
				};
			};

			break;

		case WRITE:

			if((i2c->INTFLAG & I2C_ERROR) || i2c->STATUS.RXNACK)
			{
				i2c->CTRLB = I2C_SMEN|I2C_CMD_STOP;
				
				_state = STOP; 
			}
			else
			{
				DSCI2C &dsc = *_dsc;

				__disable_irq();

				bool c = _DMA->CheckComplete() && (i2c->INTFLAG & I2C_MB);
				
				__enable_irq();

				if (c)
				{
					dsc.ack = true;

					if (_dsc->rlen > 0)
					{
						_DMA->ReadPeripheral(&i2c->DATA, _dsc->rdata, _dsc->rlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

						i2c->ADDR = ((_dsc->rlen <= 255) ? (I2C_LEN(_dsc->rlen)|I2C_LENEN) : 0) | (_dsc->adr << 1) | 1;
		
						_state = READ; 
					}
					else
					{
						i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
						
						_state = STOP; 
					};
				};
			};

			break;

		case READ:

			if((i2c->INTFLAG & I2C_ERROR) || i2c->STATUS.RXNACK)
			{
				i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
				
				_state = STOP; 
			}
			else
			{
				DSCI2C &dsc = *_dsc;

				__disable_irq();

				bool c = _DMA->CheckComplete();
				
				__enable_irq();

				if (c)
				{
					dsc.ack = true;

					i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
						
					_state = STOP; 
				};
			};

			_dsc->readedLen = _DMA->GetBytesReady(); //_dsc->rlen - DmaWRB[I2C_DMACH].BTCNT;

			break;

		case STOP:

			if (i2c->STATUS.BUSSTATE == BUSSTATE_IDLE)
			{
				_dsc->ready = true;
				
				_dsc = 0;
				
				i2c->CTRLB = I2C_SMEN;

				_state = WAIT; 

				Usic_Unlock();
			}
			else if (i2c->SYNCBUSY == 0)
			{
				i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
			};

			break;
	};

#elif defined(CPU_XMC48)

	using namespace HW;

	static TM32 tm;

	__disable_irq();

	if (twi_dsc != 0)
	{
		if (i2c->PSR_IICMode & (PCR|NACK|ACK|RIF|AIF))
		{
			tm.Reset();
		}
		else if (tm.Check(10))
		{
			result = true;

			HW::Peripheral_Disable(I2C_PID);

			I2C_Init();

			twi_dsc->ready = true;
			twi_dsc->readedLen = twi_dsc->rlen - twi_rdCount;

			DSCI2C *ndsc = twi_dsc->next;

			if (ndsc != 0)
			{
				twi_dsc->next = 0;
				twi_dsc = ndsc;

				twi_dsc->ready = false;
				twi_dsc->ack = false;
				twi_dsc->readedLen = 0;

				twi_wrPtr = (byte*)twi_dsc->wdata;	
				twi_rdPtr = (byte*)twi_dsc->rdata;	
				twi_wrPtr2 = (byte*)twi_dsc->wdata2;	
				twi_wrCount = twi_dsc->wlen;
				twi_wrCount2 = twi_dsc->wlen2;
				twi_rdCount = twi_dsc->rlen;
				twi_adr = twi_dsc->adr;

				if (twi_wrPtr2 == 0) twi_wrCount2 = 0;

				i2c->PSCR = ~0;//RIF|AIF|TBIF|ACK|NACK|PCR;

				i2c->CCR |= RIEN|AIEN;
				i2c->PCR_IICMode |= PCRIEN|NACKIEN|ARLIEN|SRRIEN|ERRIEN|ACKIEN;

				i2c->TBUF[0] = TDF_MASTER_START | (twi_dsc->adr << 1) | ((twi_wrCount == 0) ? 1 : 0);
			}
			else
			{
				i2c->CCR = I2C__CCR;
				i2c->PCR_IICMode = I2C__PCR;

				twi_lastDsc = twi_dsc = 0;
			};
		};
	}
	else
	{
		tm.Reset();
	};
	
	__enable_irq();

#endif

	return result;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

