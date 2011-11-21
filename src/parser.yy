/* Contains the little Bison parser source */

%{ 
	/* C++ Declarations */
	#include <cstdio>
	#include <string>
	#include <vector>
%}

/*** yacc/bison Declarations ***/

/* Require bison 2.3 or later */
%require "2.3"

/* 3 shift/reduce errors I couldn't fix. */
%expect 3

/* write out a header file containing the token defines */
%defines

/* use newer C++ skeleton file */
%skeleton "lalr1.cc"

/* namespace to enclose parser in */
%name-prefix="little"

/* set the parser's class identifier */
%define "parser_class_name" "Parser"

/* keep track of the current position within the input */
%locations

/* The driver is passed by reference to the parser and to the scanner. This
 * provides a simple but effective pure interface, not relying on global
 * variables. */
%parse-param { class Driver& driver }

%union {
    std::string* sval;
    int ltype;
};

/* TERMINALS */
%token <sval> IDENTIFIER
%token <sval>  INTLITERAL
%token <sval>  FLOATLITERAL
%token <sval>  STRINGLITERAL
%token FIN 0 "end of file"
%token PROGRAM SEMICOLON 
%token COMMA END TERROR
%token TBEGIN TFUNCTION READ WRITE
%token IF THEN ELSE ENDIF RETURN DO WHILE
%token FLOAT INT VOID STRING
%left LESSTHAN GREATERTHAN ISEQ
%left LPAREN RPAREN PLUS MINUS MULT DIV
%right ASSIGN

%type <sval>  id str id_list id_list_part assign_head
%type <ltype> var_type any_type func_begin

%{

	#include "../src/driver.h"
	#include "../src/scanner.h"

	/* this "connects" the bison parser in the driver to the flex scanner class
	 * object. it defines the yylex() function call to pull the next token
	 * from the current lexer object of the driver context. */
	#undef yylex
	#define yylex driver.lexer->lex
	
	std::vector< std::string> multiVars;
	bool dontPush = false;
	bool last_stmt = false;
	int numCommas = 0;
%}

%% /* RULES */
    /* Program */
program : PROGRAM id TBEGIN pgm_body END {};
id : IDENTIFIER {$$ = $1; };
pgm_body : decl func_declarations {  }
			| decl {  };
decl : decl string_decl_list {  }
            | decl var_decl_list {  }
            | /* empty */ ;

    /* Global String Declaration */
string_decl_list : string_decl_list string_decl_tail {}
            | string_decl_tail {} ;
string_decl_tail : STRING id ASSIGN str SEMICOLON {
				 driver.insertSymbolTableEntry(STRING, *$2, *$4);
				};
str : STRINGLITERAL {$$ = $1; };

    /* Variable Declaration */
var_decl_list : var_decl_list var_decl_tail {}
            | var_decl_tail {};
var_decl_tail : var_type id_list_part SEMICOLON {
					for (int i=0; i<multiVars.size(); i++)
					{
						driver.insertSymbolTableEntry((littleTypes)$1,
															 multiVars[i]);
					}
					multiVars.clear();
				};
var_type : FLOAT  { $$ = FLOAT;
					multiVars.clear(); }
            | INT { $$ = INT; 
                    multiVars.clear(); };
any_type : var_type { $$ = $1; 
					  multiVars.clear(); }
            | VOID { $$ = VOID; 
            		 multiVars.clear(); };
id_list_part : id_list id { multiVars.push_back(*$2); $$ = $2; }
            | id { multiVars.push_back(*$1); $$ = $1; } ;
id_list : id_list id COMMA { multiVars.push_back(*$2); }
            | id COMMA { multiVars.push_back(*$1); };

    /* Function Paramater List */
param_decl_list : param_decl param_decl_tail {  } ;
param_decl : var_type id { driver.addParamToFunc(*$2, (littleTypes)$1); } ;
param_decl_tail : COMMA param_decl param_decl_tail {}
            | /* empty */;

    /* Function Declarations */
func_declarations : func_decl func_declarations {}
            | func_decl {};
