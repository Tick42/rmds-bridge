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
#include "stdafx.h"

#include <utils/environment.h>
#include <utils/filesystem.h>
#include <utils/mama/types.h>

#include "UPAAsMamaFieldType.h"
#include "ToMamaFieldType.h"
#include "RMDSFileSystem.h"
#include "UPAMamaFieldMap.h"
#include "UPAMamaCommonFields.h"
#include <utils/namespacedefines.h>

// todo make this externally configurable
size_t fieldMapSize = 0x10000;

using namespace std;


using namespace utils::mama;

UpaMamaFieldMapHandler_t::UpaMamaFieldMapHandler_t(std::string fieldMapPath,  mama_fid_t nonTranslatedFieldFid_Start, bool shouldPassNonTranslated, std::string mamaDictPath, UPADictionaryWrapper_ptr_t &spUPADictionaryHandler)
    : spUPADictionaryHandler_(spUPADictionaryHandler)
    , NonTranslatedFieldFid_CurrentValue_(nonTranslatedFieldFid_Start)
    , ShouldPassNonTranslated_(shouldPassNonTranslated)
    , fieldMapPath_(fieldMapPath)
    , mamaDictPath_(mamaDictPath)
        , builtCombinedDictionary_(false)
{
    fieldMapSize_ = fieldMapSize;
    offset_ = fieldMapSize / 2;
    CreateAll(fieldMapPath, mamaDictPath);
}
UpaMamaFieldMapHandler_t::UpaMamaFieldMapHandler_t(std::string fieldMapPath,  mama_fid_t nonTranslatedFieldFid_Start, bool shouldPassNonTranslated, std::string mamaDictPath)
    : spUPADictionaryHandler_(boost::make_shared<UPADictionaryWrapper>())
    , NonTranslatedFieldFid_CurrentValue_(nonTranslatedFieldFid_Start)
    , ShouldPassNonTranslated_(shouldPassNonTranslated)
    , fieldMapPath_(fieldMapPath)
    , mamaDictPath_(mamaDictPath)
        , builtCombinedDictionary_(false)
{
    CreateAll(fieldMapPath, mamaDictPath);
}

UpaMamaFieldMapHandler_t::UpaMamaFieldMapHandler_t( std::string fieldMapPath, bool shouldPassNonTranslated, std::string mamaDictPath, UPADictionaryWrapper_ptr_t &spUPADictionaryHandler)
    : spUPADictionaryHandler_(spUPADictionaryHandler)
    , NonTranslatedFieldFid_CurrentValue_(0)
    , ShouldPassNonTranslated_(shouldPassNonTranslated)
    , fieldMapPath_(fieldMapPath)
    , mamaDictPath_(mamaDictPath)
        , builtCombinedDictionary_(false)
{
    fieldMapSize_ = fieldMapSize;
    offset_ = fieldMapSize / 2;
    if (CreateMamaDictionary(mamaDictPath))
    {
        returned<mama_fid_t> result = mamaDictionary_.getMaxFid();
        if (result.status == MAMA_STATUS_OK)
            NonTranslatedFieldFid_CurrentValue_ = result.value+1;
        else
        {
            mama_log(MAMA_LOG_LEVEL_WARN,"UpaMamaFieldMapHandler_t: failed to get both max fid of mama dictionary from path [%s] and non-translated field fid start is not configured. In such case non-translated fields won't be sent to the client.", mamaDictPath.c_str());
            shouldPassNonTranslated = false;
        }
    }
    else
    {
        mama_log(MAMA_LOG_LEVEL_WARN,"UpaMamaFieldMapHandler_t: failed to get both max fid of mama dictionary from path [%s] and non-translated filed fid start is not configured. In such case non-translated fields won't be sent to the client.", mamaDictPath.c_str());
        shouldPassNonTranslated = false;
    }
    CreateFieldMap(fieldMapPath);
}

