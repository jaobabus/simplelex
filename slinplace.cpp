#include "slinplace.hpp"

#include <string.h>



simplelexer::ParseInplaceResult simplelexer::parse_repack(char* out_buffer, size_t out_size, const char* buffer, size_t size, bool preparse)
{
    char* out_it = out_buffer;
    const char* in_it = buffer;
    const char* end = buffer + size;

    char buffered_char = '\0';
    char* type_start = nullptr;
    Type last_type = Type::None;
    size_t count = 0;
    StateContext ctx;
    ParseResult res;
    while (in_it != end + 1) {
        char ch = *in_it;
        if (in_it == end)
            ch = '\0';
        do {
            res = parse_next(&ctx, ch);
            if (res.state == res.Error) {
                return {
                    res.error,
                    {}
                };
            }
            if (res.parsed != -1) {
                bool init = last_type != ctx.type;
                auto type = ctx.type;
                if (init) { // destruct
                    switch (last_type)
                    {
                    case simplelexer::Type::StringDQ:
                    case simplelexer::Type::StringSQ:
                    case simplelexer::Type::SubProgram:
                        *(uint8_t*)out_it++ = buffered_char;
                        (uint8_t&)(type_start[1]) = (uint8_t)(out_it - type_start - 2);
                        break;
                    default:
                        break;
                    }
                    type_start = out_it;
                    last_type = ctx.type;
                }
                switch (type)
                {
                case simplelexer::Type::Whitespace:
                    if (init)
                        *out_it++ = 1;
                    break;
                case simplelexer::Type::Argument:
                    *(uint8_t*)out_it++ = res.parsed;
                    break;
                case simplelexer::Type::StringDQ:
                case simplelexer::Type::StringSQ:
                case simplelexer::Type::SubProgram:
                    if (init) {
                        *out_it = (type == Type::StringDQ ? 2 : (type == Type::StringSQ ? 3 : 14));
                        out_it += 2;
                        buffered_char = (uint8_t)res.parsed;
                    }
                    else {
                        *(uint8_t*)out_it++ = buffered_char;
                        buffered_char = (uint8_t)res.parsed;
                    }
                    break;
                case simplelexer::Type::Flag:
                    if (init)
                        *out_it++ = 4;
                    *(uint8_t*)out_it++ = res.parsed;
                    break;
                case simplelexer::Type::LongFlag:
                    if (init)
                        *out_it++ = 5;
                    *(uint8_t*)out_it++ = res.parsed;
                    break;
                case simplelexer::Type::Variable:
                    if (init)
                        *out_it++ = 6;
                    *(uint8_t*)out_it++ = res.parsed;
                    break;
                case simplelexer::Type::PipeInput:
                    if (init)
                        *out_it++ = 10;
                    break;
                case simplelexer::Type::PipeInputAppend:
                    if (init)
                        *out_it++ = 11;
                    break;
                case simplelexer::Type::PipeOutput:
                    if (init)
                        *out_it++ = 12;
                    break;
                case simplelexer::Type::PipeOutputAppend:
                    if (init)
                        *out_it++ = 13;
                    break;
                    if (init) {
                        *out_it =  14;
                        out_it += 2;
                    }
                    *(uint8_t*)out_it++ = res.parsed;
                    break;
                case simplelexer::Type::Eof:
                    *out_it++ = '\0';
                default:
                    break;
                }
                if (init)
                    count++;
            }
            if (ctx.state == End || ctx.state == EndWithSkip) {
                if (ctx.state == End)
                    res.state = res.Repeat;
                ctx.reset(Type::None);
            }
        } while (res.state == res.Repeat);
        in_it++;
    }
    if (out_it < (out_buffer + out_size))
        *out_it = '\0';
    auto ia = InplaceAccessor();
    ia.unsafe_init(out_buffer, out_it - out_buffer, count);
    return {
        Error::NoError,
        ia
    };
}

