#ifndef COMPORT__31_01_2007__15_32
#define COMPORT__31_01_2007__15_32

#include "types.h"
#include "core.h"
#include "time.h"
#include "usic.h"
#include "DMA.h"

//#ifdef CPU_XMC48
//	#include "COM_DEF.h"
//#endif

#define US2COM(v) US2CLK(v)
#define MS2COM(v) MS2CLK(v)

class ComPort : public USIC
{
  public:

	enum CONNECT_TYPE { ASYNC = 0, SYNC_M, SYNC_S };

	struct ReadBuffer
	{
		bool	recieved;
		word	len;
		word	maxLen;
		void*	data;
	};

	struct WriteBuffer
	{
		bool	transmited;
		word	len;
		void*	data;
	};

	#ifdef CPU_XMC48

	struct LLI
	{
		volatile void*	SAR;
		volatile void*	DAR;
		LLI*	LLP;
		u32		CTLL;
		u32		CTLH;
		u32		DSTAT;
	};

	#endif


  protected:

	enum STATUS485 { WRITEING = 0, WAIT_READ = 1, READING = 2, READ_END = 3 };

//	union CUSART { void *vp; T_HW::S_UART *DU; T_HW::S_USART *SU; };

	#ifdef CPU_SAME53

		T_HW::S_PORT* const _PIO_TX;
		T_HW::S_PORT* const _PIO_RX;
		T_HW::S_PORT* const _PIO_RTS;

		const u32 _MASK_TX;
		const u32 _MASK_RX;
		const u32 _MASK_RTS;

		const u32 _PMUX_TX;
		const u32 _PMUX_RX;

		const u32 _GEN_SRC;
		const u32 _GEN_CLK;

		const u32 			_TXPO;
		const u32 			_RXPO;

		DMA_CH *	const _DMA;

		u32 _CTRLA;
		u32 _CTRLB;
		u32 _CTRLC;
		u16	_BaudRateRegister;

		u32 _status;

		bool IsTransmited() { return (_uhw.usart->INTFLAG & (USART_TXC|USART_DRE)) && _DMA->CheckComplete(); }
		bool IsRecieved()	{ u32 s = _uhw.usart->INTFLAG & (USART_ERROR|USART_RXS); if (s) { _status |= s; _uhw.usart->INTFLAG = s; return true; } else return false; }
		u32	GetDmaCounter() { return _DMA->GetBytesLeft(); }
		u16	GetRecievedLen() { return _pReadBuffer->maxLen - GetDmaCounter(); }

	#elif defined(CPU_XMC48)

		T_HW::S_PORT* const _PIO_TX;
		T_HW::S_PORT* const _PIO_RX;
		T_HW::S_PORT* const _PIO_RTS;

		const byte	_PIN_TX;
		const byte	_PIN_RX;
		const byte	_PIN_RTS;
		const u32	_MASK_RTS;

		DMA_CH *	const _DMA;

		const u32	_DX0CR;
		//const u32 __DX1CR;

		//const u32 __SCTR;
		u32 __FDR;
		u32 __BRG;
		//const u32 __TCSR;
		u32 __CCR;
		u32 __PCR;


		bool IsTransmited() { return (_uhw->PSR & USIC_BUSY) == 0 && _DMA->CheckComplete(); }
		bool IsRecieved() { return (_uhw->PSR & USIC_BUSY); }
//		u16	GetDmaCounter() { return BLOCK_TS(_chdma->CTLH); }
		u32	GetDmaCounter() { return _DMA->GetDstCounter(); }
//		u16	GetRecievedLen() { return _pReadBuffer->maxLen - _prevDmaCounter; }
		u16	GetRecievedLen() { return _pReadBuffer->maxLen - GetDmaCounter(); }

	#elif defined(WIN32)

		bool IsTransmited() { return true; }
		bool IsRecieved() { return true; }
		u32	GetDmaCounter() { return 0; }
		u16	GetRecievedLen() { return 0; }

	#endif

	bool			_connected;
	byte			_status485;

	u32				_prevDmaCounter;

	ReadBuffer		*_pReadBuffer;
	WriteBuffer		*_pWriteBuffer;




	//dword			_ModeRegister;
	//dword			_pinRTS;
	//dword			_startTransmitTime;

//	dword			_startReceiveTime;
//	dword			_preReadTimeout;
	dword			_postReadTimeout;
	dword			_readTimeout;
	dword			_writeTimeout;

	CTM32			_rtm;

	void 		EnableTransmit(void* src, word count);
	void 		DisableTransmit();
	void 		EnableReceive(void* dst, word count);
	void 		DisableReceive();

	//static		bool _InitCom(byte i, ComPort* cp);
	//static		bool _DeinitCom(byte i, ComPort* cp);

	//static ComPort *_objCom1;
	//static ComPort *_objCom2;
	//static ComPort *_objCom3;

	word 		BoudToPresc(dword speed);


  public:
	  
#ifdef CPU_SAME53

	ComPort(byte num, T_HW::S_PORT* ptx, T_HW::S_PORT* prx, T_HW::S_PORT* prts, u32 mtx, u32 mrx, u32 mrts, u32 muxtx, u32 muxrx, u32 txpo, u32 rxpo, u32 gen_src, u32 gen_clk, DMA_CH *dma)
		: USIC(num), _PIO_TX(ptx), _PIO_RX(prx), _PIO_RTS(prts), _MASK_TX(mtx), _MASK_RX(mrx), _MASK_RTS(mrts),
		_PMUX_TX(muxtx), _PMUX_RX(muxrx), _GEN_SRC(gen_src), _GEN_CLK(gen_clk), _TXPO(txpo), _RXPO(rxpo), _DMA(dma), _status485(READ_END) {}

#elif defined(CPU_XMC48)

	ComPort(byte num, T_HW::S_PORT* ptx, T_HW::S_PORT* prx, T_HW::S_PORT* prts, byte pintx, byte pinrx, byte pinrts, byte dsel, DMA_CH *dma)
		: USIC(num), _PIO_TX(ptx), _PIO_RX(prx), _PIO_RTS(prts), _PIN_TX(pintx), _PIN_RX(pinrx), _PIN_RTS(pinrts), _MASK_RTS(1UL<<pinrts),
		_DMA(dma), _DX0CR(USIC_DSEL(dsel)), _connected(false), _status485(READ_END) {}

#else

	ComPort() : USIC(0), _connected(false), _status485(READ_END) {}

#endif
				bool	Connect(CONNECT_TYPE ct, dword speed, byte parity, byte stopBits);
				//bool	ConnectAsyn(byte port, dword speed, byte parity, byte stopBits);
				//bool	ConnectSync(byte port, dword speed, byte parity, byte stopBits);
				bool	Disconnect();
				bool	Update();
	virtual		void	InitHW();

				bool	Read(ComPort::ReadBuffer *readBuffer, dword preTimeout, dword postTimeout);
				bool	Write(ComPort::WriteBuffer *writeBuffer);

				void	TransmitByte(byte v);

	//static __irq void _IntHandlerCom1();
	//static __irq void _IntHandlerCom2();
	//static __irq void _IntHandlerCom3();

};

#endif // COMPORT__31_01_2007__15_32