UpaMamaFieldMapHandler_t::UpaMamaFieldMapHandler_t( std::string fieldMapPath, bool shouldPassNonTranslated, std::string mamaDictPath)
    : spUPADictionaryHandler_(boost::make_shared<UPADictionaryWrapper>())
    , NonTranslatedFieldFid_CurrentValue_(0)
    , ShouldPassNonTranslated_(shouldPassNonTranslated)
    , fieldMapPath_(fieldMapPath)
    , mamaDictPath_(mamaDictPath)
    , builtCombinedDictionary_(false)
{
    if (CreateMamaDictionary(mamaDictPath))
    {
        returned<mama_fid_t> result = mamaDictionary_.getMaxFid();
        if (result.status == MAMA_STATUS_OK)
            NonTranslatedFieldFid_CurrentValue_ = result.value+1;
        else
        {
            mama_log(MAMA_LOG_LEVEL_WARN,"UpaMamaFieldMapHandler_t: failed to get both max fid of mama dictionary from path [%s] and non-translated field fid start is not configured. In such case non-translated fields won't be sent to the client.", mamaDictPath.c_str());
            shouldPassNonTranslated = false;
        }
    }
    else
    {
        mama_log(MAMA_LOG_LEVEL_WARN,"UpaMamaFieldMapHandler_t: failed to get both max fid of mama dictionary from path [%s] and non-translated filed fid start is not configured. In such case non-translated fields won't be sent to the client.", mamaDictPath.c_str());
        shouldPassNonTranslated = false;
    }
    CreateFieldMap(fieldMapPath);
}

UpaMamaFieldMapHandler_t::~UpaMamaFieldMapHandler_t(void)
{
}

/* bridge_to_mama_csv_fields_enum - columns offsets of columns in the fieldmap.csv file */
enum bridge_to_mama_csv_fields_enum
{
    bmcfe_remark =0,
    bmcfe_source_field_fid = 1,
    bmcfe_source_field_acronym,
    bmcfe_mama_field_name,
    bmcfe_mama_fid,
    bmcfe_mama_field_type,
    bmcfe_source_field_description,
    bmcfe_max_fields,
};

