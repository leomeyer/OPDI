package org.ospdi.opdi.protocol;

import java.util.Queue;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.utils.ResourceFactory;
import org.ospdi.opdi.utils.Strings;


/** This class implements generic functions of OPDI communication protocols.
 * 
 * @author Leo
 *
 */
public abstract class AbstractProtocol {
	
	public interface IAbortable {
		public boolean isAborted();
	}

	public static final char SEPARATOR = ':';
	public static final String DISCONNECT = "Dis";
	public static final String ERROR = "Err";

	protected IDevice device;
	
	protected AbstractProtocol(IDevice device) {
		this.device = device;
	}

	protected IDevice getDevice() {
		return device;
	}

	/** Tries to send a message to the device. If the device is not connected, throws a DisconnectedException 
	 * otherwise returns the channel of the message.
	 * 
	 * @param message
	 * @return
	 * @throws DisconnectedException 
	 */
	protected int send(Message message) throws DisconnectedException {
		if (!device.isConnected()) 
			throw new DisconnectedException();
		device.sendMessages(message);
		return message.getChannel();
	}
	
	/** Sends the error message to the device. This will cause the connection to be terminated.
	 * 
	 * @param message
	 */
	protected void sendError(String message) {
		if (!device.isConnected()) return;
		device.sendMessages(new Message(0, Strings.join(SEPARATOR, ERROR, message)));
	}
	
	/** This method expects an input message with the specific channel within the specified timeout
	 * (in milliseconds). If such a message does not arrive a TimeoutException is raised. Otherwise
	 * the message is returned. This method blocks the calling thread. It should not be called
	 * from the UI thread. If the device is disconnected during waiting a DisconnectedException is raised.
	 * If the thread is interrupted an InterruptedException is thrown.
	 * Time calculation is not absolutely exact but it's guaranteed to not be less than timeout milliseconds.
	 * 
	 */
	protected Message expect(long channel, int timeout, IAbortable abortable) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException {
		if (channel < 0)
			throw new DisconnectedException();
		Queue<Message> queue = device.getInputMessages();
		long startTime = System.currentTimeMillis();
		while (System.currentTimeMillis() - startTime < timeout && 
				(abortable == null || !abortable.isAborted()) &&
				device.isConnected()) {
			Message message = null;
			synchronized(queue) {
				// check whether a message with this channel is in the queue
				for (Message m: queue) {
					if (m.getChannel() == channel) {
						message = m;
						break;				
					}
				}
				if (message != null) {
					queue.remove(message);
					return message;
				}
			}	// synchronized(queue)
			// not found
			// wait for one ms
			Thread.sleep(1);
		}
		if (abortable != null && abortable.isAborted())
			throw new InterruptedException("The operation was interrupted");
		
		// the device may have disconnected (due to an error or planned action)
		if (!device.isConnected())
			throw new DisconnectedException();
		
		// the operation timed out
		device.setError(-1, ResourceFactory.instance.getString(ResourceFactory.TIMEOUT_WAITING_FOR_MESSAGE));
		throw new TimeoutException(ResourceFactory.instance.getString(ResourceFactory.TIMEOUT_WAITING_FOR_MESSAGE));
	}
	
	// A convenience method without an abortable.
	protected Message expect(long channel, int timeout) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException {
		return expect(channel, timeout, null);
	}

	/** Initiates the protocol. Must be provided by subclasses.
	 * 
	 */
	public abstract void initiate();

	/** Sends the disconnect message to the connected device.
	 * 
	 */
	public void disconnect() {
		// send disconnect message
		try {
			send(new Message(0, DISCONNECT));
		} catch (DisconnectedException e) {
			// ignore DisconnectedExceptions
		}
		// do not expect an answer
	}

}
