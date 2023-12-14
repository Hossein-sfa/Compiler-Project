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
        if (Tok.is(Token::KW_int))
         {
            Expr *d;
            d = parseDefine();   
            if (d)
                 Vars.push_back();
             else
                goto _error1;
         } 
        if (Tok.is(Token::id))
        {
            Expr *d;
            d = parseEquation();
            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error1;
            }
            if (d)
                Vars.push_back(d);
            else
                goto _error1;
        } 
        else if (Tok.is(Token::IF)) 
        {
            Expr *d;
            d = parseIF();   
            if (d)
                Vars.push_back(d);
            else
                goto _error1;
        } 
        else if (Tok.is(Token::loopc))
        {
            Expr *d;
            d = parseLoop();   
            if (d)
                Vars.push_back(d); 
            else
                goto _error1;
        }
        advance();
    }
    return new Goal(Vars);

 _error1:
    while(Tok.getKind() != Token::eoi)
    advance();
    return nullptr;
}

Expr *Parser::parseDefine()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    if (expect(Token::KW_int))
        goto _error2;

    advance();

    if (expect(Token::id))
        goto _error2;
    Vars.push_back(Tok.getText());
    advance();

    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::id))
            goto _error2;
        Vars.push_back(Tok.getText());
        advance();
    }

    if (Tok.is(Token::equal))
    {
        advance();
        E = parseExpression();
    }

    if (expect(Token::semicolon))
        goto _error2;

    return new Define(Token::equal,Vars, E);

_error2:
    while(Tok.getKind() != Token::eoi)
    advance();
    return nullptr;
}

Expr *Parser::parseCondition()
{
    Expr *E1, *E2, *_IF;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    if (!Tok.is(Token::IF))
        goto _error3;

    advance();
    E1 = parseCondition();

    if (!Tok.isOneOf(Token::not_equal,Token::gt,Token::lt,Token::gte,Token::lte, Token::is_equal))
        goto _error3;

    advance();

    E2 = parseCondition();

    if (!Tok.is(Token::colon))
        goto _error3;

    advance();

    _IF = parseIF();

    return new Condition(E1, E2, _IF);

 _error3:  
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}


Expr *Parser::parseIF()
{
    Equation *E;
    llvm::SmallVector<Expr *> assigns;
    if (expect(Token::begin))
    {
        goto _error4;
    }

    advance();

    while (Tok.is(Token::id))
    {
        E = (Equation *)parseAssign();
        Factor *f = E->getLeft();
        Factor *t = (Factor *)E->getRight();
        if (E)
        {
            assigns.push_back(E);
        }
        else
        {
            error();
            return nullptr;
        }
    }

    if (expect(Token::end))
    {
        goto _error4;
    }

    advance();
    return new IF(assigns);

_error4: 
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}


Expr *Parser::parseCompoundCondition()
{
    Expr *E;
    if (!Tok.isOneOf(Token::number, Token::id))
        goto _error5;
    
    E = parseExpression();

    while (Tok.isOneOf(Token::AND, Token::OR))
    {
        advance();
        if (!expect(Token::id))
            goto _error5;
        equations.push_back(Tok.getText());
        advance();
    }
    
    if (!Tok.is(Token::eoi))
    {
        goto _error5;
    }
    
    return new CompoundCondition(equations);

 _error5: 
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}



Expr *Parser::parseLoop()
{
    Expr *C;
    IF *I;
    llvm::SmallVector<llvm::StringRef, 8> equations;
    if (!Tok.is(Token::loopc))
        goto _error6;

    advance();

    C = parseCompoundCondition();

    if (expect(Token::colon))
        goto _error6;
    advance();

    I = (IF*)(parseIF());

    return new Loop(C, I);

_error6: 
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseEquation()
{
    Expr *E;
    Factor *F;
    F = (Factor *)(parseFactor());


    if (!Tok.is(Token::equal) && !Tok.is(Token::plus_equal) && !Tok.is(Token::minus_equal) &&
        !Tok.is(Token::mod_equal) && !Tok.is(Token::mul_equal) && !Tok.is(Token::slash_equal))
    {
        error();
        return nullptr;
    }

    advance();
    E = parseExpr();

    if (expect(Token::semicolon))
        goto _error7;

    advance();

    return new Assignment(F, E);
    
_error7: 
        while (Tok.getKind() != Token::eoi)
            advance();
        return nullptr;
}

Expr *Parser::parseExpression()
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
    while (Tok.isOneOf(Token::mul, Token::slash,Token::mod))
    {
        BinaryOp::Operator Op =
            Tok.is(Token::mul) ? BinaryOp::Mul : (Tok.is(Token::mod? BinaryOp::mod:BinaryOp::Div));
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
        BinaryOp::Operator Op =BinaryOp::power;
        Expr *Right = parseFinal();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseEquation()
{
    Expr *E;
    if (!Tok.is(Token::id))
        goto _error8;

    advance();

    if (!Tok.isOneOf(Token::minus_equal, Token::mul_equal, Token::plus_equal, Token::slash_equal, Token::mod_equal, Token::equal))
        goto _error8;
        
    advance();

    if (Tok.is(Token::l_paren))
    {
        advance();
        E = parseExpression();
        if (!Tok.is(Token::r_paren))
        {
            goto _error8;
        }
        advance();
    }
    else
    {
        goto _error8;
    }
    
    return new Equation(E);

 _error8:
        while (Tok.getKind() != Token::eoi)
            advance();
        return nullptr;
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
            goto _error9;
        }
        advance();
    } 
    else if (Tok.isOneOf(Token::id, Token::number))
    {
        E = Tok.getText();
    }
    else
    {
        goto _error9;
    }
    
    return Final(E);

 _error9:
        while (Tok.getKind() != Token::eoi)
            advance();
        return nullptr;
}
