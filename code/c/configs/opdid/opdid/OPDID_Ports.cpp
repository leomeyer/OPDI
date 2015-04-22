
#include "opdi_port.h"
#include "opdi_platformfuncs.h"

#include "OPDID_Ports.h"

///////////////////////////////////////////////////////////////////////////////
// PortFunctions
///////////////////////////////////////////////////////////////////////////////

OPDI_Port *OPDID_PortFunctions::findPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	return port;
}

void OPDID_PortFunctions::findPorts(std::string configPort, std::string setting, std::string portIDs, PortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "") {
			OPDI_Port *port = this->findPort(configPort, setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

OPDI_DigitalPort *OPDID_PortFunctions::findDigitalPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_DIGITAL[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (OPDI_DigitalPort *)port;
}

void OPDID_PortFunctions::findDigitalPorts(std::string configPort, std::string setting, std::string portIDs, DigitalPortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "") {
			OPDI_DigitalPort *port = this->findDigitalPort(configPort, setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

OPDI_AnalogPort *OPDID_PortFunctions::findAnalogPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_ANALOG[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not an analog port: " + portID);

	return (OPDI_AnalogPort *)port;
}

void OPDID_PortFunctions::findAnalogPorts(std::string configPort, std::string setting, std::string portIDs, AnalogPortList &portList) {
	// split list at blanks
	std::stringstream ss(portIDs);
	std::string item;
	while (std::getline(ss, item, ' ')) {
		// ignore empty items
		if (item != "") {
			OPDI_AnalogPort *port = this->findAnalogPort(configPort, setting, item, true);
			if (port != NULL)
				portList.push_back(port);
		}
	}
}

OPDI_SelectPort *OPDID_PortFunctions::findSelectPort(std::string configPort, std::string setting, std::string portID, bool required) {
	// locate port by ID
	OPDI_Port *port = this->opdid->findPortByID(portID.c_str());
	// no found but required?
	if (port == NULL) { 
		if (required)
			throw Poco::DataException(configPort + ": Port required by setting " + setting + " not found: " + portID);
		return NULL;
	}

	// port type must be Digital
	if (port->getType()[0] != OPDI_PORTTYPE_SELECT[0])
		throw Poco::DataException(configPort + ": Port specified in setting " + setting + " is not a digital port: " + portID);

	return (OPDI_SelectPort *)port;
}


///////////////////////////////////////////////////////////////////////////////
// Logic Port
///////////////////////////////////////////////////////////////////////////////

OPDID_LogicPort::OPDID_LogicPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {

	this->opdid = opdid;
	this->function = UNKNOWN;
	this->funcN = -1;
	this->negate = false;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	// set the line to an invalid state
	// this will trigger the detection logic in the first call of doWork
	// so that all dependent output ports will be set to a defined state
	this->line = -1;
}

OPDID_LogicPort::~OPDID_LogicPort() {
}

void OPDID_LogicPort::configure(Poco::Util::AbstractConfiguration *config) {
	std::string function = config->getString("Function", "OR");

	try {
		if (function == "OR")
			this->function = OR;
		else
		if (function == "AND")
			this->function = AND;
		else
		if (function == "XOR")
			this->function = XOR;
		else {
			std::string prefix("ATLEAST");
			if (!function.compare(0, prefix.size(), prefix)) {
				this->function = ATLEAST;
				this->funcN = atoi(function.substr(prefix.size() + 1).c_str());
			} else {
				std::string prefix("ATMOST");
				if (!function.compare(0, prefix.size(), prefix)) {
					this->function = ATMOST;
					this->funcN = atoi(function.substr(prefix.size() + 1).c_str());
				}
			}
		}
	} catch (...) {
		throw Poco::DataException(std::string("Syntax error for LogicPort ") + this->getID() + ", Function setting: Expected OR, AND, XOR, ATLEAST <n> or ATMOST <n>");
	}

	if (this->function == UNKNOWN)
		throw Poco::DataException("Unknown function specified for LogicPort: " + function);
	if ((this->function == ATLEAST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATLEAST of LogicPort: " + function);
	if ((this->function == ATMOST) && (this->funcN <= 0))
		throw Poco::DataException("Expected positive integer for function ATMOST of LogicPort: " + function);

	this->negate = config->getBool("Negate", false);

	this->inputPortStr = config->getString("InputPorts", "");
	if (this->inputPortStr == "")
		throw Poco::DataException("Expected at least one input port for LogicPort");

	this->outputPortStr = config->getString("OutputPorts", "");
	this->inverseOutputPortStr = config->getString("InverseOutputPorts", "");
}

void OPDID_LogicPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a LogicPort cannot be changed");
}

void OPDID_LogicPort::setLine(uint8_t line) {
	throw PortError(std::string(this->getID()) + ": The line of a LogicPort cannot be set directly");
}

void OPDID_LogicPort::prepare() {
	OPDI_DigitalPort::prepare();

	if (this->function == UNKNOWN)
		throw PortError(std::string("Logic function is unknown; port not configured correctly: ") + this->id);

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "InputPorts", this->inputPortStr, this->inputPorts);
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->getID(), "InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);
}

uint8_t OPDID_LogicPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// count how many input ports are High
	size_t highCount = 0;
	DigitalPortList::iterator it = this->inputPorts.begin();
	while (it != this->inputPorts.end()) {
		uint8_t mode;
		uint8_t line;
		try {
			(*it)->getState(&mode, &line);
		} catch (Poco::Exception &e) {
			this->opdid->log(std::string("Querying port ") + (*it)->getID() + ": " + e.message());
		}
		highCount += line;
		it++;
	}

	// evaluate function
	uint8_t newLine = (this->negate ? 1 : 0);

	switch (this->function) {
	case UNKNOWN:
		return OPDI_STATUS_OK;
	case OR: if (highCount > 0) 
				 newLine = (this->negate ? 0 : 1);
		break;
	case AND: if (highCount == this->inputPorts.size()) 
				  newLine = (this->negate ? 0 : 1);
		break;
	case XOR: if (highCount == 1) 
				newLine = (this->negate ? 0 : 1);
		break;
	case ATLEAST: if (highCount >= this->funcN) 
					  newLine = (this->negate ? 0 : 1);
		break;
	case ATMOST: if (highCount <= this->funcN) 
					 newLine = (this->negate ? 0 : 1);
		break;
	}

	// change detected?
	if (newLine != this->line) {
		if (opdid->logVerbosity >= AbstractOPDID::DEBUG)
			opdid->log(std::string(this->id) + ": Detected line change (" + this->to_string(highCount) + " of " + this->to_string(this->inputPorts.size()) + " inputs port are High");
	
		OPDI_DigitalPort::setLine(newLine);

		// regular output ports
		DigitalPortList::iterator it = this->outputPorts.begin();
		while (it != this->outputPorts.end()) {
			try {
				uint8_t mode;
				uint8_t line;
				// get current output port state
				(*it)->getState(&mode, &line);
				// changed?
				if (line != newLine) {
					if (opdid->logVerbosity >= AbstractOPDID::DEBUG)
						opdid->log(std::string(this->id) + ": Changing line of port " + (*it)->getID() + " to " + (newLine == 0 ? "Low" : "High"));
					(*it)->setLine(newLine);
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
		// inverse output ports
		it = this->inverseOutputPorts.begin();
		while (it != this->inverseOutputPorts.end()) {
			try {
				uint8_t mode;
				uint8_t line;
				// get current output port state
				(*it)->getState(&mode, &line);
				// changed?
				if (line == newLine) {
					if (opdid->logVerbosity >= AbstractOPDID::DEBUG)
						opdid->log(std::string(this->id) + ": Changing line of inverse port " + (*it)->getID() + " to " + (newLine == 0 ? "High" : "Low"));
					(*it)->setLine((newLine == 0 ? 1 : 0));
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Pulse Port
///////////////////////////////////////////////////////////////////////////////

OPDID_PulsePort::OPDID_PulsePort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {

	this->opdid = opdid;
	this->negate = false;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	// set the pulse state to an invalid state
	// this will trigger the detection logic in the first call of doWork
	// so that all dependent output ports will be set to a defined state
	this->pulseState = -1;
	this->lastStateChangeTime = 0;
	this->disabledState = -1;
}

OPDID_PulsePort::~OPDID_PulsePort() {
}

void OPDID_PulsePort::configure(Poco::Util::AbstractConfiguration *config) {
	this->negate = config->getBool("Negate", false);

	std::string portLine = config->getString("Line", "");
	if (portLine == "High") {
		this->setLine(1);
	} else if (portLine == "Low") {
		this->setLine(0);
	} else if (portLine != "")
		throw Poco::DataException("Unknown Line specified; expected 'Low' or 'High'", portLine);

	std::string disabledState = config->getString("DisabledState", "");
	if (disabledState == "High") {
		this->disabledState = 1;
	} else if (disabledState == "Low") {
		this->disabledState = 0;
	} else if (disabledState != "")
		throw Poco::DataException("Unknown DisabledState specified; expected 'Low' or 'High'", disabledState);

	this->enablePortStr = config->getString("EnablePorts", "");
	this->outputPortStr = config->getString("OutputPorts", "");
	this->inverseOutputPortStr = config->getString("InverseOutputPorts", "");

	this->period = config->getInt("Period", -1);
	if (this->period <= 0)
		throw Poco::DataException("Specify a positive integer value for the Period setting of a PulsePort: " + this->to_string(this->period));
	this->periodPortStr = config->getString("PeriodPort", "");
	
	// duty cycle is specified in percent
	this->dutyCycle = config->getDouble("DutyCycle", 50) / 100.0;
	this->dutyCyclePortStr = config->getString("DutyCyclePort", "");
}

void OPDID_PulsePort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a PulsePort cannot be changed");
}

void OPDID_PulsePort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a PulsePort cannot be changed");
}

void OPDID_PulsePort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "EnablePorts", this->enablePortStr, this->enablePorts);
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);
	this->findDigitalPorts(this->getID(), "InverseOutputPorts", this->inverseOutputPortStr, this->inverseOutputPorts);

	this->periodPort = this->findAnalogPort(this->getID(), "PeriodPort", this->periodPortStr, false);
	this->dutyCyclePort = this->findAnalogPort(this->getID(), "DutyCyclePort", this->dutyCyclePortStr, false);
}

uint8_t OPDID_PulsePort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// default: pulse is enabled if the line is High
	bool enabled = (this->line == 1);

	if (this->enablePorts.size() > 0) {
		// count how many enable ports are High
		int highCount = 0;
		DigitalPortList::iterator it = this->enablePorts.begin();
		while (it != this->enablePorts.end()) {
			uint8_t mode;
			uint8_t line;
			try {
				(*it)->getState(&mode, &line);
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Querying port ") + (*it)->getID() + ": " + e.message());
			}
			highCount += line;
			it++;
		}
		// pulse is enabled if there is at least one EnabledPort with High
		enabled |= highCount > 0;
	}

	int32_t period = this->period;
	// period port specified?
	if (this->periodPort != NULL) {
		// query period port for the current relative value
		period = static_cast<int32_t>((double)this->period * this->periodPort->getRelativeValue());
	}

	// duty cycle port specified?
	if (this->dutyCyclePort != NULL) {
		// query duty cycle port for the current relative value
		this->dutyCycle = this->dutyCyclePort->getRelativeValue();
	}

	// determine new line level 
	uint8_t newState = this->pulseState;

	if (enabled) {
		// check whether the time for state change has been reached
		uint64_t timeDiff = opdi_get_time_ms() - this->lastStateChangeTime;
		// current state (logical) Low?
		if (this->pulseState == (this->negate ? 1 : 0)) {
			// time up to High reached?
			if (timeDiff > period * (1.0 - this->dutyCycle)) 
				// switch to (logical) High
				newState = (this->negate ? 0 : 1);
		} else {
			// time up to Low reached?
			if (timeDiff > period * this->dutyCycle)
				// switch to (logical) Low
				newState = (this->negate ? 1 : 0);
		}
	} else {
		// if the port is not enabled, and an inactive state is specified, set it
		if (this->disabledState > -1)
			newState = this->disabledState;
	}

	// evaluate function

	// change detected?
	if (newState != this->pulseState) {
		if (opdid->logVerbosity >= AbstractOPDID::DEBUG) {
			opdid->log(std::string(this->id) + ": Changing pulse to " + (newState == 1 ? "High" : "Low") + " (dTime: " + to_string(opdi_get_time_ms() - this->lastStateChangeTime) + " ms)");
		}
	
		this->lastStateChangeTime = opdi_get_time_ms();
		
		// set the new state
		this->pulseState = newState;

		DigitalPortList::iterator it = this->outputPorts.begin();
		// regular output ports
		while (it != this->outputPorts.end()) {
			try {
				uint8_t mode;
				uint8_t line;
				// get current output port state
				(*it)->getState(&mode, &line);
				// changed?
				if (line != newState) {
					if (opdid->logVerbosity >= AbstractOPDID::DEBUG)
						opdid->log(std::string(this->id) + ": Changing line of port " + (*it)->getID() + " to " + (newState == 0 ? "Low" : "High"));
					(*it)->setLine(newState);
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
		// inverse output ports
		it = this->inverseOutputPorts.begin();
		while (it != this->inverseOutputPorts.end()) {
			try {
				uint8_t mode;
				uint8_t line;
				// get current output port state
				(*it)->getState(&mode, &line);
				// changed?
				if (line == newState) {
					if (opdid->logVerbosity >= AbstractOPDID::DEBUG)
						opdid->log(std::string(this->id) + ": Changing line of inverse port " + (*it)->getID() + " to " + (newState == 0 ? "High" : "Low"));
					(*it)->setLine((newState == 0 ? 1 : 0));
				}
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string("Changing port ") + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
	}

	return OPDI_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Selector Port
///////////////////////////////////////////////////////////////////////////////

OPDID_SelectorPort::OPDID_SelectorPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {

	this->opdid = opdid;
	this->negate = false;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	// set the line to an invalid state
	this->line = -1;
}

OPDID_SelectorPort::~OPDID_SelectorPort() {
}

void OPDID_SelectorPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->negate = config->getBool("Negate", false);

	this->selectPortStr = config->getString("SelectPort", "");
	if (this->selectPortStr == "")
		throw Poco::DataException("You have to specify the SelectPort");

	int pos = config->getInt("Position", -1);
	if ((pos < 0) || (pos > 65535))
		throw Poco::DataException("You have to specify a SelectPort position that is greater than -1 and lower than 65536");

	this->position = pos;
}

void OPDID_SelectorPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a SelectorPort cannot be changed");
}

void OPDID_SelectorPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a SelectorPort cannot be changed");
}

void OPDID_SelectorPort::setLine(uint8_t line) {
	OPDI_DigitalPort::setLine(line);
	if (this->line == 1) {
		// set the specified select port to the specified position
		this->selectPort->setPosition(this->position);
	}
}

void OPDID_SelectorPort::prepare() {
	OPDI_DigitalPort::prepare();

	// find port; throws errors if something required is missing
	this->selectPort = this->findSelectPort(this->getID(), "SelectPort", this->selectPortStr, true);

	// check position range
	if (this->position > this->selectPort->getMaxPosition())
		throw Poco::DataException(std::string(this->getID()) + ": The specified selector position exceeds the maximum of port " + this->selectPort->getID() + ": " + to_string(this->selectPort->getMaxPosition()));
}

uint8_t OPDID_SelectorPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// check whether the select port is in the specified position
	uint16_t pos;
	this->selectPort->getState(&pos);
	if (pos == this->position) {
		if (this->line != 1) {
			if (this->opdid->logVerbosity >= AbstractOPDID::DEBUG)
				this->opdid->log(std::string(this->getID()) + ": Port " + this->selectPort->getID() + " is in position " + to_string(this->position) + ", switching SelectorPort to High");
			OPDI_DigitalPort::setLine(1);
		}
	} else {
		if (this->line != 0) {
			if (this->opdid->logVerbosity >= AbstractOPDID::DEBUG)
				this->opdid->log(std::string(this->getID()) + ": Port " + this->selectPort->getID() + " is in position " + to_string(this->position) + ", switching SelectorPort to Low");
			OPDI_DigitalPort::setLine(0);
		}
	}

	return OPDI_STATUS_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Expression Port
///////////////////////////////////////////////////////////////////////////////

#ifdef OPDI_USE_EXPRTK

OPDID_ExpressionPort::OPDID_ExpressionPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);

	// default: enabled
	this->line = 1;
}

OPDID_ExpressionPort::~OPDID_ExpressionPort() {
}

void OPDID_ExpressionPort::configure(Poco::Util::AbstractConfiguration *config) {

	this->expressionStr = config->getString("Expression", "");
	if (this->expressionStr == "")
		throw Poco::DataException("You have to specify the Expression");

	this->outputPortStr = config->getString("OutputPorts", "");
	if (this->outputPortStr == "")
		throw Poco::DataException("You have to specify at least one output port in the OutputPorts setting");
}

void OPDID_ExpressionPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of an ExpressionPort cannot be changed");
}

void OPDID_ExpressionPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of an ExpressionPort cannot be changed");
}

bool OPDID_ExpressionPort::prepareVariables(void) {
	// clear symbol table and values
	this->symbol_table.clear();
	this->portValues.clear();

	// go through dependent entities (variables) of the expression
	for (std::size_t i = 0; i < this->symbol_list.size(); ++i)
	{
		symbol_t& symbol = this->symbol_list[i];

		if (symbol.second != parser_t::e_st_variable)
			continue;

		// find port (variable name is the port ID)
		OPDI_Port *port = this->opdid->findPortByID(symbol.first.c_str(), true);

		// port not found?
		if (port == NULL) {
			throw PortError(std::string(this->getID()) + ": Expression variable did not resolve to an available port ID: " + symbol.first);
		}

		// calculate port value
		double value = 0;
		try {
			// evaluation depends on port type
			if (port->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
				// digital port: Low = 0; High = 1
				uint8_t mode;
				uint8_t line;
				((OPDI_DigitalPort *)port)->getState(&mode, &line);
				value = line;
			} else 
			if (port->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
				// analog port: relative value (0..1)
				value = ((OPDI_AnalogPort *)port)->getRelativeValue();
			} else 
			if (port->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
				// dial port: absolute value
				int64_t position;
				((OPDI_DialPort *)port)->getState(&position);
				value = position;
			} else 
			if (port->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
				// select port: current position number
				uint16_t position;
				((OPDI_SelectPort *)port)->getState(&position);
				value = position;
			} else
				// port type not supported
				return false;
		} catch (Poco::Exception &pe) {
			if (this->opdid->logVerbosity >= AbstractOPDID::DEBUG)
				this->opdid->log(std::string(this->getID()) + ": Unable to get state of port " + port->getID() + ": " + pe.message());
			return false;
		}

		this->portValues.push_back(value);

		// add reference to the port value (by port ID)
		if (!this->symbol_table.add_variable(port->getID(), this->portValues[i]))
			return false;
	}

	return true;
}

void OPDID_ExpressionPort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);

	// prepare the expression
	symbol_table.add_constants();
	expression.register_symbol_table(symbol_table);

	parser_t parser;
	parser.enable_unknown_symbol_resolver();
	// collect only variables as symbol names
	parser.dec().collect_variables() = true;

	// compile to detect variables
	if (!parser.compile(this->expressionStr, expression))
		throw Poco::Exception(std::string(this->getID()) + ": Error in expression: " + parser.error());

	// store symbol list (input variables)
	parser.dec().symbols(this->symbol_list);

	if (!this->prepareVariables()) {
		throw new Poco::Exception(std::string(this->getID()) + ": Unable to resolve variables");
	}
	parser.disable_unknown_symbol_resolver();

	if (!parser.compile(this->expressionStr, expression))
		throw Poco::Exception(std::string(this->getID()) + ": Error in expression: " + parser.error());
}

uint8_t OPDID_ExpressionPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	if (this->line == 1) {

		// prepareVariables will return false in case of errors
		if (this->prepareVariables()) {

			double value = expression.value();

			if (this->opdid->logVerbosity >= AbstractOPDID::EXTREME)
				this->opdid->log(std::string(this->getID()) + ": Expression result: " + to_string(value));

			// go through list of output ports
			PortList::iterator it = this->outputPorts.begin();
			while (it != this->outputPorts.end()) {
				try {
					if ((*it)->getType()[0] == OPDI_PORTTYPE_DIGITAL[0]) {
						if (value == 0)
							((OPDI_DigitalPort *)(*it))->setLine(0);
						else
							((OPDI_DigitalPort *)(*it))->setLine(1);
					} else 
					if ((*it)->getType()[0] == OPDI_PORTTYPE_ANALOG[0]) {
						// analog port: relative value (0..1)
						((OPDI_AnalogPort *)(*it))->setRelativeValue(value);
					} else 
					if ((*it)->getType()[0] == OPDI_PORTTYPE_DIAL[0]) {
						// dial port: absolute value
						((OPDI_DialPort *)(*it))->setPosition((int64_t)value);
					} else 
					if ((*it)->getType()[0] == OPDI_PORTTYPE_SELECT[0]) {
						// select port: current position number
						((OPDI_SelectPort *)(*it))->setPosition((uint16_t)value);
					} else
						throw PortError("");
				} catch (Poco::Exception) {
				}

				it++;
			}
		}
	}

	return OPDI_STATUS_OK;
}

#endif	// def OPDI_USE_EXPRTK
