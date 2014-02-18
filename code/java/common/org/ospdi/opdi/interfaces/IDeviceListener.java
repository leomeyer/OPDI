package org.ospdi.opdi.interfaces;

/** Defines callback functions about the connection status.
 * 
 * @author Leo
 *
 */
public interface IDeviceListener {

	/** Is called when a connection attempt has been started.
	 * 
	 * @param device
	 */
	public void connectionInitiated(IDevice device);
	
	/** Is called when a connection attempt has been aborted.
	 * 
	 * @param device
	 */
	public void connectionAborted(IDevice device);

	/** Is called when a connection attempt has failed for the given device.
	 * 
	 * @param device
	 * @param message
	 */
	public void connectionFailed(IDevice device, String message);
	
	/** Is called when a connection has been established and message processing runs.
	 * 
	 * @param device
	 */
	public void connectionOpened(IDevice device);

	/** Is called when an error occurred that led to the termination of the connection.
	 * 
	 * @param device
	 * @param message
	 */
	public void connectionError(IDevice device, String message);
	
	/** Is called when the connection is about to be terminated. */
	public void connectionTerminating(IDevice device);
	
	/** Is called when a connection has been closed.
	 * 
	 * @param device
	 */
	public void connectionClosed(IDevice device);

	/** The callback method must put the user name into the first component and the
	 * password into the second component.
	 * May return false if the handshake should be cancelled.
	 * @param namePassword
	 * @param save 
	 */
	public boolean getCredentials(IDevice device, String[] namePassword, Boolean[] save);

	/** Is called when a debug message has been received from the device.
	 * 
	 * @param device
	 * @param message
	 */
	public void receivedDebug(IDevice device, String message);

	/** Is called when the device requests that the master should reload device capabilities.
	 * 
	 * @param device
	 */
	public void receivedReconfigure(IDevice device);

	/** Is called when the device requests that the master should reload the state of the specified port(s).
	 * If the portIDs list is empty, all ports should be reloaded.
	 * @param device
	 * @param portIDs
	 */
	public void receivedRefresh(IDevice device, String[] portIDs);

	/** Is called when a device has signaled an error.
	 * The connection is closed. Listeners should cleanup resources and exit processing.
	 * @param device
	 * @param text
	 */
	public void receivedError(IDevice device, String text);	
}
