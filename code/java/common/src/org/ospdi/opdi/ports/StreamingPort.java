package org.ospdi.opdi.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.drivers.DriverFactory;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.interfaces.IDriver;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.PortErrorException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.utils.Strings;


/** Represents a streaming port on a device.
 * 
 * @author Leo
 *
 */
public class StreamingPort extends Port {
	
	/** A listener that is notified by incoming data events.
	 * 
	 * @author Leo
	 *
	 */
	public interface IStreamingPortListener {
		
		/** Is called when data has been received for this streaming port.
		 * 
		 * @param port
		 * @param data
		 */
		public void dataReceived(StreamingPort port, String data);
		
		/** Is called when the port has been unbound by the device, e. g. by issuing a reconfigure command.
		 * 
		 * @param port
		 */
		public void portUnbound(StreamingPort port);
	}
	
	public static final String MAGIC = "SP";
	
	public static final int FLAG_AUTOBIND = 1;

	protected String driverID;
	protected IDriver driver;
	protected int flags;
	protected int channel;
	protected IStreamingPortListener streamingPortListener;
	
	protected StreamingPort(String id, String name, String driverID, int flags) {
		super(null, id, name, PortType.STREAMING, PortDirCaps.BIDIRECTIONAL);
		setDriverID(driverID);
		this.flags = flags;
	}
	
	/** Constructor for deserializing from wire form
	 * 
	 * @param protocol
	 * @param parts
	 * @throws ProtocolException
	 */
	public StreamingPort(IBasicProtocol protocol, String[] parts) throws ProtocolException {
		super(protocol, null, null, PortType.STREAMING, PortDirCaps.BIDIRECTIONAL);
		
		final int ID_PART = 1;
		final int NAME_PART = 2;
		final int DRIVER_PART = 3;
		final int MAXLENGTH_PART = 4;
		final int PART_COUNT = 5;
		
		checkSerialForm(parts, PART_COUNT, MAGIC);

		setID(parts[ID_PART]);
		setName(parts[NAME_PART]);
		setDriverID(parts[DRIVER_PART]);
		flags = Strings.parseInt(parts[MAXLENGTH_PART], "flags", 0, Integer.MAX_VALUE);
	}
	
	public String serialize() {
		return Strings.join(':', MAGIC, getID(), getName(), getDriverID(), flags);
	}
	
	public String getDriverID() {
		return driverID;
	}

	protected void setDriverID(String driverID) {
		this.driverID = driverID;
		// try to get a driver instance
		driver = DriverFactory.getDriverInstance(driverID);
		if (driver != null)
			driver.attach(this);
	}

	public int getFlags() {
		return flags;
	}
	
	public int getChannel() {
		return channel;
	}
	
	public boolean isAutobind() {
		return (flags & FLAG_AUTOBIND) == FLAG_AUTOBIND;
	}

	public boolean isBound() {
		// a streaming port is bound if a channel is set
		return channel != 0;
	}
	
	/** Only the protocol implementation may set the channel on bind/unbind
	 * 
	 * @param protocol
	 * @param channel
	 * @throws ProtocolException
	 */
	public void setChannel(IBasicProtocol protocol, int channel) {
		if (protocol != getProtocol())
			throw new IllegalAccessError("Setting the channel is only allowed from the device's protocol implementation");
		this.channel = channel;
	}

	protected IStreamingPortListener getStreamingPortListener() {
		return streamingPortListener;
	}

	public void setStreamingPortListener(
			IStreamingPortListener streamingPortListener) {
		this.streamingPortListener = streamingPortListener;
	}
	
	/** Binds this port to a channel that is selected by the protocol implementation.
	 * 
	 * @throws ProtocolException 
	 * @throws DeviceException
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws TimeoutException 
	 * @throws PortAccessDeniedException 
	 */
	public void bind() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		clearError();
		try {
			getProtocol().bindStreamingPort(this);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}

	/** Unbinds this port.
	 * 
	 * @param channel
	 * @throws ProtocolException 
	 * @throws DeviceException 
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws TimeoutException 
	 * @throws PortAccessDeniedException 
	 */
	public void unbind() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		clearError();
		try {
			getProtocol().unbindStreamingPort(this);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}
	
	public void dataReceived(IBasicProtocol protocol, String data) {
		if (protocol != getProtocol())
			throw new IllegalAccessError("dataReceived is only allowed from the device's protocol implementation");
		// delegate to listener if present
		if (streamingPortListener != null)
			streamingPortListener.dataReceived(this, data);
	}

	/** Sends data to the device if this port is bound.
	 * 
	 * @param data
	 * @throws DisconnectedException 
	 */
	public void sendData(String data) throws DisconnectedException {
		getProtocol().sendStreamingData(this, data);
	}

	@Override
	public String toString() {
		return "StreamingPort id=" + getID() + "; name='" + getName() + "'; driverId=" + getDriverID() + "; flags=" + flags;
	}

	@Override
	public void refresh() {
	}

	public IDriver getDriver() {
		return driver;
	}
	
	@Override
	public void getPortState() throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException {
		// streaming ports have no queryable state
	}

	public void portUnbound(IBasicProtocol basicProtocol) {
		// reset the channel
		setChannel(basicProtocol, 0);
		// notify the listener
		if (streamingPortListener != null)
			streamingPortListener.portUnbound(this);
	}
}
