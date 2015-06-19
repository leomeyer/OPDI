#pragma once

#include "AbstractOPDID.h"

class OPDID_PortFunctions {
friend class AbstractOPDID;

protected:
	typedef std::vector<OPDI_Port *> PortList;
	typedef std::vector<OPDI_DigitalPort *> DigitalPortList;
	typedef std::vector<OPDI_AnalogPort *> AnalogPortList;

	AbstractOPDID *opdid;
	AbstractOPDID::LogVerbosity logVerbosity;

	OPDID_PortFunctions();

	virtual OPDI_Port *findPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void findPorts(std::string configPort, std::string setting, std::string portIDs, PortList &portList);

	virtual OPDI_DigitalPort *findDigitalPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void findDigitalPorts(std::string configPort, std::string setting, std::string portIDs, DigitalPortList &portList);

	virtual OPDI_AnalogPort *findAnalogPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void findAnalogPorts(std::string configPort, std::string setting, std::string portIDs, AnalogPortList &portList);

	virtual OPDI_SelectPort *findSelectPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void logWarning(std::string message);

	virtual void logNormal(std::string message);

	virtual void logVerbose(std::string message);

	virtual void logDebug(std::string message);

	virtual void logExtreme(std::string message);
};

