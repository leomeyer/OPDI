
//

#include <stdio.h>
#include <string>
#include <vector>

#include "Windows.h"

#include "stdafx.h"
#include "WindowsOPDID.h"

// the main OPDI instance is declared here
AbstractOPDID *Opdi;

BOOL WINAPI SignalHandler(_In_ DWORD dwCtrlType) {
	switch (dwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
		std::cout << "Interrupted, exiting" << std::endl;
		Opdi->shutdown();
		return TRUE;
	case CTRL_SHUTDOWN_EVENT:
		return FALSE;
	}
	return FALSE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// set console to display special characters correctly
	setlocale(LC_ALL, "");

	SetConsoleCtrlHandler(SignalHandler, true);

	// convert arguments to UTF8 strings
	std::vector<std::string> args;
	args.reserve(argc);
	for (int i = 0; i < argc; i++) {
		_TCHAR* targ = argv[i];
		std::wstring arg(targ);
		args.push_back(utf8_encode(arg));
	}

	Opdi = new WindowsOPDID();

	try
	{
		return Opdi->startup(args);
	}
	catch (Poco::Exception& e) {
		Opdi->log(e.displayText());
	}

	// signal error
	return 1;
}

