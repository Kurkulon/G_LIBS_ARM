#include "usic.h"

u32 		USIC::_busy_mask = 0;
u32 		USIC::_alloc_mask = 0;
u32 		USIC::_req_reset_mask = 0;

USIC*		USIC::_usic[USIC_NUM] = {0};

#ifdef CPU_SAME53

const byte	USIC::_usic_pid[USIC_NUM]		= {	PID_SERCOM0,		PID_SERCOM1,		PID_SERCOM2,		PID_SERCOM3,		PID_SERCOM4,		PID_SERCOM5,		PID_SERCOM6,		PID_SERCOM7 		};
USICHWT	const USIC::_usic_hw[USIC_NUM]		= { HW::I2C0,			HW::I2C1,			HW::I2C2,			HW::I2C3,			HW::I2C4,			HW::I2C5,			HW::I2C6,			HW::I2C7			};
const byte	USIC::_usic_gclk[USIC_NUM]		= {	GCLK_SERCOM0_CORE,	GCLK_SERCOM1_CORE,	GCLK_SERCOM2_CORE,	GCLK_SERCOM3_CORE,	GCLK_SERCOM4_CORE,	GCLK_SERCOM5_CORE,	GCLK_SERCOM6_CORE,	GCLK_SERCOM7_CORE	};

#elif defined(CPU_XMC48)

const byte	USIC::_usic_pid[USIC_NUM]		= {	PID_USIC0,		PID_USIC0,		PID_USIC1,		PID_USIC1,		PID_USIC2,		PID_USIC2		};
USICHWT		const USIC::_usic_hw[USIC_NUM]	= {	HW::USIC0_CH0,	HW::USIC0_CH1,	HW::USIC1_CH0,	HW::USIC1_CH1,	HW::USIC2_CH0,	HW::USIC2_CH1	};

#elif defined(CPU_LPC824)

const byte	USIC::_usic_pid[USIC_NUM]		= {	0 };
USICHWT		const USIC::_usic_hw[USIC_NUM]	= {	0 };

#elif defined(WIN32)

const byte	USIC::_usic_pid[USIC_NUM] = {0};

USICHWT	const USIC::_usic_hw[USIC_NUM] = { 0 };

#endif 


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool USIC::Usic_Connect()
{
	if (_usic_num >= USIC_NUM || (_alloc_mask & _usic_mask) || _usic[_usic_num] != 0) return false;

	_alloc_mask |= _usic_mask;

	_usic[_usic_num] = this;
	_uhw = _usic_hw[_usic_num];

#ifdef CPU_SAME53

	_ugclk = _usic_gclk[_usic_num];

#endif

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void USIC::Usic_Disconnect()
{
	_alloc_mask &= ~_usic_mask;
	_usic[_usic_num] = 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void USIC::Usic_Update()
{
	if ((_req_reset_mask & _usic_mask) && !(_busy_mask & _usic_mask))
	{
		InitHW();
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#elif defined(CPU_XMC48)

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void USIC::Usic_Update()
{
	if (_req_reset_mask & _usic_mask)
	{
		byte num = _usic_num ^ 1;

		u32 mask = (1UL<<num);

		_req_reset_mask |= mask;

		if (!(_busy_mask & (mask|_usic_mask)))
		{
			HW::Peripheral_Disable(_upid);

			InitHW();

			if ((_alloc_mask & mask) && _usic[num] != 0)
			{
				HW::Peripheral_Disable(_usic[num]->_upid);

				_usic[num]->InitHW();
			};
			
			_req_reset_mask &= ~(mask|_usic_mask);
		};
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

