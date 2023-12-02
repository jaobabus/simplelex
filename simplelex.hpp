#pragma once

#include <stdio.h>
#include <stdint.h>



/** Lexical
 *
 * BNF:
     # WARN <IGNORE_ESCAPE>: Warning escape '%s' ignored
     #

     <COMMENT>       ::= "#"
     <WORD>          ::= [a-z] | [A-Z] | [0-9] | "_"
     <SPETIAL>       ::= "%" | "*" | "+" | "," | "-" | "." | "/" | ":" | "=" | "?" | "@" | "~"
     <PIPE>          ::= "<" | ">"
     <PARENS>        ::= "(" | ")"
     <SEMICOLON>     ::= ";"
     <BASH_EVENT>    ::= "!"
     <VARIABLE_W>    ::= "$"
     <BASH_TEST>     ::= "[" | "]"
     <ESCAPE>        ::= "\\"
     <BASH_SUBS>     ::= "^"
     <SUBPROGRAM>    ::= "`"
     <BASH_UNK1>     ::= "{" | "}"
     <BASH_OR>       ::= "|"
     <BASH_AND>      ::= "&"

     <NOT_IMPL>      ::= <PARENS> | <BASH_EVENT> | <BASH_TEST> | <BASH_SUBS> | <SUBPROGRAM> | <BASH_UNK1> | <BASH_OR> | <BASH_AND>

     <HEX_DIGIT>     ::= [0-9] | [a-f] | [A-F]

     <WP_DIGIT>      ::= " " | "\t"
     <STR_DIGIT>     ::= <WP_DIGIT> | <COMMENT> | <WORD> | <SPETIAL> | <PIPE> | <PARENS> | <SEMICOLON> | <BASH_EVENT> | <VARIABLE> | <BASH_TEST> | <BASH_SUBS> | <SUBPROGRAM> | <BASH_UNK1> | <BASH_OR>
     <ARG_DIGIT>     ::= <WORD> | <SPETIAL> | <NOT_IMPL>
     <FLAG_DIGIT>    ::= <WORD> | "-"
     <VAR_DIGIT>     ::= <WORD>

     <IGNORE_ESCAPE> ::= <ANY_ESCAPE>
     <HEX_ESCAPE>    ::= "\\" ( "x" | "X" ) <HEX_DIGIT> <HEX_DIGIT>
     <COMMON_ESCAPE> ::= "\\" ( "a" | "r" | "t" | "n" | "b" | "e" | "v" )
     <UNIC_ESCAPE>   ::= "\\u" <HEX_ESCAPE> <HEX_ESCAPE> <HEX_ESCAPE> <HEX_ESCAPE>

     <ANY_ESCAPE>    ::= "\\" ( <STR_DIGIT> | "'" | "\"" | "\\" )
     <STR_ESCAPE>    ::= <HEX_ESCAPE> | <COMMON_ESCAPE> | <UNIC_ESCAPE> | <IGNORE_ESCAPE>

     <WHITESPACE>    ::= <WP_DIGIT>+
     <ARGUMENT>      ::= ( <ARG_DIGIT> | <ANY_ESCAPE> )+
     <STRING_SQ>     ::= "'" ( <STR_DIGIT> | "\"" | <STR_ESCAPE> | "\\'" )* "'"
     <STRING_DQ>     ::= "\"" ( <STR_DIGIT> | "'" | <STR_ESCAPE> | "\\\"" )* "\""
     <FLAG>          ::= "-" <WORD>+
     <LONG_FLAG>     ::= "--" <FLAG_DIGIT>+
     <VARIABLE>      ::= "$" <VAR_DIGIT>+

     <TOKEN>         ::= <WHITESPACE> | <ARGUMENT> | <STRING_SQ> | <STRING_DQ> | <FLAG> | <LONG_FLAG> | <VARIABLE>
     <EXPR>          ::= <TOKEN>+

 *  !!! BNF AND REALIZATION NOT SAME !!!
 **/


// ********************************* <Lex> ********************************* //



namespace simplelexer {



bool iswhitespace(char c);
bool isalnum(char c);
uint8_t hex2octet(char c);


enum class Error : uint8_t {
    NoError,
    NotImplemented,
    UnknownState,
    UnknownEscape,
    EscapeError,
    UnexpectedToken,
    UnexpectedEOF,
    UnallowedChar,
    TooLongArgument
};

enum class Type : uint8_t {
    None = 0,
    Whitespace,
    Eof,
    Argument,
    String,
    StringDQ,
    StringSQ,
    Flag,
    LongFlag,
    Variable,

    PipeInput,
    PipeInputAppend,
    PipeOutput,
    PipeOutputAppend,
    SubProgram,
};

enum State : uint8_t {
    // Common
    Next = 0,
    Escape = 1,
    End = 14,
    EndWithSkip = 15,

    // String
    EscapeHex = 2,  // \xA
    EscapeHex1 = 3,  // \xAB
    EscapeOct = 4,  // \12
    EscapeOct2 = 5,  // \123

    // Flag
    ShortFlag = 2,  // -...
    LongFlag = 3,  // --...

    // Variable
    VarName = 2

};

struct StateContext
{
    Type  type = Type::None;
    State state = {};
    uint8_t parsed = 0;
    void reset(Type t, State s = Next) {
        type = t;
        state = s;
        parsed = 0;
    }
};


struct TokenRef
{
    const char* data;
    uint16_t size;
    Type type;
};

struct TokenConstRef
{
    const char* data;
    uint16_t size;
    Type type;
};


struct ParseResult
{
    enum State {
        Next = 0,
        Repeat = 1,
        Error = 2,
        ErrorEof = 3
    };

    State state;
    ::simplelexer::Error error;
    int parsed;
};


ParseResult parse_next(StateContext* ctx, char c);

const char* str(Type type);
const char* str(State state, Type type);
const char* str(Error err);


}


// ********************************* <Lex> ********************************* //
