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
    llvm::SmallVector<Expr*> Vars;

    while (!Tok.is(Token::eoi))
    {
        // if (Tok.is(Token::KW_int))
        // {
        //     Expr *d;
        //     d = parseDefine();   
        //     if (d)
        //         Vars.push_back();
        //     else
        //         goto _error2;
        // } 
        if (Tok.is(Token::id))
        {
            Expr *d;
            d = parseEquation();
            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error2;
            }
            if (d)
                Vars.push_back(d);
            else
                goto _error2;
        } 
        else if (Tok.is(Token::IF)) 
        {
            Expr *d;
            d = parseIF();   
            if (d)
                Vars.push_back(d);
            else
                goto _error2;
        } 
        else if (Tok.is(Token::loopc))
        {
            Expr *d;
            d = parseLoop();   
            if (d)
                Vars.push_back(d); 
            else
                goto _error2;
        }
    }
    return new Goal(Vars);
    _error2:
    while(Tok.getKind() != Token::eoi)
    advance();
    return nullptr;
}

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

    return new Equation(Token::equal,Vars, E);
_error:
    while(Tok.getKind() != Token::eoi)
    advance();
    return nullptr;
}

Expr *Parser::parseCondition()
{
    Expr *E1, *E2, *_IF;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    if (!Tok.is(Token::IF))
        goto _error;

    advance();
    E1 = parseCondition();

    if (!Tok.isOneOf(Token::not_equal,Token::gt,Token::lt,Token::gte,Token::lte, Token::is_equal))
        goto _error;

    advance();

    E2 = parseCondition();

    if (!Tok.is(Token::colon))
        goto _error;

    advance();

    _IF = parseIF();

    return new Condition(E1, E2, _IF);
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseIF()
{
    Expr *Else = nullptr;
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

    advance();

    while (!Tok.isOneOf(Token::eoi, Token::ELSE))
    {
        if (!expect(Token::ELIF))
            goto _error;
        elifs.push_back(Tok.getText());
        advance();
    }

    if (Tok.is(Token::ELSE))
    {
        Else = Tok.getText();
    }

    return new IF(E1, E2, Else);
}

Expr *Parser::parseElif()
{
    Expr *C;
    llvm::SmallVector<llvm::StringRef, 8> equations;
    if (!Tok.is(Token::ELIF))
        goto _error;

    advance();
    while (!Tok.is(Token::colon))
    {
        advance();
        if (!expect(Token::colon))
            goto _error;
        C= Tok.getText();
        advance();
    }

    advance();
    
    if (!Tok.is(Token::begin))
    {
        goto _error;
    }

    advance();

    while (!Tok.is(Token::end))
    {
        if (!expect(Token::id))
            goto _error;
        equations.push_back(Tok.getText());
        advance();
    }

    return new ELIF(C, equations);
}

Expr *Parser::parseElse()
{
    llvm::SmallVector<llvm::StringRef, 8> equations;
    if (!Tok.is(Token::ELSE))
        goto _error;

    advance();

    if (!Tok.is(Token::colon))
        goto _error;

    advance();

    if (!Tok.is(Token::begin))
        goto _error;

    advance();

    while (!Tok.is(Token::end))
    {
        if (!expect(Token::id))
            goto _error;
        equations.push_back(Tok.getText());
        advance();
    }

    advance();
    
    if (!Tok.is(Token::eoi))
    {
        goto _error;
    }
    
    return new Else(equations);
}

Expr *Parser::parseCompoundCondition()
{
    Expr *E;
    if (!Tok.isOneOf(Token::num, Token::id))
        goto _error;
    
    E = parseExpression();

    while (Tok.isOneOf(Token::AND, Token::OR))
    {
        advance();
        if (!expect(Token::id))
            goto _error;
        equations.push_back(Tok.getText());
        advance();
    }
    
    if (!Tok.is(Token::eoi))
    {
        goto _error;
    }
    
    return new CompoundCondition(equations);
}

Expr *Parser::parseLoop()
{
    Expr *C;
    llvm::SmallVector<llvm::StringRef, 8> equations;
    if (!Tok.is(Token::loopc))
        goto _error;

    advance();

    C = parseCompoundCondition();
    advance();

    if (!Tok.is(Token::colon))
        goto _error;

    advance();

    if (!Tok.is(Token::begin))
        goto _error;

    advance();

    while (!Tok.is(Token::end))
    {
        equations.push_back(Tok.getText());
        advance();
        if (!expect(Token::semicolon))
            goto _error;
        advance();
    }

    return new Loop(C, equations);
}

Expr *Parser::parseEquation()
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
    return new Equation(F, E);
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

Expr *Parser::parseEquation()
{
    Expr *E;
    if (!Tok.is(Token::id))
        goto _error;

    advance();

    if (!Tok.isOneOf(Token::minus_equal, Token::mul_equal, Token::plus_equal, slash_equal, mod_equal, equal))
        goto _error;
        
    advance();

    if (Tok.is(Token::l_paren))
    {
        advance();
        E = parseExpression();
        if (!Tok.is(Token::r_paren))
        {
            goto _error;
        }
        advance();
    }
    else
    {
        goto _error;
    }
    
    return Equation(E);
}

Expr *Parser::parseFinal()
{
    Expr *E;
    if (Tok.is(Token::l_paren))
    {
        advance();
        E = parseExpression();
        if (!Tok.is(Token::r_paren))
        {
            goto _error;
        }
        advance();
    } 
    else if (Tok.isOneOf(Token::id, Token::number))
    {
        E = Tok.getText();
    }
    else
    {
        goto _error;
    }
    
    return Final(E);
}
