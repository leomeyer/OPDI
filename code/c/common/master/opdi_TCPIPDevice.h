#ifndef __OPDI_TCPIPDEVICE_H
#define __OPDI_TCPIPDEVICE_H

#define TIMEOUT			10000		// milliseconds
#define STANDARD_PORT	13110

#include <string>
#include <Poco\Net\StreamSocket.h>
#include <Poco\Net\SocketStream.h>
#include <Poco\URI.h>

#include "opdi_platformtypes.h"
#include <master\opdi_IODevice.h>

/** Implements IDevice for a TCP/IP device.
 * 
 * @author Leo
 *
 */
class TCPIPDevice : public IODevice
{
	
protected:
	std::string name;
	std::string host;
	int port;
	std::string psk;
	
	Poco::Net::SocketAddress address;
	Poco::Net::StreamSocket socket;

	// pointer to debug flag (logDebug)
	bool *debug;

public :
	/** Deserializing constructor */
	TCPIPDevice(std::string id, Poco::URI uri, bool *debug);
	
	/** Standard constructor */
	// TCPIPDevice(std::string name, std::string address, int port, std::string psk);
	
	virtual bool prepare() override;

	virtual std::string getName();

	virtual std::string getLabel();

	virtual std::string getAddress();
	
/*	
	protected Set<Charset> getMastersSupportedCharsets() {
		return AndroPDI.SUPPORTED_CHARSETS;
	}
*/

	virtual void logDebug(std::string message);

	virtual std::string getTCPIPAddress();

	virtual int getPort();
	
	virtual std::string getEncryptionKey();

	virtual std::string getDisplayAddress();

    virtual void tryConnect();

	virtual void close();
	
	virtual std::string getConnectionMessage(uint8_t noConfirmation);

	virtual bool tryToUseEncryption();

	bool isSupported() override;
	std::string getMasterName() override;
	char read() override;
	int read_bytes(char buffer[], int maxlength) override;
	int hasBytes() override;
	int read(char buffer[], int length) override;
	void write(char buffer[], int length) override;
};

#endif
