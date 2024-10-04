#include <webstrada/cfvariant.h>
#include <webstrada/string.h>

#include <fstream>
#include <cmath>

#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cstdio>


#define UNIMPLEMENTED std::runtime_error(std::string("Unimplemented. - ") + __PRETTY_FUNCTION__)
#define RUNTIME_WITH_STRING(text) std::runtime_error(std::string(text " - ") + __PRETTY_FUNCTION__)

#ifdef ENABLE_TRACE
  #define TRACE_FUNCTION() fprintf(stderr, "Start %s: %d\n", __PRETTY_FUNCTION__, __LINE__)
  #define TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
  #define TRACE_FUNCTION()
  #define TRACE(...)
#endif

using namespace webstrada;

struct webstrada::stringbuf {
    int ref;
    int size;
    char data[0];
};

string::string()
{
    TRACE_FUNCTION();
}

string::~string()
{
    TRACE_FUNCTION();

    clear();

    TRACE("destructed %p...\n", this);
}

string::string(const string& str)
{
    TRACE_FUNCTION();
    TRACE("str: %s\n", str.constData());

    m_buffer = str.m_buffer;
    if (m_buffer) {
        m_buffer->ref++;
        m_size = str.m_size;
    }
}

string::string(const cfvariant& variant)
{
    switch(variant.m_type) {
    case cfvariant::NotSet:
    case cfvariant::Null:
        break;
    case cfvariant::String:
        if ((variant.m_str)&&(variant.m_str->length() > 0))
        {
            int size = variant.m_str->length();
            resize(size);
            memcpy(m_buffer->data, variant.m_str->constData(), size);
        }
        break;
    default:
        throw RUNTIME_WITH_STRING("Variable is not array.");
    }
}

string::string(const char* s)
    : string(s, strlen(s))
{
    //TRACE_FUNCTION();
}

string::string(const char* s, size_t n)
{
    TRACE_FUNCTION();

    if ((s)&&(n > 0))
    {
        resize(n);

        memcpy(m_buffer->data, s, n);
    }
    else
    {
        resize(0);
    }
}

string::string(size_t n, char c)
{
    TRACE_FUNCTION();

    resize(n);

    if (n > 0)
    {
        memset(m_buffer->data, c, n);
    }
}

string& string::operator=(const string& other)
{
    if (&other == this) return *this;

    clear();
    m_buffer = other.m_buffer;
    if (m_buffer) m_buffer->ref++;
    m_size = other.m_size;

    return *this;
}

string::string(string&& other) noexcept
{
    TRACE_FUNCTION();

    std::swap(m_buffer, other.m_buffer);
    std::swap(m_size, other.m_size);
}

string& string::operator=(string&& other) noexcept
{
    TRACE_FUNCTION();

    std::swap(m_buffer, other.m_buffer);
    std::swap(m_size, other.m_size);

    return *this;
}

void string::append(char c)
{
    resize(m_size + 1);

    m_buffer->data[m_size - 1] = c;
}

void string::append(const string& s)
{
    auto n = s.m_size;

    if (n <= 0)
        return;

    size_t pos = m_size;
    resize(m_size + n);

    if (m_buffer == nullptr)
        throw RUNTIME_WITH_STRING("Variable is not array.");

    memcpy(m_buffer->data + pos, s.constData(), n);
}

void string::append(const char* s, size_t n)
{
    if (n == 0)
        return;

    size_t pos = m_size;
    resize(m_size + n);

    if (m_buffer == nullptr)
        throw RUNTIME_WITH_STRING("Variable is not array.");

    memcpy(m_buffer->data + pos, s, n);
}

void string::append(const char* s)
{
    int n = strlen(s);
    if (n == 0)
        return;

    size_t pos = m_size;
    resize(m_size + n);
    memcpy(m_buffer->data + pos, s, n);
}

char string::at(int pos) const
{
    if (pos < 0)
        throw RUNTIME_WITH_STRING("Invalid position.");

    if (pos >= m_size)
        throw RUNTIME_WITH_STRING("Invalid position.");

    return m_buffer->data[pos];
}

