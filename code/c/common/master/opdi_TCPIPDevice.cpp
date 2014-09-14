#include <sstream>

#include "Poco/RegularExpression.h"

#include "opdi_TCPIPDevice.h"
#include "master.h"
#include "main_io.h"

/** Implements IDevice for a TCP/IP device.
 * 
 * @author Leo
 *
 */

using Poco::RegularExpression;

TCPIPDevice::TCPIPDevice(std::string id, Poco::URI uri, bool *debug) : IODevice(id)
{
	this->debug = debug;

	// deserialize information
	if (uri.getScheme() != "opdi_tcp")
		throw Poco::InvalidArgumentException("Can't deserialize; schema is incorrect, expected 'opdi_tcp'");
		
	// split user information into name:password
	// assumption: ':' character does not appear in either part
	std::vector<std::string> parts;
	RegularExpression re("(.+):(.+)");
	re.split(uri.getUserInfo(), parts, 0);
	if ((parts.size() > 1) && (parts[1] != ""))
		setUser(parts[1]);
	if ((parts.size() > 2) && (parts[2] != ""))
		setPassword(parts[2]);
		
	host = uri.getHost();
	port = (uri.getPort() == 0 ? STANDARD_PORT : uri.getPort());

	if (host == "")
		throw Poco::InvalidArgumentException("Host must be specified");

	// TODO parse parameters, especially for name and PSK
	// name = uri.getHost();

}
	
bool TCPIPDevice::prepare() 
{
	try {
		address = Poco::Net::SocketAddress(host, port);
	} catch (Poco::Exception& e) {
		throw Poco::IOException("Error resolving address: " + getAddress(), e, 0);
	}

	if (*debug)
		output << "Connecting to address: " << address.host().toString() << std::endl;

	return true;
}

std::string TCPIPDevice::getName()
{
	return name;
}

std::string TCPIPDevice::getLabel()
{
	std::stringstream result;
	result << "opdi_tcp://";

	if (user != "")
		result << user << "@";

	result << host << ":" << port;

	if (name != "")
		result << "&name=" << name;

	return result.str();
}

std::string TCPIPDevice::getAddress()
{
	std::ostringstream result;
	result << "opdi_tcp://" << host << ":" << port;
	return result.str();
}

/*
	@Override
	protected Set<Charset> getMastersSupportedCharsets() {
		return AndroPDI.SUPPORTED_CHARSETS;
	}
*/

void TCPIPDevice::logDebug(std::string message)
{
	if (*debug)
		output << message << std::endl;
}

std::string TCPIPDevice::getTCPIPAddress() 
{
	return host;
}

int TCPIPDevice::getPort() 
{
	return port;
}
	
std::string TCPIPDevice::getEncryptionKey()
{
	return psk;
}

std::string TCPIPDevice::getDisplayAddress() 
{
	// TCP/IP devices may have only an address, but also a name which is different from the device name
//	return getAddress() + (name != null && !name.isEmpty() && !name.equals(getDeviceName()) ? " (" + name + ")" : "");
	return getAddress();
}

void TCPIPDevice::tryConnect()
{
	// precondition: prepare() has been called to resolve the address

	Poco::Net::IPAddress ipaddress = address.host();

	// connect to the target
	socket = Poco::Net::StreamSocket(address);
}

void TCPIPDevice::close()
{
	IODevice::close();

	socket.close();
}

std::string TCPIPDevice::getConnectionMessage(unsigned char noConfirmation)
{
	return "ConnectionMessage";
}

bool TCPIPDevice::tryToUseEncryption() {
	// encryption may be used if a pre-shared key has been specified
	return !psk.empty();
}

bool TCPIPDevice::isSupported()
{
	return true;
}

std::string TCPIPDevice::getMasterName()
{
	return opdiMasterName;
}

char TCPIPDevice::read()
{
	char result;
	socket.receiveBytes(&result, 1);

	return result;
}

void TCPIPDevice::write(char buffer[], int length)
{
	socket.sendBytes(buffer, length);
}

int TCPIPDevice::read_bytes(char buffer[], int maxlength)
{
	int count = 0;
	while ((socket.available() > 0) && (count < maxlength))
		socket.receiveBytes(&buffer[count++], 1);
	return count;
}

int TCPIPDevice::hasBytes()
{
	return socket.available();
}

int TCPIPDevice::read(char buffer[], int length)
{
	return read_bytes(buffer, length);
}