#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "typedef.h"
#include "iPackable.h"
#include "stdDefs.h"
#include "math/Half.h"

#include <vector>
#include <string>
#include <cstddef>
#include <cstring>

using namespace std;

namespace Common {

typedef boost::shared_ptr<Buffer> Buffer_ptr;

class Buffer : public boost::enable_shared_from_this<Buffer> {
public:

    Buffer(size_t maxLength = BUFFER_DEFALT_MAX_LENGTH) : m_buffer(maxLength) {
        m_maxLength = maxLength;
        m_length = 0;
        m_readerOffset = 0;
        m_writerOffset = 0;
    }

    Buffer(byte* buffer, size_t length) : m_buffer(length) {
        m_buffer.assign(buffer, buffer + length);
        m_maxLength = length;
        m_length = length;
        m_readerOffset = 0;
        m_writerOffset = length;
    }

    Buffer(const Buffer& b) {
        m_buffer = b.m_buffer;
        m_length = b.m_length;
        m_readerOffset = b.m_readerOffset;
        m_writerOffset = b.m_writerOffset;
    }

    template<class T> inline Buffer& Add(T value) {
        BOOST_STATIC_ASSERT_MSG(boost::is_arithmetic<T>::value, "Type mismatch: Arithmetic types use only");
        size_t size = sizeof (T);
        if (m_writerOffset + size > m_maxLength) {
            return *this;
        }
        *(T*)&m_buffer[m_writerOffset] = value;
        m_writerOffset += size;
        m_length = m_writerOffset > m_length ? m_writerOffset : m_length;
        return *this;
    }

    template<class T> Buffer& Add(T value, size_t offset) {
        BOOST_STATIC_ASSERT_MSG(boost::is_arithmetic<T>::value, "Type mismatch: Arithmetic types use only");
        size_t size = sizeof (T);
        if (offset + size > m_maxLength) {
            return *this;
        }
        *(T*)&m_buffer[offset] = value;
        m_length = offset + size > m_length ? offset + size : m_length;
        return *this;
    }

    inline Buffer& AddPack(Packable& value) {
        value.Pack(shared_from_this());
        return *this;
    }

    inline Buffer& AddPack(Packable& value, size_t offset) {
		size_t writeof = m_writerOffset;
        SetWriteOffset(offset);
        value.Pack(shared_from_this());
        SetWriteOffset(writeof);
        return *this;
    }

    inline Buffer& AddString(const char* pvalue) {
        size_t size = strlen(pvalue) + 1;
        return AddStringSizeFixed(pvalue, size);
    }

    inline Buffer& AddString(const char* pvalue, size_t offset) {
        size_t size = strlen(pvalue) + 1;
        return AddStringSizeFixed(pvalue, size, offset);
    }

    inline Buffer& AddString(string& value) {
        return AddString(value.c_str());
    }

    inline Buffer& AddString(string& value, size_t offset) {
        return AddString(value.c_str(), offset);
    }

    inline Buffer& AddStringSizeFixed(const char* pvalue, size_t size) {
        if (m_writerOffset + size > m_maxLength) {
            return *this;
        }

        size_t stringSize = strlen(pvalue) + 1;
        size_t paddingSize = size > stringSize ? size - stringSize : 0;

        memcpy(&m_buffer[m_writerOffset], pvalue, size - paddingSize);

        if (paddingSize) {
            memset(&m_buffer[m_writerOffset + size - paddingSize], 0, paddingSize);
        }

        m_writerOffset += size;
        m_length = m_writerOffset > m_length ? m_writerOffset : m_length;
        return *this;
    }

    inline Buffer& AddStringSizeFixed(const char* pvalue, size_t size, size_t offset) {
        if (offset + size > m_maxLength) {
            return *this;
        }

        size_t stringSize = strlen(pvalue);
        size_t paddingSize = size > stringSize ? size - stringSize : 0;

        memcpy(&m_buffer[offset], pvalue, size - paddingSize);
        memset(&m_buffer[offset + size - paddingSize], 0, paddingSize);

        m_length = offset + size > m_length ? offset + size : m_length;
        return *this;
    }

    inline Buffer& AddStringSizeFixed(string value, size_t size) {
        return AddStringSizeFixed(value.c_str(), size);
    }

    inline Buffer& AddStringSizeFixed(string value, size_t size, size_t offset) {
        return AddStringSizeFixed(value.c_str(), size, offset);
    }

    inline Buffer& AddBytes(void* pvalue, size_t size) {
        if (m_writerOffset + size > m_maxLength) {
            return *this;
        }
        AddBytes(pvalue, size, m_writerOffset);
        m_writerOffset += size;
        return *this;
    }

    inline Buffer& AddBytes(void* pvalue, size_t size, size_t offset) {
        if (offset + size > m_maxLength) {
            return *this;
        }
        memcpy(&m_buffer[offset], pvalue, size);
        m_length = offset + size > m_length ? offset + size : m_length;
        return *this;
    }

    template<class T> inline T Get() {
        BOOST_STATIC_ASSERT_MSG(boost::is_arithmetic<T>::value, "Type mismatch: Arithmetic types use only");
        size_t size = sizeof (T);
        if (m_readerOffset + size > m_length) {
            return T(0);
        }
        T value = *(T*) & m_buffer[m_readerOffset];
        m_readerOffset += size;
        return value;
    }

    template<class T> inline T Get(size_t offset) {
        BOOST_STATIC_ASSERT_MSG(boost::is_arithmetic<T>::value, "Type mismatch: Arithmetic types use only");
        size_t size = sizeof (T);
        if (offset + size > m_length) {
            return T(0);
        }
        return *(T*) & m_buffer[offset];
    }

