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
#endif

	List<DSCSPI>	_reqList;
	DSCSPI*			_dsc;

	enum STATE { WAIT = 0, WRITE, STOP };

	STATE _state;

	u32		_baud;
	u32		_MASK_CS_ALL;

public:

	S_SPIM(byte num, T_HW::S_PORT* pspck, T_HW::S_PORT* pmosi, T_HW::S_PORT* pmiso, T_HW::S_PORT* pcs, 
		u32 mspck, u32 mmosi, u32 mmiso, 
		u32 muxspck, u32 muxmosi, u32 muxmiso,
		u32* mcs, u32 mcslen, u32 dipo, u32 dopo,
		u32 gen_src, u32 gen_clk, DMA_CH *dmatx, DMA_CH *dmarx)
		: USIC(num), _dsc(0), _state(WAIT), _PIO_SPCK(pspck), _PIO_MOSI(pmosi), _PIO_MISO(pmiso), _PIO_CS(pcs), 
		_MASK_SPCK(mspck), _MASK_MOSI(mmosi), _MASK_MISO(mmiso),
		_PMUX_SPCK(muxspck), _PMUX_MOSI(muxmosi), _PMUX_MISO(muxmiso), _MASK_CS(mcs), _MASK_CS_LEN(mcslen), _DIPO(dipo), _DOPO(dopo), 
		_GEN_SRC(gen_src), _GEN_CLK(gen_clk), _DMATX(dmatx), _DMARX(dmarx) {}

			bool Connect(u32 baudrate);
			bool AddRequest(DSCSPI *d);
			bool Update();
	virtual	void InitHW();
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern void SPI_Init();
extern bool SPI_AddRequest(DSCSPI *d);
extern bool SPI_Update();
//extern i32	Get_FRAM_SPI_SessionsAdr();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // SPI_H__21_04_2022__11_18
