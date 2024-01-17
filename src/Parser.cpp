#include "Parser.h"
#include "commonVar.h"
#include <vector>
#include <iostream>
#include <map>
#include <stack>
#include <set>

std::vector<std::string> tokens;
std::map<std::string, std::vector<std::string>> dependant;
std::vector<std::string> my_stack;
std::set<std::string> lives;

// main point is that the whole input has been consumed
AST *Parser::parse()
{
    AST *Res = parseGSM();
    return Res;
}

AST *Parser::parseGSM()
{
    llvm::SmallVector<Expr *> exprs;
    while (!Tok.is(Token::eoi))
    {
        switch (Tok.getKind())
        {
        case Token::KW_int:
            Expr *d;
            d = parseDec();
            if (d)
                exprs.push_back(d);
            else
                goto _error2;
            tokens.push_back(";");
            break;
        case Token::ident:
            Expr *a;
            a = parseAssign();

            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error2;
            }
            tokens.push_back(";");
            if (a)
                exprs.push_back(a);
            else
                goto _error2;
            break;
        default:
            goto _error2;
            break;
        }
        advance(); // TODO: watch this part
    }

    while (tokens.size())
    {
        std::string current = tokens.front();
        std::vector<std::string> dependencies;
        std::string variable;
        variable = tokens.front();
        tokens.erase(tokens.begin());
        if (tokens.front() != ";")
            tokens.erase(tokens.begin());
        current = tokens.front();
        while (current != ";")
        {
            dependencies.push_back(current);
            tokens.erase(tokens.begin());
            current = tokens.front();
        }
        tokens.erase(tokens.begin());
        dependant[variable] = dependencies;
    }

    my_stack = std::vector<std::string>();
    my_stack.push_back("result");

    while (1)
    {
        std::string current = my_stack.back();
        my_stack.pop_back();
        lives.insert(current);
        for (auto i : dependant[current])
        {
            my_stack.push_back(i);
        }
        if (!my_stack.size())
            break;
    }

    for (auto i : lives)
        std::cout << i << " ";

    return new GSM(exprs);
_error2:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseDec()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;

    if (!Tok.is(Token::KW_int))
        goto _error;

    advance();

    if (expect(Token::ident))
        goto _error;
    Vars.push_back(Tok.getText());
    tokens.push_back(Tok.getText().str());
    advance();

    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::ident))
            goto _error;
        // To Do
        Vars.push_back(Tok.getText());
        advance();
    }

    if (Tok.isOneOf(Token::equal, Token::semicolon))
    {
        if (Tok.is(Token::equal))
        {
            advance();
            tokens.push_back("=");
        }
        E = parseExpr();
    }

    if (expect(Token::semicolon))
        goto _error;

    return new Declaration(Vars, E);
_error: // TODO: Check this later in case of error :)
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseAssign()
{
    Expr *E;
    Factor *F;
    F = (Factor *)(parseFactor());

    if (!Tok.is(Token::equal))
    {
        error();
        return nullptr;
    }
    tokens.push_back("=");
    advance();
    E = parseExpr();
    return new Assignment(F, E);
}

Expr *Parser::parseExpr()
{
    Expr *Left = parseTerm();
    while (Tok.isOneOf(Token::plus, Token::minus))
    {
        BinaryOp::Operator Op =
            Tok.is(Token::plus) ? BinaryOp::Plus : BinaryOp::Minus;
        advance();
        Expr *Right = parseTerm();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseTerm()
{
    Expr *Left = parseFactor();
    while (Tok.isOneOf(Token::star, Token::slash))
    {
        BinaryOp::Operator Op =
            Tok.is(Token::star) ? BinaryOp::Mul : BinaryOp::Div;
        advance();
        Expr *Right = parseFactor();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor()
{
    Expr *Res = nullptr;
    std::string str = "0";
    Res = new Factor(Factor::Number, llvm::StringRef(str));
    switch (Tok.getKind())
    {
    case Token::number:
        Res = new Factor(Factor::Number, Tok.getText());
        tokens.push_back(Tok.getText().str());
        advance();
        break;
    case Token::ident:
        Res = new Factor(Factor::Ident, Tok.getText());
        tokens.push_back(Tok.getText().str());
        advance();
        break;
    case Token::l_paren:
        advance();
        Res = parseExpr();
        if (!consume(Token::r_paren))
            break;
    default: // error handling
        if (!Res)
            error();
        while (!Tok.isOneOf(Token::r_paren, Token::star, Token::plus, Token::minus, Token::slash, Token::eoi, Token::semicolon))
            advance();
        break;
    }
    return Res;
}