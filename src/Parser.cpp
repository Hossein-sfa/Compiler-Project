#include "Parser.h"

// main point is that the whole input has been consumed
AST *Parser::parse()
{
    AST *Res = parseGoal();
    return Res;
}

AST *Parser::parseGoal()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;

    while (!Tok.is(Token::eoi))
    {
        if (Tok.is(Token::KW_int))
        {
            Expr *d;
            d = parseDefine();   
            if (d)
                exprs.push_back(d);
            else
                goto _error2;
        } 
        else if (Tok.is(Token::id))
        {
            Expr *d;
            d = parseEquation();   
            if (d)
                exprs.push_back(d);
            else
                goto _error2;
        } 
        else if (Tok.is(Token::if)) 
        {
            Expr *d;
            d = parseIF();   
            if (d)
                exprs.push_back(d);
            else
                goto _error2;
        } 
        else if (Tok.is(Token::loopc))
        {
            Expr *d;
            d = parseLoop();   
            if (d)
                exprs.push_back(d);
            else
                goto _error2;
        }
    }
}

// AST *Parser::parseExpr()
// {
//     llvm::SmallVector<Expr *> exprs;
//     while (!Tok.is(Token::semicolon))
//     {
//         switch (Tok.getKind())
//         {
//         case Token::KW_int:
//             Expr *d;
//             d = parseDec();   
//             if (d)
//                 exprs.push_back(d);
//             else
//                 goto _error2;
//             break;
//         case Token::ident:
//             Expr *a;
//             a = parseAssign();

//             if (!Tok.is(Token::semicolon))
//             {
//                 error();
//                 goto _error2;
//             }
//             if (a)
//                 exprs.push_back(a);
//             else
//                 goto _error2;
//             break;
//         default:
//             goto _error2;
//             break;
//         }
//         advance(); // TODO: watch this part
//     }
//     return new GSM(exprs);
// _error2:
//     while (Tok.getKind() != Token::eoi)
//         advance();
//     return nullptr;
// }

Expr *Parser::parseDefine()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    if (!Tok.is(Token::KW_int))
        goto _error;

    advance();

    if (expect(Token::id))
        goto _error;
    Vars.push_back(Tok.getText());
    advance();

    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::id))
            goto _error;
        Vars.push_back(Tok.getText());
        advance();
    }

    if (Tok.is(Token::equal))
    {
        advance();
        E = parseExpression();
    }

    if (expect(Token::semicolon))
        goto _error;

    return new Equation(Vars, E);
}

Expr *Parser::parseCondition()
{
    Expr *E1, *E2, *_IF;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    if (!Tok.is(Token::if))
        goto _error;

    advance();
    E1 = parseCondition();

    if (!Tok.isOneOF(Token::not_equal,Token::gt,Token::lt,Token::gte,Token::lte, Token::is_equal))
        goto _error;

    advance();

    E2 = parseCondition();

    if (!Tok.is(Token::colon))
        goto _error;

    advance();

    _IF = parseIF();

    return new Condition(E1, E2, -IF);
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseIF()
{
    Expr *E1, *E2, *E3;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    if (!Tok.is(Token::begin))
        goto _error;

    advance();
    E1 = parseEquation();

    if (!expect(Token::end))
        goto _error;

    advance();

    E2 = parseElif();
    E3 = parseElse();

    return new parseIF(E1, E2, E3);
}

Expr *Parser::parseIF()
{
    Expr *Else;
    llvm::SmallVector<llvm::StringRef, 8> equations, elifs;
    if (!Tok.is(Token::begin))
        goto _error;

    advance();
    while (!Tok.is(Token::end))
    {
        equations.push_back(Tok.getText());
        advance();
        if (!expect(Token::id))
            goto _error;
        equations.push_back(Tok.getText());
        advance();
    }

    if (!expect(Token::elif))
        goto _error;

    advance();

    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::ident))
            goto _error;
        Vars.push_back(Tok.getText());
        advance();
    }
    E2 = parseElif();
    E3 = parseElse();

    return new IF(E1, E2, E3);
}

Expr *Parser::parseDec()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    if (!Tok.is(Token::KW_type))
        goto _error;

    advance();
    if (expect(Token::KW_int))
        goto _error;

    advance();

    if (expect(Token::ident))
        goto _error;
    Vars.push_back(Tok.getText());
    advance();

    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::ident))
            goto _error;
        Vars.push_back(Tok.getText());
        advance();
    }

    if (Tok.is(Token::equal))
    {
        advance();
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

    advance();
    E = parseExpr();
    return new Assignment(F, E);
}

Expr *Parser::parseExpression()
{
    Expr *Left = parseTerm();
    while (Tok.isOneOf(Token::plus, Token::minus))
    {
        Equation::Operator Op =
            Tok.is(Token::plus) ? Equation::Plus : Equation::Minus;
        advance();
        Expr *Right = parseTerm();
        Left = new Equation(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseTerm()
{
    Expr *Left = parseFactor();
    while (Tok.isOneOf(Token::mul, Token::slash,Token::mod))
    {
        Equation::Operator Op =
            Tok.is(Token::mul) ? Equation::Mul : (Tok.is(Token::mod?Equation::mod:Equation::Div));
        advance();
        Expr *Right = parseFactor();
        Left = new Equation(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor()
{
    Expr *Left = parseFinal();
    while (Tok.is(Token::power))
    {
        Equation::Operator Op =Equation::power;
        Expr *Right = parseFinal();
        Left = new Equation(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFinal()
{
    Expr *Res = nullptr;
    switch (Tok.getKind())
    {
    case Token::number:
        Res = new Factor(Factor::Number, Tok.getText());
        advance();
        break;
    case Token::ident:
        Res = new Factor(Factor::Id, Tok.getText());
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
        while (!Tok.isOneOf(Token::r_paren, Token::star, Token::plus, Token::minus, Token::slash, Token::eoi))
            advance();
        break;
    }
    return Res;
}
