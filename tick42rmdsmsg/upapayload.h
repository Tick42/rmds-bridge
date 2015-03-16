/*
* Tick42RMDS: The Reuters RMDS Bridge for OpenMama
* Copyright (C) 2013-2015 Tick42 Ltd.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*Distributed under the Boost Software License, Version 1.0.
*    (See accompanying file LICENSE_1_0.txt or copy at
*         http://www.boost.org/LICENSE_1_0.txt)
*
*/
#ifndef UPA_PAYLOAD_H
#define UPA_PAYLOAD_H

#include <string>
#include <vector>
#include <map>
#include <sstream>

#include <unordered_map>
#include "upafieldpayload.h"
#include "upamsgutils.h"
#include <algorithm>
class UpaPayloadFieldIterator;

class UpaPayload
{
public:
    friend class UpaPayloadFieldIterator;
    //typedef std::map<mama_fid_t, UpaFieldPayload> FieldsMap_t;
    typedef std::unordered_map<mama_fid_t, UpaFieldPayload> FieldsMap_t;
    typedef FieldsMap_t::const_iterator FieldIterator_t;

    UpaPayload(mamaMsg parent) :
        parent_ (parent)
    {}

    UpaPayload() :
        parent_ (0), numDirtyFields_(0)
    {}

    UpaPayload(const UpaPayload& payload) :
        parent_ (payload.parent_),
        fields_ (payload.fields_)
    {}

    ~UpaPayload()
    {
    }

    UpaPayload& operator=(const UpaPayload& payload)
    {
        parent_ = payload.parent_;
        fields_ = payload.fields_;

        return *this;
    }

    mamaMsg getParent() const
    {
        return parent_;
    }

    void setParent(mamaMsg parent)
    {
        parent_ = parent;
    }

    void set(mama_fid_t fid, const std::string& name, const std::string& value)
    {
        fields_[fid] = UpaFieldPayload(fid, name, value);
    }

    void setPrice(mama_fid_t fid, const std::string& name, MamaPriceWrapper_ptr_t value)
    {
        //UpaFieldPayload fld = fields_[fid];

        FieldsMap_t::iterator it = fields_.find(fid);

        if (it != fields_.end())
        {
            UpaFieldPayload fld = it->second;	
            if (fld.type_ == MAMA_FIELD_TYPE_PRICE)
            {
                //fields_[fid] = UpaFieldPayload(fid, name, value);
                it->second =  UpaFieldPayload(fid, name, value);
            }
        }
        else
        {
            fields_.insert(FieldsMap_t::value_type(fid, UpaFieldPayload(fid, name, value)));
        }
    }

    void setMsg(mama_fid_t fid, const std::string& name, MamaMsgPayloadWrapper_ptr_t value)
    {
        //       UpaFieldPayload fld = fields_[fid];
        // 
        //       if (fld.type_ == MAMA_FIELD_TYPE_MSG)
        //       {
        //          fields_[fid] = UpaFieldPayload(fid, name, value);
        //       }
        FieldsMap_t::iterator it = fields_.find(fid);

        if (it != fields_.end())
        {
            UpaFieldPayload fld = it->second;	
            if (fld.type_ == MAMA_FIELD_TYPE_MSG)
            {
                //fields_[fid] = UpaFieldPayload(fid, name, value);
                it->second =  UpaFieldPayload(fid, name, value);
            }
        }
        else
        {
            fields_.insert(FieldsMap_t::value_type(fid, UpaFieldPayload(fid, name, value)));
        }
    }

