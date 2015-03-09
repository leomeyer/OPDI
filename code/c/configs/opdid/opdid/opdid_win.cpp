
//

#include <stdio.h>
#include <string>
#include <vector>

#include "Windows.h"

#include "stdafx.h"
#include "WindowsOPDID.h"

// the main OPDI instance is declared here
AbstractOPDID *Opdi;

int _tmain(int argc, _TCHAR* argv[])
{
	// set console to display special characters correctly
	setlocale(LC_ALL, "");

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