void string::clear()
{
    TRACE_FUNCTION();

    if (m_buffer) {
        m_buffer->ref--;
        if (m_buffer->ref == 0) {
            TRACE("Delete string buffer %p, buffer size: %d\n", m_buffer, m_buffer->size);
            free(m_buffer);
        }
        m_buffer = nullptr;
        m_size = 0;
    }
}

int string::compareCaseInsensitive(const string& other) const
{
    TRACE_FUNCTION();

    // An empty string has a null buffer (resize(0) -> clear()), so guard both
    // operands before dereferencing. strcasecmp() cannot take a null pointer.
    if (m_size == 0) {
        return (other.m_size == 0) ? 0 : -1;
    }
    if (other.m_size == 0)
        return 1;

    return strcasecmp(m_buffer->data, other.m_buffer->data);
}

int string::compareCaseInsensitive(const char* other) const
{
    TRACE_FUNCTION();

    if (m_size == 0) {
        return (other == nullptr || *other == '\0') ? 0 : -1;
    }
    if (other == nullptr)
        return 1;

    return strcasecmp(m_buffer->data, other);
}

int string::compareCaseInsensitive(const wchar_t* other) const
{
    TRACE_FUNCTION();

    if (m_size == 0) {
        return (other == nullptr || *other == L'\0') ? 0 : -1;
    }
    if (other == nullptr)
        return 1;

    return wcscasecmp((const wchar_t*)m_buffer->data, other);
}

const char* string::constData() const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return nullptr;

    return m_buffer->data;
}

bool string::contains(char other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    return strchr(m_buffer->data, other) != nullptr;
}

bool string::contains(const char *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    return strstr(m_buffer->data, other) != nullptr;
}

bool string::contains(const wchar_t *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    return wcsstr((const wchar_t*)m_buffer->data, other) != nullptr;
}

bool string::contains(const string &other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    return strstr(m_buffer->data, other.constData()) != nullptr;
}

bool string::containsCaseInsensitive(const char *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    return strcasestr(m_buffer->data, other) != nullptr;
}

bool string::containsCaseInsensitive(const wchar_t *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    return wcscasecmp((const wchar_t*)m_buffer->data, other) == 0;
}

bool string::containsCaseInsensitive(const string &other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    return strcasestr(m_buffer->data, other.constData()) != nullptr;
}

char* string::data()
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return nullptr;

    //m_buffer->data[m_buffer->size] = 0;

    return m_buffer->data;
}

bool string::equals(const char *other) const
{
    TRACE_FUNCTION();

    int other_size = strlen(other);

    if ((m_size == 0)&&(other_size == 0))
        return true;

    if (m_size != other_size)
        return false;

    return memcmp(m_buffer->data, other, m_size) == 0;
}

bool string::equals(const string &other) const
{
    TRACE_FUNCTION();

    if ((m_size == 0)&&(other.m_size == 0))
        return true;

    if (m_size != other.m_size)
        return false;

    return memcmp(m_buffer->data, other.m_buffer->data, m_size) == 0;
}

bool string::endsWith(const char *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    int other_size = strlen(other);

    if (other_size > m_size)
        return false;

    return strcasecmp(m_buffer->data + m_size - other_size, other) == 0;
}

bool string::endsWith(const wchar_t *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    int other_size = wcslen(other);

    if (other_size > m_size)
        return false;

    return wcscasecmp((const wchar_t*)m_buffer->data + m_size - other_size, other) == 0;
}

bool string::endsWith(const string &other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    int other_size = other.m_size;

    if (other_size > m_size)
        return false;

    return strcasecmp(m_buffer->data + m_size - other_size, other.constData()) == 0;
}

bool string::endsWithCaseInsensitive(const char *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    int other_size = strlen(other);

    if (other_size > m_size)
        return false;

    return strcasecmp(m_buffer->data + m_size - other_size, other) == 0;
}

bool string::endsWithCaseInsensitive(const wchar_t *other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    int other_size = wcslen(other);

    if (other_size > m_size)
        return false;

    return wcscasecmp((const wchar_t*)m_buffer->data + m_size - other_size, other) == 0;
}

