#ifndef SPI_H__21_04_2022__11_18
#define SPI_H__21_04_2022__11_18

#include "types.h"
#include "usic.h"
#include "list.h"
#include "DMA.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct DSCSPI
{
	DSCSPI*			next;

	void*			wdata;
	void*			rdata;
	u32				adr;
	u16				alen;
	u16				wlen;
	u16				rlen;
	volatile bool	ready;
	byte			csnum;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class S_SPIM : public USIC
{
protected:

	//const I2C_CONF _conf;

#ifdef CPU_SAME53

	T_HW::S_PORT* const _PIO_SPCK;
	T_HW::S_PORT* const _PIO_MOSI;
	T_HW::S_PORT* const _PIO_MISO;
	T_HW::S_PORT* const _PIO_CS;

	const u32 			_MASK_SPCK;
	const u32 			_MASK_MOSI;
	const u32 			_MASK_MISO;

	const u32 			_PMUX_SPCK;
	const u32 			_PMUX_MOSI;
	const u32 			_PMUX_MISO;

	const u32 			_GEN_SRC;
	const u32 			_GEN_CLK;

	const u32 * const	_MASK_CS;
	const u32			_MASK_CS_LEN;

	const u32 			_DIPO;
	const u32 			_DOPO;

	DMA_CH *	const 	_DMATX;
	DMA_CH *	const 	_DMARX;

#elif defined(CPU_XMC48)

	T_HW::S_PORT* const _PIO_SPCK;
	T_HW::S_PORT* const _PIO_MOSI;
	T_HW::S_PORT* const _PIO_MISO;
	T_HW::S_PORT* const _PIO_CS;

	const byte 			_PIN_SPCK;
	const byte 			_PIN_MOSI;
	const byte 			_PIN_MISO;

	const byte 			_MUX_SPCK;
	const byte 			_MUX_MOSI;

	const byte * const	_PIN_CS;
	const u32			_PIN_CS_LEN;

	DMA_CH *	const 	_DMA;

	const u32	_DX0CR;
	const u32	_DX1CR;
	const u32	_GEN_CLK;

	//u32 __SCTR;
	u32 __FDR;
	//u32 __BRG;
	//u32 __TCSR;
	//u32 __CCR;
	//u32 __PCR;

#endif

	List<DSCSPI>	_reqList;
	DSCSPI*			_dsc;

	enum STATE { WAIT = 0, WRITE, STOP };

	STATE _state;

	u32		_baud;
	u32		_MASK_CS_ALL;

public:

#ifdef CPU_SAME53

	S_SPIM(byte num, T_HW::S_PORT* pspck, T_HW::S_PORT* pmosi, T_HW::S_PORT* pmiso, T_HW::S_PORT* pcs, 
		u32 mspck, u32 mmosi, u32 mmiso, 
		u32 muxspck, u32 muxmosi, u32 muxmiso,
		u32* mcs, u32 mcslen, u32 dipo, u32 dopo,
		u32 gen_src, u32 gen_clk, DMA_CH *dmatx, DMA_CH *dmarx)
		: USIC(num), _PIO_SPCK(pspck), _PIO_MOSI(pmosi), _PIO_MISO(pmiso), _PIO_CS(pcs), 
		_MASK_SPCK(mspck), _MASK_MOSI(mmosi), _MASK_MISO(mmiso),
		_PMUX_SPCK(muxspck), _PMUX_MOSI(muxmosi), _PMUX_MISO(muxmiso), _GEN_SRC(gen_src), _GEN_CLK(gen_clk), _MASK_CS(mcs), _MASK_CS_LEN(mcslen), _DIPO(dipo), _DOPO(dopo), 
		 _DMATX(dmatx), _DMARX(dmarx), _dsc(0), _state(WAIT) {}

			bool CheckWriteComplete() { return _DMATX->CheckComplete() && (_uhw.spi->INTFLAG & SPI_TXC); }
			bool CheckReadComplete() { return _DMATX->CheckComplete() && _DMARX->CheckComplete(); }
			void ChipSelect(byte num)	{ _PIO_CS->CLR(_MASK_CS[num]); }
			void ChipDisable()			{ _PIO_CS->SET(_MASK_CS_ALL); }

#elif defined(CPU_XMC48)

	S_SPIM(byte num, T_HW::S_PORT* pspck, T_HW::S_PORT* pmosi, T_HW::S_PORT* pmiso, T_HW::S_PORT* pcs, 
		byte pinspck, byte pinmosi, byte pinmiso, byte muxspck, byte muxmosi, byte* pincs, u32 pincslen, DMA_CH *dma, u32 dx0cr, u32 dx1cr, u32 genclk)
		: USIC(num), _PIO_SPCK(pspck), _PIO_MOSI(pmosi), _PIO_MISO(pmiso), _PIO_CS(pcs), 
		_PIN_SPCK(pinspck), _PIN_MOSI(pinmosi), _PIN_MISO(pinmiso), _MUX_SPCK(muxspck), _MUX_MOSI(muxmosi), _PIN_CS(pincs), _PIN_CS_LEN(pincslen),
		_DMA(dma), _DX0CR(dx0cr), _DX1CR(dx1cr), _GEN_CLK(genclk), _dsc(0), _state(WAIT) {}

			void ChipSelect(byte num)	{ _PIO_CS->BCLR(_PIN_CS[num]); }
			bool CheckWriteComplete() { return _DMA->CheckComplete() /*&& (_uhw.spi->INTFLAG & SPI_TXC)*/; }
			bool CheckReadComplete() { return _DMA->CheckComplete(); }
			void ChipDisable()			{ _PIO_CS->SET(_MASK_CS_ALL); }

#elif defined(WIN32)

	S_SPIM() : USIC(0) {}

#endif

			bool Connect(u32 baudrate);
			void Disconnect();
			bool AddRequest(DSCSPI *d);
			bool Update();
			
			byte WriteReadByte(byte v);

			void WritePIO(void *data, u16 count);
			void WriteAsyncDMA(void *data, u16 count);
			void WriteSyncDMA(void *data, u16 count);

			void ReadPIO(void *data, u16 count);
			void ReadAsyncDMA(void *data, u16 count);
			void ReadSyncDMA(void *data, u16 count);

	virtual	void InitHW();
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void SPI_Init();
extern bool SPI_AddRequest(DSCSPI *d);
extern bool SPI_Update();
//extern i32	Get_FRAM_SPI_SessionsAdr();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // SPI_H__21_04_2022__11_18