bool UpaMamaFieldMapHandler_t::LoadPredefinedUpaMamaFieldsMap( std::ifstream &in_dict_file )
{
    int line_offset=0;

    //Try to tokenize the fieldmap.csv file as long as it is opened and no other failure happened
    try {
        // nothing to tokenize
        if (!in_dict_file.is_open()) return false;

        typedef boost::tokenizer< boost::escaped_list_separator<char> > tokenizer_t;

        string line;
        const int number_of_fields = bmcfe_max_fields;

        // logged_items - items used later on for logging needs.
        struct {
            string                source_field_fid;
            string                source_field_acronym;
            string                mama_field_name;
            string                  mama_fid;
            string                 mama_field_type;
            void clear() {source_field_fid.clear(); source_field_acronym.clear(); mama_field_name.clear(); mama_fid.clear(); mama_field_type.clear(); }
        } logged_items;

        // tokenize the file line by line
        for (;std::getline(in_dict_file,line); ++line_offset)
        {
            // Init
            logged_items.clear();
            tokenizer_t tokens(line);

            string tok;

            int i=0;
            source_key_t key;
            MamaField_t mama_field;
            bool parsed_mama_fid=false;
            bool parsed_mama_field_name=false;

            bool should_skip_this_item =false;

            // Tokenize line
            for (tokenizer_t::const_iterator tok_cit = tokens.begin(); i < number_of_fields && tok_cit != tokens.end(); ++tok_cit)
            {
                tok = *tok_cit;
                boost::trim(tok);

                // skip any lines with a comment symbol
                if ((tok.size() && tok[0] == '#') || tok == "remark" && tok_cit == tokens.begin() )
                {
                    should_skip_this_item=true;
                    mama_log (MAMA_LOG_LEVEL_FINEST, "loadPredefinedUpaMamaFieldsMap skipped line %d", line_offset, tok.c_str());
                    break;
                }

                // Get the column to be worked on
                bridge_to_mama_csv_fields_enum field = bridge_to_mama_csv_fields_enum(i);

                switch(field)
                {
                default:
                    mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap Unknown field %d, for line %d", i, line_offset);
                case bmcfe_source_field_acronym:
                case bmcfe_source_field_description:
                    break;
                case bmcfe_remark:
                    // here the remarked line is due to a first hush symbol in the line as it APPEARS in the spreadsheet application (like Excel or Open Office Calc)
                    if ((tok.size() && tok[0] == '#') || tok == "remark" && tok_cit == tokens.begin() )
                    {
                        should_skip_this_item=true;
                        mama_log (MAMA_LOG_LEVEL_FINEST, "loadPredefinedUpaMamaFieldsMap skipped line %d", line_offset, tok.c_str());
                    }
                    break;
                case bmcfe_source_field_fid:
                    logged_items.source_field_fid = tok;
                    try {
                        int val = boost::lexical_cast<int>(tok);
                        key = val;
                    } catch( boost::bad_lexical_cast const& ) {
                        should_skip_this_item=true;
                        if (should_skip_this_item) mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap couldn't translate line %d, OpenMAMA FID [%s]", line_offset, tok.c_str());
                    }
                    break;
                case bmcfe_mama_field_name:
                    logged_items.mama_field_name = tok;
                    mama_field.mama_field_name = tok;
                    parsed_mama_field_name = true;
                    break;
                case bmcfe_mama_fid:
                    logged_items.mama_fid = tok;
                    mama_field.has_no_mama_fid = true;
                    if (!tok.empty())
                    {
                        try {
                            uint64_t possibleMaxVal = boost::lexical_cast<int>(tok);
                            int64_t possibleNegativeVal = boost::lexical_cast<int>(tok);

                            // The parsed value should follow OpenMAMA FID rules. 0 < FID <= 65536
                            if (possibleNegativeVal < 0)
                            {
                                should_skip_this_item=true;
                                mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap couldn't translate line %d, OpenMAMA FID [%s] can't be negative!", line_offset, tok.c_str());
                            }
                            else if (possibleMaxVal > USHRT_MAX)
                            {
                                should_skip_this_item=true;
                                mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap couldn't translate line %d, OpenMAMA FID [%s] can't exceed %u", line_offset, tok.c_str(), USHRT_MAX);
                            }

                            mama_fid_t val = boost::lexical_cast<int>(tok);
                            if (val == 0)
                            {
                                should_skip_this_item=true;
                                mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap couldn't translate line %d, OpenMAMA FID [%s] can't be Zero!", line_offset, tok.c_str());
                            }

                            if (!should_skip_this_item)
                            {
                                mama_field.mama_fid = val;
                                mama_field.has_no_mama_fid = false;
                                parsed_mama_fid = true;
                            }
                        } catch( boost::bad_lexical_cast const& ) {
                            should_skip_this_item=true;
                            mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap couldn't translate line %d, OpenMAMA FID [%s]", line_offset, tok.c_str());
                        }
                    }
                    else
                    {
                        // on empty fid, fall back and take from the mama-dictionary the fid that corresponds to the name
                        mama_field.mama_fid = 0;
                        mama_field.has_no_mama_fid = true;
                        parsed_mama_fid = true;
                    }
                    break;
                case bmcfe_mama_field_type:
                    logged_items.mama_field_type = tok;

                    mamaFieldType_ originalTarget;

                    // First, get the mamaFieldType_ value
                    should_skip_this_item = !ToMamaFieldType(tok, originalTarget);
                    mama_field.mama_field_type = (UpaAsMamaFieldType)originalTarget;

                    if (should_skip_this_item)
                    {
                        // Then, if the combination of the RMDS key and the original mamaFieldType_ value (referred as originalTarget) needs extra
                        // conversion work, a special enumeration will reflect that in ToUpaAsMamaFieldTypeByFid
                        should_skip_this_item = !ToUpaAsMamaFieldTypeByFid(key, originalTarget,  mama_field.mama_field_type);
                    }
                    if (should_skip_this_item) mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap couldn't translate line %d, OpenMAMA Field Type [%s]", line_offset, tok.c_str());
                    break;
                }

                if (should_skip_this_item)
                    break;
                //----------------
                ++i;
            }

            if (should_skip_this_item)
                continue;


            returned<mamaFieldDescriptorRef> result = mamaDictionary_.getFieldDescriptorByName(mama_field.mama_field_name.c_str());
            if (parsed_mama_fid && parsed_mama_field_name)
            {
                if (mama_field.has_no_mama_fid)
                {
                    if (result.status== MAMA_STATUS_OK && result.value.valid())
                    {
                        mama_field.mama_fid = result.value.getFid();
                        mama_log (MAMA_LOG_LEVEL_FINE, "loadPredefinedUpaMamaFieldsMap Got FID [%d] from MAMA Dictionary for field name [%s]", mama_field.mama_fid, mama_field.mama_field_name.c_str());
                    }
                    else
                    {
                        //if the mama fid is empty (in the fieldmap.csv) and there is no corresponding fid from the mama-dictionary, then take the default one for non-translated fields
                        ++NonTranslatedFieldFid_CurrentValue_;
                        mama_field.mama_fid = NonTranslatedFieldFid_CurrentValue_;
                        mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap Could not find FID from MAMA Dictionary for field name [%s]. FID is defaulted to [%d]", mama_field.mama_field_name.c_str(),mama_field.mama_fid);
                    }
                }
                else if (    result.status== MAMA_STATUS_OK
                            && result.value.valid()
                            && (result.value.getFid() != mama_field.mama_fid))
                {
                    mama_log (MAMA_LOG_LEVEL_FINE, "loadPredefinedUpaMamaFieldsMap Field [%s] in the field map file [%s] has FID [%d] but the MAMA dictionary file [%s] specifies FID [%d] - FID [%d] will be used.",
                        mama_field.mama_field_name.c_str(),
                        fieldMapPath_.c_str(),
                        mama_field.mama_fid,
                        mamaDictPath_.c_str(),
                        result.value.getFid(),
                        mama_field.mama_fid);
                    mamaFieldMapConflictingFields_.insert(mama_field.mama_field_name);
                }
            }

            // if everything is OK so far, then add field to the map
            uint32_t index = fid2Index(key);
            if (index >= map_.size())
            {
                t42log_warn("attempt to insert fid %d failed - out of range", key);
            }

            map_[index] = mama_field;

            // and add to the mama to rmds map too
            mama2rmdsMap_[mama_field.mama_fid] = key;

            mama_log (MAMA_LOG_LEVEL_FINEST, "loadPredefinedUpaMamaFieldsMap New field from line [%05d]: %s[%s] -> %s[%s] as %s", line_offset, logged_items.source_field_acronym.c_str(), logged_items.source_field_fid.c_str(), logged_items.mama_field_name.c_str(), logged_items.mama_fid.c_str(), logged_items.mama_field_type.c_str());
        }
    }
    catch(...){
        return false;
        mama_log (MAMA_LOG_LEVEL_WARN, "loadPredefinedUpaMamaFieldsMap couldn't translate line [%d]", line_offset);
    }
    return true;

}

