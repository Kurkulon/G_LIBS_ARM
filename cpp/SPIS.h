#ifndef SPIS_H__08_06_2024__16_02
#define SPIS_H__08_06_2024__16_02

#if !defined(CPU_LPC812)

#include "types.h"
#include "usic.h"
#include "list.h"
#include "DMA.h"

#define US2SPIS(v) US2CTM(v)
#define MS2SPIS(v) MS2CTM(v)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class S_SPIS : public USIC
{

public:

	struct RBUF
	{
		bool	recieved;
		word	len;
		word	maxLen;
		void*	data;
	};

	struct WBUF
	{
		bool	transmited;
		word	len;
		void*	data;
	};

protected:

	//const I2C_CONF _conf;

#ifdef CPU_SAME53

	T_HW::S_PORT* const _PIO_SPCK;
	T_HW::S_PORT* const _PIO_MOSI;
	T_HW::S_PORT* const _PIO_MISO;
	T_HW::S_PORT* const _PIO_SS;

	const u32 			_PMUX_SPCK;
	const u32 			_PMUX_MOSI;
	const u32 			_PMUX_MISO;
	const u32 			_PMUX_SS;

	const u32 			_GEN_SRC;
	const u32 			_GEN_CLK;

	const u32 			_DIPO;
	const u32 			_DOPO;

	DMA_CH *	const 	_DMATX;
	DMA_CH *	const 	_DMARX;

	const byte  		_PIN_SPCK;
	const byte 			_PIN_MOSI;
	const byte 			_PIN_MISO;
	const byte 			_PIN_SS;

#elif defined(CPU_LPC824)

	DMA_CH				_DMARX;
	DMA_CH				_DMATX;

#endif

	u32					__CTRLA;
	RBUF				*_prbuf;
	WBUF				*_pwbuf;

	u32					_postReadTimeout;
	u32					_readTimeout;
	u32					_prevCounter;

	CTM32				_rtm;

	enum STATE { WAIT = 0, WRITE, READ };

	STATE _state;


	u32	GetDmaCounter() { return _DMARX->GetBytesLeft(); }
	u16	GetRecievedLen() { return _prbuf->maxLen - GetDmaCounter(); }

public:

#ifdef CPU_SAME53

	S_SPIS(byte num, T_HW::S_PORT* pspck, T_HW::S_PORT* pmosi, T_HW::S_PORT* pmiso, T_HW::S_PORT* pss, 
		byte pinspck, byte pinmosi, byte pinmiso, byte pinss, u32 muxspck, u32 muxmosi, u32 muxmiso, u32 muxss, 
		u32 dipo, u32 dopo,	u32 gen_src, u32 gen_clk, DMA_CH *dmatx, DMA_CH *dmarx)
		: USIC(num), _PIO_SPCK(pspck), _PIO_MOSI(pmosi), _PIO_MISO(pmiso), _PIO_SS(pss), 
		_PIN_SPCK(pinspck), _PIN_MOSI(pinmosi), _PIN_MISO(pinmiso), _PIN_SS(pinss),
		_PMUX_SPCK(muxspck), _PMUX_MOSI(muxmosi), _PMUX_MISO(muxmiso), _PMUX_SS(muxss), _GEN_SRC(gen_src), _GEN_CLK(gen_clk), _DIPO(dipo), _DOPO(dopo), 
		 _DMATX(dmatx), _DMARX(dmarx), _state(WAIT) {}

	bool CheckWriteComplete() { return _DMATX->CheckComplete() && (_uhw.spi->INTFLAG & SPI_TXC); }
	bool CheckReadComplete() { return _DMATX->CheckComplete() && _DMARX->CheckComplete(); }

#elif defined(CPU_LPC824)

	S_SPIS(byte num) : USIC(num), _DMARX(num*2), _DMATX(num*2+1) {}

	void ChipSelect(byte num)	{  }
	bool CheckWriteComplete()	{ return _DMATX.CheckComplete() /*&& (_uhw.spi->INTFLAG & SPI_TXC)*/; }
	bool CheckReadComplete()	{ return _DMARX.CheckComplete(); }
	void ChipDisable()			{  }

#elif defined(WIN32)

	S_SPIS() : USIC(0) {}

#endif

	bool Connect(bool cpol, bool cpha, bool LSB = false);
	void Disconnect();
	bool Update();

	bool Read(S_SPIS::RBUF *rbuf, u32 preTimeout, u32 postTimeout);
	bool Write(S_SPIS::WBUF *wbuf, u32 timeout = 0);

	//byte WriteReadByte(byte v);

	//void WritePIO(void *data, u16 count);
	//void WriteAsyncDMA(void *data, u16 count);
	//void WriteSyncDMA(void *data, u16 count);

	//void ReadPIO(void *data, u16 count);
	//void ReadAsyncDMA(void *data, u16 count);
	//void ReadSyncDMA(void *data, u16 count);

	virtual	void InitHW();
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#endif // #if !defined(CPU_LPC812)

#endif // SPIS_H__08_06_2024__16_02
