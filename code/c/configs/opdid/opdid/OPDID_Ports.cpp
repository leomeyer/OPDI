
#include "Poco/Tuple.h"
#include "Poco/Timezone.h"
#include "Poco/DateTimeFormatter.h"

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
	this->opdid->configurePort(config, this, 0);

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
	this->opdid->configurePort(config, this, 0);

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

	if (!enabled  && (this->enablePorts.size() > 0)) {
		int highCount = 0;
		DigitalPortList::iterator it = this->enablePorts.begin();
		while (it != this->enablePorts.end()) {
			uint8_t mode;
			uint8_t line;
			try {
				(*it)->getState(&mode, &line);
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string(this->getID()) + ": Error querying port " + (*it)->getID() + ": " + e.message());
			}
			highCount += line;
			it++;
			if (highCount > 0)
				break;
		}
		// pulse is enabled if there is at least one EnablePort with High
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
				(*it)->setLine(newState);
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string(this->getID()) + ": Error setting output port state: " + (*it)->getID() + ": " + e.message());
			}
			it++;
		}
		// inverse output ports
		it = this->inverseOutputPorts.begin();
		while (it != this->inverseOutputPorts.end()) {
			try {
				(*it)->setLine((newState == 0 ? 1 : 0));
			} catch (Poco::Exception &e) {
				this->opdid->log(std::string(this->getID()) + ": Error setting inverse output port state: " + (*it)->getID() + ": " + e.message());
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

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);
	// set the line to an invalid state
	this->line = -1;
}

OPDID_SelectorPort::~OPDID_SelectorPort() {
}

void OPDID_SelectorPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);

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

#ifdef OPDID_USE_EXPRTK

OPDID_ExpressionPort::OPDID_ExpressionPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);

	// default: enabled
	this->line = 1;
}

OPDID_ExpressionPort::~OPDID_ExpressionPort() {
}

void OPDID_ExpressionPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configurePort(config, this, 0);

	this->expressionStr = config->getString("Expression", "");
	if (this->expressionStr == "")
		throw Poco::DataException("You have to specify an Expression");

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
				} catch (Poco::Exception &e) {
					this->opdid->log(std::string(this->getID()) + ": Error setting output port value of port " + (*it)->getID() + ": " + e.message());
				}

				it++;
			}
		}
	}

	return OPDI_STATUS_OK;
}

#endif	// def OPDID_USE_EXPRTK


///////////////////////////////////////////////////////////////////////////////
// Timer Port
///////////////////////////////////////////////////////////////////////////////

OPDID_TimerPort::OPDID_TimerPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_OUTPUT, 0) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_OUTPUT);

	// default: enabled
	this->line = 1;
}

OPDID_TimerPort::~OPDID_TimerPort() {
}

