#pragma once

#include "UPABridgePoster.h"
#include "rmdsBridgeTypes.h"

//const int MaxCapabilities = 10;

const size_t MaxSourceCapabilities = 10;

// wrap up the source properties
class RMDSPublisherSource
{
public:
    RMDSPublisherSource(const std::string& name);

    bool InitialiseSource();

    RsslUInt32 ServiceId() const { return serviceId_; }
    void ServiceId(RsslUInt32 val) { serviceId_ = val; }

    const std::string& Name() const {return name_;}

    RsslUInt64 GetCapability(int index) const
    {
        return capabilities_[index];
    }

    size_t GetMaxCapabilities() const
    {
        return MaxSourceCapabilities;
    }

    RsslState Status() const { return status_; }
    void Status(RsslState val) { status_ = val; }
    RsslUInt64 AcceptingRequests() const { return acceptingRequests_; }
    void AcceptingRequests(RsslUInt64 val) { acceptingRequests_ = val; }
    RsslUInt64 ServiceState() const { return serviceState_; }
    void ServiceState(RsslUInt64 val) { serviceState_ = val; }

    RsslUInt64 OpenLimit() const { return openLimit_; }
    void OpenLimit(RsslUInt64 val) { openLimit_ = val; }

    RsslUInt64 OpenWindow() const { return openWindow_; }
    void OpenWindow(RsslUInt64 val) { openWindow_ = val; }

    RsslUInt64 LoadFactor() const { return loadFactor_; }
    void LoadFactor(RsslUInt64 val) { loadFactor_ = val; }

    std::string LinkName() const {return linkName_;}
    void LinkName(const std::string & val) {linkName_ = val;}

    RsslUInt64 LinkType() const {return linkType_;}
    void LinkType(RsslUInt64 val) {linkType_ = val;}

    RsslUInt64 LinkState() const {return linkState_;}
    void LinkState(RsslUInt64 val) {linkState_ = val;}

    RsslUInt64 LinkCode() const {return linkCode_;}
    void LinkCode(RsslUInt64 val) {linkCode_ = val;}

    const std::string& LinkText() const {return linkText_;}
    void LinkText(const std::string & val) {linkText_ = val;}

    const std::string& FieldDictionaryName() const { return fieldDictionaryName_; }
    void FieldDictionaryName(std::string val) { fieldDictionaryName_ = val; }

    const std::string& EnumTypeDictionaryName() const { return enumTypeDictionaryName_; }
    void EnumTypeDictionaryName(std::string val) { enumTypeDictionaryName_ = val; }

    RsslQos Qos() const { return qos_; }
    void Qos(RsslQos val) { qos_ = val; }

    bool SupportOutOfBandSnapshots() const { return supportOutOfBandSnapshots_; }
    void SupportOutOfBandSnapshots(bool val) { supportOutOfBandSnapshots_ = val; }

    bool AcceptConsumerStatus() const { return acceptConsumerStatus_; }
    void AcceptConsumerStatus(bool val) { acceptConsumerStatus_ = val; }


    // add and remove items
    const UPAPublisherItem_ptr_t& GetItem(const std::string& symbol) const;

    const UPAPublisherItem_ptr_t& AddItem(RsslChannel* chnl, RsslUInt32 streamID,
                                          const std::string& source, const std::string& symbol,
                                          RsslUInt32 serviceId, RMDSPublisherBase* publisher,
                                          bool& isNew);

    void RemoveItem(const std::string& symbol);
    bool HasItem(const std::string& symbol);

private:
    RsslUInt32 serviceId_;
    std::string name_;
    RsslUInt64    capabilities_[MaxSourceCapabilities];

    // service state
    RsslUInt64    serviceState_;
    RsslUInt64    acceptingRequests_;
    RsslState    status_;

    // load info
    RsslUInt64    openLimit_;
    RsslUInt64    openWindow_;
    RsslUInt64    loadFactor_;

    // link info
    std::string linkName_;
    RsslUInt64    linkType_;
    RsslUInt64    linkState_;
    RsslUInt64    linkCode_;
    std::string linkText_;

    // dictionary info
    std::string fieldDictionaryName_;
    std::string enumTypeDictionaryName_;

    // QOS info
    RsslQos qos_;

    // out of band snapshots
    bool supportOutOfBandSnapshots_;

    // accept consumer status
    bool acceptConsumerStatus_;

    typedef utils::collection::unordered_map<std::string, UPAPublisherItem_ptr_t> PublisherItemMap_t;
    PublisherItemMap_t publisherItemMap_;
};

