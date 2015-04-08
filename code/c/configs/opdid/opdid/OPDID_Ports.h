#pragma once

#include <sstream>
#include <list>

#include <exprtk.hpp>

#include "Poco/Util/AbstractConfiguration.h"

#include "opdi_constants.h"

#include "AbstractOPDID.h"


class OPDID_PortFunctions {
protected:
	typedef std::vector<OPDI_Port *> PortList;
	typedef std::vector<OPDI_DigitalPort *> DigitalPortList;
	typedef std::vector<OPDI_AnalogPort *> AnalogPortList;

	AbstractOPDID *opdid;

	virtual OPDI_Port *findPort(std::string configPort, std::string setting, std::string portID, bool required);

	virtual void findPorts(std::string configPort, std::string setting, std::string portIDs, PortList &portList);

	virtual OPDI_DigitalPort *findDigitalPort(std::string configPort, std::string setting, std::string portID, bool required);
	
	virtual void findDigitalPorts(std::string configPort, std::string setting, std::string portIDs, DigitalPortList &portList);

	virtual OPDI_AnalogPort *findAnalogPort(std::string configPort, std::string setting, std::string portID, bool required);
	
	virtual void findAnalogPorts(std::string configPort, std::string setting, std::string portIDs, AnalogPortList &portList);

	virtual OPDI_SelectPort *findSelectPort(std::string configPort, std::string setting, std::string portID, bool required);
};

///////////////////////////////////////////////////////////////////////////////
// Logic Port
///////////////////////////////////////////////////////////////////////////////

/** A LogicPort implements logic functions for digital ports. It supports
* the following operations:
* - OR (default): The line is High if at least one of its inputs is High
* - AND: The line is High if all of its inputs are High
* - XOR: The line is High if exactly one of its inputs is High
* - ATLEAST (n): The line is High if at least n inputs are High
* - ATMOST (n): The line is High if at most n inputs are High
* Additionally you can specify whether the output should be negated.
* The LogicPort requires at least one digital port as input. The output
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
class OPDID_LogicPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	enum LogicFunction {
		UNKNOWN,
		OR,
		AND,
		XOR,
		ATLEAST,
		ATMOST
	};

	LogicFunction function;
	size_t funcN;
	bool negate;
	std::string inputPortStr;
	std::string outputPortStr;
	std::string inverseOutputPortStr;
	DigitalPortList inputPorts;
	DigitalPortList outputPorts;
	DigitalPortList inverseOutputPorts;

	virtual uint8_t doWork(uint8_t canSend);

public:
	OPDID_LogicPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_LogicPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps);

	virtual void setMode(uint8_t mode);

	virtual void setLine(uint8_t line);

	virtual void prepare();
};


///////////////////////////////////////////////////////////////////////////////
// Pulse Port
///////////////////////////////////////////////////////////////////////////////

/** A pulse port generates a digital pulse with a defined period (measured
* in milliseconds) and a duty cycle in percent. The period and duty cycle
* can optionally be set by analog ports. The period is in this case 
* calculated as the percentage of Period. The pulse is active if the line
* of this port is set to High. If enable digital ports are specified the
* pulse is also being generated if at least one of the enable ports is High.
* The output can be normal or inverted. There are two lists of output digital
* ports which receive the normal or inverted output respectively.
*/
class OPDID_PulsePort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	bool negate;
	int32_t period;
	double dutyCycle;
	int8_t disabledState;
	std::string periodPortStr;
	std::string dutyCyclePortStr;
	std::string enablePortStr;
	std::string outputPortStr;
	std::string inverseOutputPortStr;
	typedef std::vector<OPDI_DigitalPort *> PortList;
	DigitalPortList enablePorts;
	DigitalPortList outputPorts;
	DigitalPortList inverseOutputPorts;
	OPDI_AnalogPort *periodPort;
	OPDI_AnalogPort *dutyCyclePort;

	// state
	uint8_t pulseState;
	uint64_t lastStateChangeTime;

	virtual uint8_t doWork(uint8_t canSend);

public:
	OPDID_PulsePort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_PulsePort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps);

	virtual void setMode(uint8_t mode);

	virtual void prepare();
};

///////////////////////////////////////////////////////////////////////////////
// Selector Port
///////////////////////////////////////////////////////////////////////////////

/** A SelectorPort is a DigitalPort that is High when the specified select port
*   is in the specified position and Low otherwise. If set to High it will switch
*   the select port to the specified position. If set to Low, it will do nothing.
*/
class OPDID_SelectorPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	bool negate;
	std::string selectPortStr;
	OPDI_SelectPort *selectPort;
	uint16_t position;

	virtual uint8_t doWork(uint8_t canSend);

public:
	OPDID_SelectorPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_SelectorPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps);

	virtual void setMode(uint8_t mode);

	virtual void setLine(uint8_t line);

	virtual void prepare();
};

///////////////////////////////////////////////////////////////////////////////
// Expression Port
///////////////////////////////////////////////////////////////////////////////

/** An ExpressionPort is a DigitalPort that sets the value of other ports
*   depending on the result of a calculation expression.
*   The expression is evaluated in each doWork iteration if the ExpressionPort is enabled,
*   i. e. its digital state is High (default). 
*   The expression can refer to port IDs (input variables). Although port IDs are case-
*   insensitive (a restriction of the underlying library), it is recommended to use the correct case.
*   The rules for the different port types are:
*    - A digital port is evaluated as 1 or 0 (High or Low).
*    - An analog port's relative value is evaluated in the range 0..1.
*    - A dial port's value is evaluated to its 64-bit value.
*    - A select port's value is its current item position.
*   The expression is evaluated using the ExprTk library. It queries the current state
*   of all ports to make their values available to the expression formula.
*   If a port state cannot be queried (e. g. due to an exception) the expression is not evaluated.
*   The resulting value can be assigned to a number of output ports.
*   The rules for the different port types are:
*    - A digital port is set to Low if the value is 0 and to High otherwise.
*    - An analog port's relative value is set to the range 0..1.
*      If the value is < 0, it is assumed as 0. If the value is > 1, it is assumed as 1.
*    - A dial port's value is set by casting the value to a 64-bit signed integer.
*    - A select port's position is set by casting the value to a 16-bit unsigned int.
*/
#ifdef OPDI_USE_EXPRTK

class OPDID_ExpressionPort : public OPDI_DigitalPort, protected OPDID_PortFunctions {
protected:
	std::string expressionStr;

	std::vector<double> portValues;	// holds the values of the ports for the expression evaluation

	std::string outputPortStr;
	PortList outputPorts;

	typedef exprtk::symbol_table<double> symbol_table_t;
	typedef exprtk::expression<double> expression_t;
	typedef exprtk::parser<double> parser_t;
	typedef parser_t::dependent_entity_collector::symbol_t symbol_t;
	std::deque<symbol_t> symbol_list;

	symbol_table_t symbol_table;
	expression_t expression;

	virtual bool prepareVariables(void);

	virtual uint8_t doWork(uint8_t canSend);

public:
	OPDID_ExpressionPort(AbstractOPDID *opdid, const char *id);

	virtual ~OPDID_ExpressionPort();

	virtual void configure(Poco::Util::AbstractConfiguration *config);

	virtual void setDirCaps(const char *dirCaps) override;

	virtual void setMode(uint8_t mode) override;

	virtual void prepare() override;
};

#endif // def OPDI_USE_EXPRTK