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
#ifndef UPA_FIELD_PAYLOAD_H
#define UPA_FIELD_PAYLOAD_H

#include <string>
#include "utils/t42log.h"
#include "utils/time.h"

#include "upavaluetype.h"
#include "MamaPriceWrapper.h"
#include "MamaDateTimeWrapper.h"
#include "MamaMsgWrapper.h"
#include "MamaOpaqueWrapper.h"

typedef unsigned char  mama_price_hints_t;
typedef struct mama_price_t_
{
   double              mValue;
   mama_price_hints_t  mHints;
} mama_price_t;


template <typename T>
struct MamaFieldType
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_UNKNOWN;
};

template <>
struct MamaFieldType<bool>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_BOOL;
};

template <>
struct MamaFieldType<char>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_CHAR;
};


template <>
struct MamaFieldType<int8_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_I8;
};

template <>
struct MamaFieldType<uint8_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_U8;
};

template <>
struct MamaFieldType<int16_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_I16;
};

template <>
struct MamaFieldType<uint16_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_U16;
};


template <>
struct MamaFieldType<int32_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_I32;
};

template <>
struct MamaFieldType<uint32_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_U32;
};

template <>
struct MamaFieldType<int64_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_I64;
};

template <>
struct MamaFieldType<uint64_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_U64;
};

template <>
struct MamaFieldType<MamaDateTimeWrapper_ptr_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_TIME;
};

template <>
struct MamaFieldType<float>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_F32;
};

template <>
struct MamaFieldType<double>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_F64;
};

template <>
struct MamaFieldType<std::string>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_STRING;
};

template <>
struct MamaFieldType<MamaPriceWrapper_ptr_t>
{
   static const mamaFieldType Value = MAMA_FIELD_TYPE_PRICE;
};

template <>
struct MamaFieldType<MamaOpaqueWrapper_ptr_t>
{
    static const mamaFieldType Value = MAMA_FIELD_TYPE_OPAQUE;
};

/*
* @brief get type from mamaFieldType type tag
*/
template <mamaFieldType Tag> struct TypeFromTag
{
   typedef void* Type;
};