bool string::endsWithCaseInsensitive(const string &other) const
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return false;

    int other_size = other.m_size;

    if (other_size > m_size)
        return false;

    return strcasecmp(m_buffer->data + m_size - other_size, other.constData()) == 0;
}

void string::fill(char ch, int pos, int size)
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return;

    if (pos < 0)
        pos = 0;

    if (size < 0)
        size = m_size - pos;

    if (pos >= m_size)
        return;

    if (pos + size > m_size)
        size = m_size - pos;

    memset(m_buffer->data + pos, ch, size);
}

char string::first() const
{
    if (m_buffer == nullptr)
        return '\0';

    return m_buffer->data[0];
}

string string::first(int size) const
{
    if (m_buffer == nullptr)
        return string();

    return string(m_buffer->data, size);
}

int string::indexOf(char ch, int pos) const
{
    int len = length();

    if(len > 0) {
        const char *str = constData();

        for(int c = pos; c < len; c++) {
            if (str[c] == ch)
                return c;
        }
    }

    return -1;
}

int string::indexOf(const char *str, int pos) const
{
    if (!m_buffer || !str) return -1;

    const char *found = strstr(m_buffer->data + pos, str);
    if (!found) return -1;

    return static_cast<int>(found - m_buffer->data);
}

int string::indexOf(const string &str, int pos) const
{
    int len = length();
    int sublen = str.length();

    if (sublen == 0) return pos;
    if (sublen > len - pos) return -1;

    const char* s = constData();
    const char* substr = str.constData();

    for(int c = pos; c <= len - sublen; c++) {
        if (memcmp(s + c, substr, sublen) == 0)
            return c;
    }

    return -1;
}

bool string::isEmpty() const
{
    return m_size == 0;
}

string string::last(int size) const
{
    const char *str = nullptr;

    if (size > 0) {
        auto len = length();

        if (size > len) {
            size = len;
        }

        string ret;
        str = constData();
        ret = string(str + len - size, size); // TODO: Optimize this when we have m_pos back.
        return ret;
    }

    return string();
}

int string::lastIndexOf(char ch, int pos) const
{
    int len = length();

    if(len > 0) {
        if (pos < 0 || pos >= len)
            pos = len - 1;

        const char *str = constData();

        for(int c = pos; c >= 0; c--) {
            if (str[c] == ch)
                return c;
        }
    }

    return -1;
}

int string::lastIndexOf(const char *str, int pos) const
{
    if (!str) return -1;

    int len = length();
    int sublen = strlen(str);

    if (sublen == 0) return pos >= 0 ? pos : len;
    if (sublen > len) return -1;
    if (pos < 0 || pos > len - sublen) pos = len - sublen;

    const char* s = constData();

    for(int c = pos; c >= 0; c--) {
        if (c + sublen > len) continue;
        if (memcmp(s + c, str, sublen) == 0)
            return c;
    }

    return -1;
}

int string::lastIndexOf(const string &str, int pos) const
{
    int len = length();
    int sublen = str.length();

    if (sublen == 0) return pos >= 0 ? pos : len;
    if (sublen > len) return -1;
    if (pos < 0 || pos > len - sublen) pos = len - sublen;

    const char* s = constData();
    const char* substr = str.constData();

    for(int c = pos; c >= 0; c--) {
        if (c + sublen > len) continue;
        if (memcmp(s + c, substr, sublen) == 0)
            return c;
    }

    return -1;
}

string string::left(int size) const
{
    const char *str = nullptr;

    if (size > 0) {
        auto len = length();

        if (size > len) {
            size = len;
        }

        string ret;
        str = constData();
        ret = string(str, size); // TODO: Optimize this when we have m_pos back.
        return ret;
    }

    return string();
}

int string::length() const
{
    TRACE_FUNCTION();

    return m_size;
}

string string::mid(int pos, int size) const
{
    const char *str = nullptr;

    if (size > 0) {
        auto len = length();

        if (pos + size > len) {
            size = len - pos;
        }

        if (size > 0) {
            string ret;
            str = constData();
            ret = string(str + pos, size); // TODO: Optimize this when we have m_pos back.
            return ret;
        }
    }

    return string();
}

