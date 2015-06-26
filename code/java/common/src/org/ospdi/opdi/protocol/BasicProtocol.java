package org.ospdi.opdi.protocol;

import java.util.ArrayDeque;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Queue;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.ports.AnalogPort;
import org.ospdi.opdi.ports.AnalogPort.Resolution;
import org.ospdi.opdi.ports.BasicDeviceCapabilities;
import org.ospdi.opdi.ports.DialPort;
import org.ospdi.opdi.ports.DigitalPort;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.PortFactory;
import org.ospdi.opdi.ports.SelectPort;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.utils.Strings;


/** This class implements the basic protocol for data exchange with a slave device.
 * The protocol is setup when a device is connected. It ends when the device becomes
 * disconnected, for whatever reason.
 * 
 * @author Leo
 *
 */
public class BasicProtocol extends AbstractProtocol implements IBasicProtocol {
	
	private static final String MAGIC = "BP";

	public static final String REFRESH = "Ref";
	public static final String RECONFIGURE = "Reconf";
	public static final String DEBUG = "Debug";

	protected static final int DEFAULT_TIMEOUT = 10000;
	
	protected static int CHANNEL_CONTROL = 0;
	protected static int CHANNEL_REFRESH = 1;
	protected static int CHANNEL_LOWEST_STREAMING = 2;
	protected static int CHANNEL_HIGHEST_STREAMING = 19;
	protected static int CHANNEL_LOWEST_SYNCHRONOUS = CHANNEL_HIGHEST_STREAMING + 1;
	protected static int CHANNEL_ROLLOVER = 100;
	
	protected BasicDeviceCapabilities deviceCaps;
	
	protected Integer currentChannel = CHANNEL_LOWEST_SYNCHRONOUS - 1;
	protected Hashtable<Integer, StreamingPort> boundStreamingPorts = new Hashtable<Integer, StreamingPort>();
	
	enum ExpectationMode {
		NORMAL,
		IGNORE_REFRESHES
	};
	
	protected volatile boolean expectingMessage;
	protected Queue<Message> messagesToDispatch = new ArrayDeque<Message>();
	protected volatile ExpectationMode expectationMode;
	
	
	/** Returns a new unique channel for a new protocol.
	 * 
	 * @return
	 */
	public int getSynchronousChannel(boolean isRefresh) {
		// send refresh messages on a special channel to avoid triggering an idle timeout activity reset on the device
		if (isRefresh)
			return CHANNEL_REFRESH;
		// calculate new unique channel number for synchronous protocol run
		synchronized (currentChannel) {
			int channel = currentChannel + 1;
			// prevent channel numbers from becoming too large
			if (channel >= CHANNEL_ROLLOVER)
				channel = CHANNEL_LOWEST_SYNCHRONOUS;
			currentChannel = channel;
			return channel;
		}
	}
	
	public static final int PING_INTERVAL_MS = 5000;
	
	public static final String AGREEMENT = "OK";
	public static final String DISAGREEMENT = "NOK";
	public static final String PING_MESSAGE = "ping";
	public static final String GET_DEVICE_CAPS = "gDC";
	public static final String GET_PORT_INFO = "gPI";
	public static final String GET_ALL_PORT_STATES = "gAPS";
	
	public static final String DIGITAL_PORT_STATE  = "DS";
	public static final String GET_DIGITAL_PORT_STATE  = "gDS";
	public static final String SET_DIGITAL_PORT_MODE = "sDM";
	public static final String SET_DIGITAL_PORT_LINE  = "sDL";
	
	public static final String ANALOG_PORT_STATE  = "AS";
	public static final String GET_ANALOG_PORT_STATE  = "gAS";
	public static final String SET_ANALOG_PORT_MODE = "sAM";
	public static final String SET_ANALOG_PORT_VALUE  = "sAV";
	public static final String SET_ANALOG_PORT_RESOLUTION  = "sAR";
	public static final String SET_ANALOG_PORT_REFERENCE = "sARF";
	
	public static final String GET_SELECT_PORT_LABEL  = "gSL";
	public static final String SELECT_PORT_LABEL  = "SL";
	public static final String GET_SELECT_PORT_STATE = "gSS";
	public static final String SELECT_PORT_STATE  = "SS";
	public static final String SET_SELECT_PORT_POSITION  = "sSP";

	public static final String GET_DIAL_PORT_STATE = "gDLS";
	public static final String DIAL_PORT_STATE  = "DLS";
	public static final String SET_DIAL_PORT_POSITION  = "sDLP";

