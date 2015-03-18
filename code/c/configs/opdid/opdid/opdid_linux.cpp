#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

#include "LinuxOPDID.h"

// the main OPDI instance is declared here
AbstractOPDID *Opdi;

void signal_handler(int s){
	printf("Caught signal %d\n", s);
	
	// tell the OPDI system to shut down
	if (Opdi != NULL)
		Opdi->shutdown();
}

int main(int argc, char *argv[])
{
	// install Ctrl+C intercept handler
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);
	
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

