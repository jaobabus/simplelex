#include "simplelex.hpp"




simplelexer::ParseResult simplelexer::parse_next(StateContext* ctx, char c) {
    const char eof = '\0';
    ParseResult pr = { ParseResult::Next, Error::NoError, -1 };
    switch (ctx->type) {
    case Type::None:
    {
        ctx->state = State::Next;
        switch (c) {
        case '\"':
            ctx->type = Type::StringDQ;
            break;
        case '\'':
            ctx->type = Type::StringSQ;
            break;
        case '-':
            ctx->type = Type::Flag;
            break;
        case '$':
            ctx->type = Type::Variable;
            break;
        case '<':
            ctx->type = Type::PipeInput;
            pr.state = pr.Error;
            pr.error = Error::NotImplemented;
            break;
        case '>':
            ctx->type = Type::PipeOutput;
            pr.state = pr.Error;
            pr.error = Error::NotImplemented;
            break;
        case '`':
            ctx->type = Type::SubProgram;
            pr.state = pr.Error;
            pr.error = Error::NotImplemented;
            break;
        case ' ':
        case '\t':
            ctx->type = Type::Whitespace;
            pr.parsed = c;
            break;
        case eof:
            ctx->type = Type::Eof;
            pr.parsed = c;
            break;
        default:
            ctx->type = Type::Argument;
            pr.state = pr.Repeat;
        }
    } break;

    case Type::Argument:
    {
        if (c == '\\')
            ctx->state = Escape;
        else if (ctx->state == Escape)
            pr.parsed = c, ctx->state = Next;
        else if (iswhitespace(c) || c == eof || c == '\"' || c == '\'' || c == '$' || c == '<' || c == '>' || c == '`')
            ctx->state = End;
        else
            pr.parsed = c;
    } break;

    case Type::StringDQ:
    case Type::StringSQ:
    {
        char term = (ctx->type == Type::StringSQ ? '\'' : '\"');
        if (c == '\\')
            ctx->state = Escape;
        else if (ctx->state == Escape) {
            if (c == 'x' || c == 'X')
                ctx->state = EscapeHex;
            else if (c >= '0' && c <= '3')
                ctx->parsed = c - '0', ctx->state = EscapeOct;
            else {
                pr.state = ParseResult::Error;
                pr.error = Error::EscapeError;
            }
        }
        else if (ctx->state >= EscapeHex && ctx->state <= EscapeOct2) {
            switch (ctx->state) {
            case EscapeHex:
                ctx->parsed = hex2octet(c) << 4;
                break;
            case EscapeHex1:
                ctx->parsed |= hex2octet(c);
                pr.parsed = ctx->parsed;
                return pr;
            }
            ctx->state = State(ctx->state + 1);
        }
        else if (c == term)
            ctx->state = EndWithSkip;
        else if (c == eof) {
            pr.state = pr.Error;
            pr.error = Error::UnexpectedEOF;
        }
        else
            pr.parsed = c;
    } break;

    case Type::Flag:
    {
        if (ctx->state == Next) {
            if (c == '-')
                ctx->type = Type::LongFlag, pr.state = pr.Repeat;
            else if (isalnum(c) || c == '_')
                ctx->state = ShortFlag, pr.state = pr.Repeat;
            else if (c == eof) {
                pr.state = pr.Error;
                pr.error = Error::UnexpectedEOF;
            }
            else {
                pr.parsed = c;
                pr.state = ParseResult::Error;
                pr.error = Error::UnexpectedToken;
            }
        }
        else if (ctx->state == ShortFlag) {
            if (isalnum(c) || c == '_')
                pr.parsed = c;
            else if (iswhitespace(c) || c == eof)
                ctx->state = End;
            else {
                pr.parsed = c;
                pr.state = ParseResult::Error;
                pr.error = Error::UnexpectedToken;
            }
        }
    } break;

    case Type::LongFlag:
    {
        if (ctx->state == Next)
            ctx->state = LongFlag;
        else if ((iswhitespace(c) || c == eof) && ctx->state != LongFlag)
            pr.state = ParseResult::ErrorEof;
        else if (iswhitespace(c) || c == eof)
            ctx->state = End;
        else if (isalnum(c) || c == '_' || c == '-')
            pr.parsed = c;
        else {
            pr.parsed = c;
            pr.state = ParseResult::Error;
            pr.error = Error::UnexpectedToken;
        }
    } break;

    case Type::Variable:
    {
        if (ctx->state == Next)
            pr.parsed = c, ctx->state = VarName;
        else if ((iswhitespace(c) || c == eof) && ctx->state != VarName)
            pr.state = ParseResult::ErrorEof;
        else if (iswhitespace(c) || c == eof)
            ctx->state = End;
        else if (isalnum(c) || c == '_')
            pr.parsed = c;
        else {
            pr.parsed = c;
            pr.state = ParseResult::Error;
            pr.error = Error::UnexpectedToken;
        }
    } break;

    case Type::Whitespace: {
        if (not iswhitespace(c))
            ctx->state = End;
        else
            pr.parsed = c;
    } break;

    case Type::Eof: {
        pr.state = pr.Next;
    } break;

    default:
        pr.state = ParseResult::Error;
        pr.error = Error::UnknownState;
        ctx->reset(Type::None);
        break;
    }
    return pr;
}

