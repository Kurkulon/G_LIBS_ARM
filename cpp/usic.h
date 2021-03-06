#ifndef USIC_H__05_05_2022__18_33
#define USIC_H__05_05_2022__18_33

#ifdef WIN32
#include <windows.h>
#else
#include "core.h"
#endif

#include "types.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef CPU_SAME53

#define USIC_NUM 8

typedef T_HW::SERCOM USICHWT;

#elif defined(CPU_XMC48)

#define USIC_NUM 6

typedef T_HW::USIC_CH_Type USICHWT;

#elif defined(WIN32)

#define USIC_NUM 8

#endif 

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class USIC // Universal Serial Interface Channel 
{
	protected:

		static 	u32 		_busy_mask;
		static 	u32 		_alloc_mask;
		static 	u32 		_req_reset_mask;

		static  const byte	_usic_pid[USIC_NUM];
		static	USIC*		_usic[USIC_NUM];

		static	USICHWT		const _usic_hw[USIC_NUM];

#ifdef CPU_SAME53

		static  const byte	_usic_gclk[USIC_NUM];

		byte				_ugclk;

#endif
		const byte			_usic_num;
		const u32			_usic_mask;

		const u32			_upid;
		USICHWT				_uhw;

	public:

		USIC(byte num) : _usic_num(num), _usic_mask(1UL<<num), _upid(_usic_pid[num]) {}

		bool Usic_Connect();
		void Usic_Disconnect();
		void Usic_Update();
		void Usic_Lock()	{ _busy_mask |= _usic_mask; }
		void Usic_Unlock()	{ _busy_mask &= ~_usic_mask; }

		void RequestReset() { _req_reset_mask |= _usic_mask; }
		bool CheckReset()	{ return _req_reset_mask & _usic_mask; }

		virtual void InitHW() {}
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // USIC_H__05_05_2022__18_33
