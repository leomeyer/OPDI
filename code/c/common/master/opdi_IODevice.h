#ifndef __OPDI_IODEVICE_H
#define __OPDI_IODEVICE_H

#include "opdi_IBasicProtocol.h"
#include "opdi_MessageQueueDevice.h"
#include "opdi_IDevice.h"

class DeviceException : public Poco::Exception
{
public:
	DeviceException(): Poco::Exception() {};
	DeviceException(std::string message): Poco::Exception(message, 0) {};
};

class CancelledException : public Poco::Exception
{
};

class AuthenticationException : public Poco::Exception
{
};

class ICredentialsCallback
{
public:
	virtual bool getCredentials(std::string *name, std::string *password, bool *save) = 0;
};

class IODevice;

/**
    * Runs while attempting to make an outgoing connection
    * with a device. It runs straight through; the connection either
    * succeeds or fails.
    * If a connection attempt succeeds, it calls the handshake() method
    * which has to negotiate and return the protocol that is to be
    * used for this device.
    */
class ConnectRunner : public Poco::Runnable, ICredentialsCallback
{
private:
	IODevice* device;
	IDeviceListener* csListener;
	bool aborted;
    bool done;

public:
	ConnectRunner();

	ConnectRunner(IODevice* device, IDeviceListener* csListener);
		
	bool getCredentials(std::string *user, std::string *password, bool *save) override;

	void run() override;
		
	void abort();
};


/** Defines a queued device that communicates via input and output streams.
 *  Implements the initial handshake protocol with such a device.
 * @author Leo
 *
 */
class IODevice : public MessageQueueDevice
{

protected:

	IBasicProtocol* protocol;

	std::string user;
	std::string password;
	int flags;
	std::string deviceSuppliedName;

	ConnectRunner connectRunner;

	// The thread that does the connecting
	Poco::Thread* connThread;
	/*
	// default locale of the app is preset
	@SuppressWarnings("serial")
	private List<String> preferredLocales = new ArrayList<String>() {
		{ add(Locale.getDefault().toString()); }
	};
	*/

	// default constructor
	IODevice(std::string id);

	void setDeviceSuppliedName(std::string deviceSuppliedName);

	void setFlags(int flags);

public: 
	bool hasCredentials() override;

	std::string getUser() override;

	void setUser(std::string user) override;

	std::string getPassword() override;

	void setPassword(std::string password) override;

	int getFlags();

	std::string getDeviceName() override;

	std::vector<std::string> getPreferredLocales();

	void setPreferredLocales(std::vector<std::string>) ;
	
	std::string getPreferredLocalesString();
	
	void connect(IDeviceListener* listener) override;
	
	void abortConnect() override;
	
	void disconnect(bool onError) override;
	
	IBasicProtocol* getProtocol() override;

	BasicDeviceCapabilities* getCapabilities() override;
	
	void getHandshakeMessage(unsigned int partCount, std::vector<std::string>& results);
	
	void expectAgreement();
	
	/** Returns the protocol implementation that is to be used for this device.
	 * Implements the standard handshake and uses the ProtocolFactory.
	 * Uses synchronous communication with the device.
	 * 
	 * @return
	 * @throws TimeoutException 
	 * @throws AbortedException 
	 * @throws InterruptedException 
	 * @throws IOException 
	 * @throws ProtocolException 
	 * @throws DisconnectedException 
	 * @throws DeviceException 
	 * @throws CancelledException 
	 * @throws AuthenticationException 
	 * @throws MessageException 
	 */
	IBasicProtocol* handshake(ICredentialsCallback* credCallback);

	/** Attempt to connect to the device.
	 * 
	 * @throws IOException
	 */
	virtual void tryConnect() = 0;

};

#endif
