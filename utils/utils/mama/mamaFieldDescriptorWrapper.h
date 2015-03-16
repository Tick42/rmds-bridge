#pragma once
#ifndef __UTILS_MAMA_MAMAFIELDDESCRIPTORWRAPPER_H__
#define __UTILS_MAMA_MAMAFIELDDESCRIPTORWRAPPER_H__

#include <string>
#include <mama/mama.h>
#include <mama/fielddesc.h>

namespace utils { namespace mama {

/**
 * A reference object of field descriptor takes a pointer to mamaFieldDescriptor and provide interface to it
 */
class mamaFieldDescriptorRef
{
	mamaFieldDescriptor descriptor_;
public:
	mamaFieldDescriptorRef() : descriptor_(NULL) {}
	mamaFieldDescriptorRef(mamaFieldDescriptor descriptor) : descriptor_(descriptor) {}
	mamaFieldDescriptorRef(const mamaFieldDescriptorRef &rhs) : descriptor_(rhs.descriptor_) {}
	mamaFieldDescriptorRef &operator=(const mamaFieldDescriptorRef &rhs)
	{
		if (this != &rhs && this->descriptor_ != rhs.descriptor_)
		{
			this->descriptor_ = rhs.descriptor_;
		}
		return *this;
	}
	inline mamaFieldDescriptor &get(){return descriptor_;}
	inline const mamaFieldDescriptor &get() const {return descriptor_;}
	inline bool valid() const {return descriptor_ != NULL;}
	inline mama_fid_t getFid() const {return mamaFieldDescriptor_getFid(descriptor_); }
	inline mamaFieldType getType() const {return mamaFieldDescriptor_getType(descriptor_);}
	inline std::string getName() const {return std::string (mamaFieldDescriptor_getName(descriptor_));}
	inline std::string getTypeName() const {return std::string (mamaFieldDescriptor_getTypeName(descriptor_));}
};

} /*namespace utils*/ } /*namespace mama*/

#endif //__UTILS_MAMA_MAMAFIELDDESCRIPTORWRAPPER_H__
