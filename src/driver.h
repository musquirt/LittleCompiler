/* \file driver.h Declaration of the little::Driver class. */

#ifndef LITTLE_DRIVER_H
#define LITTLE_DRIVER_H

#include <string>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include <sstream>

namespace little {

enum littleTypes
{
	INT,FLOAT,STRING,VOID
} ;

struct VarStruct_s
{
	littleTypes type;
	std::string identifier;
	std::string value;
	// for temp vars, r#; for globals, none; for locals, $-#
	std::string altName;
	bool registerOnly;
};
typedef std::map< int, std::vector< VarStruct_s> > SymbolTable_t;

struct IRNode
{
	std::string opCode;
	std::string op1;
	std::string op2;
	std::string Result;
	int ifFlags; // 0 = None, 1=startsIf, 2=startsElse, 3=endsIf
} ;

struct funcStruct_s {	
	// updating the struct
	std::string name;
	std::vector< VarStruct_s> params;
	littleTypes type; // the return type
	std::string retLoc;
	std::string assVar; // the variable that eventually gets assigned to
	std::vector< std::string> retVals; // return conditions
};

class Driver
{
public:
    Driver();
    virtual ~Driver();
    void setLiveness(bool l);
    class Scanner* lexer;
    bool debug_error;
    bool parse_file();
    bool parse_file(std::istream* is);
    void error(const class location& l, const std::string& m);
    void error(const std::string& m);
    
    // the following are static functions having to do with
    // the symbol table
	void printSymbolTable();
	void insertSymbolTableEntry(littleTypes type, std::string ident, std::string value="");
	void setScope(std::string theScope);
	SymbolTable_t symbolTable;	// the symbol table itself
	std::map< std::string, std::list< IRNode> > functionMap;
	std::string scope;			// the scope
	
	// the following are related to 3-Address Code generation
	std::list< IRNode> nodeList;
	IRNode curNode;
	void pushBackCurNode();
	void printNodeList(bool CommentOut = false);
	std::vector< std::string> treeStack;
	void interpretTree();
	std::string mostRecentTempVar;
	std::string generateLabel();
	std::stack< std::string> labelStack;
	
	// the following are related to Tiny code generation
	void tinyGeneration();
	void printTinyCode();
	
	void pushParams(std::vector< std::string> v);
	void popParams(int s);
	
	// function list stuff
	std::vector< funcStruct_s> fs;
	void createFunction(std::string name);
	void addParamToFunc(std::string name, littleTypes type);
	void addReturnToFunc(littleTypes type);
	void addRetVal(std::string id);
	bool returnExpr;
	void popRetVal();
	
	// Liveness Anaylsis stuff
	void performLivenessAnalysis();
	std::list< IRNode> subNodeList;
	std::list< IRNode> liveNodeList;
private:
	std::vector< std::string> scopeVec;
	littleTypes adjustIROpCode(IRNode &node);
	int tempVarCount;
	int tempLabelCount;
	std::string createTempVar(littleTypes varType);
	void tinyVariableDeclaration();
	void tinyPushRegisters(std::string scope = "main");
	void tinyPopRegisters(std::string scope = "main");
	int  getNumberRegistersUsed(std::string scope);
	void tinyGenerateNormalCode(std::list< IRNode> );
	void tinyGenerateLiveCode();
	std::stringstream tinyStream;
	void interpretTree(std::vector< std::string> &theStack, std::string &lastTouched);
	littleTypes getType(IRNode &node);
	int getNumLocals(std::string scope);
	int getNumLocalsAndTemps(std::string scope);
	void renameVars(std::string &op1, std::string &op2, std::string &op3, std::string scp, funcStruct_s f);
	void findFuncData(std::string s, funcStruct_s &f);
	
	// for liveness
	void functionalLiveness(std::list< IRNode> &nodes);
	bool liveness;
	std::vector< std::string> findGenSet(IRNode n, std::string fname, int &r);
	std::vector< std::string> findKillSet(IRNode n);
	void updateUseSet(std::vector< std::string> genSet,
							std::vector< std::string> killset,
							std::vector< std::string> &liveSet);
	void printLiveSet(std::list< IRNode> nodes,
					std::vector< std::vector< std::string> > live);
	void registerAllocation(std::vector< std::vector< std::string> > &live, std::list< IRNode> &nodes, funcStruct_s &f);
	std::string getNextAvailableRegister(std::map< std::string, std::string>&,
															 std::string);
	std::string getRegisterNumber(std::map< std::string, std::string> &,
															 std::string);
	void adjustNodeForRegisters(IRNode &, 
						std::map< std::string, std::string>&);
	bool isGlobalVariable(std::string s);
	void modifyTempVarAltNames(funcStruct_s f);
	bool isFunctionParameter(std::string s, std::string v);
	void overwriteFuncData(funcStruct_s &f);
};

}

#endif // LITTLE_DRIVER_H
