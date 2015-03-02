#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

#include "LinuxOPDID.h"

// the main OPDI instance is declared here
AbstractOPDID *Opdi;

int main(int argc, char *argv[])
{
	// convert arguments to vector list
	std::vector<std::string> args;
	args.reserve(argc);
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}

	Opdi = new LinuxOPDID();

	Opdi->sayHello();

	try
	{
		return Opdi->startup(args);
	}
	catch (Poco::Exception& e) {
		std::cout << e.displayText() << std::endl;
	}
}

