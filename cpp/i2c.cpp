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

		HW::Peripheral_Enable(_upid);

 	//	P5->ModePin0(A1OD);
		//P5->ModePin2(A1PP);

		_uhw->KSCFG = USIC_MODEN|USIC_BPMODEN|USIC_BPNOM|USIC_NOMCFG(0);

		__dsb(15);

		_uhw->SCTR = USIC_SDIR(1) | USIC_TRM(3) | USIC_FLE(0x3F) | USIC_WLE(7);

		_uhw->FDR = _FDR;
		_uhw->BRG = USIC_DCTQ(24)|USIC_SCLKCFG(0);
	    
		_uhw->TCSR = USIC_TDEN(1)|USIC_TDSSM(1);

		_uhw->PSCR = ~0;

		_uhw->CCR = 0;

		_uhw->DX0CR = _DX0CR;
		_uhw->DX1CR = _DX1CR;

		_uhw->CCR = USIC_MODE_I2C;

		_uhw->PCR_IICMode = I2C_STIM;

		_PIO_SCL->ModePin(_PIN_SCL, _MUX_SCL);
		_PIO_SDA->ModePin(_PIN_SDA, _MUX_SDA);

		//VectorTableExt[I2C_IRQ] = I2C_Handler;
		//CM4::NVIC->CLR_PR(I2C_IRQ);
		//CM4::NVIC->SET_ER(I2C_IRQ);

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

	if (baudrate == 0) baudrate = 1;

	#ifdef CPU_SAME53	

		u32 baud = (_GEN_CLK + baudrate/2) / baudrate;

		u32 trim = 10 + (_GEN_CLK+1500000)/3000000;

		baud = (baud > trim) ? ((baud - trim + 1) / 2) : 1;

		if (baud > 0xFF) baud = 0xFF;

		_baud = baud;

		InitHW();

	#elif defined(CPU_XMC48)

		_FDR = (1024 - (((_GEN_CLK + baudrate/2) / baudrate + 8) / 16)) | USIC_DM(1);

		InitHW();

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
		case I2C_WAIT:

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

					while (i2c->SYNCBUSY) __nop();

					i2c->CTRLB = I2C_SMEN;
					i2c->STATUS.BUSSTATE = BUSSTATE_IDLE;

					i2c->INTFLAG = ~0;

					while (i2c->SYNCBUSY) __nop();

					if (_dsc->wlen == 0)
					{
						_DMA->ReadPeripheral(&i2c->DATA, _dsc->rdata, _dsc->rlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

						i2c->ADDR = ((_dsc->rlen <= 255) ? (I2C_LEN(_dsc->rlen)|I2C_LENEN) : 0) | (_dsc->adr << 1) | 1;
						_state = I2C_READ; 
					}
					else
					{
						_DMA->WritePeripheral(_dsc->wdata, &i2c->DATA, _dsc->wlen, _dsc->wdata2, _dsc->wlen2, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_TX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

						i2c->ADDR = (_dsc->adr << 1);
						_state = I2C_WRITE; 
					};
				};
			};

			break;

		case I2C_WRITE:

			if((i2c->INTFLAG & I2C_ERROR) || i2c->STATUS.RXNACK)
			{
				while (i2c->SYNCBUSY) __nop();

				i2c->CTRLB = I2C_SMEN|I2C_CMD_STOP;
				
				_state = I2C_STOP; 
			}
			else
			{
				DSCI2C &dsc = *_dsc;

				__disable_irq();

				bool c = _DMA->CheckComplete() && (i2c->INTFLAG & I2C_MB);
				
				__enable_irq();

				if (c)
				{
					_DMA->Disable();

					dsc.ack = true;

					while (i2c->SYNCBUSY) __nop();

					if (_dsc->rlen > 0)
					{
						_DMA->ReadPeripheral(&i2c->DATA, _dsc->rdata, _dsc->rlen, DMCH_TRIGACT_BURST|(((DMCH_TRIGSRC_SERCOM0_RX>>8)+_usic_num*2)<<8), DMDSC_BEATSIZE_BYTE);

						i2c->ADDR = ((_dsc->rlen <= 255) ? (I2C_LEN(_dsc->rlen)|I2C_LENEN) : 0) | (_dsc->adr << 1) | 1;
		
						_state = I2C_READ; 
					}
					else
					{
						i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
						
						_state = I2C_STOP; 
					};
				};
			};

			break;

		case I2C_READ:

			if((i2c->INTFLAG & I2C_ERROR) || i2c->STATUS.RXNACK)
			{
				while (i2c->SYNCBUSY) __nop();

				i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
				
				_state = I2C_STOP; 
			}
			else
			{
				DSCI2C &dsc = *_dsc;

				__disable_irq();

				bool c = _DMA->CheckComplete();
				
				__enable_irq();

				if (c)
				{
					_DMA->Disable();

					dsc.ack = true;

					while (i2c->SYNCBUSY) __nop();

					i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
						
					_state = I2C_STOP; 
				};
			};

			_dsc->readedLen = _DMA->GetBytesReady(); //_dsc->rlen - DmaWRB[I2C_DMACH].BTCNT;

			break;

		case I2C_STOP:

			if (i2c->STATUS.BUSSTATE == BUSSTATE_IDLE)
			{
				_dsc->ready = true;
				
				_dsc = 0;
				
				i2c->CTRLB = I2C_SMEN;

				_state = I2C_WAIT; 

				Usic_Unlock();
			}
			else if (i2c->SYNCBUSY == 0)
			{
				i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
			};

			break;

		case I2C_RESET:

			if (i2c->STATUS.BUSSTATE == BUSSTATE_IDLE)
			{
				_dsc->ready = false;

				_reqList.Add(_dsc);
					
				_dsc = 0;
					
				_state = I2C_WAIT;

				Usic_Unlock();
			}
			else if (i2c->SYNCBUSY == 0)
			{
				i2c->CTRLB = I2C_SMEN|I2C_ACKACT|I2C_CMD_STOP;
			};

			break;
	};

