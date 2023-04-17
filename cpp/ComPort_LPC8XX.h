#ifndef COMPORT_LPC8XX_H__10_03_2023__18_56
#define COMPORT_LPC8XX_H__10_03_2023__18_56

#ifdef CPU_LPC8XX_

#include "types.h"
#include "core.h"

#define US2COM(v) US2CTM(v)
#define MS2COM(v) MS2CTM(v)

class ComPort
{
  public:

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

  protected:

	enum STATUS485 { WRITEING = 0, WAIT_READ = 1, READING = 2, READ_END = 3 };

	struct ComBase
	{
		bool used;
		T_HW::S_USART	*usart;
		const dword		maskRTS;

		byte*		rdata;
		u16			rlen;

		byte*		wdata;
		u16			wlen;

		T_HW::LPC_IHP	ir;
		T_HW::LPC_IHP	iw;

		byte			ivect;

		u32				tm;
	};

	static ComBase _bases[3];

	bool			_connected;
	byte			_status485;
	byte			_portNum;

//	word			_prevDmaCounter;

	ReadBuffer		*_pReadBuffer;
	WriteBuffer		*_pWriteBuffer;


	word			_BaudRateRegister;

	dword			_ModeRegister;
	dword			_maskRTS;

	//dword			_startTransmitTime;
	//dword			_startReceiveTime;
	//dword			_preReadTimeout;
	//dword			_postReadTimeout;

	T_HW::S_USART 	*_usart;

	void 		EnableTransmit(void* src, word count);
	void 		DisableTransmit();
	void 		EnableReceive(void* dst, word count);
	void 		DisableReceive();

	//static		bool _InitCom(byte i, ComPort* cp);
	//static		bool _DeinitCom(byte i, ComPort* cp);

	static ComPort *_objCom1;
	static ComPort *_objCom2;
	static ComPort *_objCom3;

	word 		BoudToPresc(dword speed);



  public:
	  
	ComPort() : _connected(false), _status485(READ_END) {}

	bool		Connect(byte port, dword speed, byte parity);
	bool		Disconnect();
	bool		Update();

	bool		Read(ComPort::ReadBuffer *readBuffer, dword preTimeout, dword postTimeout);
	bool		Write(ComPort::WriteBuffer *writeBuffer);

	static __irq void ReadHandler_0();
	static __irq void ReadHandler_1();
	static __irq void ReadHandler_2();

	static __irq void WriteHandler_0();
	static __irq void WriteHandler_1();
	static __irq void WriteHandler_2();
};

#endif // CPU_LPC812
#endif // COMPORT_LPC8XX_H__10_03_2023__18_56