bool UpaMamaFieldMapHandler_t::CreateFieldMap(std::string path)
{
    using namespace boost;
    using namespace utils::filesystem;
    using namespace utils;

    // initialise the vector
    offset_ = fieldMapSize / 2;

    map_.resize(fieldMapSize);

    // fill with empty values
    for(size_t index = 0; index < fieldMapSize ; index ++)
    {
        MamaField_t newValue;
        newValue.mama_fid = 0;
        newValue.mama_field_name = string("");
        newValue.mama_field_type = AS_MAMA_FIELD_TYPE_UNKNOWN;
        newValue.has_no_mama_fid = true;
        map_[index] = newValue;
    }

    // and initialise the mama to rmdslookup too
    mama2rmdsMap_.resize(fieldMapSize);
    for(size_t index = 0; index < fieldMapSize ; index ++)
    {
        mama2rmdsMap_[index] = 0;
    }


    bool result = false;
    std::string actual_path=path;

    if (has_relative_path(path))
    {
        std::string abs_path = absolute_path(path);
        if (path_exists(abs_path))
        {
            actual_path = abs_path;
        }
        else
        {
            std::string env_prefix = environment::getWombatPath();
            abs_path = complete_path(env_prefix, path);
            if (path_exists(abs_path))
            {
                actual_path = abs_path;
            }
        }
    }

    try {

        if (path_exists(actual_path))
        {
            std::ifstream in_dict_file(actual_path.c_str());
            if (result = LoadPredefinedUpaMamaFieldsMap(in_dict_file))
                t42log_info("Field mapping file successfully loaded from [%s].",actual_path.c_str());
            else
                t42log_warn("Failed to load field mapping file from [%s].",actual_path.c_str());
        }
        else
        {
            if (ShouldPassNonTranslated_)
            {
                t42log_info("No field mapping file specified, fields will be mapped from the rmds dictionary\n");
            }
            else
            {
                t42log_warn("No field mapping file was specified\n");
            }

        }
    }
    catch(...)
    {
        mama_log (MAMA_LOG_LEVEL_WARN, "upaFieldDictionary: Could not load Predefined Fields into Dictionary from file [%s].",actual_path.c_str());
        // Since We're not sure about the state of the map, clean the map and fall back to use the original fields names/types
    }
    return result;
}

bool UpaMamaFieldMapHandler_t::CreateMamaDictionary(std::string path)
{
    using namespace utils::mama;
    string actualPath= GetActualPath(path);
    if (!actualPath.empty())
    {
        mamaDictionary_ = mamaDictionaryWrapper(actualPath);
        return true;
    }
    else
    {
        // no file. just put the reserved fields in
        mamaDictionary_ = mamaDictionaryWrapper();

        if ( !AddReservedFields(mamaDictionary_ ))
        {
            t42log_warn("Errors found adding reserved field to mama dictionary\n");
        }
        else
        {
            t42log_info("Added %d reserved fields to mama dictionary\n", mamaDictionary_.getSize().value);
        }

        if (NonTranslatedFieldFid_CurrentValue_ <= mamaDictionary_.getMaxFid().value)
        {
            t42log_warn("The configured fid offset for mapping rmds fields (%d) is less than the maximum fid in the set of mama reserved fields (%d)\n This will result in one or more mama reserved field being over written\n",
                NonTranslatedFieldFid_CurrentValue_, mamaDictionary_.getMaxFid().value);
        }

    }

    return true;

}