#elif defined(CPU_XMC48)

	USICHWT	&I2C = _uhw;

	u32 psr = I2C->PSR_IICMode;

	switch (_state)
	{
		case I2C_WAIT:

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

					wrPtr	= (byte*)dsc.wdata;
					wrPtr2	= (byte*)dsc.wdata2;
					rdPtr	= (byte*)dsc.rdata;
					wlen	= dsc.wlen;
					wlen2	= dsc.wlen2;
					rlen	= dsc.rlen;

					I2C->PSCR = ~I2C_PCR;//RIF|AIF|TBIF|ACK|NACK|PCR;

					if (wlen == 0)
					{
						I2C->TBUF[0] = I2C_TDF_MASTER_START | (dsc.adr << 1) | 1;

						_state = I2C_READ;
					}
					else
					{
						I2C->TBUF[0] = I2C_TDF_MASTER_START | (dsc.adr << 1) | 0;

						_state = I2C_WRITE;
					};
				};
			};

			break;

		case I2C_WRITE:

			if (psr & (I2C_NACK | I2C_ERR | I2C_ARL | I2C_WTDF))
			{
				I2C->TBUF[0] = I2C_TDF_MASTER_STOP;

				_state = I2C_STOP;
			}
			else if (psr & I2C_ACK)
			{
				DSCI2C& dsc = *_dsc;

				dsc.ack = true;

				I2C->PSCR = I2C_ACK | I2C_PCR;

				if (wlen != 0)
				{
					I2C->TBUF[0] = I2C_TDF_MASTER_SEND | *wrPtr++; wlen--;

					if (wlen == 0 && wlen2 != 0)
					{
						wrPtr = wrPtr2;
						wlen = wlen2;
						wlen2 = 0;
					};
				}
				else if (rlen > 0)
				{
					I2C->TBUF[0] = I2C_TDF_MASTER_RESTART | (dsc.adr << 1) | 1;

					_state = I2C_READ;
				}
				else
				{
					I2C->TBUF[0] = I2C_TDF_MASTER_STOP;

					_state = I2C_STOP;
				};
			}
			else if (psr == 0)
			{
				_state = I2C_RESET;
			};

			break;

		case I2C_READ:

			if (psr & I2C_ACK)
			{
				if (psr & (I2C_SCR|I2C_RSCR))
				{
					I2C->TBUF[0] = I2C_TDF_MASTER_RECEIVE_ACK;

					I2C->PSCR = I2C_ACK | I2C_PCR;
				};
			}
			else if (psr & (I2C_RIF|I2C_AIF))
			{
//				twi_timestamp = GetMilliseconds();

				byte t = I2C->RBUF;

				if (rlen > 0)
				{
					*rdPtr++ = t; // receive data
					rlen--;
				};
					
				I2C->TBUF[0] = (rlen > 0) ? I2C_TDF_MASTER_RECEIVE_ACK : I2C_TDF_MASTER_RECEIVE_NACK; 

				I2C->PSCR = I2C_RIF | I2C_AIF | I2C_PCR;
			}
			else if(psr & (I2C_NACK|I2C_ERR|I2C_ARL|I2C_WTDF))
			{
				I2C->TBUF[0] = I2C_TDF_MASTER_STOP;
				
				_state = I2C_STOP;
			}
			else if (psr == 0)
			{
				_state = I2C_RESET;
			};

			break;

		case I2C_STOP:

			I2C->PSCR = ~I2C_PCR;

			_dsc->readedLen = _dsc->rlen - rlen;
			_dsc->ready = true;
				
			_dsc = 0;
				
			_state = I2C_WAIT;

			Usic_Unlock();

			break;

		case I2C_RESET:

			RequestReset();

			_dsc->ready = false;

			_reqList.Add(_dsc);
				
			_dsc = 0;
				
			_state = I2C_WAIT;

			Usic_Unlock();

			break;
	};
	
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

