#ifndef BANK_HPP_
#define BANK_HPP_

#include "csdl.h"
#include <vector>
#include "Program.hpp"

using namespace std;

class Bank
{
public:
	Bank(CSOUND *csound, char *bankName);
	virtual ~Bank();

	char *name;
	int bankNum;

	vector<Program> programs;

	int previousProgram;
	int currentProgram;

	void initializeGM();
};

#endif /*BANK_HPP_*/
