#ifndef MEM_H__12_10_2022__16_54
#define MEM_H__12_10_2022__16_54

#include "types.h"
#include "list.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct MB : public PtrObj<MB>
{
	u32			len;
	u32			dataOffset;
	u32			data[64];

	virtual	u32		MaxLen() { return sizeof(data); }

	void*	GetDataPtr() { return data+dataOffset; }

	MB() : len(0), dataOffset(0) {}
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template<int LEN> struct MEMB : public MB
{
#ifndef WIN32
	u32 exData[(LEN-sizeof(data)+3)>>2];
#else
	u32 exData[(LEN-256+3)>>2];
#endif

protected:

	PTR_LIST_FRIENDS(MEMB);

	static List<MEMB> _freeList;

public:

	static	MEMB*	Create()	{ MEMB *p = _freeList.Get(); if (p != 0) {p->dataOffset = 0; p->len = 0; }; return p; }
	virtual void	Destroy()	{ if (this != 0) _freeList.Add(this); }
	virtual	u32		MaxLen() { return sizeof(data)+sizeof(exData); }

					MEMB() { _freeList.Init(); _freeList.Add(this); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern MB* AllocSmallBuffer();
extern MB* AllocMediumBuffer();
extern MB* AllocHugeBuffer();
extern MB* AllocMemBuffer(u32 minLen);

#endif // MEM_H__12_10_2022__16_54