void OPDID_TimerPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configureDigitalPort(config, this);

	this->outputPortStr = this->opdid->getConfigString(config, "OutputPorts", "", true);

	// enumerate schedules of the <timer>.Schedules node
	if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
		this->opdid->log(std::string("Enumerating Timer schedules: ") + this->getID() + ".Schedules");

	Poco::Util::AbstractConfiguration *nodes = config->createView("Schedules");

	// get ordered list of schedules
	Poco::Util::AbstractConfiguration::Keys scheduleKeys;
	nodes->keys("", scheduleKeys);

	typedef Poco::Tuple<int, std::string> Item;
	typedef std::vector<Item> ItemList;
	ItemList orderedItems;

	// create ordered list of schedule keys (by priority)
	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = scheduleKeys.begin(); it != scheduleKeys.end(); ++it) {

		int itemNumber = nodes->getInt(*it, 0);
		// check whether the item is active
		if (itemNumber < 0)
			continue;

		// insert at the correct position to create a sorted list of items
		ItemList::iterator nli = orderedItems.begin();
		while (nli != orderedItems.end()) {
			if (nli->get<0>() > itemNumber)
				break;
			nli++;
		}
		Item item(itemNumber, *it);
		orderedItems.insert(nli, item);
	}

	if (orderedItems.size() == 0) {
		if (this->opdid->logVerbosity >= AbstractOPDID::NORMAL)
			this->opdid->log(std::string("Warning: No schedules configured in node ") + this->getID() + ".Schedules; is this intended?");
	}

	// go through items, create schedules in specified order
	ItemList::const_iterator nli = orderedItems.begin();
	while (nli != orderedItems.end()) {

		std::string nodeName = nli->get<1>();

		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log("Setting up timer schedule for node: " + nodeName);

		// get schedule section from the configuration
		Poco::Util::AbstractConfiguration *scheduleConfig = this->opdid->getConfiguration()->createView(nodeName);

		Schedule schedule;
		schedule.nodeName = nodeName;
		schedule.data.time.year = scheduleConfig->getInt("Year", -1);
		schedule.data.time.month = scheduleConfig->getInt("Month", -1);
		schedule.data.time.day = scheduleConfig->getInt("Day", -1);
		schedule.data.time.weekday = scheduleConfig->getInt("Weekday", -1);
		schedule.data.time.hour = scheduleConfig->getInt("Hour", -1);
		schedule.data.time.minute = scheduleConfig->getInt("Minute", -1);
		schedule.data.time.second = scheduleConfig->getInt("Second", -1);

		if ((schedule.data.time.day > -1) && (schedule.data.time.weekday > -1))
			throw Poco::DataException(nodeName + ": Please specify either Day or Weekday but not both");

		schedule.maxOccurrences = scheduleConfig->getInt("MaxOccurrences", -1);
		schedule.delayMs = scheduleConfig->getInt("Delay", 0);		// default: no deactivation
		schedule.action = SET_HIGH;									// default action

		std::string action = this->opdid->getConfigString(scheduleConfig, "Action", "", false);
		if (action == "SetHigh") {
			schedule.action = SET_HIGH;
		} else
		if (action == "SetLow") {
			schedule.action = SET_LOW;
		} else
		if (action == "Toggle") {
			schedule.action = TOGGLE;
		} else
			if (action != "")
				throw Poco::DataException(nodeName + ": Unknown schedule action; expected: 'SetHigh', 'SetLow' or 'Toggle': " + action);

		// get schedule type (required)
		std::string scheduleType = this->opdid->getConfigString(scheduleConfig, "Type", "", true);

		if (scheduleType == "Once") {
			schedule.type = ONCE;
			if ((schedule.data.time.year < 0) || (schedule.data.time.month < 0)
				|| (schedule.data.time.day < 0)
				|| (schedule.data.time.hour < 0) || (schedule.data.time.minute < 0)
				|| (schedule.data.time.second < 0))
				throw Poco::DataException(nodeName + ": You have to specify all time components for schedule type Once");

			if (schedule.data.time.weekday > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Weekday setting with schedule type Once");
		} else
		if (scheduleType == "Interval") {
			schedule.type = INTERVAL;
			if (schedule.data.time.year > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Year setting with schedule type Interval");
			if (schedule.data.time.month > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Month setting with schedule type Interval");
			if (schedule.data.time.weekday > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Weekday setting with schedule type Interval");

			if ((schedule.data.time.day < 0)
				&& (schedule.data.time.hour < 0) && (schedule.data.time.minute < 0)
				&& (schedule.data.time.second < 0))
				throw Poco::DataException(nodeName + ": You have to specify at least one of Day, Hour, Minute or Second for schedule type Interval");
		} else
/*
if (scheduleType == "Periodic") {
			schedule.type = PERIODIC;
			if (schedule.data.time.year > -1)
				throw Poco::DataException(nodeName + ": You cannot use the Year setting with schedule type Periodic");
			if ((schedule.data.time.month < 0)
				&& (schedule.data.time.day < 0) && (schedule.data.time.weekday < 0)
				&& (schedule.data.time.hour < 0) && (schedule.data.time.minute < 0)
				&& (schedule.data.time.second < 0))
				this->opdid->log(nodeName + ": Warning: No time component specified for schedule type Periodic; this schedule will execute every second");

		} else
		if (scheduleType == "Random") {
			schedule.type = RANDOM;

		} else
*/
			throw Poco::DataException(nodeName + ": Schedule type is not supported: " + scheduleType);

		this->schedules.push_back(schedule);

		nli++;
	}
}

void OPDID_TimerPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of a TimerPort cannot be changed");
}

void OPDID_TimerPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of a TimerPort cannot be changed");
}

void OPDID_TimerPort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findDigitalPorts(this->getID(), "OutputPorts", this->outputPortStr, this->outputPorts);

	if (this->line == 1) {
		// calculate all schedules
		for (ScheduleList::iterator it = this->schedules.begin(); it != this->schedules.end(); it++) {
			Schedule schedule = (*it);
			// calculate 
			Poco::Timestamp nextOccurrence = this->calculateNextOccurrence(&schedule);
			this->addNotification(new ScheduleNotification(schedule), nextOccurrence);
		}
	}
}

