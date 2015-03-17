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

	try
	{
		return Opdi->startup(args);
	}
	catch (Poco::Exception& e) {
		Opdi->log(e.displayText());
	}
	catch (...) {
		Opdi->log("An unknown error occurred. Exiting.");
	}
}