func_decl : func_begin LPAREN param_decl_list RPAREN TBEGIN func_body END {
					driver.addReturnToFunc((littleTypes)$1); }
            | func_begin LPAREN RPAREN TBEGIN func_body END
              { driver.addReturnToFunc((littleTypes)$1); };
func_begin : TFUNCTION any_type id {driver.setScope(*$3);
				driver.curNode.opCode = "LABEL";
				driver.curNode.Result = *$3; 
				driver.pushBackCurNode();
				driver.curNode.opCode = "LINK";
				driver.curNode.Result = "0"; 
				driver.pushBackCurNode();
				driver.createFunction(*$3); 
				$$ = $2; };
func_body : decl stmt_list { if (last_stmt == false) { // for funcs w/o return
								driver.curNode.opCode = "RETURN";
								driver.pushBackCurNode();
								driver.addRetVal("");
							 }
							 driver.functionMap[driver.scope] = 
							 					driver.subNodeList;
							 driver.subNodeList.clear(); };

    /* Statement List */
stmt_list : stmt stmt_list {}
            | /* empty */;
stmt : assign_stmt { if (dontPush != true) driver.pushBackCurNode();
					 else { dontPush = false;
					 		driver.curNode.opCode = "";
					 		driver.curNode.op1 = "";
					 		driver.curNode.op2 = "";
					 		driver.curNode.Result = ""; }
					 driver.treeStack.clear();
					 multiVars.clear();
					 last_stmt = false; }
            | read_stmt { driver.pushBackCurNode();
            			driver.treeStack.clear();
            			multiVars.clear();
            			last_stmt = false; }
            | write_stmt { driver.pushBackCurNode();
            			driver.treeStack.clear();
            			multiVars.clear();
            			last_stmt = false; }
            | return_stmt { driver.treeStack.clear();
            			multiVars.clear();
            			last_stmt = true; }
            | if_stmt { driver.treeStack.clear();
            			multiVars.clear();
            			last_stmt = false; }
            | do_stmt { driver.treeStack.clear();
            			multiVars.clear();
            			last_stmt = false; };

    /* Basic Statements */
assign_stmt : assign_expr SEMICOLON {  };
assign_expr : assign_head assign_expr_butt { driver.curNode.Result = *$1; };
assign_head : id { driver.fs[driver.fs.size()-1].assVar = *$1; };
assign_expr_butt : ASSIGN expr { driver.interpretTree();
				driver.curNode.opCode = "STORE";
				driver.curNode.op1 = driver.treeStack[0];
				driver.treeStack.clear(); };
read_stmt : READ LPAREN id_list_part RPAREN SEMICOLON { 
				for (int i=0; i<multiVars.size()-1; i++)
				{
					driver.curNode.opCode = "READ";
					driver.curNode.Result = multiVars[i];
					driver.pushBackCurNode();
				}
				driver.curNode.opCode = "READ";
				driver.curNode.Result = *$3; };
write_stmt : WRITE LPAREN id_list_part RPAREN SEMICOLON { 
				for (int i=0; i<multiVars.size()-1; i++)
				{
					driver.curNode.opCode = "WRITE";
					driver.curNode.Result = multiVars[i];
					driver.pushBackCurNode();
				}
				driver.curNode.opCode = "WRITE";
				driver.curNode.Result = *$3;
				 };
return_stmt : return_head return_butt { 
						driver.interpretTree();
						driver.returnExpr = false;
						driver.addRetVal(driver.treeStack[0]);
						driver.treeStack.clear();
						driver.curNode.opCode = "RETURN";
						driver.pushBackCurNode();
						 };
return_head : RETURN {  };
return_butt : expr SEMICOLON { };

    /* Expressions */
expr : factor expr_tail {  };
expr_tail : addop factor expr_tail {  }
            | /* empty */;
factor : postfix_expr factor_tail {  };
factor_tail : mulop postfix_expr factor_tail {  }
            | /* empty */;
postfix_expr : primary {  }
            | call_expr { driver.curNode.opCode = "JSR";
            			  driver.pushBackCurNode();
            			  driver.popParams(multiVars.size());
            			  // pop return value
            			  driver.popRetVal();
						  multiVars.clear(); };
