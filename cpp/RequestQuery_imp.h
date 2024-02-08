#ifndef REQUESTQUERY_IMP_H__06_02_2024__16_46
#define REQUESTQUERY_IMP_H__06_02_2024__16_46

#pragma once

#include "RequestQuery.h"

#include "CRC16.h"
#include "time.h"
#include "CRC_CCITT_DMA.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template<int LEN> struct REQUEST : public REQ
{
	u32 exData[(LEN+3)>>2];

protected:

	PTR_LIST_FRIENDS(REQUEST);

	static List<REQUEST> _freeList;

public:

	static	REQUEST*	Create()	{ REQUEST *p = _freeList.Get(); if (p != 0) {p->tryCount = 0; }; return p; }
	virtual void		Destroy()	{ if (this != 0) _freeList.Add(this); }
	virtual	u32			MaxLen()	{ return sizeof(reqData)+sizeof(exData); }
	virtual	u32			FreeCount() { return _freeList.Count(); }

						REQUEST()	{ _freeList.Init(); _freeList.Add(this); }
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//template <class T> List< PtrItem<T> > PtrItem<T>::_freeList;
template <int L> List< REQUEST<L> > REQUEST<L>::_freeList;

typedef REQUEST<REQUEST_DATA_LEN> RQST;

static RQST reqArray[8];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

REQ* AllocREQ()
{
	return RQST::Create();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void RequestQuery::Update()
{
	enum { WAIT = 0, UPDATE_CRC_1, UPDATE_CRC_2, UPDATE_CRC_3, WRITE, READ, CHECK_CRC_1, CHECK_CRC_2, CALLBACK_1 };

	switch(_state)
	{
		case WAIT:

			if (_run)
			{
				_req = Get();

				if (_req.Valid())
				{
					_req->ready = false;

					_state = UPDATE_CRC_1;

					if ((_req->updateCRC || _req->checkCRC) && _req->crcType >= _req->CRC16_NOT_VALID)
					{
						_req->updateCRC = false;
						_req->checkCRC = false;
					};
				};
			}
			else
			{
				_req.Free();
			};

			break;

		case UPDATE_CRC_1:

			if (_req->wb.data != 0 && _req->wb.len != 0)
			{
				if (_req->updateCRC)
				{
					_crc = 0xFFFF;
					_crcLen = _req->wb.len;
					_crcPtr = (byte*)_req->wb.data;

					_state = (_req->crcType == _req->CRC16) ? UPDATE_CRC_3 : UPDATE_CRC_2;
				}
				else
				{
					com->Write(&_req->wb);
					_state = WRITE;
				};
			}
			else
			{
				_req.Free();

				_state = WAIT;
			}; 

			break;

		case UPDATE_CRC_2:

			if (CRC_CCITT_DMA_Async(_crcPtr, _crcLen, _crc))
			{
				_state = UPDATE_CRC_3;
			};

			break;

		case UPDATE_CRC_3:
		{
			if (_req->crcType == _req->CRC16)
			{
				u16 len = 105;

				if (_crcLen < len) len = _crcLen;

				_crcLen -= len;

				_crc = GetCRC16(_crcPtr, len, _crc, 0);

				_crcPtr += len;
			}
			else if (CRC_CCITT_DMA_CheckComplete(&_crc))
			{
				_crcPtr += _crcLen;
				_crcLen = 0;	
			};

			if (_crcLen == 0)
			{
				DataPointer p(_crcPtr);
				*p.w = _crc;
				_req->wb.len += 2;
				_req->updateCRC = false;

				com->Write(&_req->wb);
				_state = WRITE;
			};

			break;
		};

		case WRITE:

			if (!com->Update())
			{
				if (_req->rb.data != 0 && _req->rb.maxLen != 0)
				{
					com->Read(&_req->rb, _req->preTimeOut, _req->postTimeOut); 

					_state = READ;
				}
				else
				{
					_state = CALLBACK_1;
				};
			};

			break;

		case READ:

			if (!com->Update())
			{
				if (_req->checkCRC && _req->rb.recieved)
				{
					_crc = 0xFFFF;
					_crcLen = _req->rb.len;
					_crcPtr = (byte*) _req->rb.data;

					_state = (_req->crcType == _req->CRC16) ? CHECK_CRC_2 : CHECK_CRC_1;
				}
				else
				{
					_req->crcOK = false;
					_state = CALLBACK_1;
				};
			};

			break;

		case CHECK_CRC_1:

			if (CRC_CCITT_DMA_Async(_crcPtr, _crcLen, _crc))
			{
				_state = CHECK_CRC_2;
			};

		case CHECK_CRC_2:

			if (_req->crcType == _req->CRC16)
			{
				u16 len = 105;

				if (_crcLen < len) len = _crcLen;

				_crcLen -= len;

				_crc = GetCRC16(_crcPtr, len, _crc, 0);

				_crcPtr += len;
			}
			else if (CRC_CCITT_DMA_CheckComplete(&_crc))
			{
				_crcLen = 0;	
			};

			if (_crcLen == 0)
			{
				_req->crcOK = _crc == 0;
				_state = CALLBACK_1;
			};

			break;

		case CALLBACK_1:

			if (_req->CallBack != 0)
			{
				_req->ready = _req->CallBack(_req);
			}
			else
			{
				_req->ready = true;
			};

		default:

			_state = WAIT;

			break;
	};
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // REQUESTQUERY_IMP_H__06_02_2024__16_46