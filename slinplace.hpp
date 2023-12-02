#pragma once


#include "simplelex.hpp"




namespace simplelexer {




class InplaceAccessor
{
public:
    InplaceAccessor() : _buffer(nullptr), _size(0), _count(0), _cache_enabled(false), _cache{0} {}

    void enable_cache(bool v = true) { _cache_enabled = v; }

    bool parse(char* data, size_t size) { return false; }
    void unsafe_init(char* data, size_t size, size_t count) { _buffer = data; _size = size; _count = count; parse_cache(); }

    TokenRef operator[](size_t index);
    TokenConstRef operator[](size_t index) const;

    size_t size() const noexcept { return _count; }

    char* buffer() { return _buffer; }
    size_t buffer_size() const noexcept { return _size; }

private:
    template<typename T>
    static T min(T a, T b) { return (a < b ? a : b); }
    template<typename T>
    static T max(T a, T b) { return (a > b ? a : b); }
    void parse_cache();
    char* find(size_t index) const;

private:
    char* _buffer;
    size_t _size;
    size_t _count;

    bool _cache_enabled;
    uint8_t _cache[8];

};


struct ParseInplaceResult {
    Error error;
    InplaceAccessor accessor;
};

/** Parse inplace
 * 
 * Convert to internal format
 * [header(size)][data[size]]
 * 
 * header:
 *     data is:
 *         case Type::Eof:
 *             header = (uint8_t)0
 *         case Type::Whitespace:
 *             header = (uint8_t)1
 *             data = {}
 *         case Type::Argument:
 *             header = (uint8_t)input[0]
 *             data = { input }
 *             size = while input[i] > 21 && input[i] < 0x80
 *         case Type::StringDQ or Type::StringSQ:
 *             size = len(input)
 *             a = (data is Type::StringDQ ? 2 : 3)
 *             header = { (uint8_t)a, (uint8_t)size }
 *             data = { parsed(input) }
 *             size = len(parsed(input))
 *         case Type::Flag or Type::LongFlag:
 *             a = (data is Type::Flag ? 4 : 5)
 *             data = { parsed(input) }
 *             size = while input[i] > 21 && input[i] < 0x80
 *         case Type::Variable:
 *             header = (uint8_t)6
 *             data = { parsed(input) }
 *             size = while input[i] > 21 && input[i] < 0x80
 *         case Type::PipeInput:
 *             header = (uint8_t)10
 *         case Type::PipeInputAppend:
 *             header = (uint8_t)11
 *         case Type::PipeOutput:
 *             header = (uint8_t)12
 *         case Type::PipeOutputAppend:
 *             header = (uint8_t)13
 *         case Type::SubProgram:
 *             header = (uint8_t)14
 *             data = { parsed(input) }
* 
 */
ParseInplaceResult parse_repack(char* out_buffer, size_t out_size, const char* buffer, size_t size, bool preparse = true);

Type get_type(char c);
uint8_t get_size(const char* c);
uint8_t get_header_size(Type t);



}