    void setVectorMsg(mama_fid_t fid, const std::string& name, MamaMsgVectorWrapper_ptr_t value)
    {
        //UpaFieldPayload fld = fields_[fid];

        //if (fld.type_ == MAMA_FIELD_TYPE_VECTOR_MSG)
        //{
        //   fields_[fid] = UpaFieldPayload(fid, name, value);
        //}

        FieldsMap_t::iterator it = fields_.find(fid);

        if (it != fields_.end())
        {
            UpaFieldPayload fld = it->second;	
            if (fld.type_ == MAMA_FIELD_TYPE_VECTOR_MSG)
            {
                //fields_[fid] = UpaFieldPayload(fid, name, value);
                it->second =  UpaFieldPayload(fid, name, value);
            }
        }
        else
        {
            fields_.insert(FieldsMap_t::value_type(fid, UpaFieldPayload(fid, name, value)));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //
    void setVectorString(mama_fid_t fid, const std::string& name, const char *value[],
        size_t numElements)
    {
        fields_[fid] = UpaFieldPayload(fid, name, value, numElements);
    }

    //////////////////////////////////////////////////////////////////////////
    //
    template<typename T>
    void setVector(mama_fid_t fid, const std::string& name, const T value[],
        size_t numElements)
    {
        fields_[fid] = UpaFieldPayload(fid, name, value, numElements);
    }

    //////////////////////////////////////////////////////////////////////////
    //
    template <typename T>
    void set(mama_fid_t fid, const std::string& name, T value)
    {
        //fields_[fid] = UpaFieldPayload(fid, name, value);

        FieldsMap_t::iterator it = fields_.find(fid);
        if (it != fields_.end())
        {
            //fields_[fid] = UpaFieldPayload(fid, name, value);
            it->second =  UpaFieldPayload(fid, name, value);
        }
        else
        {
            fields_.insert(FieldsMap_t::value_type(fid, UpaFieldPayload(fid, name, value)));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //
    UpaPayload::FieldsMap_t::const_iterator cfindField(mama_fid_t fid, const char *name) const
    {
        // The non-const version of findField() doesn't modify the UpaPayload, so it's
        // safe to strip the const-ness here
        return const_cast<UpaPayload *>(this)->findField(fid, name);
    }

    UpaPayload::FieldsMap_t::iterator findField(mama_fid_t fid, const char *name)
    {
        // Prefer the fid, as that is much more efficient.
        FieldsMap_t::iterator itField;
        if (0 != fid)
        {
            itField = fields_.find(fid);
            if (itField != fields_.end())
            {
                return itField;
            }
        }
        if (name != 0)
        {
            // Searching for the field by name requires O(n) time.
            for (itField = fields_.begin(); itField != fields_.end(); ++itField)
            {
                if (itField->second.name_ == name)
                {
                    return itField;
                }
            }
        }
        return fields_.end();
    }

    mama_status get(mama_fid_t fid, const char *name, const char** value/*out*/) const 
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
        {
            return MAMA_STATUS_NOT_FOUND;
        }

        mama_status ret = MAMA_STATUS_OK;
        try 
        {
            *value = boost::get<std::string>(itField->second.data_).c_str();
        }
        catch (boost::bad_get&)
        {
            ret = MAMA_STATUS_WRONG_FIELD_TYPE;
        }

        return ret;
    }

    mama_status get(mama_fid_t fid, const char *name, int8_t & value/*out*/) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
        {
            return MAMA_STATUS_NOT_FOUND;
        }

        return itField->second.get(value);
    }

    mama_status get(mama_fid_t fid, const char *name, uint8_t & value/*out*/) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
            return MAMA_STATUS_NOT_FOUND;

        return itField->second.get(value);
    }

    mama_status get(mama_fid_t fid, const char *name, int16_t & value/*out*/) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
            return MAMA_STATUS_NOT_FOUND;

        return itField->second.get(value);
    }

    mama_status get(mama_fid_t fid, const char *name, uint16_t & value/*out*/) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
            return MAMA_STATUS_NOT_FOUND;