	public static final String BIND_STREAMING_PORT  = "bSP";
	public static final String UNBIND_STREAMING_PORT  = "uSP";

	public BasicProtocol(IDevice device) {
		super(device);
	}
	
	@Override
	public String getMagic() {
		return MAGIC;
	}

	public static String getMAGIC() {
		return MAGIC;
	}

	@Override
	public void initiate() {

		synchronized (boundStreamingPorts) {
			// clear streaming port bindings
			for (StreamingPort sp: boundStreamingPorts.values()) {
				sp.setChannel(this, 0);
			}
			// remove bindings
			boundStreamingPorts.clear();
		}
		
		// synchronous channel number reset
		currentChannel = CHANNEL_LOWEST_SYNCHRONOUS - 1;

		// Start the ping thread
		new Thread(new Runnable() {
			@Override
			public void run() {
				// send ping message on the control channel (channel == 0)
				Message ping = new Message(CHANNEL_CONTROL, Strings.join(SEPARATOR, PING_MESSAGE));
				while (getDevice().isConnected()) {
					try {
						// wait for the specified time after the last message
						while (System.currentTimeMillis() - getDevice().getLastSendTimeMS() < PING_INTERVAL_MS)
							Thread.sleep(10);
						send(ping);
						// sleep at least as long as specified by the interval
						Thread.sleep(PING_INTERVAL_MS);
					} catch (InterruptedException e) {
						Thread.currentThread().interrupt();
						return;
					} catch (DisconnectedException e) {
						e.printStackTrace();
						// the device has disconnected
						return;
					}
				}
			}
		}).start();
	}

	protected Message expect(long channel, int timeout, IAbortable abortable, ExpectationMode mode) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, PortAccessDeniedException, PortErrorException {
		try {
			// set flag: expecting a message
			expectingMessage = true;
			this.expectationMode = mode;
			
			return super.expect(channel, timeout, abortable);
			
		} finally {
			expectingMessage = false;
			
			// ignore duplicate messages
			HashSet<String> messages = new HashSet<String>();
			// dispatch messages collected during expect
			while (!messagesToDispatch.isEmpty()) {
				Message m = messagesToDispatch.poll();
				if (messages.contains(m.toString()))
					continue;
				messages.add(m.toString());
				try {
					dispatch(m);
				} catch (Throwable t) {
					// ignore all errors here
				}
			}
		}
	}

	protected Message expect(long channel, int timeout, ExpectationMode mode)  throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, PortAccessDeniedException, PortErrorException {
		return this.expect(channel, timeout, null, mode);
	}

	@Override
	protected Message expect(long channel, int timeout)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, PortAccessDeniedException,
			PortErrorException {
		return this.expect(channel, timeout, ExpectationMode.NORMAL);
	}
	
	@Override
	public boolean dispatch(Message message) {
		
		// analyze control channel messages
		if (message.getChannel() == 0) {
			// received a disconnect?
			if (message.getPayload().equals(DISCONNECT)) {
				// disconnect the device (this sends a disconnect message back)
				device.disconnect(false);
			}
			else {
				// messages other than DISCONNECT are processed later in case
				// we're expecting a message

				if (expectingMessage) {
					messagesToDispatch.add(message);
					return true;
				}
				
				// ignore RECONFIGURE if specified
				if (message.getPayload().equals(RECONFIGURE) && expectationMode != ExpectationMode.IGNORE_REFRESHES) {
					// clear cached device capabilities
					deviceCaps = null;
					// reset bound streaming ports (reconfigure on the device unbinds streaming ports)
					synchronized (boundStreamingPorts) {
						for (StreamingPort sPort: boundStreamingPorts.values()) {
							sPort.portUnbound(this);
						}
						boundStreamingPorts.clear();
					}
					device.receivedReconfigure();
				}
				else {
					// split message parts
					String[] parts = Strings.split(message.getPayload(), SEPARATOR);
					
					// received a debug message?
					if (parts[0].equals(DEBUG)) {
						device.receivedDebug(Strings.join(1, SEPARATOR, (Object[])parts));
					} else
						// ignore REFRESH if specified
					if (parts[0].equals(REFRESH) && expectationMode != ExpectationMode.IGNORE_REFRESHES) {
						// remaining components are port IDs
						String[] portIDs = new String[parts.length - 1];
						System.arraycopy(parts, 1, portIDs, 0, parts.length - 1);
						device.receivedRefresh(portIDs);
					} else
					if (parts[0].equals(ERROR)) {
						// remaining optional components contain the error information
						int error = 0;
						String text = null;
						if (parts.length > 1) {
							error = Integer.parseInt(parts[1]);
						}
						if (parts.length > 2) {
							text = Strings.join(2, SEPARATOR, (Object[])parts);
						}
						device.setError(error, text);
					}
				}
			}
		}
		
		synchronized (boundStreamingPorts) {
			// check whether the channel is bound to a streaming port
			StreamingPort sp = boundStreamingPorts.get(message.getChannel());
			if (sp != null) {
				// channel is bound; receive data
				sp.dataReceived(this, message.getPayload());
				return true;
			}
		}
				
		return false;
	}

