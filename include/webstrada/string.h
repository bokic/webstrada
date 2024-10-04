#pragma once

#include <vector>
#include <compare>


namespace webstrada {

class cfvariant;

struct stringbuf;

class string//: public std::string
{
public:
    string();
    virtual ~string();
    string(const string& str);
    string(const cfvariant& variant);
    string(const char* s);
    string(const char* s, size_t n);
    string(size_t n, char c);

    string& operator=(const string& other);
    string(string&& other) noexcept;
    string& operator=(string&& other) noexcept;

    void append(char c);
    void append(const string& s);
    void append(const char* s, size_t n);
    void append(const char* s);
    char at(int pos) const;
    void clear();
    int compareCaseInsensitive(const string& other) const;
    int compareCaseInsensitive(const char* other) const;
    int compareCaseInsensitive(const wchar_t* other) const;
    const char* constData() const;
    bool contains(char other) const;
    bool contains(const char *other) const;
    bool contains(const wchar_t *other) const;
    bool contains(const string &other) const;
    bool containsCaseInsensitive(const char *other) const;
    bool containsCaseInsensitive(const wchar_t *other) const;
    bool containsCaseInsensitive(const string &other) const;
    char* data();
    bool equals(const char *other) const;
    bool equals(const string &other) const;
    bool endsWith(const char *other) const;
    bool endsWith(const wchar_t *other) const;
    bool endsWith(const string &other) const;
    bool endsWithCaseInsensitive(const char *other) const;
    bool endsWithCaseInsensitive(const wchar_t *other) const;
    bool endsWithCaseInsensitive(const string &other) const;
    void fill(char ch, int pos, int size = -1);
    char first() const;
    string first(int size) const;
    int indexOf(char ch, int pos = 0) const;
    int indexOf(const char *str, int pos = 0) const;
    int indexOf(const string &str, int pos = 0) const;
    // indexOf
    // insert
    bool isEmpty() const;
    // isLower
    // isNull
    // isUpper
    // last
    string last(int size) const;
    int lastIndexOf(char ch, int pos = -1) const;
    int lastIndexOf(const char *str, int pos = -1) const;
    int lastIndexOf(const string &str, int pos = -1) const;
    string left(int size) const;
    int length() const;
    string mid(int pos, int size) const;
    // number int to str with base
    static string number(int n, int base = 10);
    //static string number(unsigned int n, int base = 10);
    //static string number(unsigned long n, int base = 10);
    static string number(long long n, int base = 10);
    //static string number(unsigned long long n, int base = 10);
    static string number(double n, char format = 'g', int percision = 6);
    void prepend(char ch);
    // push_back
    // push_front
    // remove
    string& remove(int pos, int size);
    string& removeAt(int pos);
    string& removeFirst();
    string& removeLast();
    // repeated
    // replace
    string &replace(int position, int size, char ch);
    void reserve(size_t size);
    void resize(size_t size);
    // section
    // setNum
    // simplified
    // size
    std::vector<string> split(char separator, bool keep_empty = true) const;
    // split
    // squeeze
    bool startWith(const char *str) const;
    // swap
    // toDouble
    // toInt
    int toInt(bool *ok = nullptr, int base = 10) const;
    // toLong
    // toLower
    void toLower();
    // toShort
    // toUInt
    // to...
    // toUpper
    void toUpper();
    string toHtmlEscaped() const;
    string trimmed() const;
    void truncate(int size);

    // operators
    string operator+(const char *other);
    string operator+(const string &other);
    string operator+=(int num);
    string operator+=(const char ch);
    string operator+=(const char *other);
    string operator+=(const string &other);

    std::strong_ordering operator<=> (const char *other) const;
    std::strong_ordering operator<=> (const string &other) const;

    //bool operator==(const string &other) const;
    //bool operator==(const char *other) const;

    string operator=(const char *value);

    string percent_decode() const;

private:
    void mutate();
    stringbuf *m_buffer = nullptr;
    int m_size = 0;
};

string operator+(const char *first, const string &second);
string operator+(const string &first, const string &second);

std::strong_ordering operator<=> (const char *first, const string &second);

};