Poco::Timestamp OPDID_TimerPort::calculateNextOccurrence(Schedule *schedule) {

	if (schedule->type == ONCE) {
		// validate
		if ((schedule->data.time.month < 1) || (schedule->data.time.month > 12))
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Month specification in schedule " + schedule->nodeName);
		if ((schedule->data.time.day < 1) || (schedule->data.time.day > Poco::DateTime::daysOfMonth(schedule->data.time.year, schedule->data.time.month)))
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Day specification in schedule " + schedule->nodeName);
		if (schedule->data.time.hour > 23)
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Hour specification in schedule " + schedule->nodeName);
		if (schedule->data.time.minute > 59)
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Minute specification in schedule " + schedule->nodeName);
		if (schedule->data.time.second > 59)
			throw Poco::DataException(std::string(this->getID()) + ": Invalid Second specification in schedule " + schedule->nodeName);
		// create timestamp via datetime
		Poco::DateTime result = Poco::DateTime(schedule->data.time.year, schedule->data.time.month, schedule->data.time.day, 
			schedule->data.time.hour, schedule->data.time.minute, schedule->data.time.second);
		// ONCE values are specified in local time; convert to UTC
		result.makeUTC(Poco::Timezone::tzd());
		return result.timestamp();
	} else
	if (schedule->type == INTERVAL) {
		Poco::Timestamp result;	// now
		// add interval values (ignore year and month because those are not well defined in seconds)
		if (schedule->data.time.second > 0)
			result += schedule->data.time.second * result.resolution();
		if (schedule->data.time.minute > 0)
			result += schedule->data.time.minute * 60 * result.resolution();
		if (schedule->data.time.hour > 0)
			result += schedule->data.time.hour * 60 * 60 * result.resolution();
		if (schedule->data.time.day > 0)
			result += schedule->data.time.day * 24 * 60 * 60 * result.resolution();
		return result;
	} else
	if (schedule->type == PERIODIC) {
	} else
	if (schedule->type == RANDOM) {
		// determine next point in type randomly
	} else
		// return default value (now; must not be enqueued)
		return Poco::Timestamp();
}

void OPDID_TimerPort::addNotification(ScheduleNotification::Ptr notification, Poco::Timestamp timestamp) {
	Poco::Timestamp now;

	// for debug output: convert UTC timestamp to local time
	Poco::LocalDateTime ldt(timestamp);
	if (timestamp > now) {
		if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
			this->opdid->log(std::string(this->getID()) + ": Next scheduled time for node " + 
				notification->schedule.nodeName + " is: " + Poco::DateTimeFormatter::format(ldt, "%Y-%m-%d %H:%M:%S"));
		// add with the specified activation time
		this->queue.enqueueNotification(notification, timestamp);
	} else {
		if (this->opdid->logVerbosity >= AbstractOPDID::NORMAL)
			this->opdid->log(std::string(this->getID()) + ": Warning: Scheduled time for node " + 
				notification->schedule.nodeName + " lies in the past, ignoring: " + Poco::DateTimeFormatter::format(ldt, "%Y-%m-%d %H:%M:%S"));
/*
		this->opdid->log(std::string(this->getID()) + ": Timestamp is: " + Poco::DateTimeFormatter::format(timestamp, "%Y-%m-%d %H:%M:%S"));
		this->opdid->log(std::string(this->getID()) + ": Now is      : " + Poco::DateTimeFormatter::format(now, "%Y-%m-%d %H:%M:%S"));
*/
	}
}

uint8_t OPDID_TimerPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	// timer not active?
	if (this->line != 1)
		return OPDI_STATUS_OK;

	// get next object from the priority queue
	Poco::Notification::Ptr notification = this->queue.dequeueNotification();
	// notification will only be a valid object if a schedule is due
	if (notification) {
		try {
			ScheduleNotification::Ptr workNf = notification.cast<ScheduleNotification>();

			if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
				this->opdid->log(std::string(this->getID()) + ": Timer reached scheduled time for node: " + workNf->schedule.nodeName);

			workNf->schedule.occurrences++;

			// calculate next occurrence depending on type; maximum ocurrences must not have been reached
			if ((workNf->schedule.type != ONCE) && (workNf->schedule.type != _DEACTIVATE) 
				&& ((workNf->schedule.maxOccurrences < 0) || (workNf->schedule.occurrences < workNf->schedule.maxOccurrences))) {

				Poco::Timestamp nextOccurrence = this->calculateNextOccurrence(&workNf->schedule);
				if (nextOccurrence > Poco::Timestamp()) {
					// add with the specified occurrence time
					this->addNotification(workNf, nextOccurrence);
				}
			}

			// need to deactivate?
			if ((workNf->schedule.type != _DEACTIVATE) && (workNf->schedule.delayMs > 0)) {
				// enqueue the notification for the deactivation
				Schedule deacSchedule = workNf->schedule;
				deacSchedule.type = _DEACTIVATE;
				ScheduleNotification *notification = new ScheduleNotification(deacSchedule);
				Poco::Timestamp deacTime;
				deacTime += workNf->schedule.delayMs * Poco::Timestamp::resolution() / 1000;
				if (this->opdid->logVerbosity >= AbstractOPDID::VERBOSE)
					this->opdid->log(std::string(this->getID()) + ": Scheduled deactivation time for node " + deacSchedule.nodeName + " is: UTC " + Poco::DateTimeFormatter::format(deacTime, "%Y-%m-%d %H:%M:%S"));
				// add with the specified deactivation time
				this->addNotification(notification, deacTime);
			}

			// set the output ports' state
			int8_t outputLine = -1;	// assume: toggle
			if (workNf->schedule.action == SET_HIGH)
				outputLine = (workNf->schedule.type == _DEACTIVATE ? 0 : 1);
			if (workNf->schedule.action == SET_LOW)
				outputLine = (workNf->schedule.type == _DEACTIVATE ? 1 : 0);

			DigitalPortList::iterator it = this->outputPorts.begin();
			while (it != this->outputPorts.end()) {
				try {
					// toggle?
					if (outputLine < 0) {
						// get current output port state
						uint8_t mode;
						uint8_t line;
						(*it)->getState(&mode, &line);
						// set new state
						outputLine = (line == 1 ? 0 : 1);
					}

					// set output line
					(*it)->setLine(outputLine);
				} catch (Poco::Exception &e) {
					this->opdid->log(std::string(this->getID()) + ": Error setting output port state: " + (*it)->getID() + ": " + e.message());
				}
				it++;
			}
		} catch (Poco::Exception &e) {
			this->opdid->log(std::string(this->getID()) + ": Error processing timer schedule: " + e.message());
		}
	}

	return OPDI_STATUS_OK;
}

