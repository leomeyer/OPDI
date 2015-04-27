#pragma once

#include "AbstractOPDID.h"

class LinuxOPDID : public AbstractOPDID
{
public:
	LinuxOPDID(void);

	virtual ~LinuxOPDID(void);

	virtual void print(const char *text);

	virtual void println(const char *text);

	virtual void printe(const char *text);

	virtual void printlne(const char *text);

	int HandleTCPConnection(int csock);

	int setupTCP(std::string interfaces, int port);

	IOPDIDPlugin *getPlugin(std::string driver);
};

