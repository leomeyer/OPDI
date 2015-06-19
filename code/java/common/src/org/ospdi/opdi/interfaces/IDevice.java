package org.ospdi.opdi.interfaces;

import java.util.Queue;

import org.ospdi.opdi.devices.DeviceInfo;
import org.ospdi.opdi.protocol.Message;

/** This interface describes a remote device.
 * 
 * @author Leo
 *
 */
public interface IDevice {
	
	/** This enum is used to indicate the status of a remote device. */
	public enum DeviceStatus {
		
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
		ERROR
	}
	
	/** Is used to indicate that this device must use encryption. 
	 * The device may choose one of the supported encryptions. */
	public static final int FLAG_ENCRYPTION_REQUIRED = 1;
	/** Is used to indicate that this device may not use encryption. */
	public static final int FLAG_ENCRYPTION_NOT_ALLOWED = 2;
	/** Is used to indicate that this device requires authentication. 
	 * Authentication without encryption causes the password to be sent in plain text! */
	public static final int FLAG_AUTHENTICATION_REQUIRED = 4;
	
	/** Must return a serialized form that can later be deserialized by a deserializing constructor
	 * (a constructor with only one string argument).
	 * @return
	 */
	public String serialize();

	/** This method checks whether the device is supported on the current hardware. This covers the case
	 * that settings have been copied from another device with different capabilities.
	 * @return
	 */
	public boolean isSupported();
	
	/** The display name of the device. */
	public String getName();
	
	/** The display label of the device. */
	public String getLabel();
	
	/** The unique address of the device. */
	public String getAddress();
	
	public DeviceInfo getDeviceInfo();

	public void setDeviceInfo(DeviceInfo deviceInfo);
	
	/** The resource id that identifies the icon. */
	public int getImageResource();

	/** The status of the device. */
	public DeviceStatus getStatus();
	
	/** The status as text representation. */
	public String getStatusText();

	/** Specifies whether to use encryption when connecting to the device.
	 * This influences whether a selection of encryption methods is presented to the device
	 * during the handshake. If false, no encryption methods are sent.
	 * @return
	 */
	public boolean tryToUseEncryption();
	
	/** Attempt to connect the device using the callback functions in the listener.
	 *  Has no effect if the device is already connected.
	 * @param csListener
	 */
	public void connect(IDeviceListener csListener);
	
	/** Abort a connection attempt.
	 * If no connection attempt is running, this has no effect.
	 */
	public void abortConnect();

	/** Disconnect an active connection.
	 * If no connection is active, this has no effect.
	 */
	public void disconnect(boolean onError);
	
	/** Returns true if a connection is being setup.
	 * 
	 * @return
	 */
	public boolean isConnecting();
	
	/** Returns true if the device is connected. 
	 *
	 * @return
	 */
	public boolean isConnected();
	
	/** Sends the given messages to the device.
	 * 
	 * @param messages
	 */
	public void sendMessages(Message... messages);
	
	/** Returns the input message queue of the device.
	 * Access to methods of this queue MUST be synchronized on the queue!
	 * @return
	 */
	public Queue<Message> getInputMessages();
	
	/** Gets the protocol implementation for this device.
	 * 
	 * @return
	 */
	public IBasicProtocol getProtocol();

	/** Adds a listener for connection events.
	 * 
	 * @param listener
	 */
	public void addConnectionListener(IDeviceListener listener);
	
	/** Removes a listener for connection events.
	 * 
	 * @param listener
	 */
	public void removeConnectionListener(IDeviceListener listener);

	/** Returns the name of the device as specified by the device.
	 * This value is only valid during a connection.
	 * @return
	 */
	public String getDeviceName();

	/** Returns the display address that contains useful information for the user.
	 * 
	 * @return
	 */
	public String getDisplayAddress();
	
	/** The user that is used for authentication.
	 * Must return null if the user is not set.
	 * @return
	 */
	public String getUser();

	/** Sets the user that is used for authentication.
	 * null if no user is to be set.
	 * @return
	 */
	public void setUser(String user);
	
	/** The password that is used for authentication.
	 * Must return null if the password is not set.
	 * @return
	 */
	public String getPassword();

	/** Sets the password that is used for authentication.
	 * null if no password is to be set.
	 * @return
	 */
	public void setPassword(String password);

	/** Returns true if credentials are set.
	 * 
	 * @return
	 */
	public boolean hasCredentials();
	
	/** Returns true if there is an encrypted connection established with this device.
	 * 
	 * @return
	 */
	public boolean usesEncryption();

	/** Returns the message that is shown to the user before connecting.
	 * Should be of the form "Do you really want to connect?"
	 * If null is returned, the device connects right away.
	 * If noConfirmation is true, the returned message should be null if possible.
	 * @return
	 */
	public String getConnectionMessage(boolean noConfirmation);
	
	/** Logs a debug message (on the master).
	 * 
	 * @param message
	 */
	public void logDebug(String message);

	/** Is called when a Reconfigure message has been received. Causes reloading of device capabilities.
	 */
	public void receivedReconfigure();

	/** Is called when a debug message has been received from the slave.
	 * 
	 * @param message
	 */
	public void receivedDebug(String message);

	/** Is called when a request to poll the specified ports has been received.
	 * If the portIDs list is empty, all possible ports should be polled.
	 * @param portIDs
	 */
	public void receivedRefresh(String[] portIDs);

	/** Is called when a device is in an error state. 
	 * error may contain the error number. If this number is > 0, the error has been signaled from the device.
	 * If error is -1, the error occurred on the master.
	 * text can be null or optionally contain additional information.
	 * Devices do not send or expect any more messages after signaling an error.
	 * @param error
	 * @param text
	 */
	public void setError(int error, String text);

	/** Returns the time in ms of the last message send (as returned by System.currentTimeMillis()).
	 * 
	 * @return
	 */
	public long getLastSendTimeMS();
}