void OPDID_TimerPort::setLine(uint8_t line) {
	bool wasLow = (this->line == 0);

	OPDI_DigitalPort::setLine(line);

	// set to Low?
	if (this->line == 0) {
		if (!wasLow) {
			// clear all schedules
			this->queue.clear();
		}
	}
	
	// set to High?
	if (this->line == 1) {
		if (wasLow) {
			// recalculate all schedules
			for (ScheduleList::iterator it = this->schedules.begin(); it != this->schedules.end(); it++) {
				Schedule schedule = (*it);
				// calculate 
				Poco::Timestamp nextOccurrence = this->calculateNextOccurrence(&schedule);
				this->addNotification(new ScheduleNotification(schedule), nextOccurrence);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Error Detector Port
///////////////////////////////////////////////////////////////////////////////

OPDID_ErrorDetectorPort::OPDID_ErrorDetectorPort(AbstractOPDID *opdid, const char *id) : OPDI_DigitalPort(id, id, OPDI_PORTDIRCAP_INPUT, 0) {
	this->opdid = opdid;

	OPDI_DigitalPort::setMode(OPDI_DIGITAL_MODE_INPUT_FLOATING);

	// make sure to trigger state change at first doWork
	this->line = -1;
}

OPDID_ErrorDetectorPort::~OPDID_ErrorDetectorPort() {
}

void OPDID_ErrorDetectorPort::configure(Poco::Util::AbstractConfiguration *config) {
	this->opdid->configureDigitalPort(config, this);

	this->inputPortStr = this->opdid->getConfigString(config, "InputPorts", "", true);
	this->negate = config->getBool("Negate", false);
}

void OPDID_ErrorDetectorPort::setDirCaps(const char *dirCaps) {
	throw PortError(std::string(this->getID()) + ": The direction capabilities of an ErrorDetectorPort cannot be changed");
}

void OPDID_ErrorDetectorPort::setMode(uint8_t mode) {
	throw PortError(std::string(this->getID()) + ": The mode of an ErrorDetectorPort cannot be changed");
}

void OPDID_ErrorDetectorPort::prepare() {
	OPDI_DigitalPort::prepare();

	// find ports; throws errors if something required is missing
	this->findPorts(this->getID(), "InputPorts", this->inputPortStr, this->inputPorts);
}

uint8_t OPDID_ErrorDetectorPort::doWork(uint8_t canSend)  {
	OPDI_DigitalPort::doWork(canSend);

	int8_t newState = 0;

	// if any port has an error, set the line state to 1
	PortList::iterator it = this->inputPorts.begin();
	while (it != this->inputPorts.end()) {
		if ((*it)->hasError()) {
			if (this->opdid->logVerbosity >= AbstractOPDID::EXTREME)
				this->opdid->log(std::string(this->getID()) + ": Detected error on port: " + (*it)->getID());
			newState = 1;
			break;
		}
		it++;
	}

	if (negate)
		newState = (newState == 0 ? 1 : 0);

	// change?
	if (this->line != newState) {
		if (this->opdid->logVerbosity >= AbstractOPDID::DEBUG)
			this->opdid->log(std::string(this->getID()) + ": Changing line state to: " + (newState == 1 ? "High" : "Low"));
		this->line = newState;
		this->doSelfRefresh();
	}

	return OPDI_STATUS_OK;
}