template<> struct TypeFromTag<MAMA_FIELD_TYPE_I8>           {typedef int8_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_U8>           {typedef uint8_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_I16>          {typedef int16_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_U16>          {typedef uint16_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_I32>          {typedef int32_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_U32>          {typedef uint32_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_I64>          {typedef int64_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_U64>          {typedef uint64_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_F32>          {typedef float Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_F64>          {typedef double Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_TIME>         {typedef MamaDateTimeWrapper_ptr_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_PRICE>        {typedef MamaPriceWrapper_ptr_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_STRING>       {typedef std::string Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_BOOL>         {typedef bool Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_CHAR>         {typedef char Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_VECTOR_MSG>   {typedef MamaMsgVectorWrapper_ptr_t Type;};
template<> struct TypeFromTag<MAMA_FIELD_TYPE_OPAQUE>       {typedef MamaOpaqueWrapper_ptr_t Type;};

struct UpaFieldPayload
{
   mama_fid_t fid_;
   const char* name_;
   mamaFieldType type_;
   ValueType_t data_;

   std::string dataVector1_;

   // Special case for vector strings, as we need to store std::string[] but return const char *[]
   boost::shared_ptr< std::vector<std::string> > stringVector1_;
   boost::shared_ptr< std::vector<const char *> > stringVector2_;
//   bool dirty_;

   UpaFieldPayload()
      : fid_(0)
      , name_ ()
      , type_ (MAMA_FIELD_TYPE_UNKNOWN)
      , data_ ()
      //, dirty_(false)
   {}

   UpaFieldPayload(const mama_fid_t fid, const char* name, const char* value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MAMA_FIELD_TYPE_STRING)
      , data_ (std::string(value))
      //, /*dirty_*/(true)
   {}

   //UpaFieldPayload(const mama_fid_t fid, const std::string &name, const mama_datetime & value)
   //    : fid_    (fid)
   //    , name_ (name)
   //    , type_ (MAMA_FIELD_TYPE_TIME)
   //    , data_ (value)
   //{}
   UpaFieldPayload(const mama_fid_t fid, const char* name, const MamaDateTimeWrapper_ptr_t& value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MAMA_FIELD_TYPE_TIME)
      , data_ (value)
     // , /*dirty_*/(true)
   {}

   UpaFieldPayload(const mama_fid_t fid, const char* name, const MamaPriceWrapper_ptr_t& value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MAMA_FIELD_TYPE_PRICE)
      , data_ (value)
     // , /*dirty_*/(true)
   {}

   UpaFieldPayload(const mama_fid_t fid, const char* name, const MamaMsgPayloadWrapper_ptr_t& value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MAMA_FIELD_TYPE_MSG)
      , data_ (value)
      //, /*dirty_*/(true)
   {}

   UpaFieldPayload(const mama_fid_t fid, const char* name, const MamaOpaqueWrapper_ptr_t& value)
       : fid_    (fid)
       , name_ (name)
       , type_ (MAMA_FIELD_TYPE_OPAQUE)
       , data_ (value)
       //, /*dirty_*/(true)
   {}

   UpaFieldPayload(const mama_fid_t fid, const char* name, const MamaMsgVectorWrapper_ptr_t& value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MAMA_FIELD_TYPE_VECTOR_MSG)
      , data_ (value)
     // , /*dirty_*/(true)
   {}

   UpaFieldPayload(const mama_fid_t fid, const char* name, const char *value[], size_t numElements)
      : fid_    (fid)
      , name_ (name)
      , type_ (MAMA_FIELD_TYPE_VECTOR_STRING)
      , data_()
      //, /*dirty_*/(true)
   {
      stringVector1_.reset(new std::vector<std::string>());
      stringVector1_->reserve(numElements);

      stringVector2_.reset(new std::vector<const char*>());
      stringVector2_->reserve(numElements);

      for (size_t ii = 0; ii < numElements; ++ii)
      {
         stringVector1_->emplace_back(value[ii]);
         stringVector2_->emplace_back(stringVector1_->back().c_str());
      }
   }

   template<typename T>
   UpaFieldPayload(const mama_fid_t fid, const char* name, const T value[], size_t numElements)
      : fid_    (fid)
      , name_ (name)
      , type_ (MamaFieldType<T>::Value)
      , data_()
     // , /*dirty_*/(true)
   {
      dataVector1_.resize(numElements * sizeof(T));
      memcpy(const_cast<char *>(dataVector1_.c_str()), value, numElements * sizeof(T));
   }

   template <typename T>
   UpaFieldPayload(const mama_fid_t fid, const char* name, T value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MamaFieldType<T>::Value)
      , data_ (value)
      //, /*dirty_*/(true)
   {}

   UpaFieldPayload(const mama_fid_t fid, const char* name, bool value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MamaFieldType<bool>::Value)
      , data_ (value)
      //, /*dirty_*/(true)
   {
   }

   UpaFieldPayload(const mama_fid_t fid, const char* name, char value)
      : fid_    (fid)
      , name_ (name)
      , type_ (MamaFieldType<char>::Value)
      , data_ (value)
     // , /*dirty_*/(true)
   {
   }

   //This macro is undef-ed later on
#define CHECK_UPA_FIELD_PAYLOAD \
   do { \
   if (this->type_ == MAMA_FIELD_TYPE_UNKNOWN) return MAMA_STATUS_INVALID_ARG; \
   } while(0)

   mama_status get(const char*& value/*out*/) const //fixme get rid of
   {
      mama_status ret = MAMA_STATUS_OK;
      CHECK_UPA_FIELD_PAYLOAD;

      try
      {
         value = boost::get<std::string>(data_).c_str();
      }
      catch (boost::bad_get&)
      {
         ret = MAMA_STATUS_WRONG_FIELD_TYPE;
      }

      return ret;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(int8_t & value) const
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (int8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (int8_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(uint8_t & value) const
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (uint8_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (uint8_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible. a special boolean conversion for mama_bool_t
   */
   mama_status getMamaBool(mama_bool_t & value) const
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_F64:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_TIME:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_TIME>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_PRICE:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_STRING:
            {
               // Currently properties_GetPropertyValueAsBoolean is the API that shows what OpenMAMA considers as boolean representation of string. so it is used for consistency.
               // mama_bool_t is int8_t which is physically compatible with char when it comes to boolean values
               std::string str = boost::get<TypeFromTag<MAMA_FIELD_TYPE_STRING>::Type>(data_);
               value = (mama_bool_t)properties_GetPropertyValueAsBoolean(str.c_str());
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_BOOL:
            value = mama_bool_t(boost::get<TypeFromTag<MAMA_FIELD_TYPE_BOOL>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_CHAR:
            value = (mama_bool_t)(boost::get<TypeFromTag<MAMA_FIELD_TYPE_CHAR>::Type>(data_) != 0);
            return MAMA_STATUS_OK;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(int16_t & value) const //fixme
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (int16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (int16_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(uint16_t & value) const //fixme
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (uint16_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (uint16_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(int32_t & value) const //fixme
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (int32_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
             value = (int32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_CHAR>::Type>(data_);
             return MAMA_STATUS_OK;

         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(uint32_t & value) const //fixme
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (uint32_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (uint32_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   * the int64_t value of time field is time in microseconds without TimeZone.
   */
   mama_status get(int64_t & value) const //fixme
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (int64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (int64_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(uint64_t & value) const //fixme
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (uint64_t)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            {
               mamaDateTime p = boost::get<MamaDateTimeWrapper_ptr_t>(data_)->getMamaDateTime();
               mamaDateTime_getEpochTimeMicroseconds(p, &value);
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (uint64_t)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }


   /**
   * Get the field value, converted if possible.
   */
   mama_status get(double & value) const //fixme #1 next see spreadsheet
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               mamaPrice_getValue(p, &value);
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   */
   mama_status get(float & value) const //fixme #1 next see spreadsheet
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            value = (float)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_);
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               double tmp;
               mamaPrice_getValue(p, &tmp);
               value = (float)tmp;
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }



   mama_status getChar(char & value) const
   {
       CHECK_UPA_FIELD_PAYLOAD;
       try {
           switch(type_)
           {
           case MAMA_FIELD_TYPE_I8:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_I16:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_I32:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_I64:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_U8:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_U16:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_U32:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_U64:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_);
               return MAMA_STATUS_OK;
           case MAMA_FIELD_TYPE_F32:
               return MAMA_STATUS_WRONG_FIELD_TYPE;
           case MAMA_FIELD_TYPE_F64:
               return MAMA_STATUS_WRONG_FIELD_TYPE;
           case MAMA_FIELD_TYPE_TIME:
               return MAMA_STATUS_WRONG_FIELD_TYPE;
           case MAMA_FIELD_TYPE_PRICE:
               return MAMA_STATUS_WRONG_FIELD_TYPE;
           case MAMA_FIELD_TYPE_STRING:
               return MAMA_STATUS_WRONG_FIELD_TYPE;
           case MAMA_FIELD_TYPE_BOOL:
               return MAMA_STATUS_WRONG_FIELD_TYPE;
           case MAMA_FIELD_TYPE_CHAR:
               value = (char)boost::get<TypeFromTag<MAMA_FIELD_TYPE_CHAR>::Type>(data_);
               return MAMA_STATUS_OK;
           default:
               return MAMA_STATUS_WRONG_FIELD_TYPE;
           }
       }
       catch (boost::bad_get&)
       {
           return MAMA_STATUS_WRONG_FIELD_TYPE;
       }
       return MAMA_STATUS_WRONG_FIELD_TYPE;
   }


   /**
   * Get the field value, converted if possible.
   */
   mama_status getPrice(mamaPrice value) const
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_I8:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I8>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I16:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I16>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I32:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I32>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_I64:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_I64>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U8:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U8>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U16:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U16>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U32:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U32>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_U64:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_U64>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F32:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F32>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
            mamaPrice_setValue(value, (double)boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_PRICE:
            {
               mamaPrice p = boost::get<MamaPriceWrapper_ptr_t>(data_)->getMamaPrice();
               mamaPrice_copy(value, p);
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   /**
   * Get the field value, converted if possible.
   * The mamaDateTime value from int64_t field is always in microseconds without TimeZone.
   */
   mama_status getDateTime(mamaDateTime value) const
   {
      CHECK_UPA_FIELD_PAYLOAD;
      try {
         switch(type_)
         {
         case MAMA_FIELD_TYPE_U64:
             //Time will be given in microseconds
            mamaDateTime_setEpochTimeMicroseconds(value, boost::get<uint64_t>(data_));
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_TIME:
            {
               mamaDateTime p = boost::get<MamaDateTimeWrapper_ptr_t>(data_)->getMamaDateTime();
               mamaDateTime_copy(value, p);
            }
            return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_F64:
             //Time will given in seconds!
             mamaDateTime_setEpochTimeF64(value, boost::get<TypeFromTag<MAMA_FIELD_TYPE_F64>::Type>(data_));
             return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_STRING:
             {
                 const char *asString = boost::get<std::string>(data_).c_str();
                 uint32_t year = atoi(&asString[0]);
                 uint32_t month = atoi(&asString[5]) - 1;
                 uint32_t day = atoi(&asString[8]) - 1;
                 uint32_t hour = atoi(&asString[11]);
                 uint32_t minute = atoi(&asString[14])
                 uint32_t second = atoi(&asString[17]);
                 uint32_t microsecond = atoi(&asString[20]);
                 if (year != 0 && month != 0 && day != 0)
                 {
                     mamaDateTime_setEpochTimeExt(value,
                                                  utils::time::GetSeconds(year, month, day),
                                                  0);
                     mamaDateTime_setHints(value, MAMA_DATE_TIME_HAS_DATE);
                 }
                 if (hour != 0 && minute != 0 && second != 0 && microsecond != 0)
                 {
                     mamaDateTime_setTime(value
                         , hour
                         , minute
                         , second
                         , microsecond
                         );
                 }
             }
             return MAMA_STATUS_OK;
         case MAMA_FIELD_TYPE_BOOL:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         case MAMA_FIELD_TYPE_CHAR:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
         }
      }
      catch (boost::bad_get&)
      {
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      }
      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   // don't copy the msg
   mama_status getMsg(msgPayload value) const
   {
      CHECK_UPA_FIELD_PAYLOAD;

      if (data_.which() == 12)
      {
         const MamaMsgPayloadWrapper_ptr_t& p = boost::get<MamaMsgPayloadWrapper_ptr_t>(data_);
         value = p->getMamaMsgPayload();
         return MAMA_STATUS_OK;
      }

      return MAMA_STATUS_WRONG_FIELD_TYPE;
   }

   mama_status getOpaque(const void ** data, size_t * length) const
   {

       if (data_.which() == 17)
       {
           const MamaOpaqueWrapper_ptr_t& p = boost::get<MamaOpaqueWrapper_ptr_t>(data_);
           *length = p->Length();
           *data = p->Data();
           return MAMA_STATUS_OK;
       }

       return MAMA_STATUS_WRONG_FIELD_TYPE;

   }

   mama_status getVectorMsg(const msgPayload** value, size_t* resultLen) const
   {
      mama_status ret = MAMA_STATUS_OK;
      CHECK_UPA_FIELD_PAYLOAD;

      // todo - need to change the vector wrapper so that it is a vector of msgPayLoad not the payliad wrppaer class
      const MamaMsgVectorWrapper_ptr_t& msgVec = boost::get<MamaMsgVectorWrapper_ptr_t>(data_);
      *resultLen = msgVec->getVectorLength();
      *value = msgVec->GetVector();

      return ret;
   }

   template <typename T>
   mama_status get(T& value/*out*/) const
   {
      mama_status ret = MAMA_STATUS_OK;
      CHECK_UPA_FIELD_PAYLOAD;

      try
      {
         value = boost::get<T>(data_);
      }
      catch (boost::bad_get&)
      {
         ret = MAMA_STATUS_WRONG_FIELD_TYPE;
      }

      return ret;
   }

   mama_status get(std::string& value/*out*/) const
   {
      mama_status ret = MAMA_STATUS_OK;
      CHECK_UPA_FIELD_PAYLOAD;

      value = boost::apply_visitor(ValueTypeToString(), data_);
      return ret;
   }

   mama_status get(const char ***result, mama_size_t *size)
   {
      CHECK_UPA_FIELD_PAYLOAD;

      // There is a problem with the MAMA .Net layer
      // where the size parameter is declared as a uint and this
      // is always a 32-bit quantity. As a result, this causes the
      // adjacent object on the heap to be overwritten in a 64-bit
      // environment, because sizeof(mama_size_t) is 64-bits.
      //
      // Our experiments have shown that the adjacent object in this
      // particular case appears to be the "result" value.
      // The problem should, and will, be fixed in a future version
      // of Open MAMA but, in the meantime, provided these two
      // assignments are executed in this order, the "size"-assignment
      // does not appear to corrupt the "result" value.
      //

      *size = stringVector2_->size();
      *result = &((*stringVector2_)[0]);
      return MAMA_STATUS_OK;
   }

#undef CHECK_UPA_FIELD_PAYLOAD

   template <typename T>
   mama_status set(const T& value)
   {
      if (MamaFieldType<T>::Value != type_ && type_ != MAMA_FIELD_TYPE_UNKNOWN )
         return MAMA_STATUS_WRONG_FIELD_TYPE;
      if (type_ == MAMA_FIELD_TYPE_UNKNOWN)
         type_ = MamaFieldType<T>::Value;
      data_ = value;
      return MAMA_STATUS_OK;
   }
};

#endif