call_expr : id LPAREN expr_list RPAREN { 
						driver.curNode.opCode = "PUSH";
						driver.curNode.op1 = "";
						driver.curNode.op2 = "";
						driver.curNode.Result = "";
						driver.pushBackCurNode();
						driver.pushParams(multiVars);
						driver.curNode.Result = *$1;
						dontPush = true;
					}
            | id LPAREN RPAREN { driver.curNode.Result = *$1;
            					 dontPush = true; };
expr_list : expr expr_list_tail { 
								  if (numCommas == 0 &&
								  		driver.treeStack.empty() == false)
								  {
								      driver.interpretTree();
								      multiVars.clear();
								      multiVars.push_back(driver.treeStack[0]);
								  }
								  numCommas = 0; };
expr_list_tail : COMMA expr expr_list_tail { 
						driver.interpretTree();
						multiVars.push_back(driver.treeStack[0]);
						for (int i=numCommas; i<multiVars.size()-1; i++) {
							multiVars.erase(multiVars.begin());
						}
						numCommas++;
						}
            | /* empty */;
start_primary_paren : LPAREN { driver.treeStack.push_back("("); };
primary : start_primary_paren expr RPAREN { driver.treeStack.push_back(")"); }
            | id { driver.treeStack.push_back(*$1);
            	   multiVars.push_back(*$1);   }
            | INTLITERAL { driver.treeStack.push_back(*$1);
            	   multiVars.push_back(*$1); }
            | FLOATLITERAL { driver.treeStack.push_back(*$1);
            	   multiVars.push_back(*$1); };
addop : PLUS { driver.treeStack.push_back("ADD"); }
            | MINUS { driver.treeStack.push_back("SUB"); };
mulop : MULT { driver.treeStack.push_back("MULT"); } 
            | DIV { driver.treeStack.push_back("DIV"); };

    /* Complex Statements and Condition */ 
just_if : IF { driver.curNode.Result = driver.generateLabel(); };
if_stmt : just_if LPAREN cond RPAREN THEN stmt_list else_part ENDIF { 
				driver.curNode.opCode = "LABEL";
				driver.curNode.Result = driver.labelStack.top();
				driver.labelStack.pop();
				driver.pushBackCurNode(); };
else_part : just_else stmt_list {  }
            | /* empty */ {  };
just_else : ELSE{ std::string tempLabel = driver.labelStack.top();
				driver.labelStack.pop();
				driver.curNode.Result = driver.generateLabel();
				driver.curNode.opCode = "JUMP";
				driver.pushBackCurNode();
				driver.curNode.opCode = "LABEL";
				driver.curNode.Result = tempLabel;
				driver.pushBackCurNode(); };
cond : expr compop expr {
			driver.interpretTree();
			driver.curNode.op1 = driver.treeStack[0];
			driver.curNode.op2 = driver.treeStack[1];
			driver.curNode.Result = driver.labelStack.top();
			driver.pushBackCurNode();
			driver.treeStack.clear(); };
compop : LESSTHAN { driver.curNode.opCode = "GE"; }
            | GREATERTHAN { driver.curNode.opCode = "LE"; } 
            | ISEQ { driver.curNode.opCode = "NE"; };
just_do : DO { driver.curNode.Result = driver.generateLabel();
				driver.curNode.opCode = "LABEL";
				driver.pushBackCurNode();
				driver.curNode.Result = driver.generateLabel(); };
do_stmt : just_do stmt_list WHILE LPAREN cond RPAREN SEMICOLON { 
				std::string tempLabel = driver.labelStack.top();
				driver.labelStack.pop();
				driver.curNode.opCode = "JUMP";
				driver.curNode.Result = driver.labelStack.top();
				driver.labelStack.pop();
				driver.pushBackCurNode();
				driver.curNode.opCode = "LABEL";
				driver.curNode.Result = tempLabel;
				driver.pushBackCurNode(); };

%% /* Additional Code */

void little::Parser::error(const Parser::location_type& l,
			    const std::string& m)
{
    driver.error(l, m);
}
