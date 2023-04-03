#ifndef I2C_H__21_04_2022__11_17
#define I2C_H__21_04_2022__11_17

#include "types.h"
#include "usic.h"
#include "list.h"
#include "DMA.h"
#include "time.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct DSCI2C
{
	DSCI2C*			next;

	void*			wdata;
	void*			rdata;
	void*			wdata2;
	u16				wlen;
	u16				wlen2;
	u16				rlen;
	u16				readedLen;
	byte			adr;
	volatile bool	ready;
	volatile bool	ack;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//struct I2C_CONF
//{
//#ifdef CPU_SAME53
//	
//	T_HW::S_PORT* PIO_SCL;
//	T_HW::S_PORT* PIO_SDA;
//
//	u32 MASK_SCL;
//	u32 MASK_SDA;
//
//	u32 PMUX_SCL;
//	u32 PMUX_SDA;
//
//	u32 GEN_SRC;
//	u32 GEN_CLK;
//
//	DMA_CH *DMA;
//
//#elif defined(CPU_XMC48)
//
//#endif
//
//};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class S_I2C : public USIC
{
protected:

	//const I2C_CONF _conf;

#ifdef CPU_SAME53

	T_HW::S_PORT* const _PIO_SCL;
	T_HW::S_PORT* const _PIO_SDA;

	const u32 _MASK_SCL;
	const u32 _MASK_SDA;

	const u32 _PMUX_SCL;
	const u32 _PMUX_SDA;

	const u32 _GEN_SRC;
	const u32 _GEN_CLK;

	DMA_CH *	const _DMA;

#elif defined(CPU_XMC48)

	T_HW::S_PORT* const _PIO_SCL;
	T_HW::S_PORT* const _PIO_SDA;

	const byte _PIN_SCL;
	const byte _PIN_SDA;

	const byte _MUX_SCL;
	const byte _MUX_SDA;

	DMA_CH *	const _DMA;

	const u32 _DX0CR;
	const u32 _DX1CR;
	const u32 _GEN_CLK;

	u32 _FDR;

	byte	*wrPtr;
	byte	*wrPtr2;
	byte	*rdPtr;
	u16 	wlen;
	u16		wlen2;
	u16 	rlen;

#elif defined (CPU_LPC824) || defined(CPU_LPC8XX)

	const byte	_PIN_SCL;
	const byte	_PIN_SDA;
	const u32	_GEN_CLK;

	u32			_CFG;  
	u32			_CLKDIV; 
	u32			_MSTTIME; 

	static S_I2C *_i2c0;
	static __irq void I2C0_Handler();

	#ifdef CPU_LPC824

		static __irq void I2C1_Handler();
		static __irq void I2C2_Handler();
		static __irq void I2C3_Handler();

		static S_I2C *_i2c1;
		static S_I2C *_i2c2;
		static S_I2C *_i2c3;

	#endif

	void IRQ_Handler();

	byte	*wrPtr;
	byte	*wrPtr2;
	byte	*rdPtr;
	u16 	wlen;
	u16		wlen2;
	u16 	rlen;

	void	Write(DSCI2C *d);

#elif defined(WIN32)

	//byte fram_I2c_Mem[0x10000];

#endif

	List<DSCI2C>	_reqList;
	DSCI2C*			_dsc;

	enum STATE { I2C_WAIT = 0, I2C_WRITE, I2C_READ, I2C_STOP, I2C_RESET };

	STATE _state;

	u32	_baud;
	u32	_prevCount;

	TM32 _tm;

public:

#ifdef CPU_SAME53

	S_I2C(byte num, T_HW::S_PORT* pio_scl, u32 mask_scl, u32 pmux_scl, T_HW::S_PORT* pio_sda, u32 mask_sda, u32 pmux_sda, u32 gen_src, u32 gen_clk, DMA_CH *dma)
		: USIC(num), _PIO_SCL(pio_scl), _PIO_SDA(pio_sda), _MASK_SCL(mask_scl), _MASK_SDA(mask_sda), _PMUX_SCL(pmux_scl), _PMUX_SDA(pmux_sda), 
			_GEN_SRC(gen_src), _GEN_CLK(gen_clk), _DMA(dma), _dsc(0), _state(I2C_WAIT) {}

#elif defined(CPU_XMC48)

	S_I2C(byte num, T_HW::S_PORT* pio_scl, byte pin_scl, byte mux_scl, T_HW::S_PORT* pio_sda, byte pin_sda, byte mux_sda, DMA_CH *dma, u32 dx0cr, u32 dx1cr, u32 genclk)
		: USIC(num), _PIO_SCL(pio_scl), _PIO_SDA(pio_sda), _PIN_SCL(pin_scl), _PIN_SDA(pin_sda), _MUX_SCL(mux_scl), _MUX_SDA(mux_sda), _DMA(dma),
		_DX0CR(dx0cr), _DX1CR(dx1cr), _GEN_CLK(genclk), _dsc(0), _state(I2C_WAIT) {}

#elif defined (CPU_LPC824) || defined(CPU_LPC8XX)

	S_I2C(byte num, byte pinscl, byte pinsda, u32 genclk)
		: USIC(num), _PIN_SCL(pinscl), _PIN_SDA(pinsda), _GEN_CLK(genclk), _dsc(0), _state(I2C_WAIT) {}

#endif

			bool Connect(u32 baudrate);
			bool AddRequest(DSCI2C *d);
			bool Update();
	virtual	void InitHW();
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void I2C_Init();
extern bool I2C_AddRequest(DSCI2C *d);
extern bool I2C_Update();
//extern i32	Get_FRAM_I2C_SessionsAdr();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // I2C_H__21_04_2022__11_17
