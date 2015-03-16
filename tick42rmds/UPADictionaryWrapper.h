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
#ifndef __UPADICTIONARYHANDLER_H__ 
#define __UPADICTIONARYHANDLER_H__ 

/* RMDS Dictionary */
static const char DictionaryFileName[] = "RDMFieldDictionary";

/* dictionary download name */
static const char DictionaryDownloadName[] = "RWFFld";

/* enum table file name */
static const char EnumTableFileName[] = "enumtype.def";

/* enum table download name */
static const char EnumTableDownloadName[] = "RWFEnum";

/*
 *  load_status_t consists of two flags:
 *  loaded and loaded_from_file
 *  Used as the result of load-functions in the class UPADictionaryWrapper, either dictionary entries or enums
 */
struct load_status_t : boost::equality_comparable<load_status_t>
{
	bool loaded;
	bool loaded_from_file;
	load_status_t() : loaded(false), loaded_from_file(false) {}
	
	load_status_t &operator=(const load_status_t &rhs) 
	{
		if (this!=&rhs)
		{
			loaded = rhs.loaded;
			loaded_from_file = rhs.loaded_from_file;
		}
		return *this;
	}
	load_status_t (const load_status_t &rhs) {*this = rhs;}
	bool operator==(const load_status_t &rhs) const
	{ 
		return (loaded == rhs.loaded) 
			&& (loaded_from_file == rhs.loaded_from_file);
	}
};


// wrapper around the RMDS dictionary

class UPADictionaryWrapper
{
public:
	UPADictionaryWrapper();
	~UPADictionaryWrapper();

	// operations
	load_status_t LoadFieldDictionary(std::string filename);
	load_status_t LoadFieldDictionary();
	load_status_t LoadEnumTypeDictionary(std::string filename);
	load_status_t LoadEnumTypeDictionary();
	inline bool LoadFullDictionary(std::string fieldDictionary, std::string enumTypeDictionary) 
	{
		LoadFieldDictionary(fieldDictionary);
		LoadEnumTypeDictionary(enumTypeDictionary);
		return isComplete();
	}
	inline bool LoadFullDictionary() 
	{
		LoadFieldDictionary();
		LoadEnumTypeDictionary();
		return isComplete();
	}
	bool clear();


	// update state
	void SetFieldsLoaded(bool val)
	{
		fieldDictionaryStatus_.loaded = val;
	}

	void SetEnumsLoaded( bool val)
	{
		enumTypeDictionaryStatus_.loaded = val;
	}


	// access
	RsslDataDictionary * RsslDictionary()
	{
		return &dictionary_;
	}

	// info
	inline std::string GetLastErrorText() const {return std::string(lastErrTxt_);}
	inline const RsslDictionaryEntry *GetDictionaryEntry(RsslFieldId fieldId) const {return dictionary_.entriesArray[fieldId];}

	inline RsslRet DumpDataDictionary(FILE *fileptr) const {return rsslPrintDataDictionary(fileptr, const_cast<RsslDataDictionary*>(&dictionary_));}
	inline RsslRet PrintDataDictionary() const {return DumpDataDictionary(stdout);}
	inline const RsslDataDictionary & GetRawDictionary() const {return dictionary_;}
	inline RsslDataDictionary & GetRawDictionary() {return dictionary_;}

	// predicates
	load_status_t GetFieldsStatus() const { return fieldDictionaryStatus_;}
	load_status_t GetEnumTypesStatus() const { return enumTypeDictionaryStatus_;}
	bool isInitialized() const {return dictionary_.isInitialized == RSSL_TRUE;}
	bool isComplete() const { return (dictionary_.isInitialized == RSSL_TRUE) && fieldDictionaryStatus_.loaded && enumTypeDictionaryStatus_.loaded;}

private:
	RsslDataDictionary dictionary_; /* data dictionary */
	load_status_t fieldDictionaryStatus_; /* is dictionary loaded/loaded from file */
	load_status_t enumTypeDictionaryStatus_; /* is enum table loaded/loaded from file  */

	char *lastErrTxt_;//[256]; /* last error row buffer */
	RsslBuffer lastErrorTextBuffer_; /* last error size and row pointer to its buffer */

	// class is non-copyable, use shared_ptr for reference copy 
	UPADictionaryWrapper &operator=(const UPADictionaryWrapper &rhs);
	UPADictionaryWrapper(const UPADictionaryWrapper &rhs);
};

#endif //__UPADICTIONARYHANDLER_H__ 