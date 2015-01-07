#include <string>
#include <sstream>
#include <iostream>

#include "Poco/NumberFormatter.h"
#include "opdi_constants.h"

#include "opdi_IODevice.h"
#include <opdi_strings.h>
#include <master\opdi_AbstractProtocol.h>
#include <master\opdi_StringTools.h>
#include <master\opdi_ProtocolFactory.h>

class IODevice;

class ConnectingListener : public IDeviceListener 
{
protected:
	IODevice *device;
	IDeviceListener* outerListener;

public:
	ConnectingListener(IODevice* aDevice, IDeviceListener* listener)
	{
		device = aDevice;
		outerListener = listener;
	}
	
	void connectionOpened(IDevice* aDevice) override
	{
		// clear remaining messages
		device->clearQueues();
				
		// add the listener
		device->addConnectionListener(outerListener);
				
		// start the messaging thread
		device->startMessageProcessing(device->getProtocol());

		// start the protocol
		device->getProtocol()->initiate();
				
		device->setStatus(CONNECTED, "");
	}
			
	void connectionInitiated(IDevice* aDevice) override
	{
		device->setStatus(CONNECTING, "");
		outerListener->connectionInitiated(device);
	}
			
	void connectionAborted(IDevice* aDevice) override
	{
		device->setStatus(DISCONNECTED, "");
		outerListener->connectionAborted(device);
	}

	void connectionFailed(IDevice* aDevice, std::string message) override 
	{
		device->setStatus(DISCONNECTED, message);
		outerListener->connectionFailed(device, message);
	}

	void connectionTerminating(IDevice* aDevice) override
	{
		device->setStatus(DISCONNECTING, "");
		outerListener->connectionTerminating(device);
	}
			
	void connectionClosed(IDevice* aDevice) override
	{
		device->setStatus(DISCONNECTED, "");
		outerListener->connectionClosed(device);
	}

	void connectionError(IDevice* aDevice, std::string message) override
	{
		device->setStatus(ERR, message);
		outerListener->connectionError(device, message);
	}
			
	bool getCredentials(IDevice* device, std::string *user, std::string *password, bool *save) override
	{
		return outerListener->getCredentials(device, user, password, save);
	}
			
	void receivedDebug(IDevice* device, std::string message) override
	{
		outerListener->receivedDebug(device, message);
	}
			
	void receivedReconfigure(IDevice* device) override 
	{
		outerListener->receivedReconfigure(device);
	}
			
	void receivedRefresh(IDevice* device, std::vector<std::string> portIDs) override 
	{
		outerListener->receivedRefresh(device, portIDs);
	}
			
	void receivedError(IDevice* device, std::string text) override
	{
		outerListener->receivedError(device, text);
	}
};


IODevice::IODevice(std::string id) : MessageQueueDevice(id)
{
	flags = 0;
	protocol = NULL;
}

bool IODevice::hasCredentials()
{
	return !user.empty();
}

std::string IODevice::getUser()
{
	return user;
}

void IODevice::setUser(std::string user)
{
	this->user = user;
}

std::string IODevice::getPassword()
{
	return password;
}

void IODevice::setPassword(std::string password)
{
	this->password = password;
}

int IODevice::getFlags()
{
	return flags;
}

void IODevice::setFlags(int flags)
{
	this->flags = flags;
}

std::string IODevice::getDeviceName()
{
	return deviceSuppliedName;
}

void IODevice::setDeviceSuppliedName(std::string deviceSuppliedName)
{
	this->deviceSuppliedName = deviceSuppliedName;
}

std::string IODevice::getPreferredLocalesString()
{
	return "";
}

void IODevice::connect(IDeviceListener *listener)
{
	if (isConnected()) return;
	// connection attempt in progress?
	if (isConnecting()) {
		abortConnect();
	}
		
	// all connections begin without encryption
	clearEncryption();
		
	connectRunner = ConnectRunner(this, new ConnectingListener(this, listener));

	// start the connection thread
	connThread = new Poco::Thread();
	connThread->start(connectRunner);
}
	
