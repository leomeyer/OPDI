#include <sstream>

#include "Poco/RegularExpression.h"

#include "opdi_SerialDevice.h"
#include "opdi_main_io.h"

/** Implements IDevice for a TCP/IP device.
 *
 * @author Leo
 *
 */

using Poco::RegularExpression;

SerialDevice::SerialDevice(std::string id, Poco::URI uri, bool *debug) : IODevice(id)
{
	this->uri = uri;
	this->debug = debug;

	this->baudRate = 115000;
	this->protocol = "8N1";
	this->serialPort = new ctb::SerialPort();

	// deserialize information
	if (uri.getScheme() != "opdi_com")
		throw Poco::InvalidArgumentException("Can't deserialize; schema is incorrect, expected 'opdi_com'");

	// split user information into name:password
	// assumption: ':' character does not appear in either part
	std::vector<std::string> parts;
	RegularExpression re("(.+):(.+)");
	re.split(uri.getUserInfo(), parts, 0);
	if ((parts.size() > 1) && (parts[1] != ""))
		setUser(parts[1]);
	if ((parts.size() > 2) && (parts[2] != ""))
		setPassword(parts[2]);

	comport = uri.getHost() + (uri.getPath() != "" ? "/" + uri.getPath() : "");

	if (comport == "")
		throw Poco::InvalidArgumentException("Serial port must be specified");

	// TODO parse parameters, especially for name and PSK
	// name = uri.getHost();
}

bool SerialDevice::prepare()
{
	return true;
}

std::string SerialDevice::getName()
{
	return name;
}

std::string SerialDevice::getLabel()
{
	std::stringstream result;
	result << "opdi_com://";

	if (user != "")
		result << user << "@";

	result << this->comport << "?baudRate=" << this->baudRate << "&protocol=" << this->protocol;

	if (name != "")
		result << "&name=" << name;	// TODO escape URL syntax

	return result.str();
}

std::string SerialDevice::getAddress()
{
	std::ostringstream result;
	result << "opdi_com://" << this->comport;
	return result.str();
}

/*
	@Override
	protected Set<Charset> getMastersSupportedCharsets() {
		return AndroPDI.SUPPORTED_CHARSETS;
	}
*/

void SerialDevice::logDebug(std::string message)
{
	if (*debug)
		output << message << std::endl;
}

std::string SerialDevice::getSerialAddress()
{
	return this->comport;
}

std::string SerialDevice::getEncryptionKey()
{
	return this->psk;
}

std::string SerialDevice::getDisplayAddress()
{
	// TCP/IP devices may have only an address, but also a name which is different from the device name
//	return getAddress() + (name != null && !name.isEmpty() && !name.equals(getDeviceName()) ? " (" + name + ")" : "");
	return this->getAddress();
}

void SerialDevice::tryConnect()
{
	if (*debug)
		output << "Opening serial port: " << comport << std::endl;

	if (this->serialPort->Open(this->comport.c_str(), baudRate,
							protocol.c_str(),
							ctb::SerialPort::NoFlowControl) >= 0) {
		this->device = this->serialPort;
	} else {
		throw Poco::ApplicationException(std::string(this->getID()) + ": Unable to open serial port: " + this->comport);
	}
}

void SerialDevice::close()
{
	IODevice::close();

	this->serialPort->Close();
}

std::string SerialDevice::getConnectionMessage(unsigned char noConfirmation)
{
	return "ConnectionMessage";
}

bool SerialDevice::tryToUseEncryption() {
	// encryption may be used if a pre-shared key has been specified
	return !this->psk.empty();
}

bool SerialDevice::isSupported()
{
	return true;
}

std::string SerialDevice::getMasterName()
{
	return "Master";
}

char SerialDevice::read()
{
	char buf;
	int code = this->device->Read(&buf, 1);
	// error?
	if (code < 0)
		return code;
	// byte read?
	if (code > 0) {
		return buf;
	}
	// nothing available
	return -1;
}

void SerialDevice::write(char buffer[], int length)
{
	if (!this->device->IsOpen())
		throw Poco::ApplicationException("serial device not open, cannot write");
	this->device->Write(buffer, length);
}

int SerialDevice::read_bytes(char buffer[], int maxlength)
{
	int count = 0;
	char c = -1;
	while ((count < maxlength) && ((c = read()) > 0))
		buffer[count++] = c;
	return count;
}

int SerialDevice::hasBytes()
{
	char buf;
	int code = this->device->Read(&buf, 1);
	// error?
	if (code < 0)
		return code;
	// byte read?
	if (code > 0) {
		this->device->PutBack(buf);
		return 1;
	}
	// nothing available
	return 0;
}

int SerialDevice::read(char buffer[], int length)
{
	return read_bytes(buffer, length);
}
