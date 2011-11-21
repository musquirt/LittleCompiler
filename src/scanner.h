#ifndef LITTLE_SCANNER_H
#define LITTLE_SCANNER_H

// Flex expects the signature of yylex to be defined in the macro YY_DECL, and
// the C++ parser expects it to be declared. We can factor both as follows.
#ifndef YY_DECL
#define	YY_DECL						\
    little::Parser::token_type				\
    little::Scanner::lex(				\
	little::Parser::semantic_type* yylval,		\
	little::Parser::location_type* yylloc		\
    )
#endif

#ifndef __FLEX_LEXER_H
#define yyFlexLexer LittleFlexLexer
#include "FlexLexer.h"
#undef yyFlexLexer
#endif

#include "../generated/parser.tab.hh"

namespace little {

class Scanner : public LittleFlexLexer
{
public:
    Scanner(std::istream* arg_yyin = 0,
	    std::ostream* arg_yyout = 0);

    virtual ~Scanner();

    /* yylex, my old enemy, we meet again... */
    virtual Parser::token_type lex(Parser::semantic_type* yylval, 
    		Parser::location_type* yylloc);

    /** Enable debug output (via arg_yyout) if compiled into the scanner. */
    void set_debug(bool b);
};

} // namespace little

#endif // LITTLE_SCANNER_H
