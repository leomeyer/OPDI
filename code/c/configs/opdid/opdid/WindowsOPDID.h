#pragma once

#include "AbstractOPDID.h"

// Utility functions
// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr);

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str);


class WindowsOPDID : public AbstractOPDID
{

public:
	WindowsOPDID(void);

	virtual ~WindowsOPDID(void);

	virtual uint32_t getTimeMs();

	virtual void print(const char *text);

	virtual void println(const char *text);

	int HandleTCPConnection(int *csock);

	int setupTCP(std::string interfaces, int port);

	IOPDIDPlugin *getPlugin(std::string driver);
};


// The plugin DLL entry function that returns the plugin instance
typedef IOPDIDPlugin* (__cdecl *GetOPDIDPluginInstance_t)();