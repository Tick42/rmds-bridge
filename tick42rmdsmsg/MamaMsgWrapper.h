#pragma once

#include <mama/msg.h>

#include <vector>
#include "upapayloadimpl.h"

//////////////////////////////////////////////////////////////////////////
//
class MamaMsgWrapper
{
public:
	MamaMsgWrapper(mamaMsg msg);
	~MamaMsgWrapper();

	mamaMsg getMamaMsg() const;

private:
	mamaMsg mamaMsg_;
};

//////////////////////////////////////////////////////////////////////////
//
class MamaMsgPayloadWrapper
{
public:

	MamaMsgPayloadWrapper(msgPayload msg);
	~MamaMsgPayloadWrapper();

	msgPayload getMamaMsgPayload() const;

private:
	msgPayload msgPayload_;
};

//////////////////////////////////////////////////////////////////////////
//
#if 0
class MamaMsgVectorWrapper
{
public:

	MamaMsgVectorWrapper(mamaMsg * vector, mama_size_t len)
		: msgVector_(vector), length_(len)
	{}

	~MamaMsgVectorWrapper()
	{
		for(int i = 0; i < (int)length_; i++)
		{
			mamaMsg_clear(msgVector_[i]);
			mamaMsg_destroy(msgVector_[i]);
		}

	}

	mamaMsg * getMamaMsgVector() const
	{
		return msgVector_;
	}

	mama_size_t getVectorLength() const
	{
		return length_;
	}

private:


	mamaMsg * msgVector_;
	mama_size_t length_;
};

#else

class MamaMsgVectorWrapper
{
public:

	MamaMsgVectorWrapper()
		: length_(0)
	{}

	~MamaMsgVectorWrapper();
	//{
	//	for(int i = 0; i < (int)length_; i++)
	//	{
	//		msgPayload p = payloadvector_[i];
	//		msgPayload_destroy(msgPayload(p)); 
	//	}

	//}

	//msgPayload * getMamaMsgVector() const
	//{
	//	return msgVector_;
	//}

	void addMessage(msgPayload payload)
	{
		payloadvector_.push_back(payload);
		++length_;
	}
	mama_size_t getVectorLength() const
	{
		return length_;
	}

	msgPayload * GetVector()
	{
		return payloadvector_.data();
	}
private:

	typedef std::vector<msgPayload> MsgPayloadVector_t;

	MsgPayloadVector_t payloadvector_;

	//msgPayload * msgVector_;
	mama_size_t length_;
};

#endif