bool UpaMamaFieldMapHandler_t::CreateAll( std::string fieldMapPath, std::string mamaDictPath )
{
    bool result = CreateMamaDictionary(mamaDictPath);

    if (NonTranslatedFieldFid_CurrentValue_ == 0)
    {
        // fid offset has not been specified
        NonTranslatedFieldFid_CurrentValue_ = mamaDictionary_.getMaxFid().value + 1;
    }
    return  CreateFieldMap(fieldMapPath) && result;
}


namespace /*annonymous*/ {

struct from_to_key
{
    RsslDataTypes from;
    mamaFieldType_ to;
    from_to_key(RsslDataTypes from, mamaFieldType_ to) : from(from), to(to) {}
    from_to_key() : from(RSSL_DT_UNKNOWN), to(MAMA_FIELD_TYPE_UNKNOWN) {}
    from_to_key(const from_to_key &rhs)
    {
        *this = rhs;
    }
    from_to_key &operator=(const from_to_key &rhs)
    {
        if (this != &rhs)
        {
            from = rhs.from;
            to = rhs.to;
        }
        return *this;
    }
    bool operator==(const from_to_key &rhs) const
    {
        if (this != &rhs)
        {
            if (from == rhs.from && to == rhs.to)
                return true;
            return false;
        }
        return true;
    }

    inline bool compare(const from_to_key &rhs) const
    {
        return *this == rhs;
    }
};

struct ihash : std::unary_function<from_to_key, std::size_t>
{
    std::size_t operator()(const from_to_key &x) const
    {
        return x.from + x.to * 0x100; //this is more than enough for both
    }
};

typedef utils::collection::unordered_map<from_to_key, UpaAsMamaFieldType, ihash> SpecialConversionMap_t;
static  SpecialConversionMap_t SpecialConversionMap;

static bool IsInitSpecialConversionMap = false;

static void InitializeSpecialConversionMap()
{
    // Maintain here all future combinations needed for UPA
    SpecialConversionMap.insert(std::make_pair(from_to_key(RSSL_DT_REAL,MAMA_FIELD_TYPE_F32),RSSL_DT_REAL_AS_MAMA_FIELD_TYPE_F32));
    SpecialConversionMap.insert(std::make_pair(from_to_key(RSSL_DT_ENUM,MAMA_FIELD_TYPE_STRING),RSSL_DT_ENUM_AS_MAMA_FIELD_TYPE_STRING));
    SpecialConversionMap.insert(std::make_pair(from_to_key(RSSL_DT_DATE,MAMA_FIELD_TYPE_TIME),RSSL_DT_DATE_AS_MAMA_FIELD_TYPE_TIME));
    SpecialConversionMap.insert(std::make_pair(from_to_key(RSSL_DT_TIME,MAMA_FIELD_TYPE_TIME),RSSL_DT_TIME_AS_MAMA_FIELD_TYPE_TIME));
    IsInitSpecialConversionMap = true;
}

} //namespace /*annonymous*/


bool UpaMamaFieldMapHandler_t::ToUpaAsMamaFieldTypeByFid(source_key_t key, const mamaFieldType_ originalTarget, UpaAsMamaFieldType &to)
{
    bool result = false;
    if (!IsInitSpecialConversionMap)
        InitializeSpecialConversionMap();

    if (spUPADictionaryHandler_ && spUPADictionaryHandler_->GetFieldsStatus().loaded)
    {
        const RsslDictionaryEntry *rsslDictionaryEntry = spUPADictionaryHandler_->GetDictionaryEntry(key);

        SpecialConversionMap_t::const_iterator cit = SpecialConversionMap.find(from_to_key((RsslDataTypes)rsslDictionaryEntry->rwfType,originalTarget));
        if (cit != SpecialConversionMap.end())
        {
            result = true;
            to = cit->second;
        }
        else
        {
            result = UpaToMamaFieldType(rsslDictionaryEntry->rwfType, rsslDictionaryEntry->fieldType, to);
        }
    }
    else
    {
        mama_log(MAMA_LOG_LEVEL_WARN, "UpaMamaFieldMapHandler_t::ToUpaAsMamaFieldTypeByFid() There was no dictionary to translate the key[%d] ", key);
    }
    return result;
}