simplelexer::Type simplelexer::get_type(char ch)
{
    uint8_t c = (uint8_t)ch;
    if (c >= 0x20 && c < 0x80)
        return Type::Argument;
    switch (c) {
    case 0:
        return Type::Eof;
    case 1:
        return Type::Whitespace;
    case 2:
        return Type::StringDQ;
    case 3:
        return Type::StringSQ;
    case 4:
        return Type::Flag;
    case 5:
        return Type::LongFlag;
    case 6:
        return Type::Variable;
    case 10:
        return Type::PipeInput;
    case 11:
        return Type::PipeInputAppend;
    case 12:
        return Type::PipeOutput;
    case 13:
        return Type::PipeOutputAppend;
    case 14:
        return Type::SubProgram;
    default:
        return Type::None;
    }
}

uint8_t simplelexer::get_size(const char* str)
{
    Type t = get_type(*str);
    if (t == Type::Whitespace || t == Type::PipeInput || t == Type::PipeInputAppend || t == Type::PipeOutput || t == Type::PipeOutputAppend)
        return 0;
    else if (t == Type::StringDQ || t == Type::StringSQ || t == Type::SubProgram)
        return *(str + 1);
    else if (t == Type::Argument || t == Type::Flag || t == Type::LongFlag || t == Type::Variable) {
        uint8_t size = 0;
        str++;
        while (*str >= '\x20' && *str <= '\x7F')
            str++, size++;
        return size + (t == Type::Argument ? 1 : 0);
    }
    else
        return 0;
}

uint8_t simplelexer::get_header_size(Type t)
{
    switch (t)
    {
    case simplelexer::Type::SubProgram:
    case simplelexer::Type::StringDQ:
    case simplelexer::Type::StringSQ:
        return 2;
    case simplelexer::Type::Whitespace:
    case simplelexer::Type::PipeInput:
    case simplelexer::Type::PipeInputAppend:
    case simplelexer::Type::PipeOutput:
    case simplelexer::Type::PipeOutputAppend:
    case simplelexer::Type::Flag:
    case simplelexer::Type::LongFlag:
    case simplelexer::Type::Variable:
        return 1;
    case simplelexer::Type::Argument:
        return 0;
    default:
        return 0;
    }
}

simplelexer::TokenRef simplelexer::InplaceAccessor::operator[](size_t index)
{
    auto ptr = find(index);
    if (not ptr)
        return simplelexer::TokenRef{ nullptr, 0, Type::None };
    return simplelexer::TokenRef{ ptr + get_header_size(get_type(*ptr)), get_size(ptr), get_type(*ptr) };
}

simplelexer::TokenConstRef simplelexer::InplaceAccessor::operator[](size_t index) const
{
    auto ptr = find(index);
    if (not ptr)
        return simplelexer::TokenConstRef{ nullptr, 0, Type::None };
    return simplelexer::TokenConstRef{ ptr + get_header_size(get_type(*ptr)), get_size(ptr), get_type(*ptr) };
}

void simplelexer::InplaceAccessor::parse_cache()
{
    constexpr size_t cache_size = sizeof(_cache) / sizeof(_cache[0]);
    size_t off = 0;
    for (size_t i = 0; i < min<size_t>(cache_size, _count); i++) {
        _cache[i] = (uint8_t)(get_header_size(get_type(*(_buffer + off))) + get_size((_buffer + off)));
        off += _cache[i];
    }
}

char* simplelexer::InplaceAccessor::find(size_t index) const
{
    if (index >= _count)
        return nullptr;
    constexpr size_t cache_size = sizeof(_cache) / sizeof(_cache[0]);
    char* ptr = _buffer;
    if (_cache_enabled) {
        for (uint8_t i = 0; i < min<size_t>(cache_size, index); i++) { // min...
            ptr += _cache[i];
        }
        index -= min<size_t>(cache_size, index); // min...
    }
    while (index > 0)
        ptr += get_header_size(get_type(*ptr)) + get_size(ptr), index--;
    return ptr;
}