        return itField->second.get(value);
    }

    mama_status get(mama_fid_t fid, const char *name, int32_t & value/*out*/) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
            return MAMA_STATUS_NOT_FOUND;

        return itField->second.get(value);
    }

    mama_status get(mama_fid_t fid, const char *name, uint32_t & value/*out*/) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
            return MAMA_STATUS_NOT_FOUND;

        return itField->second.get(value);
    }

    mama_status get(mama_fid_t fid, const char *name, const char*** value, size_t* resultLen) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);

        if (itField == fields_.end())
        {
            return MAMA_STATUS_NOT_FOUND;
        }

        return (const_cast<UpaFieldPayload *>(&itField->second))->get(value, resultLen);
    }

    mama_status get(mama_fid_t fid, const char *name, const msgPayload** value, size_t* resultLen) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);

        if (itField == fields_.end())
        {
            return MAMA_STATUS_NOT_FOUND;
        }

        mama_status ret = MAMA_STATUS_OK;


        // todo - need to change the vector wrapper so that it is a vector of msgPayLoad not the payload wrapper class
        MamaMsgVectorWrapper_ptr_t msgVec = (MamaMsgVectorWrapper_ptr_t)boost::get<MamaMsgVectorWrapper_ptr_t>(itField->second.data_);
        *value = msgVec->GetVector();
        *resultLen = msgVec->getVectorLength();

        return ret;
    }

    // for others

    template <typename T>
    mama_status get(mama_fid_t fid, const char *name, T& value/*out*/) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
        {
            return MAMA_STATUS_NOT_FOUND;
        }

        return itField->second.get(value);
    }

    mama_status getAsString(mama_fid_t fid, const char *name, std::string& value) const
    {
        FieldsMap_t::const_iterator itField = cfindField(fid, name);
        if (itField == fields_.end())
            return MAMA_STATUS_NOT_FOUND;

        mama_status ret = MAMA_STATUS_OK;
        if (itField->second.type_ == MAMA_FIELD_TYPE_STRING)
        {
            try 
            {
                value = boost::get<std::string>(itField->second.data_);
            }
            catch (boost::bad_get&)
            {
                ret = MAMA_STATUS_WRONG_FIELD_TYPE;
            }
        }
        else if (itField->second.type_ == MAMA_FIELD_TYPE_TIME)
        {
            try 
            {
                uint64_t ms = boost::get<uint64_t>(itField->second.data_);
                value = epochTimeToString(ms);
            }
            catch (boost::bad_get&)
            {
                ret = MAMA_STATUS_WRONG_FIELD_TYPE;
            }
        }
        else
        {
            std::ostringstream oss;

            oss << itField->second.data_;
            value = oss.str();
        }

        return ret;
    }

    inline mama_status getField(mama_fid_t fid, const char *name, msgFieldPayload* result) const
    {
        FieldsMap_t::const_iterator found = cfindField(fid, name);
        if (found == fields_.end())
        {
            *result = 0;
            return MAMA_STATUS_NOT_FOUND;
        }

        *result = (msgFieldPayload)&found->second;

        return MAMA_STATUS_OK;
    }

    mama_status iterateFields(mamaMsgField field,
        mamaMsgIteratorCb cb, void *closure) const
    {
        // if its an image then want to send all the fields
        // get the message type
        //int32_t msgType;
        //try {
        //    get(1, 0, msgType);
        //} 
        //catch (boost::bad_get&)
        //{
        //    msgType = MAMA_STATUS_WRONG_FIELD_TYPE;
        //}


        //bool allFields = msgType == (MAMA_MSG_TYPE_INITIAL);

        for (FieldIterator_t fld = fields_.begin(); fld != fields_.end(); ++fld)
        {
            //if (allFields || fld->second.dirty_)
            //{
                mamaMsgFieldImpl_setPayload (field, (msgFieldPayload)&fld->second);
                (cb)(parent_, field, closure);
            //}

        }

        return MAMA_STATUS_OK;
    }

    void clear()
    {
        //FieldsMap_t::iterator fld;
        //for ( fld = fields_.begin(); fld != fields_.end(); ++fld)
        //{
        //    fld->second.dirty_ = false;
        //}

        //numDirtyFields_ = 0;
        fields_.clear();
    }

    //void markAllDirty()
    //{
    //    // mark all the fields as dirty 
    //    // this will force the iterator to behave as if the message is an image
    //    FieldsMap_t::iterator fld;
    //    for ( fld = fields_.begin(); fld != fields_.end(); ++fld)
    //    {
    //        fld->second.dirty_ = false;
    //    }

    //    numDirtyFields_ = fields_.size();
    //}

    size_t numFields() const
    {
        // with this implementation of the payload there is no way to track the number of dirty fields as a field value might me updated 
        // outside of the context of the payload - the best we can do is walk the collection and count the dirty fields

        //size_t numDirty = 0;

        //FieldsMap_t::const_iterator it = fields_.begin();
        //while(it != fields_.end())
        //{
        //    if (it->second.dirty_)
        //    {
        //        numDirty++;
        //    }
        //    ++it;

        //}

        //return numDirty;

              return fields_.size();
    }

    void apply(UpaPayload * src)
    {
        // merge src into this

        FieldsMap_t::iterator itSrc = src->fields_.begin();
        while ( itSrc != src->fields_.end())
        {
            UpaFieldPayload fieldSrc = itSrc->second;

            FieldsMap_t::iterator itDest = fields_.find(fieldSrc.fid_);
            if (itDest != fields_.end())
            {
                // just copy the field value
                fields_[fieldSrc.fid_] = fieldSrc;

            }
            else
            {
                fields_[fieldSrc.fid_] = UpaFieldPayload(fieldSrc);
            }

            ++itSrc;
        }
    }

private:
    mamaMsg parent_;

    size_t numDirtyFields_;
    FieldsMap_t fields_;
};


/** 
* @brief iterator wrapper implementation for the payload to support all APIs of tick42rmdsmsgPayloadIter_XXXXX
*/
class UpaPayloadFieldIterator
{
    const UpaPayload* payloadContext_; 
    UpaPayload::FieldsMap_t::const_iterator current_;

