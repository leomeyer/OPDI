#include <string.h>
#include <vector>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <Poco/NumberParser.h>
#include "Poco/NumberFormatter.h"

#include "opdi_OPDIMessage.h"
#include "opdi_StringTools.h"


/** Represents a message that is sent to or from a device.
 *
 * @author Leo
 *
 */
OPDIMessage::OPDIMessage(int channel, std::string payload)
{
	this->channel = channel;
	this->payload = payload;
}

OPDIMessage::OPDIMessage(int channel, std::string payload, int checksum)
{
	this->channel = channel;
	this->payload = payload;
	this->checksum = checksum;
}

int calcChecksum(char *message) {
	int myChecksum = 0;
	bool colonFound = false;
	// add everything before the last colon
	for (int i = strlen(message) - 1; i >= 0; i--)
		if (colonFound)
			// add unsigned byte values
			myChecksum += message[i] & 0xFF;
		else
			colonFound = message[i] == OPDIMessage::SEPARATOR;
	return myChecksum;
}

/** Tries to decode a message from its serial form.
 *
 * @return
 */
OPDIMessage* OPDIMessage::decode(char *serialForm/*, Charset encoding*/)
{
	std::string message(serialForm);
	// split at ":"
	std::vector<std::string> parts;
	StringTools::split(message, SEPARATOR, parts);
	// valid form?
	if (parts.size() < 3) 
		throw MessageException("Message part number too low");
	// last part must be checksum
	int checksum = 0;
	try {
		checksum = Poco::NumberParser::parseHex(parts[parts.size() - 1]);
	} catch (Poco::SyntaxException nfe) {
		throw MessageException("Message checksum invalid (not a hex number)");
	}
	// calculate content checksum
	int calcCheck = calcChecksum(serialForm);
	// checksums not equal?
	if (calcCheck != checksum) {
		throw MessageException("Message checksum invalid: " + Poco::NumberFormatter::formatHex(calcCheck) + ", expected: " + Poco::NumberFormatter::formatHex(checksum));
	}
	// checksum is ok
	// join payload again (without channel and checksum)
	std::string content = StringTools::join(1, 1, SEPARATOR, parts);
	try {
		// the first part is the channel
		int pid = Poco::NumberParser::parse(parts[0]);
		// the payload doesn't contain the channel
		return new OPDIMessage(pid, content, checksum);
	} catch (Poco::SyntaxException nfe) {
		throw MessageException("Message channel number invalid");
	}
}

int OPDIMessage::encode(int encoding, char buffer[], int maxlength)
{
	// message content is channel plus payload
	std::stringstream content;

	content << "" << channel;
	content << SEPARATOR;
	content << payload;

	// calculate message checksum over the bytes to transfer
	checksum = 0;
	std::string data = content.str();
	const char *bytes = data.c_str();
	for (unsigned int i = 0; i < data.length(); i++) {
		if (bytes[i] == TERMINATOR)
			throw MessageException("Message terminator may not appear in payload");
		// add unsigned bytes for payload
		checksum += bytes[i] & 0xFF;
	}
	// A message is terminated by the checksum and a \n
	content << ":";
	content << std::setfill('0') << std::setw(4) << std::hex << (checksum & 0xffff);
	content << TERMINATOR;
	data = content.str();
	bytes = data.c_str();
	strncpy(buffer, bytes, maxlength);
	return strlen(bytes);
}

std::string OPDIMessage::getPayload() {
	return payload;
}

int OPDIMessage::getChannel() {
	return channel;
}

std::string OPDIMessage::toString()
{
	return Poco::NumberFormatter::format(channel) + ":" + payload;
}
