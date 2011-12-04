/* Implementation of the little::Driver class. */

#include <fstream>
#include <sstream>
#include <cctype>

#include "driver.h"
#include "scanner.h"

#define GLOBAL_SCOPE "0GLOBAL_SCOPE_RESERVED"
#define TEMP_LABEL_PRE "lpTmpLbl"
#define TEMP_VAR_PRE "lpTmpVar"
#define TEMP_VAR_LEN 8
#define MAX_NUM_REGISTERS 4
#define STACK_OFFSET 6

namespace little {

Driver::Driver()
    : debug_error(false), liveness(false)
{
	scope = GLOBAL_SCOPE;
	scopeVec.clear();
	nodeList.clear();
	treeStack.clear();
	tempVarCount = 0;
	tempLabelCount = 0;
	mostRecentTempVar   = "!0!";
	tinyStream.str("");
	returnExpr = false;
	
	functionMap.clear();
	subNodeList.clear();
	//liveNodeList.clear();
}

Driver::~Driver()
{
}

void Driver::setLiveness(bool l) {
	liveness = l;
}

bool Driver::parse_file()
{
    Scanner scanner(&std::cin);
    this->lexer = &scanner;

    Parser parser(*this);
    
    if (parser.parse() == 0)
    {
    	return true;
    }
    else
    {
    	return false;
    }
}

bool Driver::parse_file(std::istream* is) {
	Scanner scanner(is);
    this->lexer = &scanner;

    Parser parser(*this);
    
    if (parser.parse() == 0)
    {
    	return true;
    }
    else
    {
    	return false;
    }
}

void Driver::printSymbolTable()
{
	if (symbolTable.count(0) == 0)
	{
		symbolTable[0].clear(); // no globals
	}
	SymbolTable_t::iterator it;
	for (it = symbolTable.begin(); it != symbolTable.end(); it++)
	{
		if (it != symbolTable.begin())
		{
			std::cout << std::endl;
		}
		// first print the scope
		if (it->first == 0)
		{
			std::cout << "Printing Global Symbol Table" << std::endl;
		}
		else
		{
			std::cout << "Printing Symbol Table for "
				      << scopeVec[it->first-1]
					  << std::endl;
		}
		
		std::vector< VarStruct_s>::iterator varIt;
		for (varIt = it->second.begin(); varIt != it->second.end(); varIt++)
		{
			// print the name
			std::cout << "name: " << varIt->identifier << " type ";
			// print the type
			switch (varIt->type)
			{
				case INT:
				{
					std::cout << "INT ";
					break;
				}
				case FLOAT:
				{
					std::cout << "FLOAT ";
					break;
				}
				case STRING:
				{
					std::cout << "STRING value: ";
					// also print the value for the string
					std::cout << varIt->value;
					break;
				}
			}
			std::cout << std::endl;
		}
	}
	return;
}

void Driver::insertSymbolTableEntry(littleTypes type, std::string ident,
														 std::string value)
{
	// create a new VarStruct_s with given values
	VarStruct_s newData;
	newData.type = type;
	newData.identifier = ident;
	newData.value = value;
	
	// also generate the altName
	if (scope != GLOBAL_SCOPE) {
		if (ident.find(TEMP_VAR_PRE) != std::string::npos) {
			newData.altName = "r" + ident.substr(TEMP_VAR_LEN);
		}
		else {
			std::stringstream tstr;
			tstr << "$-" << symbolTable[scopeVec.size()].size()+1;
			newData.altName = tstr.str();
		}
	}
	else {
		newData.altName = "";
	}
	// push the new data into the SymbolTable
	symbolTable[scopeVec.size()].push_back(newData);
	return;
}

void Driver::setScope(std::string theScope)
{
	scope = theScope;
	// this code creates an entry in the symbol table for
	// this scope, so even if there are no real entries
	// the scope name will still be printed
	scopeVec.push_back(scope);
	symbolTable[scopeVec.size()].clear();
	return;
}

void Driver::error(const class location& l,
		   const std::string& m)
{
	if (debug_error == true)
	{
    	std::cerr << l << ": " << m << std::endl;
	}
}

void Driver::error(const std::string& m)
{
	if (debug_error == true)
	{
    	std::cerr << m << std::endl;
	}
}

void Driver::pushBackCurNode()
{
	if (curNode.opCode != "GE" && curNode.opCode != "LE" &&
		curNode.opCode != "NE" && curNode.opCode != "LABEL" &&
		curNode.opCode != "JUMP" && curNode.opCode != "RETURN" &&
		curNode.opCode != "LINK" && curNode.opCode != "PUSH" &&
		curNode.opCode != "POP" && curNode.opCode != "JSR")
	{
		adjustIROpCode(curNode);
	}
	nodeList.push_back(curNode);
	subNodeList.push_back(curNode);
	
	curNode.opCode = "";
	curNode.op1 = "";
	curNode.op2 = "";
	curNode.Result = "";
	curNode.ifFlags = 0;
	return;
}

void Driver::printNodeList(bool commentOut)
{
	std::list< IRNode>::iterator nodeIt;
	for (nodeIt = nodeList.begin(); nodeIt != nodeList.end(); nodeIt++)
	{
		if (commentOut == true)
		{
			std::cout << "; ";
		}
		if (nodeIt->opCode.empty() != true)
		{
			std::cout << nodeIt->opCode;
			if (nodeIt->op1.empty() != true)
			{
				std::cout << " " << nodeIt->op1;
			}
			if (nodeIt->op2.empty() != true)
			{
				std::cout << " " << nodeIt->op2;
			}
	
			std::cout << " " << nodeIt->Result << std::endl;
		}
	}
	return;
}

void Driver::interpretTree()
{
	std::string eh;
	interpretTree(treeStack, eh);
	return;
}

void Driver::interpretTree(std::vector< std::string> &theStack, std::string &lastTouched)
{
	std::vector< std::string>::iterator it;
	int subExpr = 0;
	std::vector< std::string> subStack;
	int numOpsInParens = 0;
	for (it = theStack.begin(); it != theStack.end(); it++)
	{
		if (*it == "(")
		{
			subExpr++;
		}
		else if (*it == ")")
		{
			subExpr--;
			if (subExpr == 0)
			{
				interpretTree(subStack, lastTouched);
				it = theStack.erase(it-numOpsInParens,it+1);
				theStack.insert(it, mostRecentTempVar);
				subStack.clear();
				numOpsInParens = 0;
			}
		}
		
		if (subExpr != 0)
		{
			if (*it != "(" || subExpr > 1)
			{
				subStack.push_back(*it);
			}
			numOpsInParens++;
		}
	}
	
	for (it = theStack.begin(); it != theStack.end(); it++)
	{
		if (*it == "MULT" || *it == "DIV")
		{
			IRNode newNode;
			newNode.opCode = *it;
			newNode.op1 = *(it-1);
			newNode.op2 = *(it+1);
			littleTypes tempType = adjustIROpCode(newNode);
			newNode.Result = createTempVar(tempType);
			nodeList.push_back(newNode);
			subNodeList.push_back(newNode);
			it = theStack.erase(it-1,it+2);
			theStack.insert(it, mostRecentTempVar);
		}
	}
	
	for (it = theStack.begin(); it != theStack.end(); it++)
	{
		if (*it == "ADD" || *it == "SUB")
		{
			IRNode newNode;
			newNode.opCode = *it;
			newNode.op1 = *(it-1);
			newNode.op2 = *(it+1);
			littleTypes tempType = adjustIROpCode(newNode);
			newNode.Result = createTempVar(tempType);
			nodeList.push_back(newNode);
			subNodeList.push_back(newNode);
			it = theStack.erase(it-1,it+2);
			theStack.insert(it, mostRecentTempVar);
		}
	}
	lastTouched = mostRecentTempVar;
	return;
}

std::string Driver::generateLabel()
{
	std::stringstream tstream;
	tstream << TEMP_LABEL_PRE << tempLabelCount;
	tempLabelCount++;
	labelStack.push(tstream.str());
	return tstream.str();
}

std::string Driver::createTempVar(littleTypes varType)
{
	std::stringstream tstream;
	tstream << TEMP_VAR_PRE << tempVarCount;
	insertSymbolTableEntry(varType, tstream.str());
	tempVarCount++;
	mostRecentTempVar = tstream.str();
	return tstream.str();
}

littleTypes Driver::adjustIROpCode(IRNode &node)
{
	if (getType(node) == FLOAT)
	{
		node.opCode += "F";
		return FLOAT;
	}
	else if (getType(node) == INT)
	{
		node.opCode += "I";
		return INT;
	}
	else {
		return STRING;
	}
	
}

littleTypes Driver::getType(IRNode &node)
{
	std::vector< VarStruct_s> varStructs = symbolTable[scopeVec.size()];
	std::vector< VarStruct_s> globalStructs = symbolTable[0];
	varStructs.insert(varStructs.end(), globalStructs.begin(), globalStructs.end()); // global vars
	for (std::vector< VarStruct_s>::iterator it = varStructs.begin();
			it != varStructs.end(); it++)
	{
		if (it->identifier == node.Result ||
			it->identifier == node.op1    ||
			it->identifier == node.op2)
		{
			// since we don't mix types, we're just
			// going to return after we find what we're
			// looking for
			if (it->type == FLOAT)
			{
				return FLOAT;
			}
			else if (it->type == INT)
			{
				return INT;
			}
			else if (it->type == STRING)
			{
				return STRING;
			}
		}
	}
	
	// if we get here, we'd better damn well check the parameters
	for (int i=0; i<fs.size(); i++) {
		if (fs[i].name == scope) {
			for (int j=0; j<fs[i].params.size(); j++) {
				if (fs[i].params[j].identifier == node.Result ||
						fs[i].params[j].identifier == node.op1    ||
						fs[i].params[j].identifier == node.op2)
				{
					return fs[i].params[j].type;
				}
			}
		}
	}
	
	return INT;
}

void Driver::printTinyCode()
{
	std::cout << tinyStream.str();
}

void Driver::tinyGeneration()
{
	tinyStream.str("");
	// generate variable declaration
	tinyVariableDeclaration();
	// initial push
	tinyStream << "push" << std::endl;
	tinyPushRegisters(GLOBAL_SCOPE);
	tinyStream << "jsr main" << std::endl;
	//tinyPopRegisters(); // apparently not
	tinyStream << "sys halt" << std::endl;
	
	if (!liveness) {
		tinyGenerateNormalCode(nodeList);
	} else {
		tinyGenerateNormalCode(liveNodeList);
	}
	
	tinyStream << "end" << std::endl;
	return;
}

void Driver::tinyVariableDeclaration() // for globals only
{
	std::vector< VarStruct_s>::iterator varIt;
	for (varIt = symbolTable[0].begin();
			varIt != symbolTable[0].end(); varIt++)
	{
		if (varIt->identifier.find(TEMP_VAR_PRE) == std::string::npos)
		{
			if (varIt->type != STRING)
			{
				tinyStream << "var " << varIt->identifier << std::endl;
			}
			else
			{
				tinyStream << "str " << varIt->identifier << " "
						   << varIt->value << std::endl;
			}
		}
	}
	return;
}

void Driver::tinyPushRegisters(std::string scp)
{
	/*// push locals
	if (scp != GLOBAL_SCOPE)
	{
		// first check the local variables
		int scpnm = -1;
		for (int i=0; i<scopeVec.size(); i++) {
			if (scopeVec[i] == scp) {
				scpnm = i+1;
				break;
			}
		}
		if (scpnm != -1) {
			std::vector< VarStruct_s>::iterator vIt;
			for (vIt = symbolTable[scpnm].begin(); 
						vIt != symbolTable[scpnm].end(); vIt++)
			{
				tinyStream << "push " << vIt->altName << std::endl;
			}
		}
	}*/
	for(int i=0; i<MAX_NUM_REGISTERS; i++)
	{
		tinyStream << "push r" << i << std::endl;
	}
	return;
}

void Driver::tinyPopRegisters(std::string scope)
{
	/*// pop locals
	if (scope != GLOBAL_SCOPE)
	{
		// first check the local variables
		int scpnm = -1;
		for (int i=0; i<scopeVec.size(); i++) {
			if (scopeVec[i] == scope) {
				scpnm = i+1;
				break;
			}
		}
		if (scpnm != -1) {
			std::vector< VarStruct_s>::reverse_iterator vIt;
			for (vIt = symbolTable[scpnm].rbegin(); 
						vIt != symbolTable[scpnm].rend(); vIt++)
			{
				tinyStream << "pop " << vIt->altName << std::endl;
			}
		}
	}*/
	for(int i=MAX_NUM_REGISTERS-1; i>=0; i--)
	{
		tinyStream << "pop r" << i << std::endl;
	}
	return;
}

void Driver::renameVars(std::string &op1, std::string &op2, 
						std::string &result, std::string scp, funcStruct_s f) {
	SymbolTable_t::iterator it;
	if (scp != GLOBAL_SCOPE)
	{
		// first check the local variables
		int scpnm = -1;
		for (int i=0; i<scopeVec.size(); i++) {
			if (scopeVec[i] == scp) {
				scpnm = i+1;
				break;
			}
		}
		if (scpnm != -1) {
			std::vector< VarStruct_s>::iterator vIt;
			for (vIt = symbolTable[scpnm].begin(); 
						vIt != symbolTable[scpnm].end(); vIt++)
			{
				if (op1 == vIt->identifier)
				{
					op1 = vIt->altName;
				}
				else if (op2 == vIt->identifier) 
				{
					op2 = vIt->altName;
				}
				else if (result == vIt->identifier) 
				{
					result = vIt->altName;
				}
			}
		}
		
		// second check the parameters
		for (int i=0; i<f.params.size(); i++) {
			if (op1 == f.params[i].identifier)
			{
				op1 = f.params[i].altName;
			}
			else if (op2 == f.params[i].identifier) 
			{
				op2 = f.params[i].altName;
			}
			else if (result == f.params[i].identifier) 
			{
				result = f.params[i].altName;
			}
		}
	}
	return;
}

void Driver::pushParams(std::vector< std::string> v)
{
	for (int i=0; i<v.size(); i++)
	{
		std::vector< VarStruct_s>::iterator it;
		for (it = symbolTable[scopeVec.size()].begin();
					it != symbolTable[scopeVec.size()].end(); it++)
		{
			if (v[i] == it->identifier) {
				// push identifier
				IRNode newNode;
				newNode.opCode = "PUSH";
				newNode.Result = it->identifier;
				nodeList.push_back(newNode);
				subNodeList.push_back(newNode);
			}
		}
	}
	return;
}

void Driver::popParams(int s) {
	for (int i=0; i<s; i++) {
		IRNode newNode;
		newNode.opCode = "POP";
		nodeList.push_back(newNode);
		subNodeList.push_back(newNode);
	}
}

void Driver::tinyGenerateNormalCode(std::list< IRNode> theNodes)
{
	std::list< IRNode>::iterator nodeIt;
	bool startFunction = true;
	std::string cs = GLOBAL_SCOPE;
	funcStruct_s theFunc;
	int numRets = 0;

	for (nodeIt=theNodes.begin(); nodeIt!=theNodes.end(); nodeIt++)
	{
		//tinyStream << nodeIt->opCode << " " << nodeIt->op1 << " " << nodeIt->op2 << " " << nodeIt->Result << std::endl;
		
		std::string op1 = nodeIt->op1;
		std::string op2 = nodeIt->op2;
		std::string result = nodeIt->Result;
		renameVars(op1, op2, result, cs, theFunc);
		// find a temp var that is not any of those
		std::string theTemp = "r0";
		int tempNum = 0;
		bool tempFound = false;
		while (tempFound == false) {
			if (theTemp == op1 || theTemp == op2 || theTemp == result) {
				tempNum++;
				std::stringstream tstr;
				tstr << "r" << tempNum;
				theTemp = tstr.str();
			}
			else {
				tempFound = true;
			}
		}
		
		if (nodeIt->opCode == "LABEL")
		{
			tinyStream << "label " << result << std::endl;

			if (startFunction == true) {
				startFunction = false;
				cs = result;
				findFuncData(result, theFunc);
			}
		}
		else if (nodeIt->opCode.find("STORE") != std::string::npos)
		{
			tinyStream << "push " << theTemp << std::endl;
			tinyStream << "move " << op1 << " " << theTemp
					   << std::endl;
			tinyStream << "move " << theTemp << " "
					   << result << std::endl;
			tinyStream << "pop " << theTemp << std::endl;
		}
		else if (nodeIt->opCode.find("ADD") != std::string::npos)
		{
			// move the first op to the temp register
			tinyStream << "move " << op1 << " "
				       << result << std::endl;
			// add the second op to the first op and store
			// in the temp register
			if (nodeIt->opCode == "ADDI")
			{
				tinyStream << "addi ";
			}
			else
			{
				tinyStream << "addr ";
			}
			tinyStream << op2 << " " << result << std::endl;
		}
		else if (nodeIt->opCode.find("SUB") != std::string::npos)
		{
			// move the first op to the temp register
			tinyStream << "move " << op1 << " "
				       << result << std::endl;
			// add the second op to the first op and store
			// in the temp register
			if (nodeIt->opCode == "SUBI")
			{
				tinyStream << "subi ";
			}
			else
			{
				tinyStream << "subr ";
			}
			tinyStream << op2 << " " << result << std::endl;
		}
		else if (nodeIt->opCode.find("DIV") != std::string::npos)
		{
			// move the first op to the temp register
			tinyStream << "move " << op1 << " "
				       << result << std::endl;
			// add the second op to the first op and store
			// in the temp register
			if (nodeIt->opCode == "DIVI")
			{
				tinyStream << "divi ";
			}
			else
			{
				tinyStream << "divr ";
			}
			tinyStream << op2 << " " << result << std::endl;
		}
		else if (nodeIt->opCode.find("MULT") != std::string::npos)
		{
			// move the first op to the temp register
			tinyStream << "move " << op1 << " "
				       << result << std::endl;
			// add the second op to the first op and store
			// in the temp register
			if (nodeIt->opCode == "MULTI")
			{
				tinyStream << "muli ";
			}
			else
			{
				tinyStream << "mulr ";
			}
			tinyStream << op2 << " " << result << std::endl;
		}
		else if (nodeIt->opCode == "GE")
		{
			if (nodeIt->op2.find(TEMP_VAR_PRE) == std::string::npos)
			{
				tinyStream << "move " << op2 << " "
						   << theTemp << std::endl;
				op2 = theTemp;
			}
			if (getType(*nodeIt) == FLOAT)
			{
				tinyStream << "cmpr ";
			}
			else
			{
				tinyStream << "cmpi ";
			}
			tinyStream << op1 << " " << op2 << std::endl;
			tinyStream << "jge " << result << std::endl;
		}
		else if (nodeIt->opCode == "LE")
		{
			if (nodeIt->op2.find(TEMP_VAR_PRE) == std::string::npos)
			{
				tinyStream << "move " << op2 << " "
						   << theTemp << std::endl;
				op2 = theTemp;
			}
			if (getType(*nodeIt) == FLOAT)
			{
				tinyStream << "cmpr ";
			}
			else
			{
				tinyStream << "cmpi ";
			}
			tinyStream << op1 << " " << op2 << std::endl;
			tinyStream << "jle " << result << std::endl;
		}
		else if (nodeIt->opCode == "NE")
		{
			if (nodeIt->op2.find(TEMP_VAR_PRE) == std::string::npos)
			{
				tinyStream << "move " << op2 << " "
						   << theTemp << std::endl;
				op2 = theTemp;
			}
			if (getType(*nodeIt) == FLOAT)
			{
				tinyStream << "cmpr ";
			}
			else
			{
				tinyStream << "cmpi ";
			}
			tinyStream << op1 << " " << op2 << std::endl;
			tinyStream << "jne " << result << std::endl;
		}
		else if (nodeIt->opCode == "JUMP")
		{
			tinyStream << "jmp " << result << std::endl;
		}
		else if (nodeIt->opCode.find("WRITE") != std::string::npos)
		{
			if (nodeIt->opCode == "WRITEI")
			{
				tinyStream << "sys writei ";
			}
			else if (nodeIt->opCode == "WRITEF")
			{
				tinyStream << "sys writer ";
			}
			else {
				tinyStream << "sys writes ";
			}
			tinyStream << result << std::endl;
		}
		else if (nodeIt->opCode.find("READ") != std::string::npos)
		{
			if (nodeIt->opCode == "READI")
			{
				tinyStream << "sys readi ";
			}
			else
			{
				tinyStream << "sys readr ";
			}
			tinyStream << result << std::endl;
		}
		else if (nodeIt->opCode == "RETURN")
		{
			// move return value to position
			std::string id = theFunc.retVals[numRets];
			if (theFunc.type != VOID && id != "") 
			{
				if (isdigit(id[0]) == true || id[0] == '.') { // literal
					tinyStream << "move " << id << " "
							   << theFunc.retLoc << std::endl;
				}
				else { // variable
					std::string dstr1; //dummy str
					std::string dstr2; //dummy str
					std::string theName = id;
					renameVars(dstr1, dstr2, theName, cs, theFunc);
					// only one can be stack var/mem id and we know
					// that retLoc is a stack var
					tinyStream << "push " << theTemp << std::endl;
					tinyStream << "move " << theName << " "
							   << theTemp  << std::endl;
					tinyStream << "move " << theTemp << " "
							   << theFunc.retLoc << std::endl;
					tinyStream << "pop " << theTemp << std::endl;
				}
			}
			tinyStream << "unlnk" << std::endl;
			tinyStream << "ret" << std::endl;
			if (numRets == theFunc.retVals.size()-1) {
				startFunction = true;
				numRets = 0;
			}
			else {
				numRets++;
			}
		}
		else if (nodeIt->opCode == "JSR") {
			tinyPushRegisters(cs);
			tinyStream << "jsr " << result << std::endl;
			tinyPopRegisters(cs);
		}
		else if (nodeIt->opCode == "PUSH") {
			tinyStream << "push " << result << std::endl;
		}
		else if (nodeIt->opCode == "POP") {
			tinyStream << "pop " << result << std::endl;
		}
		else if (nodeIt->opCode == "LINK") {
			int numLocals = getNumLocals(cs);
			tinyStream << "link " << numLocals << std::endl;

		}
	}
}

int Driver::getNumLocalsAndTemps(std::string scope) {
	int scopeNum = -1;
	for (int i=0; i<scopeVec.size(); i++)
	{
		if (scopeVec[i] == scope)
		{
			scopeNum = i + 1;
			i = scopeVec.size();
		}
	}
	int num = 0;
	if (symbolTable.count(scopeNum) == 1) {
		num = symbolTable[scopeNum].size();
	}
	return num;
}

int Driver::getNumLocals(std::string scope) {
	int scopeNum = -1;
	for (int i=0; i<scopeVec.size(); i++)
	{
		if (scopeVec[i] == scope)
		{
			scopeNum = i + 1;
			i = scopeVec.size();
		}
	}
	int num = 0;
	if (symbolTable.count(scopeNum) == 1) {
		num = symbolTable[scopeNum].size();
		std::vector< VarStruct_s>::iterator it;
		for (it=symbolTable[scopeNum].begin();
					it!=symbolTable[scopeNum].end(); it++) {
			if (it->identifier.find(TEMP_VAR_PRE) != std::string::npos) {
				num--;
			}
		}
	}
	return num;
}

void Driver::createFunction(std::string name) {
	funcStruct_s newfunc;
	newfunc.name = name;
	fs.push_back(newfunc);
	return;
}
void Driver::addParamToFunc(std::string name, littleTypes type) {
	VarStruct_s theVar;
	theVar.identifier = name;
	theVar.type = type;
	std::stringstream tstr;
	tstr << "$" << (6+fs[fs.size()-1].params.size());
	theVar.altName = tstr.str();
	fs[fs.size()-1].params.push_back(theVar);
	return;
}

void Driver::addReturnToFunc(littleTypes type) {
	fs[fs.size()-1].type = type;
	std::stringstream tstr;
	tstr << "$" << (6+fs[fs.size()-1].params.size());
	fs[fs.size()-1].retLoc = tstr.str();
	return;
}

void Driver::findFuncData(std::string s, funcStruct_s &f) {
	for (int i=0; i<fs.size(); i++) {
		if (fs[i].name == s) {
			f = fs[i];
			return;
		}
	}
	return;
}

void Driver::overwriteFuncData(funcStruct_s &f) {
	for (int i=0; i<fs.size(); i++) {
		if (fs[i].name == f.name) {
			fs[i] = f;
			return;
		}
	}
	return;
}

void Driver::addRetVal(std::string id) {
	fs[fs.size()-1].retVals.push_back(id);
	return;
}

void Driver::popRetVal() {
	IRNode newNode;
	newNode.opCode = "POP";
	newNode.Result = fs[fs.size()-1].assVar;
	nodeList.push_back(newNode);
	subNodeList.push_back(newNode);
	return;
}

void Driver::performLivenessAnalysis() {
	if (liveness == false) {
		return;
	}
	
	std::map< std::string, std::list< IRNode> >::iterator it;
	for (it = functionMap.begin(); it != functionMap.end(); it++) {
		functionalLiveness(it->second);
		liveNodeList.insert(liveNodeList.end(), it->second.begin(), it->second.end());
	}
	
	return;
}

void Driver::functionalLiveness(std::list< IRNode> &nodes) {
	// get func data so we can get parameters
	funcStruct_s f;
	findFuncData(nodes.front().Result, f);
	modifyTempVarAltNames(f);
	
	std::vector< std::vector< std::string> > liveVec;
	std::vector< std::string> live;
	
	nodes.reverse();
	std::list< IRNode>::iterator it;
	int returnNumber = f.retVals.size()-1;
	std::stack< std::vector< std::string> > liveVecStack;
	std::stack< bool > elseStack;
	
	for (it=nodes.begin(); it!=nodes.end(); it++) {
		std::vector< std::string> gen;
		std::vector< std::string> kill;
		
		gen = findGenSet(*it, nodes.back().Result, returnNumber);
		kill = findKillSet(*it);
		
		updateUseSet(gen, kill, live);
		
		
		if (it->ifFlags == 1) { // IF
			// merge the current live set and original
			
			if (elseStack.top()) {
				// merge the if and else
				std::vector< std::string> elsePart;
				elsePart = liveVecStack.top();
				liveVecStack.pop();
				
				live.insert(live.end(), elsePart.begin(), elsePart.end());
				
			}
			
			liveVecStack.pop(); // pop original value
			elseStack.pop();
		} else if (it->ifFlags == 2) { // ELSE
			// push back "current" live set
			std::vector< std::string> liveTemp = live;
			live = liveVecStack.top();
			liveVecStack.push(liveTemp);
			elseStack.push(true);
		} else if (it->ifFlags == 3) { // ENDIF
			// push back current live set
			liveVecStack.push(live);
			elseStack.push(false);
		}
		
		liveVec.push_back(live);
	}
	
	liveVec.erase(liveVec.end());
	liveVec.insert(liveVec.begin(),live);
	
	nodes.reverse();
	registerAllocation(liveVec, nodes, f);
	overwriteFuncData(f);
	
	//printLiveSet(nodes, liveVec);
	
	return;
}

void Driver::modifyTempVarAltNames(funcStruct_s f) {
	int num_scope = 0;
	for (int i=0; i<scopeVec.size(); i++) {
		if (scopeVec[i] == f.name) {
			num_scope = i+1;
			break;
		}
	}
	if (symbolTable.count(num_scope) == 1) {
		int tempNum = getNumLocals(f.name)+1;
		for (int i=0; i<symbolTable[num_scope].size(); i++) {
			if (symbolTable[num_scope][i].identifier.find(TEMP_VAR_PRE) !=
					 std::string::npos) {
				std::stringstream tstr;
				tstr << "$-" << tempNum;
				symbolTable[num_scope][i].altName = tstr.str();
				tempNum++;
			}
		}
	}
	return;
}

void Driver::registerAllocation(std::vector< std::vector< std::string> > &live, std::list< IRNode> &nodes, funcStruct_s &f) {
	std::vector< std::vector< std::string> >::reverse_iterator it;
	std::list< IRNode>::iterator nIt;
	std::map< std::string, std::string> regMap;
	int ReturnNum = 0;

	for (it=live.rbegin(), nIt=nodes.begin(); 
			it!=live.rend(), nIt!=nodes.end(); it++, nIt++) {
		std::vector< std::string>::iterator sIt;
		std::stringstream out;
		std::stringstream out1;
		
		std::vector< std::string> liveVars;
		if (it->size() > MAX_NUM_REGISTERS) {
			// redefine liveVars as the last 4 variables
			std::vector< std::string>::iterator liveIt;
			int i = 0;
			for (liveIt=it->begin(); liveIt!=it->end(); liveIt++) {
				if (i >= MAX_NUM_REGISTERS) {
					liveVars.erase(liveVars.begin());
				}
				liveVars.push_back(*liveIt);
				i++;
			}
		} else {
			liveVars = *it;
		}

		for (sIt=liveVars.begin(); sIt!=liveVars.end(); sIt++) {
			out1 << *sIt << ",";
			bool alreadyReg = false;
			std::map< std::string, std::string>::iterator rIt;
			for (rIt=regMap.begin(); rIt!=regMap.end(); rIt++) {
				if (*sIt == rIt->second) {
					alreadyReg = true;
					break;
				}
			}
			if (alreadyReg != true) {
				if (regMap.size() == MAX_NUM_REGISTERS) {
					// spill a register that won't be used
					std::map< std::string, std::string>::iterator rIt;
					for (rIt=regMap.begin(); rIt!=regMap.end(); rIt++) {
						std::string reg = rIt->second;
						if (reg != nIt->op1 && reg != nIt->op2 &&
												 reg != nIt->Result) {
							out << "Spilling " << reg  << std::endl;
							IRNode newNode;
							newNode.opCode = "STOREI";
							newNode.op1    = rIt->first;
							newNode.Result = rIt->second;
							nodes.insert(nIt, newNode);
							regMap.erase(rIt->first);
							break;
						}
					}
				}

				getNextAvailableRegister(regMap, *sIt);
			}


		}
		if (nIt->opCode == "RETURN") {
			// change the retVal to a register if necessary
			std::string retval = f.retVals[ReturnNum];
			if (!f.retVals[ReturnNum].empty() &&
						 !isdigit(f.retVals[ReturnNum][0]) &&
						 f.retVals[ReturnNum][0] != '.') {
				std::string rn = 
						getRegisterNumber(regMap, f.retVals[ReturnNum]);
				if (!rn.empty()) {
					f.retVals[ReturnNum] = rn;
				}
			}
			ReturnNum++;
		}
		IRNode newNode = *nIt;
		adjustNodeForRegisters(newNode, regMap);
		std::list< IRNode>::iterator tIt;
		tIt = nodes.insert(nIt, newNode);
		nodes.erase(nIt);
		nIt = tIt;
		std::stringstream out3;
		out3 << (*nIt).opCode;
		if (!(*nIt).op1.empty()) out3 << " " << (*nIt).op1;
		if (!(*nIt).op2.empty()) out3 << " " << (*nIt).op2;
		if (!(*nIt).Result.empty()) out3 << " " << (*nIt).Result;
		//std::cout << "(" << out1.str() << ")" << std::endl << out3.str()<< std::endl;
	}

	return;
}

void Driver::adjustNodeForRegisters(IRNode &n,
						 std::map< std::string, std::string>& m) {
	// search through n's stuff and swap out the variables for registers
	if (!n.op1.empty()) {
		std::string r = getRegisterNumber(m, n.op1);
		if (r != "") {
			n.op1 = r;
		}
	}
	if (!n.op2.empty()) {
		std::string r = getRegisterNumber(m, n.op2);
		if (r != "") {
			n.op2 = r;
		}
	}
	if (!n.Result.empty()) {
		std::string r = getRegisterNumber(m, n.Result);
		if (r != "") {
			n.Result = r;
		}
	}
	//
	return;
}

std::string Driver::getRegisterNumber(std::map< std::string, std::string> &theMap, std::string s) {
	if (theMap.count("r0") == 1 && theMap["r0"] == s) {
		return "r0";
	} else if (theMap.count("r1") == 1 && theMap["r1"] == s) {
		return "r1";
	} else if (theMap.count("r2") == 1 && theMap["r2"] == s) {
		return "r2";
	} else if (theMap.count("r3") == 1 && theMap["r3"] == s){
		return "r3";
	} else {
		return "";
	}
}

std::string Driver::getNextAvailableRegister(std::map< std::string,
										std::string>& theMap, 
										std::string value) {
	if (theMap.count("r0") == 0) {
		theMap["r0"] = value;
		return "r0";
	} else if (theMap.count("r1") == 0) {
		theMap["r1"] = value;
		return "r1";
	} else if (theMap.count("r2") == 0) {
		theMap["r2"] = value;
		return "r2";
	} else {
		theMap["r3"] = value;
		return "r3";
	}
}

void Driver::printLiveSet(std::list< IRNode> nodes,
					std::vector< std::vector< std::string> > live) {
	std::list< IRNode>::reverse_iterator it;
	std::vector< std::vector< std::string> >::reverse_iterator lIt;
	for (it=nodes.rbegin(), lIt=live.rbegin(); 
				it!=nodes.rend(), lIt!=live.rend(); it++, lIt++) {
		std::stringstream out;
		out << it->opCode << " ";
		if (it->op1 != "") out << it->op1 << " ";
		if (it->op2 != "") out << it->op2 << " ";
		if (it->Result != "") out << it->Result;
		std::cout << out.str() << std::endl;
		out.str("");

		out << "    (";
		std::vector< std::string> v = *lIt;
		for (int i=0; i<v.size(); i++) {
			out << v[i];
			if (i != v.size()-1) {
				out << ",";
			}
		}
		out << ")";
		std::cout << out.str() << std::endl;
	}
}

void Driver::updateUseSet(std::vector< std::string> genSet,
							std::vector< std::string> killSet,
							std::vector< std::string> &liveSet) {
	// first erase any variables in the kill set
	std::vector< std::string>::iterator killIt;
	for (killIt = killSet.begin(); killIt != killSet.end(); killIt++) {
		std::vector< std::string>::iterator it;
		for (it = liveSet.begin(); it != liveSet.end(); it++) {
			if (*it == *killIt) {
				liveSet.erase(it);
				break;
			}
		}
	}
	
	// now add in any variables in the gen set
	std::vector< std::string>::iterator genIt;
	for (genIt = genSet.begin(); genIt != genSet.end();genIt++) {
		bool notFound = true;
		std::vector< std::string>::iterator it;
		for (it = liveSet.begin(); it != liveSet.end(); it++) {
			if (*it == *genIt) {
				std::string temp = *it;
				liveSet.erase(it);
				liveSet.push_back(temp);
				notFound = false;
				break;
			}
		}
		if (notFound) {
			liveSet.push_back(*genIt);
		}
	}
	
	return;
}

bool Driver::isGlobalVariable(std::string s) {
	std::vector< VarStruct_s>::iterator it;
	for (it=symbolTable[0].begin(); it!=symbolTable[0].end(); it++) {
		if (it->identifier == s) {
			return true;
		}
	}
	return false;
}

bool Driver::isFunctionParameter(std::string s, std::string v) {
	funcStruct_s f;
	findFuncData(s, f);
	for (int i=0; i<f.params.size(); i++) {
		if (v == f.params[i].identifier) {
			return true;
		}
	}
	return false;
}

std::vector< std::string> Driver::findGenSet(IRNode n, std::string f, int &r) {
	
	std::vector< std::string> v;
	if (n.opCode.find("ADD") != std::string::npos ||
			n.opCode.find("SUB") != std::string::npos ||
			n.opCode.find("MUL") != std::string::npos ||
			n.opCode.find("DIV") != std::string::npos) {
		// no literals
		if (isdigit(n.op1[0]) == false &&
				n.op1[0] != '.' && !isGlobalVariable(n.op1)
				&& !isFunctionParameter(f, n.op1)) {
			v.push_back(n.op1);
		}
		if (isdigit(n.op2[0]) == false &&
				n.op2[0] != '.' && !isGlobalVariable(n.op2)
				&& !isFunctionParameter(f, n.op2)) {
			v.push_back(n.op2);
		}
	} else if (n.opCode.find("STORE") != std::string::npos) {
		if (isdigit(n.op1[0]) == false &&
				n.op1[0] != '.'  && !isGlobalVariable(n.op1)
				&& !isFunctionParameter(f, n.op1)) {
			v.push_back(n.op1);
		}
	} else if (n.opCode.find("WRITE") != std::string::npos) {
		if (isdigit(n.Result[0]) == false &&
				n.Result[0] != '.' && !isGlobalVariable(n.Result)
				&& !isFunctionParameter(f, n.Result)) {
			v.push_back(n.Result);
		}
	} else if (n.opCode.find("PUSH") != std::string::npos) {
		if (n.Result != "" && !isFunctionParameter(f, n.Result) &&
				!isGlobalVariable(n.Result)) {
			v.push_back(n.Result);
		}
	} else if (n.opCode.find("RETURN") != std::string::npos) {
		funcStruct_s g;
		findFuncData(f, g);
		std::string t = g.retVals[r];
		if (t != "" && isdigit(t[0]) == false &&
				t[0] != '.' && !isGlobalVariable(t)
				&& !isFunctionParameter(f, t)) {
			v.push_back(t);
		}
		r--;
	}
	
	return v;
}

std::vector< std::string> Driver::findKillSet(IRNode n) {
	std::vector< std::string> v;
	if (n.opCode.find("STORE") != std::string::npos) {
		v.push_back(n.Result);
	} else if (n.opCode.find("READ") != std::string::npos) {
		v.push_back(n.Result);
	} else if (n.opCode.find("ADD") != std::string::npos ||
					n.opCode.find("SUB") != std::string::npos ||
					n.opCode.find("MUL") != std::string::npos ||
					n.opCode.find("DIV") != std::string::npos) {
		v.push_back(n.Result);
	} else if (n.opCode.find("POP") != std::string::npos) {
		v.push_back(n.Result);
	}
	
	return v;
}

} // namespace little
