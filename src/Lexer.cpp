#include "Lexer.h"

// classifying characters
namespace charinfo
{
    // ignore whitespaces
    LLVM_READNONE inline bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\f' || c == '\v' ||
               c == '\r' || c == '\n';
    }

    LLVM_READNONE inline bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    LLVM_READNONE inline bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    LLVM_READNONE inline bool isDoubleOperation(char c)
    {
        return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '!' || c == '<' || c == '>';
    }
}

void Lexer::next(Token &token)
{
    while (*BufferPtr && charinfo::isWhitespace(*BufferPtr))
    {
        ++BufferPtr;
    }
    // make sure we didn't reach the end of input
    if (!*BufferPtr)
    {
        token.Kind = Token::eoi;
        return;
    }
    // collect characters and check for keywords or ident
    if (charinfo::isLetter(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isLetter(*end))
            ++end;
        llvm::StringRef Name(BufferPtr, end - BufferPtr);
        Token::TokenKind kind;
        if (Name == "int")
            kind = Token::KW_int;
        else if (Name == "begin")
            kind = Token::begin;
        else if (Name == "end")
            kind = Token::End;
        else if (Name == "and")
            kind = Token::AND;
        else if (Name == "or")
            kind = Token::OR ;
        else if (Name == "if")
            kind = Token::IF;
        else if (Name == "elif")
            kind = Token::ELIF;
        else if (Name == "else")
            kind = Token::ELSE;
        else if (Name == "loopc")
            kind = Token::loopc;
        else
            kind = Token::id;
        // generate the token
        formToken(token, end, kind);
        return;
    }
    // check for numbers
    else if (charinfo::isDigit(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isDigit(*end))
            ++end;
        formToken(token, end, Token::number);
        return;
    }
    else
    {
        const char *end = BufferPtr + 1;
        // while (charinfo::is(*end))
        //     ++end;
        llvm::StringRef Name(BufferPtr, end - BufferPtr);
        Token::TokenKind kind;
        if (Name == "+") {
            if (end == '=')
                kind = Token::plus_equal;
            else
                kind = Token::plus;
        } else if (Name == "-") {
            if (end == '=')
                kind = Token::minus_equal;
            else
                kind = Token::minus;
        } else if (Name == "*") {
            if (end == '=')
                kind = Token::mul_equal;
            else
                kind = Token::mul;
        } else if (Name == "/") {
            if (end == '=')
                kind = Token::slash_equal;
            else
                kind = Token::slash;
        } else if (Name == ">") {
            if (end == '=')
                kind = Token::gte;
            else
                kind = Token::gt;
        } else if (Name == "<") {
            if (end == '=')
                kind = Token::lte;
            else
                kind = Token::lt;
        } else if (Name == "!") {
            if (end == '=')
                kind = Token::not_equal;
            else
                kind = Token::unknown;
        } else if (Name == "=") {
            if (end == "=")
                kind = Token::is_equal;
            else
                kind = Token::equal;
        } else if (Name == "%") {
            if (end == "=")
                kind = Token::mod_equal;
            else
                kind = Token::mod;
        } else if (Name == ",")
            kind = Token::comma;
        else if (Name == ";")
            kind = Token::semicolon
        else if (Name == ":")
            kind = Token::colon;
        else if (Name == "^")
            kind = Token::power;
        else if (Name == "(")
            kind = Token::l_paren;
        else if (Name == ")")
            kind = Token::r_paren;
        else
            kind = Token::unknown;

        if (isDoubleOperation(Name) && end == '=')
            end++;
        // generate the token
        formToken(token, end, kind);
        return;
    }
}

void Lexer::formToken(Token &Tok, const char *TokEnd,
                      Token::TokenKind Kind)
{
    Tok.Kind = Kind;
    Tok.Text = llvm::StringRef(BufferPtr, TokEnd - BufferPtr);
    BufferPtr = TokEnd;
}