    void GetPack(Packable& packable) {
        packable.Unpack(shared_from_this());
    }

    void GetPack(Packable& packable, size_t offset) {
        size_t readerof = m_readerOffset;
        SetReaderOffset(offset);
        packable.Unpack(shared_from_this());
        SetReaderOffset(readerof);
    }

    inline byte* GetBytes(size_t size) {
        if (m_readerOffset + size > m_length) {
            return NULL;
        }
        byte* value = new byte[size];
        memcpy(value, &m_buffer[m_readerOffset], size);
        m_readerOffset += size;
        return value;
    }

    inline byte* GetBytes(size_t size, size_t offset) {
        if (offset + size > m_length) {
            return NULL;
        }
        byte* value = new byte[size];
        memcpy(value, &m_buffer[offset], size);
        return value;
    }

    template<class T> inline T GetString() {
        //BOOST_STATIC_ASSERT_MSG(true == false, "Type mismatch use GetString<string>() or GetString<char*>()");
    }

    template<class T> inline T GetString(size_t offset) {
        //BOOST_STATIC_ASSERT_MSG(true == false, "Type mismatch use GetString<string>() or GetString<char*>()");
    }

    template<class T> inline T GetStringSizeFixed(size_t size) {
        //BOOST_STATIC_ASSERT_MSG(true == false, "Type mismatch use GetString<string>() or GetString<char*>()");
    }

    template<class T> inline T GetStringSizeFixed(size_t size, size_t offset) {
        //BOOST_STATIC_ASSERT_MSG(true == false, "Type mismatch use GetString<string>() or GetString<char*>()");
    }

    inline byte* Data() {
        return &m_buffer[0];
    }

    inline size_t Length() {
        return m_length;
    }

    inline size_t MaxLength() {
        return m_maxLength;
    }

	inline void Clear()
	{
		memset(Data(), 0, MaxLength()); 
	}

    inline void SetLength(size_t length) {
        m_length = length < m_maxLength ? length : m_maxLength;
    }

    inline void SetMaxLength(size_t length) {
        m_buffer.resize(length);
        m_maxLength = length;
    }

    inline void SetReaderOffset(size_t offset) {
        m_readerOffset = offset > m_maxLength ? m_maxLength : offset;
    }

    inline void SetWriteOffset(size_t offset) {
        m_writerOffset = offset > m_maxLength ? m_maxLength : offset;
    }
    
    inline size_t GetReaderOffset() {
        return m_readerOffset;
    }

    inline size_t GetWriteOffset() {
        return m_writerOffset;
    }
protected:
    vector<byte> m_buffer;
    size_t m_maxLength;
    size_t m_length;
    size_t m_readerOffset;
    size_t m_writerOffset;
};

using namespace math;

template<> inline Buffer& Buffer::Add(Half value) {
    return Add((uint16)value);
}

template<> inline Buffer& Buffer::Add(Half value, size_t offset) {
    return Add((uint16)value, offset);
}

template<> inline Half Buffer::Get<Half>() {
    return Get<uint16>();
}

template<> inline Half Buffer::Get<Half>(size_t offset) {
    return Get<uint16>(offset);
}

template<> inline char* Buffer::GetString<char*>() {
    if (m_readerOffset >= m_length) {
        return NULL;
    }
    size_t size = strlen((char*) &m_buffer[m_readerOffset]) + 1;
    if (m_readerOffset + size > m_length) {
        return NULL;
    }
    char* value = new char[size];
    strcpy(value, (char*) &m_buffer[m_readerOffset]);
    m_readerOffset += size;
    return value;
}

template<> inline char* Buffer::GetString<char*>(size_t offset) {
    if (offset >= m_length) {
        return NULL;
    }
    size_t size = strlen((char*) &m_buffer[offset]) + 1;
    if (offset + size >= m_length) {
        return NULL;
    }
    char* value = new char[size];
    strcpy(value, (char*) &m_buffer[offset]);
    return value;
}

template<> inline string Buffer::GetString() {
    char* vreturn = GetString<char*>();
    if (!vreturn) {
        return string();
    }
    string value(vreturn);
    delete[] vreturn;
    return value;
}

template<> inline string Buffer::GetString(size_t offset) {
    char* vreturn = GetString<char*>(offset);
    if (!vreturn) {
        return string();
    }
    string value(vreturn);
    delete[] vreturn;
    return value;
}

template<> inline char* Buffer::GetStringSizeFixed(size_t size) {
    if (m_readerOffset >= m_length) {
        return NULL;
    }

    if (m_readerOffset + size > m_length) {
        return NULL;
    }
    char* value = new char[size + 1];
    memcpy(value, (char*) &m_buffer[m_readerOffset], size);
    value[size] = 0;
    m_readerOffset += size;
    return value;
}

template<> inline char* Buffer::GetStringSizeFixed(size_t size, size_t offset) {
    if (offset >= m_length) {
        return NULL;
    }

    if (offset + size >= m_length) {
        return NULL;
    }
    char* value = new char[size + 1];
    memcpy(value, (char*) &m_buffer[offset], size);
    value[size] = 0;
    return value;
}

template<> inline string Buffer::GetStringSizeFixed(size_t size) {
    char* vreturn = GetStringSizeFixed<char*>(size);
    if (!vreturn) {
        return string();
    }
    string value(vreturn);
    delete[] vreturn;
    return value;
}

template<> inline string Buffer::GetStringSizeFixed(size_t size, size_t offset) {
    char* vreturn = GetStringSizeFixed<char*>(size, offset);
    if (!vreturn) {
        return string();
    }
    string value(vreturn);
    delete[] vreturn;
    return value;
}

} //namespace common

#endif