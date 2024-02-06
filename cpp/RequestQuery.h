#ifndef REQUESTQUERY_H__06_02_2024__16_44
#define REQUESTQUERY_H__06_02_2024__16_44

#pragma once

#include "ComPort.h"
#include "list.h"
#include "flash.h"


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct REQ  : public PtrObj<REQ>
{
	enum CRC_TYPE { CRC16 = 0, CRC16_CCIT, CRC16_NOT_VALID };

	bool		ready;
	bool		crcOK;
	bool		checkCRC;
	bool		updateCRC;

	typedef bool tRsp(Ptr<REQ> &q);

	u16			tryCount;

	CRC_TYPE	crcType;
	
	tRsp		*CallBack;
	Ptr<MB>		rsp;

	ComPort::WriteBuffer wb;
	ComPort::ReadBuffer rb;

	u32		preTimeOut, postTimeOut;

	u32		reqData[1];

protected:

	virtual void _FreeCallBack() { rsp.Free(); }

public:

	virtual	u32		MaxLen() { return sizeof(reqData); }
	virtual	u32		FreeCount() = 0;

	REQ() : tryCount(0) { }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class RequestQuery
{
	Ptr<REQ>		_req;

	ListPtr<REQ>	reqList;
	
	byte			_state;

	u16				_crc;
	u16 			_crcLen;

	byte*			_crcPtr;


	ComPort			*com;

	//u32			count;

	bool			_run;

public:

				RequestQuery(ComPort *p) : _run(true), _state(0), com(p) {}
	void		Add(const Ptr<REQ>& req)	{ reqList.Add(req); }
	Ptr<REQ>	Get()						{ return reqList.Get(); }
	//bool Empty() { return reqList.Empty(); }
	//bool Idle() { return (_first == 0) && (_req == 0); }
	bool Stoped() { return !_req.Valid(); }
	void Update();
	void Stop() { _run = false; }
	void Start() { _run = true; }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

extern REQ* AllocREQ();

#endif //REQUESTQUERY_H__06_02_2024__16_44