const char* simplelexer::str(Type type)
{
    switch (type)
    {
    case simplelexer::Type::Eof:
        return "EOF";
    case simplelexer::Type::None:
        return "None";
    case simplelexer::Type::Whitespace:
        return "Whitespace";
    case simplelexer::Type::Argument:
        return "Argument";
    case simplelexer::Type::StringDQ:
        return "StringDQ";
    case simplelexer::Type::StringSQ:
        return "StringSQ";
    case simplelexer::Type::Flag:
        return "Flag";
    case simplelexer::Type::LongFlag:
        return "LongFlag";
    case simplelexer::Type::Variable:
        return "Variable";
    case simplelexer::Type::PipeInput:
        return "PipeInput";
    case simplelexer::Type::PipeInputAppend:
        return "PipeInputAppend";
    case simplelexer::Type::PipeOutput:
        return "PipeOutput";
    case simplelexer::Type::PipeOutputAppend:
        return "PipeOutputAppend";
    case simplelexer::Type::SubProgram:
        return "SubProgram";
    default:
        return "Unknown";
    }
}

const char* simplelexer::str(State state, Type type)
{
    switch (state) {
    case Next:
        return "Next";
    case Escape:
        return "Escape";
    case End:
        return "End";
    case EndWithSkip:
        return "EndWithSkip";
    default:
        ;
    }

    switch (type)
    {
    case simplelexer::Type::Eof:
        return "EOF";
    case simplelexer::Type::StringDQ:
    case simplelexer::Type::StringSQ:
        switch (state) {
        case EscapeHex:
            return "EscapeHex";
        case EscapeHex1:
            return "EscapeHex1";
        case EscapeOct:
            return "EscapeOct";
        case EscapeOct2:
            return "EscapeOct2";
        default:
            return "Unknown";
        }
    case simplelexer::Type::Flag:
        switch (state)
        {
        case simplelexer::ShortFlag:
            return "ShortFlag";
        case simplelexer::LongFlag:
            return "LongFlag";
        default:
            return "Unknown";
        }
    case simplelexer::Type::Variable:
        switch (state)
        {
        case simplelexer::VarName:
            return "VarName";
        default:
            return "Unknown";
        }
    default:
        return "Unknown";
    }
}

const char* simplelexer::str(Error err)
{
    switch (err)
    {
    case simplelexer::Error::NoError:
        return "NoError";
    case simplelexer::Error::NotImplemented:
        return "NotImplemented";
    case simplelexer::Error::UnknownState:
        return "UnknownState";
    case simplelexer::Error::UnknownEscape:
        return "UnknownEscape";
    case simplelexer::Error::EscapeError:
        return "EscapeError";
    case simplelexer::Error::UnexpectedToken:
        return "UnexpectedToken";
    case simplelexer::Error::UnexpectedEOF:
        return "UnexpectedEOF";
    case simplelexer::Error::UnallowedChar:
        return "UnallowedChar";
    case simplelexer::Error::TooLongArgument:
        return "TooLongArgument";
    default:
        return "Unknown";
    }
}

bool simplelexer::iswhitespace(char c) {
    return c == ' ' || c == '\t';
}

bool simplelexer::isalnum(char c) {
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9';
}

uint8_t simplelexer::hex2octet(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}
