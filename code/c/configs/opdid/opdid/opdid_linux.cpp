#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

#include "opdi_constants.h"

#include "LinuxOPDID.h"

// the main OPDI instance is declared here
AbstractOPDID *Opdi;

void signal_handler(int s) {
	std::cout << "Interrupted, exiting" << std::endl;

	// tell the OPDI system to shut down
	if (Opdi != NULL)
		Opdi->shutdown();
}

void signal_handler_term(int s) {
	std::cout << "Terminated, exiting" << std::endl;

	// tell the OPDI system to shut down
	if (Opdi != NULL)
		Opdi->shutdown();
}

int main(int argc, char *argv[], char *envp[])
{
	Opdi = new LinuxOPDID();

	// install Ctrl+C intercept handler
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	sigIntHandler.sa_handler = signal_handler_term;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGTERM, &sigIntHandler, NULL);

	// convert arguments to vector list
	std::vector<std::string> args;
	args.reserve(argc);
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}

	// create a map of environment parameters (important: uppercase, prefix $)
	std::map<std::string, std::string> environment;
	size_t counter = 0;
	char *env = envp[counter];
	while (env) {
		std::string envStr(env);
		char *envVar = strtok(&envStr[0], "=");
		if (envVar != NULL) {
			char *envValue = strtok(NULL, "");
			if (envValue != NULL) {
				std::string envKey(envVar);
				// convert key to uppercase
				std::transform(envKey.begin(), envKey.end(), envKey.begin(), toupper);
				environment[std::string("$") + envKey] = std::string(envValue);
			}
		}
		counter++;
		env = envp[counter];
	}

	int exitcode;
	try
	{
		exitcode = Opdi->startup(args, environment);
	}
	catch (Poco::Exception& e) {
		Opdi->logError(e.displayText());
		// signal error
		exitcode = OPDI_DEVICE_ERROR;
	}
	catch (std::exception& e) {
		Opdi->logError(e.what());
		exitcode = OPDI_DEVICE_ERROR;
	}
	catch (...) {
		Opdi->logError("An unknown error occurred. Exiting.");
		exitcode = OPDI_DEVICE_ERROR;
	}

	Opdi->logNormal("OPDID exited with code " + Opdi->to_string(exitcode));

	return exitcode;
}