string string::number(int n, int base)
{
    if (n == 0) return string("0");

    string ret;
    bool negative = false;
    long long val = n;
    if (val < 0) {
        negative = true;
        val = -val;
    }

    static const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    if (base > 16) base = 16;
    if (base <  2) base =  2;

    while (val > 0) {
        int digit = val % base;
        val /= base;
        ret.prepend(digits[digit]);
    }

    if (negative) {
        ret.prepend('-');
    }

    return ret;
}

string string::number(long long n, int base)
{
    if (n == 0) return string("0");

    string ret;
    bool negative = false;
    unsigned long long uval = static_cast<unsigned long long>(n);
    if (n < 0) {
        negative = true;
        uval = static_cast<unsigned long long>(-(n + 1)) + 1ull; // avoid LLONG_MIN negation overflow
    }

    static const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    if (base > 16) base = 16;
    if (base <  2) base =  2;

    while (uval > 0) {
        int digit = uval % base;
        uval /= base;
        ret.prepend(digits[digit]);
    }

    if (negative) {
        ret.prepend('-');
    }

    return ret;
}

string string::number(double n, char format, int percision)
{
    string ret;
    char fmt[16];
    std::snprintf(fmt, sizeof(fmt), "%%.%d%c", percision, format);
    char buf[128];
    std::snprintf(buf, sizeof(buf), fmt, n);
    ret.append(buf);
    return ret;
}

void string::prepend(char ch)
{
    TRACE_FUNCTION();

    if (m_buffer)
    {
        mutate();

        int len = this->length();

        resize(len + 1);

        memmove(m_buffer->data + 1, m_buffer->data, len);
    }
    else
    {
        resize(1);
    }

    m_buffer->data[0] = ch;
}

string& string::remove(int pos, int size)
{
    char *str = nullptr;
    int len = 0;

    if ((size <=0)||(pos < 0))
        return *this;

    len = length();

    if (pos + size > len) {
        resize(pos);
        return *this;
    }

    str = data();
    // Shift the tail [pos+size, len) left by `size`; the copy length is the
    // bytes after the removed span (len - size - pos), not len - size — the
    // latter reads past the end of the buffer (heap-buffer-overflow, caught by
    // ASAN during CFKillBoard killmail parsing).
    memmove(str + pos, str + pos + size, static_cast<size_t>(len - size - pos));
    resize(len - size);

    return *this;
}

string& string::removeAt(int pos)
{
    return remove(pos, 1);
}

string& string::removeFirst()
{
    return remove(0, 1);
}

string& string::removeLast()
{
    return remove(length() - 1, 1);
}

string& string::replace(int position, int size, char ch)
{
    char *str = nullptr;
    int len = 0;

    len = length();

    if ((size <= 0)||(position >= len))
        return *this;

    remove(position + 1, size - 1);
    str = data();
    str[position] = ch;

    return *this;
}

void string::reserve(size_t size)
{
    TRACE_FUNCTION();

    if (size == m_size)
        return;

    if ((m_buffer != nullptr)&&(size < m_buffer->size))
        return;

    if (m_buffer == nullptr) {
        int aligned_size = (((size + 1) / 256) * 256) + 256;

        m_buffer = reinterpret_cast<stringbuf *>(malloc(offsetof(stringbuf, data) + aligned_size));
        if (m_buffer == nullptr)
            throw RUNTIME_WITH_STRING("Out of memory");

        m_buffer->ref = 1;
        m_buffer->size = aligned_size;
        TRACE("Created string buffer %p, buffersize: %d\n", m_buffer, m_buffer->size);

        m_size = size;
        m_buffer->data[size] = 0;

        return;
    }

    if (size > m_size) {
        mutate();

        if (size > m_buffer->size - 1) {
            TRACE("Buffer too small. Buffer size %d, needed: %ld\n", m_buffer->size, size);

            int aligned_size = (((size + 1) / 256) * 256) + 256;

            stringbuf *new_buffer = reinterpret_cast<stringbuf *>(malloc(offsetof(stringbuf, data) + aligned_size));
            if (new_buffer == nullptr)
                throw RUNTIME_WITH_STRING("Out of memory");

            new_buffer->ref = 1;
            new_buffer->size = aligned_size;
            TRACE("Created new string buffer %p, buffersize: %d\n", new_buffer, new_buffer->size);

            memcpy(new_buffer->data, m_buffer->data, m_size);
            new_buffer->data[m_size] = 0;

            m_buffer->ref--;
            if (m_buffer->ref == 0) {
                TRACE("Delete string buffer %p, buffer size: %d\n", m_buffer, m_buffer->size);
                free(m_buffer);
            }

            m_buffer = new_buffer;
        }

        m_size = size;
        m_buffer->data[size] = 0;
    }
}

