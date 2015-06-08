#ifndef __OPDI_SERIALDEVICE_H
#define __OPDI_SERIALDEVICE_H

#define TIMEOUT			10000		// milliseconds

#include <string>

// serial port library
#include "ctb-0.16/ctb.h"

#include "Poco/URI.h"

#include "opdi_platformtypes.h"

#include "opdi_IODevice.h"

/** Implements IDevice for a serial device.
 * 
 * @author Leo
 *
 */
class SerialDevice : public IODevice
{
	
protected:
	Poco::URI uri;
	std::string name;
	std::string comport;
	int baudRate;
	std::string protocol;
	std::string psk;
	
	ctb::IOBase* device;
	ctb::SerialPort* serialPort;

	// pointer to debug flag (logDebug)
	bool *debug;

public :
	/** Deserializing constructor */
	SerialDevice(std::string id, Poco::URI uri, bool *debug);
	
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

	virtual std::string getSerialAddress();
	
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

#endif		// __OPDI_SERIALDEVICE_H