UpaMamaFieldMapHandler_t::DictionaryMap_ptr_t UpaMamaFieldMapHandler_t::CombineDictionaries()
{
    // merge both dictionaries into one merged_dictionary and then create combined dictionary.


    DictionaryMap_ptr_t merged_dictionary(new DictionaryMap);
    DictionaryNameSet nameSet;

    // 2. traverse through all the mamaDictionary and put each item in the the merged_dictionary
    returned<size_t> resultSize = mamaDictionary_.getSize();
    if (resultSize.status == MAMA_STATUS_OK)
    {
        for(size_t i=0; i < resultSize.value; ++i)
        {
            returned<mamaFieldDescriptorRef> resultDesciptor = mamaDictionary_.getFieldDescriptorByIndex((unsigned short)i);
            // copy only fields with non conflicting fids with fieldmap
            if (    resultDesciptor.status == MAMA_STATUS_OK &&
                    mamaFieldMapConflictingFields_.find(resultDesciptor.value.getName()) == mamaFieldMapConflictingFields_.end())
                (*merged_dictionary)[resultDesciptor.value.getFid()] = dictionary_item(resultDesciptor.value.getName(), resultDesciptor.value.getType());
                nameSet.emplace(resultDesciptor.value.getName());
        }
    }

    // set if we are going to allow addition of reuters fields to the dictionary other than those in the field map
    if (ShouldPassNonTranslated_)
    {
        // add the content of the RMDS dictionary to the field map
        RsslDataDictionary * pRsslDict = spUPADictionaryHandler_->RsslDictionary();

        if (pRsslDict->numberOfEntries > 0)
        {
            int fid;
            for( fid = RSSL_MIN_FID -1; fid < RSSL_MAX_FID; fid ++)
            {
                // dont want to do anything when the fid goes through 0. Its not valid in the rssl world
                if (0 == fid)
                {
                    continue;;
                }

                RsslDictionaryEntry * pEntry = getDictionaryEntry(pRsslDict, fid);

                if (pEntry != 0)
                {
                    // if its not in the map then add it
                    int mapIndex = fid2Index(fid);
                    if (map_[mapIndex].has_no_mama_fid)
                    {
                        // this fid is not mapped yet, so add it to the map...
                        //
                        // put everything into a mama_fielsd structure
                        MamaField_t mamafld;
                        string fldName(pEntry->acronym.data, pEntry->acronym.length);
                        mamafld.mama_field_name = fldName;
                        UpaAsMamaFieldType targetType;
                        UpaToMamaFieldType(pEntry->rwfType, pEntry->fieldType, targetType);
                        mamafld.mama_field_type = targetType;
                        mamafld.mama_fid =  ++NonTranslatedFieldFid_CurrentValue_;
                        mamafld.has_no_mama_fid = false;

                        // and map it for both directions
                        map_[mapIndex] = mamafld;
                        mama2rmdsMap_[mamafld.mama_fid] = fid;
                    }

                }
            }
        }
        else
        {
            t42log_warn("Empty RMDS dictionary found when building field map\n");
        }


    }

    for(UpaMamaFieldsMap_t::const_iterator cit = map_.begin(); cit != map_.end(); ++cit)
    {
        if (!(*cit).has_no_mama_fid)
        {
            mamaFieldType type;
            if (ToMamaFieldType((*cit).mama_field_type, type))
            {
                // Check if this name is already in the merged and don't add if it is (probably a mama reserved field that is in RMDS dictionary)
                DictionaryNameSet::iterator it = nameSet.find((*cit).mama_field_name);
                if (it == nameSet.end())
                {
                    (*merged_dictionary)[(*cit).mama_fid] = dictionary_item((*cit).mama_field_name, type);
                }
            }
        }
        else
        {
            mamaFieldType type;
            // if theres no field name for this mapa element then no point in looking in the dictionary for it
            if ((*cit).mama_field_name.size() > 0)
            {
                if (ToMamaFieldType((*cit).mama_field_type, type))
                {
                    returned<mamaFieldDescriptorRef> resultDesciptor = mamaDictionary_.getFieldDescriptorByName((*cit).mama_field_name);
                    if (resultDesciptor.status == MAMA_STATUS_OK && resultDesciptor.value.valid())
                    {
                        // Check if this name is already in the merged and don't add if it is (probably a mama reserved field that is in RMDS dictionary)
                        DictionaryNameSet::const_iterator itName = nameSet.find((*cit).mama_field_name);
                        if (itName == nameSet.end())
                        {
                            (*merged_dictionary)[resultDesciptor.value.getFid()] = dictionary_item((*cit).mama_field_name, type);
                        }
                    }
                }
            }
        }
    }

    return merged_dictionary;
}

