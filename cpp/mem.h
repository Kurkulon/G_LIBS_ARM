#ifndef MEM_H__12_10_2022__16_54
#define MEM_H__12_10_2022__16_54

#include "types.h"
#include "list.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct MB : public PtrObj<MB>
{
	u32			len;
	u32			dataOffset;

protected:

	u32			_data[1];

public:

	virtual	u32		MaxLen() { return sizeof(_data); }
	virtual	u32		FreeCount() = 0;

	void*	GetDataPtr() { return (byte*)_data+dataOffset; }

	MB() : len(0), dataOffset(0) {}
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template<int LEN> struct MEMB : public MB
{
	u32 exData[(LEN+3)>>2];

protected:

	PTR_LIST_FRIENDS(MEMB);

	static List<MEMB> _freeList;

public:

	static	MEMB*	Create()	{ MEMB *p = _freeList.Get(); if (p != 0) {p->dataOffset = 0; p->len = 0; p->alcc = true;}; return p; }
	virtual void	Destroy()	{ if (this != 0) this->alcc = false, _freeList.Add(this); }
	virtual	u32		MaxLen()	{ return sizeof(_data)+sizeof(exData); }
	virtual	u32		FreeCount() { return _freeList.Count(); }

					MEMB() { _freeList.Init(); _freeList.Add(this); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern MB* AllocSmallBuffer();
extern MB* AllocMediumBuffer();
extern MB* AllocHugeBuffer();
extern MB* AllocMemBuffer(u32 minLen);

#endif // MEM_H__12_10_2022__16_54
