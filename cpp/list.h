#ifndef LIST_H__03_02_2016__14_38
#define LIST_H__03_02_2016__14_38

#pragma once

#include "types.h"

#ifdef WIN32

#include <intrin.h>

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define PTR_LIST_FRIENDS(v) friend class Ptr<v>;friend class List<v>;friend class ListPtr<v>;friend class ListRef<v>

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class LockBase
{

#ifdef WIN32

public:

	u32		locks_count;

protected:

	long	locked;

    enum {LOCK_IS_FREE = 0, LOCK_IS_TAKEN = 1};

	__forceinline void Lock()		{ while (_InterlockedCompareExchange(&locked, LOCK_IS_TAKEN, LOCK_IS_FREE) == LOCK_IS_TAKEN) locks_count++; }
	__forceinline void Unlock()	{ _InterlockedExchange(&locked, LOCK_IS_FREE);  }

#else

protected:

	__forceinline void Lock()	{ __disable_irq(); }
	__forceinline void Unlock()	{ __enable_irq();  }

#endif

};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> struct List : public LockBase
{

protected:

	T *first;
	T *last;

	u32 count;

	List* _selfPtr;

	void Assert(T* r) { DEBUG_ASSERT(HW::RamCheck(r) && r->next == 0); }

  public:

	void Init() { if (_selfPtr != this) first = 0, last = 0, count = 0, _selfPtr = this; }
	
	List() { Init(); }

	T*		Get();
	void	Add(T* r);

	bool	Empty() { return first == 0; }

	u32		Count() { return count; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> T* List<T>::Get()
{
	Lock();

	T* r = first;

	if (r != 0)
	{
		first = (T*)r->next;

		if (first == 0)
		{
			last = 0;
		};

		r->next = 0;

		count--;
	};

	Unlock();
	
	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> void List<T>::Add(T* r)
{
	if (r == 0)
	{
		return;
	};

	Assert(r);

	Lock();

	if (last == 0)
	{
		first = last = r;
	}
	else
	{
		last->next = r;
		last = r;
	};

	//r->next = 0;

	count++;

	Unlock();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct ListItem
{
	//friend class List<ListItem>;

	ListItem	*next;
	void		*item;

	static	List<ListItem> freeItemList;

	static	ListItem*	Alloc()	{ return freeItemList.Get(); }
			void		Free()	{ freeItemList.Add(this); }

			ListItem() : next(0), item(0) { freeItemList.Init(); freeItemList.Add(this); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> struct ListPtr;
template <class T> struct ListRef;

template <class T> class Ptr
{
	friend class ListPtr<T>;
	friend class ListRef<T>;

protected:

	T* ptr;

public:

	Ptr() : ptr(0)					{}
	Ptr(T* p) : ptr(p)				{ if (ptr != 0) ptr->IncCount(); }
	Ptr(const Ptr& p) : ptr(p.ptr)	{ if (ptr != 0) ptr->IncCount(); }

	Ptr& operator=(const Ptr& p)	{ if (ptr != p.ptr) { if (p.ptr != 0)	p.ptr->IncCount();	Free(); ptr = p.ptr;	}; return *this; }
	Ptr& operator=(T* p)			{ if (ptr != p)		{ if (p != 0)		p->IncCount();		Free(); ptr = p;		}; return *this; }
	~Ptr()							{ Free(); }
	bool Valid() const				{ return ptr != 0; }
	void Alloc()					{ Free(); ptr = T::Create(); if (ptr != 0) { DEBUG_ASSERT(ptr->_count == 0); ptr->SetCount(); }; }
	void Free()						{ if (ptr != 0) { ptr->DecCount(this); ptr = 0;}; }
	T* operator->()					{ return ptr; }
	T& operator*()					{ return *ptr; }
	u32	 Count() const				{ return (ptr != 0) ? ptr->Count() : 0; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> struct PtrObj : public LockBase
{
	PTR_LIST_FRIENDS(T);

protected:

	PtrObj* volatile next;
	//volatile bool alcc;
	//Ptr<T> *volatile last_ptr;

private:

	volatile i32 _count;

protected:

	__forceinline void IncCount()			{ Lock(); /*DEBUG_ASSERT(alcc);*/ _count++; Unlock(); }
	__forceinline void DecCount(Ptr<T> *p)	{ Lock(); if (_count != 0) { /*DEBUG_ASSERT(alcc);*/ _count--; if (_count == 0) /*last_ptr = p,*/ Destroy(); } else { /*DEBUG_ASSERT(!alcc);*/ }; Unlock(); }
	__forceinline void SetCount()			{ /*DEBUG_ASSERT(alcc);*/ _count = 1; }

public:

	virtual	void	Destroy() = 0;

	u32		Count() { return _count; }

	void Assert() { DEBUG_ASSERT(next == 0); }

	PtrObj() : next(0), _count(0)/*, alcc(false), last_ptr(0)*/ { }
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> struct ListPtr : public LockBase
{

protected:

	T* first;
	T* last;

	u32 count;

	ListPtr* _selfPtr;

	void Assert(T* r) { DEBUG_ASSERT(r->next == 0); }

public:

	void Init() { if (_selfPtr != this) first = 0, last = 0, count = 0, _selfPtr = this; }

	ListPtr() { Init(); }

	Ptr<T> Get();

	void	Add(const Ptr<T> &r);

	bool	Empty() { return first == 0; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> Ptr<T> ListPtr<T>::Get()
{
	Lock();

	Ptr<T> r(first);

	if (r.ptr != 0)
	{
		//DEBUG_ASSERT(r.ptr->alcc);
		DEBUG_ASSERT(r.ptr->_count >= 2);

		first = (T*)r.ptr->next;

		r.ptr->next = 0;

		if (first == 0)	last = 0;

		count--;

		r->DecCount(&r);
	};

	Unlock();

	//if (r.Valid()) DEBUG_ASSERT(r.ptr->alcc);

	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> void ListPtr<T>::Add(const Ptr<T>& r)
{
	if (!r.Valid())	return;

	//DEBUG_ASSERT(r.ptr->alcc);
	DEBUG_ASSERT(r.ptr->_count != 0);

	r.ptr->IncCount();

	DEBUG_ASSERT(r.ptr->next == 0);

	Lock();

	if (last == 0)
	{
		first = last = r.ptr;
	}
	else
	{
		last->next = r.ptr;
		last = r.ptr;
	};

	count++;

	//r.ptr->next = 0;

	Unlock();

	//DEBUG_ASSERT(r.ptr->alcc);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> struct ListRef : public LockBase
{

protected:

	ListItem* first;
	ListItem* last;

	ListRef* _selfPtr;

public:

	void Init() { if (_selfPtr != this) first = 0, last = 0, _selfPtr = this; }

	ListRef() { Init(); }

	Ptr<T> Get();

	bool Add(const Ptr<T> &r);

	bool Empty() { return first == 0; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> Ptr<T> ListRef<T>::Get()
{
	Lock();

	Ptr<T> r;

	ListItem* i = first;

	if (i != 0)
	{
		r = (T*)(i->item);

		first = i->next;

		i->next = 0;

		if (first == 0) last = 0;

		if (r.Valid()) r->DecCount((Ptr<T>*)~0);
		
		i->Free();
	};

	Unlock();

	return r;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> bool ListRef<T>::Add(const Ptr<T>& r)
{
	if (!r.Valid()) return false;

	ListItem *item = ListItem::Alloc(); 

	if (item == 0) return false;

	r.ptr->IncCount();

	item->item = r.ptr;

	Lock();

	if (last == 0)
	{
		first = last = item;
	}
	else
	{
		last->next = item;
		last = item;
	};

	Unlock();

	item->next = 0;

	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <class T> struct PtrItem;

template <class T> struct PtrItem : public PtrObj<T>
{
	PTR_LIST_FRIENDS(PtrItem);

protected:

	static List<PtrItem> _freeList;

	static	T*		Create()	{ T* p = (T*)_freeList.Get(); return p; };

	virtual	void	_FreeCallBack() {}
	virtual	void	Destroy() { _FreeCallBack(); _freeList.Add(this); }

public:

	PtrItem() { _freeList.Init(); _freeList.Add(this); }
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // LIST_H__03_02_2016__14_38