void string::resize(size_t size)
{
    TRACE_FUNCTION();

    if (size == m_size)
        return;

    if (size == 0) {
        clear();

        return;
    }

    if (m_buffer == nullptr) {
        int aligned_size = (((size + 1) / 256) * 256) + 256;

        m_buffer = reinterpret_cast<stringbuf *>(malloc(offsetof(stringbuf, data) + aligned_size));
        if (m_buffer == nullptr)
            throw RUNTIME_WITH_STRING("Out of memory");

        m_buffer->ref = 1;
        m_buffer->size = aligned_size;
        TRACE("Created string buffer %p, buffersize: %d\n", m_buffer, m_buffer->size);

        m_size = size;
        m_buffer->data[size] = 0;

        return;
    }

    if (size > m_size) {
        mutate();

        if (size > m_buffer->size - 1) {
            TRACE("Buffer too small. Buffer size %d, needed: %ld\n", m_buffer->size, size);
            int aligned_size = (((size + 1) / 256) * 256) + 256;
            void *old_buffer = m_buffer;
            m_buffer = reinterpret_cast<stringbuf *>(realloc(m_buffer, offsetof(stringbuf, data) + aligned_size));
            if (m_buffer == nullptr) {
                free(old_buffer);
                throw RUNTIME_WITH_STRING("Out of memory");
            }

            m_buffer->size = aligned_size;
            TRACE("Reallocated string buffer %p, buffersize: %d\n", m_buffer, m_buffer->size);
        }
    }

    m_size = size;
    // Keep the buffer NUL-terminated at the new size (also covers shrinking).
    // The buffer has spare capacity, so writing data[size] is always safe.
    if (m_buffer != nullptr)
        m_buffer->data[size] = 0;
}

std::vector<string> string::split(char separator, bool keep_empty) const
{
    std::vector<string> ret;

    int pos = 0;

    int len = length();

    while(1)
    {
        int newpos = indexOf(separator, pos);
        if (newpos == -1)
            break;

        int item_size = newpos - pos;
        if ((item_size > 0)||(keep_empty == true))
            ret.push_back(mid(pos, newpos - pos));

        pos = newpos + 1;
    }

    if ((pos < len)||((pos == len)&&(keep_empty == true)))
        ret.push_back(last(len - pos));

    return ret;
}

bool string::startWith(const char *str) const
{
    if (str == nullptr)
        return true;

    int len = strlen(str);
    if (len == 0)
        return true;

    if (len > m_size)
        return false;

    return memcmp(str, m_buffer->data, len) == 0;
}

int string::toInt(bool *ok, int base) const
{
    int ret = 0;
    const char *str = nullptr;

    str = constData();

    switch(base) {
    case 1:
        UNIMPLEMENTED;
        break;
    case 8:
        UNIMPLEMENTED;
        break;
    case 10:
        for(int c = 0; c < length(); c++) {
            ret *= 10;
            char digit = str[c];

            if ((digit >= '0')&&(digit <= '9')) {
                ret += digit - '0';
                continue;
            }

            if (ok)
                *ok = false;
            return 0;
        }
        break;
    case 16:
        for(int c = 0; c < length(); c++) {
            ret *= 16;
            char digit = str[c];

            if ((digit >= '0')&&(digit <= '9')) {
                ret += digit - '0';
                continue;
            } else {
                if ((digit >= 'a')&&(digit <= 'z')) {
                    digit -= 'a' - 'A';
                }
                if ((digit >= 'A')&&(digit <= 'Z')) {
                    ret += digit - 'A' + 10;
                    continue;
                }
            }

            if (ok)
                *ok = false;
            return 0;
        }
        break;
    default:
        UNIMPLEMENTED;
        return 0;
    }

    if (ok)
        *ok = true;

    return ret;
}

