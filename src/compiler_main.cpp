#include <iostream>
#include <string>
#include <fstream>
#include <cstring>

#include "driver.h"

//#define PRINT_TINY
//#define PRINT_TABLE
//#define PRINT_NODES

int main(int argc, char *argv[])
{
    little::Driver driver;
    bool result = false;
    bool livenessAnalysis = false;
    std::string filename;
    
    for (int i=1; i<argc; i++) {
    	if (strcmp(argv[i],"-live") == 0) {
    		driver.setLiveness(true);
    	} else {
    		filename = argv[i];
    	}
    }
	if (filename.empty() == false) {
		std::ifstream is;
		is.open(filename.c_str(), std::ios_base::in);
		result = driver.parse_file(&is);
	}
    else {
    	result = driver.parse_file();
    }
    if (result == true)
    {
    	driver.tinyGeneration();
    	/* Code for printing junk */
    	#ifdef PRINT_TABLE
    		driver.printSymbolTable();
    	#endif
    	
    	#ifdef PRINT_NODES
    		driver.printNodeList(true);
    	#endif
    	#ifdef PRINT_TINY
    		driver.printTinyCode();
    	#endif
    	
    	driver.performLivenessAnalysis();
    }
    else
    {
    	std::cout << "Not accepted" << std::endl;
    }

	return 0;
}

