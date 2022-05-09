#ifndef USIC_H__05_05_2022__18_33
#define USIC_H__05_05_2022__18_33

#ifdef WIN32
#include <windows.h>
#else
#include "core.h"
#endif

#include "types.h"

#ifdef CPU_SAME53

#define USIC_NUM 6

#elif defined(CPU_XMC48)

#define USIC_NUM 8

#elif defined(WIN32)

#define USIC_NUM 8

#endif 



class USIC
{
	protected:

		static 	u32 	_busy_mask;
		static 	u32 	_alloc_mask;

		static	USIC*	_usic[USIC_NUM];

	public:


		USIC() {}

		virtual void InitHW() {}


};

#endif // USIC_H__05_05_2022__18_33
