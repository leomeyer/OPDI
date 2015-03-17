
#include "Poco/NumberFormatter.h"

#include "opdi_constants.h"
#include "opdi_port.h"

#include "opdi_DigitalPort.h"
#include "opdi_StringTools.h"
#include "opdi_AbstractProtocol.h"

DigitalPort::DigitalPort(IBasicProtocol& protocol, std::vector<std::string> parts) : OPDIPort(protocol, "", "", PORTTYPE_DIGITAL, PORTDIRCAP_UNKNOWN)
{
	int ID_PART = 1;
	int NAME_PART = 2;
	int DIR_PART = 3;
	int FLAGS_PART = 4;
	int PART_COUNT = 5;
		
	checkSerialForm(parts, PART_COUNT, OPDI_digitalPort);

	setID(parts[ID_PART]);
	setName(parts[NAME_PART]);
	setDirCaps((PortDirCaps)AbstractProtocol::parseInt(parts[DIR_PART], "PortDirCaps", 0, 2));
	flags = AbstractProtocol::parseInt(parts[FLAGS_PART], "flags", 0, std::numeric_limits<int>::max());

	mode = OPDI_DIGITAL_MODE_UNKNOWN;
	line = OPDI_DIGITAL_LINE_UNKNOWN;
}
	
std::string DigitalPort::serialize()
{
	return StringTools::join(':', OPDI_digitalPort, getID(), getName(), Poco::NumberFormatter::format(getDirCaps()), Poco::NumberFormatter::format(flags));
}
	
bool DigitalPort::hasPullup()
{
	return (flags & OPDI_DIGITAL_PORT_HAS_PULLUP) == OPDI_DIGITAL_PORT_HAS_PULLUP;
}

bool DigitalPort::hasPulldown()
{
	return (flags & OPDI_DIGITAL_PORT_HAS_PULLDN) == OPDI_DIGITAL_PORT_HAS_PULLDN;
}
	
// Retrieve all port state from the device
void DigitalPort::getPortState()
{
	// Request port state
	getProtocol().getPortState(this);
}
	
void DigitalPort::setPortState(IBasicProtocol& protocol, int8_t mode)
{
	if (&protocol != &getProtocol())
		throw Poco::AssertionViolationException("Setting the port state is only allowed from its protocol implementation");
	try {
		checkMode(mode);
	} catch (Poco::Exception e) {
		throw ProtocolException(e.displayText());
	}
	this->mode = mode;
}

void DigitalPort::setPortLine(IBasicProtocol& protocol, int8_t line)
{
	if (&protocol != &getProtocol())
		throw Poco::AssertionViolationException("Setting the port state is only allowed from its protocol implementation");
	this->line = line;
}
	
void DigitalPort::checkMode(int8_t portMode)
{
	// configure for output?
	if (portMode == OPDI_DIGITAL_MODE_OUTPUT)
		if (getDirCaps() == PORTDIRCAP_INPUT)
			throw Poco::InvalidArgumentException("Can't configure input only digital port for output: ID = " + getID());
	// configure for input?
	if (portMode == OPDI_DIGITAL_MODE_INPUT_FLOATING || portMode == OPDI_DIGITAL_MODE_INPUT_PULLUP || portMode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN)
		if (getDirCaps() == PORTDIRCAP_OUTPUT)
			throw Poco::InvalidArgumentException("Can't configure output only digital port for input: ID = " + getID());
	// check pullup/pulldown
	if (portMode == OPDI_DIGITAL_MODE_INPUT_PULLUP && !hasPullup())
		throw Poco::InvalidArgumentException("Digital port has no pullup: ID = " + getID());
	if (portMode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN && !hasPulldown())
		throw Poco::InvalidArgumentException("Digital port has no pullup: ID = " + getID());
}

/** Configures the port in the given mode. Throws an IllegalArgumentException if the mode
	* is not supported.
	* @param portMode
	*/
void DigitalPort::setMode(int8_t portMode)
{
	checkMode(portMode);		
	// set the mode
	getProtocol().setPortMode(this, portMode);
}
	
int8_t DigitalPort::getMode()
{
	if (mode == OPDI_DIGITAL_MODE_UNKNOWN)
		getPortState();
	return mode;
}

std::string DigitalPort::getModeText()
{
	switch (mode)
	{
	case OPDI_DIGITAL_MODE_UNKNOWN: return "Unknown";
	case OPDI_DIGITAL_MODE_INPUT_FLOATING: return "Input, floating"; 
	case OPDI_DIGITAL_MODE_INPUT_PULLUP: return "Input, pullup on";
	case OPDI_DIGITAL_MODE_INPUT_PULLDOWN: return "Input, pulldown on";
	case OPDI_DIGITAL_MODE_OUTPUT: return "Output";
	}
	return "Invalid mode value: " + Poco::NumberFormatter::format(mode);
}

void DigitalPort::setLine(int8_t portLine)
{
	if (getDirCaps() == PORTDIRCAP_INPUT)
		throw Poco::InvalidArgumentException("Can't set state on input only digital port: ID = " + getID());
	// mode set to input?
	if (mode == OPDI_DIGITAL_MODE_INPUT_FLOATING || mode == OPDI_DIGITAL_MODE_INPUT_PULLUP ||mode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN)
		throw Poco::InvalidArgumentException("Can't set state on digital port configured as input: ID = " + getID());
	// mode output or unknown; set the line state
	getProtocol().setPortLine(this, portLine);
}
	
int8_t DigitalPort::getLine()
{
	if (line == OPDI_DIGITAL_LINE_UNKNOWN)
		// get state from the device
		getPortState();
	return line;
}

std::string DigitalPort::getLineText()
{
	switch (line)
	{
		case OPDI_DIGITAL_LINE_UNKNOWN: return "Unknown";
		case OPDI_DIGITAL_LINE_LOW: return "Low";
		case OPDI_DIGITAL_LINE_HIGH: return "High";
	}
	return "Invalid line value: " + Poco::NumberFormatter::format(line);
}

std::string DigitalPort::toString() 
{
	return "DigitalPort id=" + getID() + "; name='" + getName() + "'; dirCaps=" + getDirCapsText() + 
		"; hasPullup=" + (hasPullup() ? "true" : "false") + "; hasPulldown=" + (hasPulldown() ? "true" : "false") + 
		"; mode=" + getModeText() + "; line=" + getLineText();
}

void DigitalPort::refresh()
{
	mode = OPDI_DIGITAL_MODE_UNKNOWN;
	line = OPDI_DIGITAL_LINE_UNKNOWN;
}

