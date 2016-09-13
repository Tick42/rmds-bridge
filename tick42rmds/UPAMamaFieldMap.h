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
#pragma once
#ifndef __UPAMAMAFIELDMAP_H__
#define __UPAMAMAFIELDMAP_H__

#include <utils/mama/mamaDictionaryWrapper.h>
#include <utils/t42log.h>

#include "UPAAsMamaFieldType.h"
#include "UPADictionaryWrapper.h"
#include "rmdsBridgeTypes.h"
#include <utils/namespacedefines.h>

typedef RsslFieldId source_key_t;



// map between RMDS fids and mama fids
// in each case  implement as a sparse vector as the lookup is required for every field in every update
//
// The map is built from an (optional) external field mapping file and from the RMDS dictionary.  The optional field mapping file will map specific RMDS fields to named mama fields and fids.
// The rest of the rmds fields are mapped with the same name but with an adjustment to the fid to avoid clashes with mama fids.
// So, for example the field mapping file could be used to map the rmds field BID to the mama field wBidPrice. If not mapping is specified then it will be mapped to BID but with a fid
// greater than the largest used by the mama fields
//
// The field mapping also deals with RMDS fids being signed and mama fids unsigned.
//
// The translation is bidirectional

// rmds to mama is used by the subscriber
typedef std::vector<MamaField_t> UpaMamaFieldsMap_t;

// mama to rmds is used by the publisher
typedef std::vector<RsslInt32> MamaUPAFieldsMap_t;

class UpaMamaFieldMapHandler_t;
typedef boost::shared_ptr<UpaMamaFieldMapHandler_t> UpaMamaFieldMap_ptr_t;


class UpaMamaFieldMapHandler_t
{

public:

    /**
     * @brief Constructor
     *
     * @param fieldMapPath: The path to fieldmap.csv file
     * @param nonTranslatedFieldFid_Start: the first FID to be sent to the client on the first non-translated field (a field that is missing from the fieldmap.csv). this one is incremented on each given non-translated field
     * @param shouldPassNonTranslated: says whether non-translated field should be passed on to the client or not.
     * @param [optional]spUPADictionaryHandler: an RMDS dictionary handler. That is the dictionary used in special cases where special conversion is needed between the source RMDS type to an OpenMAMA type.
     */
    UpaMamaFieldMapHandler_t(std::string fieldMapPath,  mama_fid_t nonTranslatedFieldFid_Start, bool shouldPassNonTranslated, std::string mamaDictPath, UPADictionaryWrapper_ptr_t &spUPADictionaryHandler);
    UpaMamaFieldMapHandler_t(std::string fieldMapPath,  mama_fid_t nonTranslatedFieldFid_Start, bool shouldPassNonTranslated, std::string mamaDictPath);
    /**
     * @brief Constructor
     * This constructor is used when the fid for non-translated fields is not set - which means in turn that the max-fid from the underlaying mama-dictionary will be used
     * @param fieldMapPath: The path to fieldmap.csv file
     * @param shouldPassNonTranslated: says whether non-translated field should be passed on to the client or not.
     * @param [optional]spUPADictionaryHandler: an RMDS dictionary handler. That is the dictionary used in special cases where special conversion is needed between the source RMDS type to an OpenMAMA type.
     */
    UpaMamaFieldMapHandler_t(std::string fieldMapPath,  bool shouldPassNonTranslated, std::string mamaDictPath, UPADictionaryWrapper_ptr_t &spUPADictionaryHandler);
    UpaMamaFieldMapHandler_t(std::string fieldMapPath,  bool shouldPassNonTranslated, std::string mamaDictPath);
    ~UpaMamaFieldMapHandler_t();

    /**
     * @brief set an RMDS dictionary handler. That is the dictionary used in special cases where special conversion is needed between the source RMDS type to an OpenMAMA type.
     *
     * @param spUPADictionaryHandler: The RMDS dictionary object.
     * @return: true on success
     */
    inline bool SetUPADictionaryHandler(UPADictionaryWrapper_ptr_t &spUPADictionaryHandler)
    {
        spUPADictionaryHandler_=spUPADictionaryHandler;
        return spUPADictionaryHandler_ ? true : false;
    }

