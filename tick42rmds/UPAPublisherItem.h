#pragma once

#include "rmdsBridgeTypes.h"
#include "RMDSSubscriber.h"

using namespace std;

typedef struct PubFieldListClosure
{
	UPAPublisherItem_ptr_t publisher;
	RsslFieldList * fieldList;
	RsslEncodeIterator * itEncode;
} PubFieldListClosure_t;

// implements the UPA side of a mama publisher - encodes and sends the messages

class UPAPublisherItem
{
public:

	// static factory
	static UPAPublisherItem_ptr_t CreatePublisherItem(RsslChannel * chnl, RsslUInt32 streamId, string source, string symbol, RsslUInt32 serviceId, RMDSPublisherBase * publisher );

	virtual bool Initialise(UPAPublisherItem_ptr_t ptr,  RMDSPublisherBase * publisher );
	bool Shutdown()
	{
		sharedptr_.reset();
		return true;
	}

	virtual ~UPAPublisherItem();

	UPAPublisherItem_ptr_t Shared_ptr()
	{
		return sharedptr_;
	}

	std::string Symbol() const { return symbol_; }
	std::string Source() const { return source_; }

	void AddChannel(RsslChannel * chnl, RsslUInt32 streamId);

	// returns true if item should be letp open
	bool RemoveChannel(RsslChannel * chnl, RsslUInt32 streamId);

	static RsslRet SendItemRequestReject(RsslChannel* chnl, RsslInt32 streamId, RsslUInt8 domainType, RsslStateCodes code, std::string stateText,  RsslBool isPrivateStream);

	void ProcessRecapMessage( mamaMsg msg );
	void PublishMessage( mamaMsg msg );


	// iterate the mama message
	static void MAMACALLTYPE mamaMsgIteratorCb(const mamaMsg msg, const mamaMsgField  field, void* closure);
	// process field - called into from callback
	void EncodeField( 	RsslFieldList * fieldList, 	RsslEncodeIterator * itEncode, const mamaMsgField  field);


	RsslRealHints MamaPrecisionToRsslHint(mamaPricePrecision p,  uint16_t fid);

private:
	UPAPublisherItem(RsslChannel * chnl, RsslUInt32 streamId,  string source, string symbol, RsslUInt32 serviceId);

	UPAPublisherItem_ptr_t sharedptr_;

	// manage multiplex onto several channels
	// If we have multiple connections then we may get the same item connected on multiple channels

	// when we add a channel to the item then we require a refresh to be sent on that channel. So put channels on pendingRefreshist until there 
	// has been a refresh sent. Then move them onto the update list

	// we need to maintain a stream list per channel as there might be more than one request for the same thing on different streams

	typedef std::list<RsslInt32> StreamList_t;
	typedef struct
	{
		RsslChannel * channel_;
		StreamList_t updateStreamList_;
		StreamList_t refreshStreamList_;
	} UpaChannel_t;

	typedef std::map<RsslChannel*, UpaChannel_t *> ChannelMap_t;
	ChannelMap_t channelMap_;

	std::string symbol_;
	std::string source_;

	RsslUInt32 serviceId_;

	// build the outgoing rssl message
	bool BuildPublishMessage(UpaChannel_t * upaChnl, RsslBuffer* rsslMessageBuffer, mamaMsg msg);

	mamaDictionary mamaDictionary_;
	UpaMamaFieldMap_ptr_t upaFieldMap_;
	RsslDataDictionary * rmdsDictionary_;

	// flag used to set Rssl message flags. messages from interactive publisher are unsolicited
	bool solicitedMessages_;
};


typedef boost::shared_ptr<UPAPublisherItem> UPAPublisherItem_ptr_t;