utils::mama::mamaDictionaryWrapper UpaMamaFieldMapHandler_t::GetCombinedMamaDictionary()
{
    if (!builtCombinedDictionary_)
    {
        // then need to build the combined dictionary
        mamaDictionaryCombined_ = utils::mama::mamaDictionaryWrapper();

        UpaMamaFieldMapHandler_t::DictionaryMap_ptr_t merged_dictionary = CombineDictionaries();

        for (DictionaryMap::const_iterator cit = merged_dictionary->cbegin(); cit != merged_dictionary->cend(); ++cit)
        {
            mamaDictionaryCombined_.createFieldDescriptor(cit->first, cit->second.name, cit->second.type);
        }

        builtCombinedDictionary_ = true;

    }

    return mamaDictionaryCombined_;
}

/**
 * Linear search of the mama fields to get a fid from a name
 */
mama_fid_t UpaMamaFieldMapHandler_t::GetMamaFid(string mamaFieldName)
{
    size_t len = map_.size();
    for (size_t i = 0; i < len; ++i)
    {
        MamaField_t v = map_[i];
        if (mamaFieldName == v.mama_field_name)
        {
            return v.mama_fid;
        }
    }
    return (mama_fid_t) 0;
}

bool UpaMamaFieldMapHandler_t::AddReservedFields( mamaDictionaryWrapper dict )
{
    // just add the reserved types ... it would be nice if there was an iterator
    bool foundErrors = false;
    const CommonFields &commonFields = UpaMamaCommonFields::CommonFields();
    const BookFields &bookFields = UpaMamaCommonFields::BookFields();

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldMsgType->mFid, MamaReservedFieldMsgType->mName, MamaReservedFieldMsgType->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldMsgType->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldMsgStatus->mFid, MamaReservedFieldMsgStatus->mName, MamaReservedFieldMsgStatus->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldMsgStatus->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldMsgNum->mFid, MamaReservedFieldMsgNum->mName, MamaReservedFieldMsgNum->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldMsgNum->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldMsgTotal->mFid, MamaReservedFieldMsgTotal->mName, MamaReservedFieldMsgTotal->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldMsgTotal->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldSeqNum->mFid, MamaReservedFieldSeqNum->mName, MamaReservedFieldSeqNum->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldSeqNum->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldFeedName->mFid, MamaReservedFieldFeedName->mName, MamaReservedFieldFeedName->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldFeedName->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldFeedHost->mFid, MamaReservedFieldFeedHost->mName, MamaReservedFieldFeedHost->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldFeedHost->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldFeedGroup->mFid, MamaReservedFieldFeedGroup->mName, MamaReservedFieldFeedGroup->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldFeedGroup->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldItemSeqNum->mFid, MamaReservedFieldItemSeqNum->mName, MamaReservedFieldItemSeqNum->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldItemSeqNum->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldSendTime->mFid, MamaReservedFieldSendTime->mName, MamaReservedFieldSendTime->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldSendTime->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldAppDataType->mFid, MamaReservedFieldAppDataType->mName, MamaReservedFieldAppDataType->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldAppDataType->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldAppMsgType->mFid, MamaReservedFieldAppMsgType->mName, MamaReservedFieldAppMsgType->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldAppMsgType->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldSenderId->mFid, MamaReservedFieldSenderId->mName, MamaReservedFieldSenderId->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldSenderId->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldMsgQual->mFid, MamaReservedFieldMsgQual->mName, MamaReservedFieldMsgQual->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldMsgQual->mName);
        foundErrors = true;
    }


    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldConflateQuoteCount->mFid, MamaReservedFieldConflateQuoteCount->mName, MamaReservedFieldConflateQuoteCount->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldConflateQuoteCount->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldEntitleCode->mFid, MamaReservedFieldEntitleCode->mName, MamaReservedFieldEntitleCode->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldEntitleCode->mName);
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), MamaReservedFieldSymbolList->mFid, MamaReservedFieldSymbolList->mName, MamaReservedFieldSymbolList->mType, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n", MamaReservedFieldSymbolList->mName);
        foundErrors = true;
    }

    // mamda common fields

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), commonFields.wSymbol.mama_fid, commonFields.wSymbol.mama_field_name.c_str(), (mamaFieldType)commonFields.wSymbol.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wSymbol");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), commonFields.wIssueSymbol.mama_fid, commonFields.wIssueSymbol.mama_field_name.c_str(), (mamaFieldType)commonFields.wIssueSymbol.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wIssueSymbol");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), commonFields.wIndexSymbol.mama_fid, commonFields.wIndexSymbol.mama_field_name.c_str(), (mamaFieldType)commonFields.wIndexSymbol.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wIndexSymbol");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), commonFields.wPartId.mama_fid, commonFields.wPartId.mama_field_name.c_str(), (mamaFieldType)commonFields.wPartId.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPartId");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), commonFields.wSeqNum.mama_fid, commonFields.wSeqNum.mama_field_name.c_str(), (mamaFieldType)commonFields.wSeqNum.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wSeqNum");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), commonFields.wSrcTime.mama_fid, commonFields.wSrcTime.mama_field_name.c_str(),  (mamaFieldType)commonFields.wSrcTime.mama_field_type, 0))    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wSrcTime");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),  commonFields.wLineTime.mama_fid, commonFields.wLineTime.mama_field_name.c_str(),  (mamaFieldType)commonFields.wLineTime.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wLineTime");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),  commonFields.wActivityTime.mama_fid, commonFields.wActivityTime.mama_field_name.c_str(), (mamaFieldType)commonFields.wActivityTime.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wActivityTime");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), commonFields.wPubId.mama_fid, commonFields.wPubId.mama_field_name.c_str(),  (mamaFieldType)commonFields.wPubId.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPubId");
        foundErrors = true;
    }

    // Mamda book fields

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wBookTime.mama_fid, bookFields.wBookTime.mama_field_name.c_str(),  (mamaFieldType)bookFields.wBookTime.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wBookTime");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),bookFields.wNumLevels.mama_fid, bookFields.wNumLevels.mama_field_name.c_str(), (mamaFieldType)bookFields.wNumLevels.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wNumLevels");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wPriceLevels.mama_fid, bookFields.wPriceLevels.mama_field_name.c_str(), (mamaFieldType)bookFields.wPriceLevels.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPriceLevels");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wPlAction.mama_fid, bookFields.wPlAction.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlAction.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlAction");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wPlPrice.mama_fid, bookFields.wPlPrice.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlPrice.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlPrice");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wPlSide.mama_fid, bookFields.wPlSide.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlSide.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlSide");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wPlSize.mama_fid, bookFields.wPlSize.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlSize.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlSize");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wPlSizeChange.mama_fid, bookFields.wPlSizeChange.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlSizeChange.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlSizeChange");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wPlTime.mama_fid, bookFields.wPlTime.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlTime.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlTime");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),bookFields.wPlNumEntries.mama_fid, bookFields.wPlNumEntries.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlNumEntries.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlNumEntries");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),  bookFields.wPlNumAttach.mama_fid, bookFields.wPlNumAttach.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlNumAttach.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlNumAttach");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),  bookFields.wPlEntries.mama_fid, bookFields.wPlEntries.mama_field_name.c_str(), (mamaFieldType)bookFields.wPlEntries.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wPlEntries");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wEntryId.mama_fid, bookFields.wEntryId.mama_field_name.c_str(), (mamaFieldType)bookFields.wEntryId.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wEntryId");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wEntryAction.mama_fid, bookFields.wEntryAction.mama_field_name.c_str(),  (mamaFieldType)bookFields.wEntryAction.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wEntryAction");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),bookFields.wEntryReason.mama_fid, bookFields.wEntryReason.mama_field_name.c_str(), (mamaFieldType)bookFields.wEntryReason.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wEntryReason");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wEntrySize.mama_fid, bookFields.wEntrySize.mama_field_name.c_str(), (mamaFieldType)bookFields.wEntrySize.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wEntrySize");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wEntryTime.mama_fid, bookFields.wEntryTime.mama_field_name.c_str(), (mamaFieldType)bookFields.wEntryTime.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wEntryTime");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(), bookFields.wEntryStatus.mama_fid, bookFields.wEntryStatus.mama_field_name.c_str(), (mamaFieldType)bookFields.wEntryStatus.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wEntryStatus");
        foundErrors = true;
    }

    if (MAMA_STATUS_OK != mamaDictionary_createFieldDescriptor(dict.get(),bookFields.wBookType.mama_fid, bookFields.wBookType.mama_field_name.c_str(), (mamaFieldType)bookFields.wBookType.mama_field_type, 0))
    {
        t42log_warn("Failed to add reserved field %s to mama dictionary\n","wBookType");
        foundErrors = true;
    }

    return !foundErrors;
}
