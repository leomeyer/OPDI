
#include "Poco/NumberFormatter.h"

#include "opdi_constants.h"
#include "opdi_port.h"

#include "opdi_SelectPort.h"
#include "opdi_StringTools.h"
#include "opdi_AbstractProtocol.h"

SelectPort::SelectPort(IBasicProtocol& protocol, std::vector<std::string> parts) : OPDIPort(protocol, "", "", PORTTYPE_SELECT, PORTDIRCAP_UNKNOWN)
{
	int ID_PART = 1;
	int NAME_PART = 2;
	int DIR_PART = 3;
	int FLAGS_PART = 4;
	int PART_COUNT = 5;
		
	checkSerialForm(parts, PART_COUNT, OPDI_selectPort);

	setID(parts[ID_PART]);
	setName(parts[NAME_PART]);
	labelCount = AbstractProtocol::parseInt(parts[DIR_PART], "labelCount", 0, std::numeric_limits<uint16_t>::max());
	flags = AbstractProtocol::parseInt(parts[FLAGS_PART], "flags", 0, std::numeric_limits<int>::max());

	position = -1;

	// get port labels from slave
	for (int i = 0; i < labelCount; i++)
		this->positionLabels.push_back(protocol.getLabel(this, i));
}
	
std::string SelectPort::serialize()
{
	return StringTools::join(':', OPDI_selectPort, getID(), getName(), Poco::NumberFormatter::format(getDirCaps()), Poco::NumberFormatter::format(flags));
}
	
void SelectPort::loadLabels(void) {
	for (uint16_t i = 0; i < this->labelCount; i++) {
		
	}
}

// Retrieve all port state from the device
void SelectPort::getPortState()
{
	// Request port state
	getProtocol().getPortState(this);
}
	
void SelectPort::setPortPosition(IBasicProtocol& protocol, uint16_t position)
{
	if (&protocol != &getProtocol())
		throw Poco::AssertionViolationException("Setting the port state is only allowed from its protocol implementation");
	try {
		checkPosition(position);
	} catch (Poco::Exception e) {
		throw ProtocolException(e.displayText());
	}
	this->position = position;
}

void SelectPort::checkPosition(uint16_t position)
{
//	if (portMode == OPDI_DIGITAL_MODE_INPUT_PULLDOWN && !hasPulldown())
//		throw Poco::InvalidArgumentException("Digital port has no pullup: ID = " + getID());
}

void SelectPort::setPosition(uint16_t position)
{
	if (position > this->getMaxPosition())
		throw Poco::InvalidArgumentException("Position exceeds maximum of " + to_string(this->getMaxPosition()));
	getProtocol().setPosition(this, position);
}

uint16_t SelectPort::getMaxPosition()
{
	return this->positionLabels.size() - 1;
}

uint16_t SelectPort::getPosition()
{
	if (this->position < 0)
		// get state from the device
		getPortState();
	return this->position;
}

std::string SelectPort::getPositionText()
{
	if (this->position < 0)
		return "Unknown";

	return this->positionLabels[this->position];
}

std::string SelectPort::toString() 
{
	return "SelectPort id=" + getID() + "; name='" + getName() + "'; labelCount=" + to_string(labelCount) + 
		"; position=" + getPositionText();
}

void SelectPort::refresh()
{
	this->position = -1;
}