    /**
     * @brief set an RMDS dictionary handler. That is the dictionary used in special cases where special conversion is needed between the source RMDS type to an OpenMAMA type.
     * Works only with translated types!
     *
     * @param key: key input of RMDS FID
     * @param value: value with information to what type and fid to translate.
     * @return: true on success
     */
    inline bool FindField(const RsslFieldId &fid, MamaField_t &value) const
    {
        uint32_t index = fid2Index(fid);
        if (index >= map_.size())
        {
            t42log_warn("attempt to look up fid %d failed - out of range", fid);
            return false;
        }
        if (map_[index].mama_fid    == 0)
        {
            return false;
        }

        value = map_[index];
        return true;
    }
    /**
     * @brief Gives translation information (client FID, client field name and client type) for a given FID that comes from the RMDS source
     * Will try to translate also non-translated fields.
     * @param key: key input of RMDS FID
     * @param value: value with information to what type and fid to translate.
     * @return: true on success
     */
    inline bool GetTranslatedField(source_key_t key, MamaField_t &value)
    {
        if (FindField(key, value))
        {
            return true;
        }
        else if (ShouldPassNonTranslated_)
        {
            uint32_t index = fid2Index(key);
            if (index >= map_.size())
            {
                t42log_warn("attempt to look up fid %d failed - out of range", key);
                return false;
            }
            MamaField_t newValue;
            newValue.mama_fid = ++NonTranslatedFieldFid_CurrentValue_;
            const RsslDictionaryEntry *rsslDictionaryEntry = spUPADictionaryHandler_->GetDictionaryEntry(key);
         if ((rsslDictionaryEntry == NULL) || (rsslDictionaryEntry->acronym.data == NULL))
         {
            t42log_warn("attempt to look up fid %d failed - dictionary entry is NULL", key);
            return false;
         }
         newValue.mama_field_name = rsslDictionaryEntry->acronym.data;
            UpaAsMamaFieldType targetType;
            UpaToMamaFieldType(rsslDictionaryEntry->rwfType, rsslDictionaryEntry->fieldType, targetType);
            newValue.mama_field_type = targetType;
            map_[index] = newValue;
            value = newValue;

            // add to the mama to upa lookup
            // note that the mama fields start at 0 so no fid to index offset
            mama2rmdsMap_[newValue.mama_fid] = key;
            return true;
        }
        return false;
    }

    inline RsslInt32 GetRMDSFidFromMAMAFid(mama_fid_t mamaFid)
    {
      RsslInt32 mappedFid = (mama2rmdsMap_.size() > mamaFid) ? mama2rmdsMap_[mamaFid] : 0;
      if (0 == mappedFid)
      {
         static std::set<mama_fid_t> warned;
         if (warned.insert(mamaFid).second)
         {
            t42log_error("Mama fid %d has no equivalent in RMDS\n", mamaFid);
         }
      }
        return mappedFid;
    }

    /**
     * @brief Combines the two dictionaries, the MAMA Dictionary and the Fields map dictionary, This dictionary is later on used for publishing
     * @return: true on success
     */
    struct dictionary_item
    {
        std::string name;
        mamaFieldType type;
        dictionary_item(const std::string &name, mamaFieldType type) : name(name), type(type) {}
        dictionary_item() : name(), type() {}
        dictionary_item &operator=(const dictionary_item &rhs)
        {
            if ( this != &rhs)
            {
                name = rhs.name;
                type = rhs.type;
            }
            return *this;
        }
        dictionary_item(const dictionary_item &rhs) : name(rhs.name), type(rhs.type) {}
    };
    typedef utils::collection::unordered_map<mama_fid_t, dictionary_item> DictionaryMap;
    typedef utils::collection::unordered_set<std::string> DictionaryNameSet;
    typedef boost::shared_ptr<DictionaryMap> DictionaryMap_ptr_t;
    DictionaryMap_ptr_t CombineDictionaries();
    /*
     * @brief Creates a combined dictionary for later use. every call creates a new dictionary.
     * That is, every non-translated field is added too on the fly, So the every call create
     * a different combined dictionary every time.
     * @return: mamaDictionaryWrapper
     */
    utils::mama::mamaDictionaryWrapper GetCombinedMamaDictionary();