	@Override
	public BasicDeviceCapabilities getDeviceCapabilities() throws TimeoutException, ProtocolException, DeviceException, InterruptedException, DisconnectedException {
		// has cached device capabilities?
		if (deviceCaps != null)
			return deviceCaps;

		int channel = getSynchronousChannel(false);
		
		// request device capabilities from the slave
		send(new Message(channel, GET_DEVICE_CAPS));
		Message capResult;
		try {
			// ignore refreshes because they would trigger immediate reloading of device capabilities
			capResult = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getDeviceCapabilities should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getDeviceCapabilities should never signal a port error", e);
		}
		
		// decode the serial form
		// this may issue callbacks on the protocol which do not have to be threaded
		deviceCaps = new BasicDeviceCapabilities(this, channel, capResult.getPayload());

		return deviceCaps;		
	}	
	
	@Override
	public Port getPortInfo(String portID, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		send(new Message(channel, Strings.join(SEPARATOR, GET_PORT_INFO, portID)));
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getPortInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getPortInfo should never signal a port error", e);
		}
		
		// check the port magic
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);

		// let the factory create the port
		return PortFactory.createPort(this, parts);
	}

	protected void expectDigitalPortState(DigitalPort port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		Message m = expect(channel, DEFAULT_TIMEOUT);
		
		final int PREFIX = 0;
		final int ID = 1;
		final int MODE = 2;
		final int LINE = 3;
		final int PARTS_COUNT = 4;
		
		String[] parts = Strings.split(m.getPayload(), SEPARATOR);
		if (parts.length != PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(DIGITAL_PORT_STATE))
			throw new ProtocolException("unexpected reply, expected: " + DIGITAL_PORT_STATE);
		if (!parts[ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");

		// set port state
		port.setPortState(this, 
				DigitalPort.PortMode.values()[Strings.parseInt(parts[MODE], "mode", 0, DigitalPort.PortMode.values().length - 2)]);	// ignore UNKNOWN
		
		port.setPortLine(this, 
				DigitalPort.PortLine.values()[Strings.parseInt(parts[LINE], "line", 0, DigitalPort.PortLine.values().length - 2)]);	// ignore UNKNOWN
	}

	@Override
	public void setPortMode(DigitalPort port, DigitalPort.PortMode portMode)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {		
		expectDigitalPortState(port, send(new Message(getSynchronousChannel(false), Strings.join(SEPARATOR, SET_DIGITAL_PORT_MODE, port.getID(), portMode.ordinal()))));
	}
	
	
	@Override
	public void setPortLine(DigitalPort digitalPort, DigitalPort.PortLine portLine) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {		
		expectDigitalPortState(digitalPort, send(new Message(getSynchronousChannel(false), Strings.join(SEPARATOR, SET_DIGITAL_PORT_LINE, digitalPort.getID(), portLine.ordinal()))));
	}

	@Override
	public int getPortState(DigitalPort digitalPort) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		int channel = getSynchronousChannel(digitalPort.isRefreshing());

		expectDigitalPortState(digitalPort, send(new Message(channel, Strings.join(SEPARATOR, GET_DIGITAL_PORT_STATE, digitalPort.getID()))));

		return channel;
	}

	/// Analog port
	
	protected void expectAnalogPortState(AnalogPort port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		Message m = expect(channel, DEFAULT_TIMEOUT);
		
		final int PREFIX = 0;
		final int ID = 1;
		final int MODE = 2;
		final int REFERENCE = 3;
		final int RESOLUTION = 4;
		final int VALUE = 5;
		final int PARTS_COUNT = 6;
		
		String[] parts = Strings.split(m.getPayload(), SEPARATOR);
		if (parts.length != PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(ANALOG_PORT_STATE))
			throw new ProtocolException("unexpected reply, expected: " + ANALOG_PORT_STATE);
		if (!parts[ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");
		
		// set port state; configuration first
		port.setPortState(this, 
				AnalogPort.PortMode.values()[Strings.parseInt(parts[MODE], "mode", 0, AnalogPort.PortMode.values().length - 1)],
				AnalogPort.Reference.values()[Strings.parseInt(parts[REFERENCE], "reference", 0, AnalogPort.Reference.values().length - 1)],
				AnalogPort.Resolution.values()[Strings.parseInt(parts[RESOLUTION], "resolution", 0, AnalogPort.Resolution.values().length - 1)]);
		
		// important to set configuration before calling getMaxValue()
		port.setPortValue(this, Strings.parseInt(parts[VALUE], "analog value", 0, port.getMaxValue()));
	}

	@Override
	public void setPortValue(AnalogPort analogPort, int value) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		expectAnalogPortState(analogPort, send(new Message(getSynchronousChannel(false), Strings.join(SEPARATOR, SET_ANALOG_PORT_VALUE, analogPort.getID(), value))));
	}

	@Override
	public int getPortState(AnalogPort analogPort) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException {
		int channel = getSynchronousChannel(analogPort.isRefreshing());
		send(new Message(channel, Strings.join(SEPARATOR, GET_ANALOG_PORT_STATE, analogPort.getID())));
		expectAnalogPortState(analogPort, channel);
		return channel;
	}

	@Override
	public void setPortMode(AnalogPort analogPort, AnalogPort.PortMode mode)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		expectAnalogPortState(analogPort, send(new Message(getSynchronousChannel(false), Strings.join(SEPARATOR, SET_ANALOG_PORT_MODE, analogPort.getID(), mode.ordinal()))));
	}

	@Override
	public void setPortResolution(AnalogPort analogPort, Resolution resolution)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		expectAnalogPortState(analogPort, send(new Message(getSynchronousChannel(false), Strings.join(SEPARATOR, SET_ANALOG_PORT_RESOLUTION, analogPort.getID(), resolution.ordinal()))));
	}

	@Override
	public void setPortReference(AnalogPort analogPort, AnalogPort.Reference reference)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		expectAnalogPortState(analogPort, send(new Message(getSynchronousChannel(false), Strings.join(SEPARATOR, SET_ANALOG_PORT_REFERENCE, analogPort.getID(), reference.ordinal()))));
	}
	
	@Override
	public String getLabel(SelectPort port, int pos)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		
		// send request
		int channel = getSynchronousChannel(port.isRefreshing());
		send(new Message(channel, Strings.join(SEPARATOR, GET_SELECT_PORT_LABEL, port.getID(), pos)));
		
		Message m = expect(channel, DEFAULT_TIMEOUT);
		
		final int PREFIX = 0;
		final int ID = 1;
		final int POS = 2;
		final int LABEL = 3;
		final int PARTS_COUNT = 4;
		
		String[] parts = Strings.split(m.getPayload(), SEPARATOR);
		if (parts.length != PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(SELECT_PORT_LABEL))
			throw new ProtocolException("unexpected reply, expected: " + SELECT_PORT_LABEL);
		if (!parts[ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");
		if (!parts[POS].equals("" + pos))
			throw new ProtocolException("wrong position");

		return parts[LABEL];
	}

	protected void expectSelectPortPosition(SelectPort port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		Message m = expect(channel, DEFAULT_TIMEOUT);
		
		final int PREFIX = 0;
		final int ID = 1;
		final int POS = 2;
		final int PARTS_COUNT = 3;
		
		String[] parts = Strings.split(m.getPayload(), SEPARATOR);
		if (parts.length != PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(SELECT_PORT_STATE))
			throw new ProtocolException("unexpected reply, expected: " + SELECT_PORT_STATE);
		if (!parts[ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");

		port.setPortPosition(this, Strings.parseInt(parts[POS], "position", 0, port.getPosCount() - 1));
	}

	@Override
	public int getPosition(SelectPort port) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException {
		// send request
		int channel = getSynchronousChannel(port.isRefreshing());
		send(new Message(channel, Strings.join(SEPARATOR, GET_SELECT_PORT_STATE, port.getID())));
		
		expectSelectPortPosition(port, channel);
		
		return channel;
	}
	
	@Override
	public void setPosition(SelectPort port, int pos)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		// send request
		int channel = getSynchronousChannel(false);
		send(new Message(channel, Strings.join(SEPARATOR, SET_SELECT_PORT_POSITION, port.getID(), pos)));
		
		expectSelectPortPosition(port, channel);
	}

	protected void expectDialPortPosition(DialPort port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		Message m = expect(channel, DEFAULT_TIMEOUT);
		
		final int PREFIX = 0;
		final int ID = 1;
		final int POS = 2;
		final int PARTS_COUNT = 3;
		
		String[] parts = Strings.split(m.getPayload(), SEPARATOR);
		if (parts.length != PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(DIAL_PORT_STATE))
			throw new ProtocolException("unexpected reply, expected: " + DIAL_PORT_STATE);
		if (!parts[ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");

		port.setPortPosition(this, Strings.parseLong(parts[POS], "position", Long.MIN_VALUE, Long.MAX_VALUE));
	}

	@Override
	public int getPosition(DialPort port) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException {
		// send request
		int channel = getSynchronousChannel(port.isRefreshing());
		send(new Message(channel, Strings.join(SEPARATOR, GET_DIAL_PORT_STATE, port.getID())));
		
		expectDialPortPosition(port, channel);
		
		return channel;
	}
	
	@Override
	public void setPosition(DialPort port, long pos)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		// send request
		int channel = getSynchronousChannel(false);
		send(new Message(channel, Strings.join(SEPARATOR, SET_DIAL_PORT_POSITION, port.getID(), pos)));
		
		expectDialPortPosition(port, channel);
	}

	@Override
	public boolean bindStreamingPort(StreamingPort streamingPort)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {

		// determine the channel to bind to
		int bChannel = 0;
		synchronized (boundStreamingPorts) {
			// take the first free streaming channel number
			for (int i = CHANNEL_LOWEST_STREAMING; i <= CHANNEL_HIGHEST_STREAMING; i++) {
				if (!boundStreamingPorts.containsKey(i)) {
					bChannel = i;
					break;
				}
			}
		}
		// no free channel number found?
		if (bChannel == 0)
			return false;

		int channel = getSynchronousChannel(false);
		// send binding message
		send(new Message(channel, Strings.join(SEPARATOR, BIND_STREAMING_PORT, streamingPort.getID(), bChannel)));			
		
		// expect OK or NOK
		Message m = expect(channel, DEFAULT_TIMEOUT);
		
		String[] parts = Strings.split(m.getPayload(), SEPARATOR);
		if (parts[0].equals(DISAGREEMENT))
			return false;
		if (!parts[0].equals(AGREEMENT))
			throw new ProtocolException("OK expected");
			
		// binding successful
		synchronized (boundStreamingPorts) {
			// remember binding
			boundStreamingPorts.put(bChannel, streamingPort);
			
			// notify streaming port
			streamingPort.setChannel(this, bChannel);
		}
		
		return true;
	}

	@Override
	public void unbindStreamingPort(StreamingPort streamingPort)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException, PortErrorException {
		
		synchronized (boundStreamingPorts) {
			// verify that the port is bound
			if (boundStreamingPorts.get(streamingPort.getChannel()) != streamingPort)
				// port is not bound
				return;
		}
		
		int channel = getSynchronousChannel(false);
		// send unbinding message
		send(new Message(channel, Strings.join(SEPARATOR, UNBIND_STREAMING_PORT, streamingPort.getID())));			
		
		// expect any reply
		expect(channel, DEFAULT_TIMEOUT);
			
		synchronized (boundStreamingPorts) {
			// unregister binding
			boundStreamingPorts.remove(streamingPort.getChannel());
			
			// notify streaming port
			streamingPort.setChannel(this, 0);
		}
	}

	@Override
	public void sendStreamingData(StreamingPort streamingPort, String data)
			throws DisconnectedException {

		synchronized (boundStreamingPorts) {
			// verify that the port is bound
			if (boundStreamingPorts.get(streamingPort.getChannel()) != streamingPort)
				// port is not bound
				return;
			
			// send data
			send(new Message(streamingPort.getChannel(), data));
		}
	}

	@Override
	public Port findPortByID(String portID) throws TimeoutException, ProtocolException, DeviceException, InterruptedException, DisconnectedException, PortAccessDeniedException, PortErrorException {
		BasicDeviceCapabilities bdc = getDeviceCapabilities();
		if (bdc == null) return null;
		return bdc.findPortByID(portID);
	}
}