void IODevice::abortConnect() 
{
	// not yet connected?
	if (connThread->isRunning()) {
		connectRunner.abort();
	} else
		// connection thread has finished -> regular disconnect
		close();
}
	
void IODevice::disconnect(bool onError) 
{
	if (getProtocol())
		getProtocol()->disconnect();
	MessageQueueDevice::disconnect(onError);
}
	
IBasicProtocol* IODevice::getProtocol() 
{
	return protocol;
}

BasicDeviceCapabilities* IODevice::getCapabilities()
{
	if (!getProtocol())
		throw DeviceException("Can't load capabilities; no protocol");

	return getProtocol()->getDeviceCapabilities();
}

void IODevice::getHandshakeMessage(int expectedPartCount, std::vector<std::string>& results)
{
	if (!isConnecting()) return;
	
	Message* m = receiveHandshakeMessage(AbstractProtocol::HANDSHAKE_TIMEOUT /*, new IAbortable() {
		@Override
		public boolean isAborted() {
			return !isConnecting();
		}
	} */);

	// split the message
	StringTools::split(m->getPayload(), AbstractProtocol::SEPARATOR, results);
	int channel = m->getChannel();

	// free memory
	delete m;

	if (channel != 0)
		throw ProtocolException("Invalid handshake message");
		
/*
	if (results.size != 0) {
		throw ProtocolException("Invalid handshake message");
	}
*/

	// devices may disconnect or send error messages at any time
	if (results[0] == OPDI_Disconnect)
		throw DisconnectedException();
	else if (results[0] == OPDI_Error) {
		if (results.size() > 1) 
			throw DeviceException(results[1]);
		else
			throw DeviceException();
	}
	else if (results[0] == OPDI_Disagreement) {
		std::stringstream msg;
		msg << (results.size() > 1 ? results[1] : "") << (results.size() > 2 ? results[2] : "");

		if (msg.str().size() > 1) {
			std::stringstream msg2;
			msg2 << "The device cancelled the handshake because: " << msg.str();
			throw ProtocolException(msg2.str());
		}
		else
			throw ProtocolException(msg.str());
	}

	if (results.size() != expectedPartCount)
		throw ProtocolException("Invalid handshake: number of expected parts did not match");
}
	
void IODevice::expectAgreement()
{
	// expect an agreement
	int REPLY = 0;
	std::vector<std::string> reply;
	getHandshakeMessage(1, reply);
	if (reply[REPLY] == OPDI_Disagreement)
		throw DisagreementException();
	if (reply[REPLY] != OPDI_Agreement)
		throw ProtocolException("Agreement expected");				
}