void string::toLower()
{
    TRACE_FUNCTION();

    bool should_write = false;

    if ((m_buffer == nullptr)||(m_size == 0))
    {
        return;
    }

    for(int c = 0; c < m_size; c++)
    {
        char ch = m_buffer->data[c];

        if ((ch >= 'A')&&(ch <= 'Z'))
        {
            should_write = true;
            break;
        }
    }

    if(!should_write)
    {
        return;
    }

    mutate();

    for(int c = 0; c < m_size; c++)
    {
        char ch = m_buffer->data[c];

        if ((ch >= 'A')&&(ch <= 'Z'))
        {
            m_buffer->data[c] = m_buffer->data[c] + ('a' - 'A');
        }
    }
}

void string::toUpper()
{
    TRACE_FUNCTION();

    bool should_write = false;

    if ((m_buffer == nullptr)||(m_size == 0))
    {
        return;
    }

    for(int c = 0; c < m_size; c++)
    {
        char ch = m_buffer->data[c];

        if ((ch >= 'a')&&(ch <= 'z'))
        {
            should_write = true;
            break;
        }
    }

    if(!should_write)
    {
        return;
    }

    mutate();

    for(int c = 0; c < m_size; c++)
    {
        char ch = m_buffer->data[c];

        if ((ch >= 'a')&&(ch <= 'z'))
        {
            m_buffer->data[c] = m_buffer->data[c] - ('a' - 'A');
        }
    }
}

string string::toHtmlEscaped() const
{
    string ret;

    for(int c = 0; c < m_size; c++)
    {
        const char ch = m_buffer->data[c];
        switch(ch)
        {
        case '<': ret.append("&lt;");  break;
        case '>': ret.append("&gt;");  break;
        case '&': ret.append("&amp;"); break;
        case '\"': ret.append("&quot;"); break;
        case '\'': ret.append("&apos;"); break;
        default: ret.append(ch);
        }
    }

    return ret;
}

string string::trimmed() const
{
    int start = 0, end = m_size - 1;

    while (start <= end && isspace((unsigned char)m_buffer->data[start])) start++;
    while (end >= start && isspace((unsigned char)m_buffer->data[end])) end--;

    return string(m_buffer->data + start, end - start + 1);
}

void string::truncate(int size)
{
    reserve(size);

    resize(size);
}

string string::operator+(const char *other)
{
    TRACE_FUNCTION();

    string ret = *this;

    ret += other;

    return ret;
}

string string::operator+(const string &other)
{
    TRACE_FUNCTION();

    string ret = *this;

    ret += other;

    return ret;
}

string webstrada::operator+(const char *first, const string &second)
{
    TRACE_FUNCTION();

    string ret;

    int first_len = strlen(first);

    int new_size = first_len + second.length();

    ret.resize(new_size);

    if (ret.data() == nullptr)
        throw RUNTIME_WITH_STRING("Variable is not array.");

    memcpy(ret.data(), first, first_len);
    memcpy(ret.data() + first_len, second.constData(), second.length());
    ret.data()[new_size] = 0;

    return ret;
}

string webstrada::operator+(const string &first, const string &second)
{
    TRACE_FUNCTION();

    string ret;

    int first_len = first.length();
    int second_len = second.length();

    int new_size = first_len + second_len;

    ret.resize(new_size);

    if (ret.data() == nullptr)
        throw RUNTIME_WITH_STRING("Variable is not array.");


    memcpy(ret.data(), first.constData(), first_len);
    memcpy(ret.data() + first_len, second.constData(), second_len);

    return ret;
}

string string::operator+=(int num)
{
    string tmp = string::number(num);

    this->append(tmp);

    return *this;
}

string string::operator+=(const char ch)
{
    TRACE_FUNCTION();

    resize(m_size + 1);

    m_buffer->data[m_size - 1] = ch;

    return *this;
}

