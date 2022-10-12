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
	u32 exData[(LEN-sizeof(data)+3)>>2];

protected:

	PTR_LIST_FRIENDS(MEMB);

	static List<MEMB> _freeList;

public:

	static	MEMB*	Alloc()	{ return _freeList.Get(); }
	virtual void	Free()	{ _freeList.Add(this); }
	virtual	u32		MaxLen() { return sizeof(data)+sizeof(exData); }

					MEMB() { _freeList.Init(); _freeList.Add(this); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern Ptr<MB> GetSmallBuffer();
extern Ptr<MB> GetMediumBuffer();
extern Ptr<MB> GetHugeBuffer();
extern Ptr<MB> GetMemBuffer(u32 minLen);

#endif // MEM_H__12_10_2022__16_54