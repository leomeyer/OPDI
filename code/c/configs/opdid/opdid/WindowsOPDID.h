#pragma once

#include "AbstractOPDID.h"

// Utility functions
// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr);

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str);

namespace opdid {

class WindowsOPDID : public AbstractOPDID
{

public:
	WindowsOPDID(void);

	virtual ~WindowsOPDID(void);

//	virtual uint32_t getTimeMs();

	virtual void print(const char* text);

	virtual void println(const char* text) override;

	virtual void printe(const char* text);

	virtual void printlne(const char* text) override;

	int HandleTCPConnection(int* csock);

	int setupTCP(std::string interfaces, int port) override;

	IOPDIDPlugin* getPlugin(std::string driver) override;

	virtual std::string getCurrentUser(void) override;

	virtual void switchToUser(std::string newUser) override;
};

// The plugin DLL entry function that returns the plugin instance
typedef IOPDIDPlugin* (__cdecl* GetOPDIDPluginInstance_t)(int majorVersion, int minorVersion, int patchVersion);

}		// namespace opdid

