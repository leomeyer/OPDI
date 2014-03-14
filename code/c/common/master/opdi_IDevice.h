#ifndef __OPDI_IDEVICE_H
#define __OPDI_IDEVICE_H

#include <vector>
#include <string>

#include <POCO/NotificationQueue.h>

#include "opdi_platformtypes.h"

#include "opdi_Message.h"
#include "opdi_IBasicProtocol.h"

enum DeviceStatus
{
		/** The device is not connected; or a connection attempt failed. */
		DISCONNECTED,
		/** The device is currently being connected. */
		CONNECTING,
		/** The device is connected and ready to accept commands. */
		CONNECTED,
		/** The device is being disconnected. */
		DISCONNECTING,
		/** A connection error has occurred or the device has indicated 
		 * an error condition which makes it unusable for control purposes. */
		ERR
};

class IDevice;

class IDeviceListener
{
public:
	virtual void connectionOpened(IDevice* device) = 0;

	virtual void connectionInitiated(IDevice* aDevice) = 0;

	virtual void connectionAborted(IDevice* device) = 0;

	virtual void connectionFailed(IDevice* aDevice, std::string message) = 0;

	virtual void connectionTerminating(IDevice* device) = 0;

	virtual void connectionClosed(IDevice* device) = 0;

	virtual void connectionError(IDevice* device, std::string message) = 0;

	virtual bool getCredentials(IDevice* device, std::string *user, std::string *password, bool *save) = 0;

	virtual void receivedDebug(IDevice* device, std::string message) = 0;

	virtual void receivedReconfigure(IDevice* device) = 0;

	virtual void receivedRefresh(IDevice* device, std::vector<std::string> portIDs) = 0;

	virtual void receivedError(IDevice* device, std::string text) = 0;
};

/** This interface describes a remote device.
 * 
 * @author Leo
 *
 */
class IDevice
{
protected:
	// internal ID for management purposes
	// Used to identify devices, see operator ==
	std::string id;

	IDevice(std::string id);

public:

	// support for find(); defines equality on ID
	bool operator== (const std::string anID) const 
	{
		return id == anID;
	}

	// support for find(); defines equality on ID
	bool operator== (const IDevice* aDevice) const 
	{
		return id == aDevice->id;
	}

	std::string getID();

	/** This method checks whether the device is supported on the current hardware. This covers the case
	 * that settings have been copied from another device with different capabilities.
	 * @return
	 */
	virtual bool isSupported() = 0;

	virtual bool prepare() = 0;

	/** Returns the name of the master. */
	virtual std::string getMasterName() = 0;

	virtual void logDebug(std::string) = 0;
	
	/** The display name of the device. */
	virtual std::string getName() = 0;
	
	/** The display label of the device. */
	virtual std::string getLabel() = 0;
	
	/** The unique address of the device. */
	virtual std::string getAddress() = 0;
	
	/** The status of the device. */
	virtual DeviceStatus getStatus() = 0;
	
	/** The status as text representation. */
	virtual std::string getStatusText() = 0;

	/** Specifies whether to use encryption when connecting to the device.
	 * This influences whether a selection of encryption methods is presented to the device
	 * during the handshake. If false, no encryption methods are sent.
	 * @return
	 */
	virtual bool tryToUseEncryption() = 0;
	
	/** Attempt to connect the device using the callback functions in the listener.
	 *  Has no effect if the device is already connected.
	 *  The device owns the device listener and frees it after closing.
	 * @param csListener
	 */
	virtual void connect(IDeviceListener* csListener) = 0;
	
	/** Abort a connection attempt.
	 * If no connection attempt is running, this has no effect.
	 */
	virtual void abortConnect() = 0;

	/** Disconnect an active connection.
	 * If no connection is active, this has no effect.
	 */
	virtual void disconnect(bool onError) = 0;
	
	/** Returns 1 if a connection is being setup.
	 * 
	 * @return
	 */
	virtual bool isConnecting() = 0;
	
	/** Returns 1 if the device is connected. 
	 *
	 * @return
	 */
	virtual bool isConnected() = 0;
	
	/** Sends the given messages to the device.
	 * 
	 * @param messages
	 */
	virtual void sendMessage(Message* message) = 0;
	
	/** Returns the input message queue of the device.
	 * Access to methods of this queue MUST be synchronized on the queue!
	 * @return
	 */
	virtual Poco::NotificationQueue* getInputMessages() = 0;
	
	/** Gets the protocol implementation for this device.
	 * 
	 * @return
	 */
	virtual IBasicProtocol* getProtocol() = 0;

	/** Adds a listener for connection events.
	 * 
	 * @param listener
	 */
	virtual void addConnectionListener(IDeviceListener* listener) = 0;
	
	/** Removes a listener for connection events.
	 * 
	 * @param listener
	 */
	virtual void removeConnectionListener(IDeviceListener* listener) = 0;

	/** Returns the name of the device as specified by the device.
	 * This value is only valid during a connection.
	 * @return
	 */
	virtual std::string getDeviceName() = 0;

	/** Returns the display address that contains useful information for the user.
	 * 
	 * @return
	 */
	virtual std::string getDisplayAddress() = 0;
	
	/** The user that is used for authentication.
	 * Must return null if the user is not set.
	 * @return
	 */
	virtual std::string getUser() = 0;

	/** Sets the user that is used for authentication.
	 * null if no user is to be set.
	 * @return
	 */
	virtual void setUser(std::string user) = 0;
	
	/** The password that is used for authentication.
	 * Must return null if the password is not set.
	 * @return
	 */
	virtual std::string getPassword() = 0;

	/** Sets the password that is used for authentication.
	 * null if no password is to be set.
	 * @return
	 */
	virtual void setPassword(std::string password) = 0;

	/** Returns true if credentials are set.
	 * 
	 * @return
	 */
	virtual bool hasCredentials() = 0;
	
	/** Returns true if there is an encrypted connection established with this device.
	 * 
	 * @return
	 */
	virtual bool usesEncryption() = 0;

	/** Returns the message that is shown to the user before connecting.
	 * Should be of the form "Do you really want to connect?"
	 * If null is returned, the device connects right away.
	 * If noConfirmation is true, the returned message should be null if possible.
	 * @return
	 */
	virtual std::string getConnectionMessage(uint8_t noConfirmation) = 0;
	
	/** Is called when a Reconfigure message has been received. Causes reloading of device capabilities.
	 */
	virtual void receivedReconfigure() = 0;

	/** Is called when a debug message has been received from the slave.
	 * 
	 * @param message
	 */
	virtual void receivedDebug(std::string message) = 0;

	/** Is called when a request to poll the specified ports has been received.
	 * If the portIDs list is empty, all possible ports should be polled.
	 * @param portIDs
	 */
	virtual void receivedRefresh(std::vector<std::string> portIDs) = 0;

	/** Is called when a device is in an error state. 
	 * error may contain the error number. If this number is > 0, the error has been signaled from the device.
	 * If error is -1, the error occurred on the master.
	 * text can be null or optionally contain additional information.
	 * Devices do not send or expect any more messages after signaling an error.
	 * @param error
	 * @param text
	 */
	virtual void setError(int error, std::string text) = 0;

	/** Returns the time in ms of the last message send (as returned by GetTickCount()).
	 * 
	 * @return
	 */
	virtual long getLastSendTimeMS() = 0;
};

inline IDevice::IDevice(std::string id)
{
	this->id = id;
}

inline std::string IDevice::getID()
{
	return id;
}
#endif
