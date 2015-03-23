#pragma once

#include <sstream>
#include <list>

#include "Poco/Util/AbstractConfiguration.h"

#include "opdi_constants.h"

#include "AbstractOPDID.h"

///////////////////////////////////////////////////////////////////////////////
// Digital Logic Port
///////////////////////////////////////////////////////////////////////////////

/** A DigitalLogicPort implements logic functions for digital ports. It supports
* the following operations:
* - OR (default): The line is High if at least one of its inputs is High
* - AND: The line is High if all of its inputs are High
* - XOR: The line is High if exactly one of its inputs is High
* - ATLEAST (n): The line is High if at least n inputs are High
* - ATMOST (n): The line is High if at most n inputs are High
* Additionally you can specify whether the output should be negated.
* The DigitalLogicPort requires at least one digital port as input. The output
* can optionally be distributed to an arbitrary number of digital ports.
* Processing occurs in the OPDI waiting event loop, i. e. about once a ms.
* All input ports' state is queried. If the logic function results in a change
* of this port's state the new state is set on the output ports. This means that
* there is no unnecessesary continuous state propagation.
* If the port is not hidden it will perform a self-refresh when the state changes.
* Non-hidden output ports whose change is changed will be refreshed.
* You can also specify inverted output ports who will be updated with the negated
* state of this port.
*/
class OPDID_DigitalLogicPort : public OPDI_DigitalPort {

	enum LogicFunction {
		UNKNOWN,
		OR,
		AND,
		XOR,
		ATLEAST,
		ATMOST
	};

	AbstractOPDID *opdid;
	LogicFunction function;
	int funcN;
	bool negate;
	std::string inputPortStr;
	std::string outputPortStr;
	std::string inverseOutputPortStr;
	typedef std::vector<OPDI_DigitalPort *> PortList;
	PortList inputPorts;
	PortList outputPorts;
	PortList inverseOutputPorts;

protected:

	virtual uint8_t doWork(uint8_t canSend);

	virtual OPDI_DigitalPort *findDigitalPort(std::string setting, std::string portID, bool required);
	
	virtual void findDigitalPorts(std::string setting, std::string portIDs, PortList &portList);

public:
	OPDID_DigitalLogicPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_DigitalLogicPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps);

	virtual void setMode(uint8_t mode);

	virtual void setLine(uint8_t line);

	virtual void prepare();
};