    mama_fid_t GetMamaFid(std::string mamaFieldName);

private:
    /**
     * @brief create the whole fields map. calls later on the parser loadPredefinedUpaMamaFieldsMap
     * @param path: The path to fieldmap.csv
     * @return: true on success
     */
    bool CreateFieldMap(std::string path);
    /**
     * @brief create mama dictionary from file
     * @param path: The path to mama_dict.txt (could be some other file name)
     * @return: true on success
     */
    bool CreateMamaDictionary(std::string path);
    /**
     * @brief create the fields map and the mama dictionary from the corresponding file paths
     * @param fieldMapPath: The path to fieldmap.csv
     * @param mamaDictPath: The path to mama_dict.txt (could be some other file name)
     * @return: true on success
     */
    bool CreateAll(std::string fieldMapPath, std::string mamaDictPath);
    /**
     * @brief the fieldmap.csv parser, responsible on creating the fields map from the file.
     * @param in_dict_file: the handle to the file fieldmap.csv
     * @return: true on success
     */
    bool LoadPredefinedUpaMamaFieldsMap(std::ifstream &in_dict_file);
    /**
     * @brief Takes a Reuters FID and target regular mamaFieldType_ type and yields UpaAsMamaFieldType. UpaAsMamaFieldType is a superset of mamaFieldType_ and holds special enumerator for special conversion between RMDS and OpenMAMA.
     *
     * @param key: Originating RMDS key.
     * @param originalTarget: The target from fieldmap.csv, where in that file there is only valid mamaFieldType_ values
     * @param to: The internal representation of the target type. If needed, will have a special enumerator that represent special types that exist only in UpaAsMamaFieldType, otherwise 'to' and 'originalTarget' are binary equivalent.
     * @return: true on success
     */
    bool ToUpaAsMamaFieldTypeByFid(source_key_t key, const mamaFieldType_ originalTarget, UpaAsMamaFieldType &to);
    // --- this class is observable only and non-copyable, if needed use a reference, a pointer or a smart pointer for it.
    UpaMamaFieldMapHandler_t(const UpaMamaFieldMapHandler_t&);
    UpaMamaFieldMapHandler_t &operator=(const UpaMamaFieldMapHandler_t&);


    bool AddReservedFields(utils::mama::mamaDictionaryWrapper dict);

    size_t offset_;
    size_t fieldMapSize_;

    uint32_t fid2Index(RsslFieldId fid) const
    {
        return (uint32_t)(fid + offset_);
    }

    uint32_t Index2fid(uint32_t index)
    {
        return (uint32_t)(index - offset_);
    }

    UPADictionaryWrapper_ptr_t spUPADictionaryHandler_;
    utils::mama::mamaDictionaryWrapper mamaDictionary_;
    // The combined version of mama dictionary + dictionary from the fieldmap.csv + non translated fields (implicitly)
    utils::mama::mamaDictionaryWrapper mamaDictionaryCombined_;

    bool builtCombinedDictionary_;
    mama_fid_t NonTranslatedFieldFid_CurrentValue_; //should be incremented each time a non translated field is added
    bool ShouldPassNonTranslated_;
    std::string fieldMapPath_;
    std::string mamaDictPath_;

    /*
     * This one holds all the fields whose names are the same but have different FIDs on both the MAMA dictionary (mamaDictionary_) and and the fields map (map_)
     */
    std::set<std::string> mamaFieldMapConflictingFields_;
    /**
     * The raw translation map_, maybe used in order to traverse all over it when the whole dictionary is needed
     */

    // this one maps rmds fids to mama
    UpaMamaFieldsMap_t map_;

    // and this maps mama fids to rmds
    MamaUPAFieldsMap_t mama2rmdsMap_ ;

};


#endif //__UPAMAMAFIELDMAP_H__