string string::operator+=(const char *other)
{
    TRACE_FUNCTION();

    mutate();

    int old_size = m_size;
    resize(m_size + strlen(other));

    if (m_buffer == nullptr)
        throw RUNTIME_WITH_STRING("Variable is not array.");

    memcpy(m_buffer->data + old_size, other, strlen(other));

    return *this;
}

string string::operator+=(const string &other)
{
    TRACE_FUNCTION();

    if(m_buffer == nullptr) {
        if (other.m_buffer == nullptr)
            return *this;
        other.m_buffer->ref++;
        m_buffer = other.m_buffer;
        m_size = other.m_size;
        return *this;
    }

    int old_size = m_size;
    resize(m_size + other.m_size);

    if (m_buffer == nullptr)
        throw RUNTIME_WITH_STRING("Variable is not array.");

    memcpy(m_buffer->data + old_size, other.m_buffer->data, other.m_size);

    return *this;
}

void string::mutate()
{
    TRACE_FUNCTION();

    if (m_buffer == nullptr)
        return;

    if (m_buffer->ref > 1) {
        TRACE("String is shared will do deep copy!\n");
        auto old_buffer = m_buffer;
        int aligned_size = (((m_size + 1) / 256) * 256) + 256;
        m_buffer->ref--;
        m_buffer = reinterpret_cast<stringbuf *>(malloc(offsetof(stringbuf, data) + aligned_size));
        if (m_buffer == nullptr)
            throw RUNTIME_WITH_STRING("Out of memory");
        m_buffer->ref = 1;
        m_buffer->size = aligned_size;
        TRACE("Mutate created string buffer %p, buffersize: %d\n", m_buffer, m_buffer->size);
        memcpy(m_buffer->data, old_buffer->data, m_size);
        m_buffer->data[m_size] = 0;
    }
}


std::strong_ordering string::operator<=> (const string &other) const
{
    const char *s1 = constData();
    const char *s2 = other.constData();

    if (s1 == s2) return std::strong_ordering::equal;
    if (!s1) return std::strong_ordering::less;
    if (!s2) return std::strong_ordering::greater;

    int res = strcasecmp(s1, s2);
    if (res < 0) return std::strong_ordering::less;
    if (res > 0) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

std::strong_ordering string::operator<=> (const char *other) const
{
    const char *s1 = constData();

    if (s1 == other) return std::strong_ordering::equal;
    if (!s1) return std::strong_ordering::less;
    if (!other) return std::strong_ordering::greater;

    int res = strcasecmp(s1, other);
    if (res < 0) return std::strong_ordering::less;
    if (res > 0) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

string string::percent_decode() const
{
    string ret = *this;

    if ((m_buffer != nullptr)&&(m_buffer->size > 0))
    {
        ret.mutate();

        int pos = 0;
        while (pos < ret.length())
        {
            char c = ret.at(pos);
            if (c == '+')
            {
                ret.replace(pos, 1, ' ');
                pos++;
            }
            else if (c == '%')
            {
                auto hex_string = ret.mid(pos + 1, 2);
                bool ok = false;
                int ch = hex_string.toInt(&ok, 16);
                if (ok == true && hex_string.length() == 2)
                {
                    ret.replace(pos, 3, (char)ch);
                    pos++;
                }
                else
                {
                    pos++;
                }
            }
            else
            {
                pos++;
            }
        }
    }

    return ret;
}

string string::operator=(const char *value)
{
    TRACE_FUNCTION();

    if (value == nullptr)
    {
        resize(0);
        return *this;
    }

    int n = strlen(value);

    resize(n);

    memcpy(m_buffer->data, value, n);

    return *this;
}

/*bool string::operator==(const string &other) const
{
    return equals(other);
}

bool string::operator==(const char *other) const
{
    return equals(other);
}*/

std::strong_ordering webstrada::operator<=>(const char *first, const string &second)
{
    auto ordering = second <=> first;
    if (ordering == std::strong_ordering::less) return std::strong_ordering::greater;
    if (ordering == std::strong_ordering::greater) return std::strong_ordering::less;
    return std::strong_ordering::equal;
}