IBasicProtocol* IODevice::handshake(ICredentialsCallback* credCallback)
{
	std::string supportedEncryptions = ""; // (this.tryToUseEncryption() ? StringTools::join(',', std::vector<std::string>("AES")) : "");
	// send handshake message
	Message handshake(0, StringTools::join(AbstractProtocol::SEPARATOR, OPDI_Handshake, OPDI_Handshake_version, Poco::NumberFormatter::format(flags), supportedEncryptions));
		
	////////////////////////////////////////////////////////////
	///// Send: Handshake
	////////////////////////////////////////////////////////////
		
	sendSynchronous(&handshake);
		
	////////////////////////////////////////////////////////////
	///// Receive: Handshake Reply
	////////////////////////////////////////////////////////////
		
	int HSHAKE = 0;
	int VERSION = 1;
	int ENCODING = 2;
	int ENCRYPTION = 3;
	int FLAGS = 4;
	int PROTOCOLS = 5;
	int PART_COUNT = 6;

	std::vector<std::string> parts;
	getHandshakeMessage(PART_COUNT, parts);
		
	if (parts[HSHAKE] != OPDI_Handshake) {
		throw ProtocolException("Unexpected handshake message");
	}
				
	// check version
	if (AbstractProtocol::HANDSHAKE_VERSION_DOUBLE < AbstractProtocol::parseDouble(parts[VERSION], "Handshake version", 0, std::numeric_limits<double>::max())) {
		// send error message to the device
		sendSynchronous(&Message(0, OPDI_Disagreement));
		throw ProtocolException("Handshake version not supported"); //, parts[VERSION]);
	}

	// encoding name supplied?
	if (parts[ENCODING] != "") {
		// find encoding
			/*
		try {
			Charset encoding = Charset.forName(parts[ENCODING]);
			if (!getMastersSupportedCharsets().contains(encoding))
				throw new UnsupportedCharsetException(parts[ENCODING]);
				
			// encoding is valid
			// encoding to use for future messages
			setEncoding(encoding);
		} catch (Exception e) {				
			// send error message to the device
			sendSynchronous(new Message(0, DISAGREEMENT));
				
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.ENCODING_NOT_SUPPORTED) + parts[ENCODING]);
		}
		*/
	}
		
	// encryption specified?
	if (parts[ENCRYPTION] != "") {
		/*
		// find encryption
		try {
			Encryption encryption = Encryption.valueOf(parts[ENCRYPTION]);
			// encryption to use for future messages
			setEncryption(encryption);
		} catch (DeviceException e) {
			throw e;			
		} catch (Exception e) {
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + parts[ENCRYPTION]);
		}
		*/
	}
		
	int deviceFlags = 0;
	try {
		deviceFlags = AbstractProtocol::parseInt(parts[FLAGS], "flags", std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
	} catch (Poco::Exception e) {
		throw ProtocolException("Flags invalid"); // + parts[FLAGS]);
	}
		
	// check flags
	if ((flags & OPDI_FLAG_ENCRYPTION_REQUIRED) == OPDI_FLAG_ENCRYPTION_REQUIRED) {
		if (getEncryption() == NO_ENCRYPTION)
			// the device did not specify an encryption method
			throw ProtocolException("Encryption required but device did not specify an encryption method");
	}
	if ((flags & OPDI_FLAG_ENCRYPTION_NOT_ALLOWED) == OPDI_FLAG_ENCRYPTION_NOT_ALLOWED) {
		if (getEncryption() != NO_ENCRYPTION)
			// the device did specify an encryption method but it's not allowed
			throw ProtocolException("Device specified an encryption method but prohibited encryption");
	}
		
	// determine the protocol
	std::vector<std::string> protos;
	StringTools::split(parts[PROTOCOLS], ',', protos);
		
	IBasicProtocol* prot = NULL;		
	// try each protocol indicator

	for (std::vector<std::string>::iterator proto = protos.begin(); proto != protos.end(); ++proto) {
		IBasicProtocol* protocol = ProtocolFactory::getProtocol(this, (*proto));
		if (protocol != NULL) {
			prot = protocol;
			break;
		}
	}
		
	// protocol found?
	if (prot == NULL) {
		// no match for supported protocols found
		sendSynchronous(&Message(0, OPDI_Disagreement));
		// no possible protocols found
		throw ProtocolException("No supported protocol found");
	}

	////////////////////////////////////////////////////////////
	///// Send: Protocol Select
	////////////////////////////////////////////////////////////
		
	std::string preferredLanguages = getPreferredLocalesString();
	sendSynchronous(&Message(0, StringTools::join(AbstractProtocol::SEPARATOR, prot->getMagic(), preferredLanguages, getMasterName())));
		
	////////////////////////////////////////////////////////////
	///// Receive: Slave Name
	////////////////////////////////////////////////////////////
		
	int DEVICE_NAME = 1;

	if (getEncryption() != 0) {
		// If this particular message fails there may be something wrong with the encryption method.
		// Possibly the master's key is wrong or an encryption error has been detected on the device.
		// We can't receive any error messages from the device though, because it can't send something we
		// understand because encryption is broken. Therefore we handle a timeout specially here.
		try 
		{
			getHandshakeMessage(2, parts);
		}
		catch (TimeoutException te) {
			throw DeviceException("Timeout received. Maybe there is something wrong with the encryption key?");
		}
	}
	else
		getHandshakeMessage(2, parts);
		
	if (parts[0] != OPDI_Agreement) {
		return NULL;
	}
		
	// set the name of the device
	setDeviceSuppliedName(parts[DEVICE_NAME]);
		
	// authentication required?
	if ((deviceFlags & OPDI_FLAG_AUTHENTICATION_REQUIRED) == OPDI_FLAG_AUTHENTICATION_REQUIRED) {
			
		std::string user = getUser();
		std::string password = getPassword();
		// assume that password should be saved if username and password is there
		bool save = (user != "" && password != "");
		// incomplete credentials?
		if (user == "" || password == "") {
			// get credentials from callback
			if (!credCallback->getCredentials(&user, &password, &save)) {
				// cancelled
				throw CancelledException();
			}
		}
			
		////////////////////////////////////////////////////////////
		///// Send: Authentication
		////////////////////////////////////////////////////////////
			
		Message auth = Message(0, StringTools::join(AbstractProtocol::SEPARATOR, OPDI_Auth, user, password));
		sendSynchronous(&auth);

		try {

			////////////////////////////////////////////////////////////
			///// Receive: Confirmation
			////////////////////////////////////////////////////////////
				
			expectAgreement();
		} catch (DisagreementException e) {
			// on error clear password
			setPassword("");
				
			sendSynchronous(&Message(0, OPDI_Disconnect));
			throw AuthenticationException();
		}
			
		// authentication ok; should we save?
		if (save) {
			setUser(user);
			setPassword(password);
		}
	}

	////////////////////////////////////////////////////////////
	///// Handshake completed
	////////////////////////////////////////////////////////////
	this->protocol = prot;

	return prot;
}


ConnectRunner::ConnectRunner()
{
}

ConnectRunner::ConnectRunner(IODevice* device, IDeviceListener* csListener) {
	this->device = device;
	this->csListener = csListener;
}

bool ConnectRunner::getCredentials(std::string *user, std::string *password, bool *save) 
{
	return csListener->getCredentials(device, user, password, save);
}
		
void ConnectRunner::run() {

	csListener->connectionInitiated(device);
				
	// Attempt to connect to the device
	try {
		aborted = false;

		device->tryConnect();

		if (aborted) {
			device->close();
			return;
		}

		// get the protocol
		IBasicProtocol* protocol = device->handshake(this);
					
		if (protocol == NULL)
			throw DisagreementException();
					
	} catch (Poco::IOException& e) {
		device->close();
		if (aborted) return;
		csListener->connectionFailed(device, (std::string)"IO failure: " + e.displayText());
		return;
/*		} catch (InterruptedException e) {
		close();
		if (aborted) return;
		csListener.connectionFailed(device, ResourceFactory.instance.getString(ResourceFactory.OPERATION_WAS_ABORTED));
		Thread.currentThread().interrupt();
		return; */
	} catch (TimeoutException&) {
		device->close();
		if (aborted) return;
		csListener->connectionFailed(device, "Timeout: The device did not respond");
		return;
	} catch (ProtocolException& e) {
		// abort the connection
		try {
			device->sendSynchronous(&Message(0, OPDI_Disconnect));
		} catch (Poco::Exception e1) {
		}
		device->close();
		if (aborted) return;
		csListener->connectionFailed(device, (std::string)"Protocol error: " + e.displayText());
		return;
	} catch (DisconnectedException&) {
		device->close();
		if (aborted) return;
		csListener->connectionFailed(device, "The device closed the connection");
		return;
	} catch (DeviceException& e) {
		device->close();
		if (aborted) return;
		csListener->connectionError(device, e.displayText());
		return;
	} catch (AuthenticationException&) {
		device->close();
		if (aborted) return;
		csListener->connectionFailed(device, "Authentication failed; invalid user name or password");
		return;				
	} catch (CancelledException&) {
		device->close();
		if (aborted) return;
		csListener->connectionAborted(device);
		return;				
	} catch (Poco::Exception& e) {
		device->close();
		if (aborted) return;
		csListener->connectionFailed(device, e.displayText());
		return;
	}
				
	device->setStatus(CONNECTED);
	csListener->connectionOpened(device);
}
		
void ConnectRunner::abort() {
	aborted = true;
    csListener->connectionAborted(device);			
}