    // hold state for mama's notion of begin (which is infront of the boost/stl begin)
    bool mamaBegin_;
public:
    typedef UpaPayload::FieldsMap_t::const_iterator const_iterator;
    /** 
    * @brief Associate the iterator with another payload and reset position
    * @param payload another payload
    */
    UpaPayloadFieldIterator(const UpaPayload & payload) 
        : payloadContext_(&payload)
        , current_ (payload.fields_.begin())
        , mamaBegin_(true)
    {
    }
    /** 
    * @brief Copy constructor that basically associate with another payload from rhs and get rhs position
    * @param rhs - another iterator object. 
    */
    UpaPayloadFieldIterator(const UpaPayloadFieldIterator& rhs) 
        : payloadContext_(rhs.payloadContext_)
        , current_ ((rhs.payloadContext_)->fields_.begin())
    {
    }
    /** 
    * @brief Copy constructor that basically associate with another payload from rhs and get rhs position
    * @param rhs - another iterator object. 
    */
    UpaPayloadFieldIterator &operator=(const UpaPayloadFieldIterator& rhs)
    {
        if (this != &rhs)
        {
            payloadContext_ = rhs.payloadContext_;
            current_ = rhs.current_;
        }
        return *this;
    }
    /** 
    * @brief Associate the iterator with another payload and reset position
    * @param payload another payload
    */
    void associate(const UpaPayload& payload)
    {
        mamaBegin_ = true;
        payloadContext_ = &payload;
        current_ = payload.fields_.begin();
    }
    /** 
    * @brief next item on the payload
    * @return iterator 
    */
    const_iterator next()
    {
        if (current_==end())
            return end();

        //// mama iterator expects begin to point in front of the first item
        //// so if we are at the "start" in mama terms then just return the first item for next, if its dirty
        //// otherwise if its not dirty, fall through and find the first dirty item

        if (mamaBegin_)
        {
            mamaBegin_ = false;
            return beginInternal();
        }

        //const_iterator tmp = NextDirty();
        //return tmp;
		return ++current_;
    }
    /** 
    * @brief predicate: is current item has next item
    * @return true if next item on the payload exists
    */
    bool has_next() const
    {
        //UpaPayload::FieldsMap_t::const_iterator tmp = current_;
        //return ++tmp != payloadContext_->fields_.end();
        return InternalHasNext();
    }
    /** 
    * @brief beginning position of the payload
    * @return iterator
    */

    // take the const qualifier off this because we want to set the state that maps the begin point
    const_iterator begin() 
    {

        mamaBegin_ = true;
        return payloadContext_->fields_.begin();
    }

    const_iterator beginInternal()
    {
        return payloadContext_->fields_.begin();
    }

    /** THIS ONE IS MOST PROBABLY DEPRACATED AND SHOULD NOT BE USED
    * @brief used the same way as end() is used in STL
    * @return iterator of one position past the final position in the payload
    */
    const_iterator end() const
    {
        return payloadContext_->fields_.end();
    }
    /** 
    * @brief current position on the payload
    * @return iterator
    */
    const_iterator current() const
    {
        return current_;
    }
    const UpaPayload *payloadContext() const
    {
        return payloadContext_;
    }
private:

    //const_iterator NextDirty()
    //{
    //    // if its an image then want to send all the fields
    //    // get the message type
    //    int32_t msgType;
    //    try {
    //        payloadContext_->get(1, 0, msgType);
    //    } 
    //    catch (boost::bad_get&)
    //    {
    //        // if we fail the message type call then only return field with dirty set
    //        msgType = MAMA_MSG_TYPE_UPDATE;
    //    }


    //    bool allFields = msgType == (MAMA_MSG_TYPE_INITIAL);

    //    if (allFields)
    //    {
    //        current_++;
    //    }
    //    else
    //    {
    //        current_++;
    //        while(current_ != payloadContext_->fields_.end() && current_->second.dirty_ == false)
    //        {
    //            current_++;
    //        }
    //    }


    //    return current_;
    //}


    bool InternalHasNext() const
    {
        //// if its an image then want to send all the fields
        //// get the message type
        //int32_t msgType;
        //try {
        //    payloadContext_->get(1, 0, msgType);
        //} 
        //catch (boost::bad_get&)
        //{
        //    // if we fail the message type call then only return field with dirty set
        //    msgType = MAMA_MSG_TYPE_UPDATE;
        //}


        //bool allFields = msgType == (MAMA_MSG_TYPE_INITIAL);

        //bool gotNext = false;

        //if (allFields)
        //{
        //    //just check the next one
        //    UpaPayload::FieldsMap_t::const_iterator tmp = current_;
        //    gotNext = ( ++tmp != payloadContext_->fields_.end());
        //}
        //else
        //{
        //    //have to look for it
        //    UpaPayload::FieldsMap_t::const_iterator tmp = current_;
        //    while(++tmp != payloadContext_->fields_.end())
        //    {
        //        if (tmp->second.dirty_)
        //        {
        //            // found a dirty field
        //            gotNext = true;
        //            break;
        //        }
        //    }

        //}

		//just check the next one
		UpaPayload::FieldsMap_t::const_iterator tmp = current_;
		bool gotNext = ( ++tmp != payloadContext_->fields_.end());

        return gotNext;
    }
    UpaPayloadFieldIterator();
};

#endif
