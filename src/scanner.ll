/** Define the little Flex lexical scanner */

%{ 
	/* C++ Declarations */
	#include <string>
	#include "../src/scanner.h"

	/* import the parser's token type into a local typedef */
	typedef little::Parser::token token;
	typedef little::Parser::token_type token_type;

	#define SAVE_TOKEN yylval->sval = new std::string(yytext, yyleng)

	/* redefine yyterminate */
	#define yyterminate() return token::FIN
	#define yywrap() 1

	/* The C++ scanner uses STL streams. */
	#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/

/* enable c++ scanner class generation */
%option c++

/* change the name of the scanner class. results in "LittleFlexLexer" */
%option prefix="Little"

/* the manual says "somewhat more optimized" */
%option batch

/* enable scanner to generate debug output. disable this for release
 * versions. */
%option debug

/* no support for include files is planned */
%option noyywrap nounput 

/* enables the use of start condition stacks */
%option stack

/* The following paragraph suffices to track locations accurately. Each time
 * yylex is invoked, the begin position is moved onto the end position. */
%{
#define YY_USER_ACTION  yylloc->columns(yyleng);
%}

%% /*** Regular Expressions Part ***/

 /* code to place at the beginning of yylex() */
%{
    // reset location
    yylloc->step();
%}

    /* RULES SECTION */
    /* COMMENTS */
--.*    continue;
    /* KEYWORDS */
PROGRAM  return token::PROGRAM;
BEGIN    return token::TBEGIN;
END      return token::END;
FUNCTION return token::TFUNCTION;
READ     return token::READ;
WRITE    return token::WRITE;
IF       return token::IF;
THEN     return token::THEN;
ELSE     return token::ELSE;
ENDIF    return token::ENDIF;
RETURN   return token::RETURN;
DO       return token::DO;
WHILE    return token::WHILE;
FLOAT    return token::FLOAT;
INT      return token::INT;
VOID     return token::VOID;
STRING   return token::STRING;       

    /* OPERATORS */
:=  return token::ASSIGN;
\+  return token::PLUS; 
-   return token::MINUS; 
\*  return token::MULT; 
\/  return token::DIV; 
=   return token::ISEQ; 
\<  return token::LESSTHAN; 
>   return token::GREATERTHAN; 
\(  return token::LPAREN; 
\)  return token::RPAREN; 
;   return token::SEMICOLON; 
,   return token::COMMA; 

    /* IDENTIFIERS */
[0-9A-Za-z]{32,}            return token::TERROR;
[0-9]+[A-Za-z]+             return token::TERROR;
[0-9]*\.[0-9]+[A-Za-z\.]	return token::TERROR;
[0-9]*\.[0-9]+              SAVE_TOKEN; return token::FLOATLITERAL;
[0-9]{1,}                   SAVE_TOKEN; return token::INTLITERAL;
[A-Za-z][A-Za-z0-9]{0,30}   SAVE_TOKEN; return token::IDENTIFIER;
\"[^\"\r\n]*\"              SAVE_TOKEN; return token::STRINGLITERAL;

\n                          continue;
.                           continue;


%% /*** Additional Code ***/

namespace little {

Scanner::Scanner(std::istream* in,
		 std::ostream* out)
    : LittleFlexLexer(in, out)
{
}

Scanner::~Scanner()
{
}

void Scanner::set_debug(bool b)
{
    yy_flex_debug = b;
}

}

/* This implementation of LittleFlexLexer::yylex() is required to fill the
 * vtable of the class LittleFlexLexer. We define the scanner's main yylex
 * function via YY_DECL to reside in the Scanner class instead. */

#ifdef yylex
#undef yylex
#endif

int LittleFlexLexer::yylex()
{
    std::cerr << "in LittleFlexLexer::yylex() !" << std::endl;
    return 0;
}

/* When the scanner receives an end-of-file indication from YY_INPUT, it then
 * checks the yywrap() function. If yywrap() returns false (zero), then it is
 * assumed that the function has gone ahead and set up `yyin' to point to
 * another input file, and scanning continues. If it returns true (non-zero),
 * then the scanner terminates, returning 0 to its caller. */

/*int LittleFlexLexer::yywrap()
{
    return 1;
}*/
