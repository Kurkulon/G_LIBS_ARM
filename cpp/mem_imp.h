#ifndef MEM_IMP_H__12_10_2022__17_05
#define MEM_IMP_H__12_10_2022__17_05

#include "mem.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <int L> List< MEMB<L> > MEMB<L>::_freeList;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define MEMBS MEMB<SMALL_BUF_LEN>
#define MEMBM MEMB<MEDIUM_BUF_LEN>
#define MEMBH MEMB<HUGE_BUF_LEN>

MEMBS	small_buffer[NUM_SMALL_BUF];
MEMBM	medium_buffer[NUM_MEDIUM_BUF];
MEMBH	huge_buffer[NUM_HUGE_BUF];

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MB* AllocSmallBuffer()
{
	return MEMBS::Create();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MB* AllocMediumBuffer()
{
	return MEMBM::Create();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MB* AllocHugeBuffer()
{
	return MEMBH::Create();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

MB* AllocMemBuffer(u32 minLen)
{
	if (minLen > MEDIUM_BUF_LEN)
	{
		return (minLen <= HUGE_BUF_LEN) ? AllocHugeBuffer() : 0;
	}
	else if (minLen > SMALL_BUF_LEN)
	{
		MB *p = AllocMediumBuffer();

		if (p == 0) p = AllocHugeBuffer();

		return p;
	}
	else
	{
		MB *p = AllocSmallBuffer();

		if (p == 0) p = AllocMediumBuffer();

		if (p == 0) p = AllocHugeBuffer();

		return p;
	};
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // MEM_IMP_H__12_10_2022__17_05
