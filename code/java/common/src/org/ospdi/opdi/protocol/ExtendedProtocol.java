package org.ospdi.opdi.protocol;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.devices.DeviceInfo;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.interfaces.IDeviceCapabilities;
import org.ospdi.opdi.ports.AnalogPort;
import org.ospdi.opdi.ports.BasicDeviceCapabilities;
import org.ospdi.opdi.ports.DialPort;
import org.ospdi.opdi.ports.DigitalPort;
import org.ospdi.opdi.ports.ExtendedDeviceCapabilities;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.PortGroup;
import org.ospdi.opdi.ports.SelectPort;
import org.ospdi.opdi.utils.Strings;

public class ExtendedProtocol extends BasicProtocol {

	private static final String MAGIC = "EP";

	public static final String GET_ALL_PORT_INFOS = "gAPI";

	public static final String GET_EXTENDED_PORT_INFO = "gEPI";
	public static final String EXTENDED_PORT_INFO = "EPI";
	public static final String GET_EXTENDED_PORT_STATE = "gEPS";
	public static final String EXTENDED_PORT_STATE = "EPS";

	public static final String GET_GROUP_INFO = "gGI";
	public static final String GROUP_INFO = "GI";
	public static final String GET_EXTENDED_GROUP_INFO = "gEGI";
	public static final String EXTENDED_GROUP_INFO = "EGI";

	public static final String GET_EXTENDED_DEVICE_INFO = "gEDI";
	public static final String EXTENDED_DEVICE_INFO = "EDI";

	public static final String GET_ALL_SELECT_PORT_LABELS = "gASL";

	protected Map<String, PortGroup> groupCache = new HashMap<String, PortGroup>();
	
	public ExtendedProtocol(IDevice device) {
		super(device);
	}
	
	@Override
	public String getMagic() {
		return MAGIC;
	}

	public static String getMAGIC() {
		return MAGIC;
	}

	protected void expectExtendedPortInfo(Port port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal a port error", e);
		}
		
		parseExtendedPortInfo(port, channel, message);
	}

	protected void parseExtendedPortInfo(Port port, int channel, Message message)
			throws ProtocolException, TimeoutException, InterruptedException,
			DisconnectedException, DeviceException {
		final int PREFIX = 0;
		final int PORT_ID = 1;
		final int INFO = 2;
		final int PARTS_COUNT = 3;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(EXTENDED_PORT_INFO))
			throw new ProtocolException("unexpected reply, expected: " + EXTENDED_PORT_INFO);
		if (!parts[PORT_ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");
		// info is optional
		if (parts.length > INFO)
			port.setExtendedPortInfo(parts[INFO]);
	}

	@Override
	public Port getPortInfo(String portID, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		Port result = super.getPortInfo(portID, channel);
		
		if (result != null) {
			// send extended port info request on the same channel
			send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_PORT_INFO, portID)));
			expectExtendedPortInfo(result, channel);
		}

		return result;
	}
	
	@Override
	public List<String> getSelectPortLabels(SelectPort port)
			throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException {

		List<String> result = new ArrayList<String>(port.getPosCount());

		int channel = this.getSynchronousChannel(false);

		send(new Message(channel, Strings.join(SEPARATOR, GET_ALL_SELECT_PORT_LABELS, port.getID())));
		
		try {
			int counter = 0;
			while (counter < port.getPosCount()) {
				Message message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
				result.add(parseSelectPortLabel(port, counter, message));
				
				counter++;
			}
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getAllSelectPortLabels should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getAllSelectPortLabels should never signal a port error", e);
		}
		
		return result;
	}

	protected void expectExtendedPortState(Port port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
		} catch (PortAccessDeniedException e) {
			return;
		} catch (PortErrorException e) {
			return;
		}
		
		parseExtendedPortState(port, message);
	}

	protected void parseExtendedPortState(Port port, Message message)
			throws ProtocolException {
		final int PREFIX = 0;
		final int PORT_ID = 1;
		final int INFO = 2;
		final int PARTS_COUNT = 3;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(EXTENDED_PORT_STATE))
			throw new ProtocolException("unexpected reply, expected: " + EXTENDED_PORT_STATE);
		if (!parts[PORT_ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");
		// info is optional
		if (parts.length > INFO)
			port.setExtendedPortState(parts[INFO]);
	}
	
	@Override
	public int getPortState(AnalogPort analogPort) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException {
		int channel = super.getPortState(analogPort);

		send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_PORT_STATE, analogPort.getID())));
		expectExtendedPortState(analogPort, channel);
		
		return channel;
	}

	@Override
	public int getPortState(DigitalPort digitalPort) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException {
		int channel = super.getPortState(digitalPort);

		send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_PORT_STATE, digitalPort.getID())));
		expectExtendedPortState(digitalPort, channel);
		
		return channel;
	}
	
	@Override
	public int getPosition(DialPort port) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException {
		int channel = super.getPosition(port);

		send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_PORT_STATE, port.getID())));
		expectExtendedPortState(port, channel);
		
		return channel;
	}
	
	@Override
	public int getPosition(SelectPort port) throws TimeoutException,
			InterruptedException, DisconnectedException, DeviceException,
			ProtocolException, PortAccessDeniedException, PortErrorException {
		int channel = super.getPosition(port);

		send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_PORT_STATE, port.getID())));
		expectExtendedPortState(port, channel);
		
		return channel;
	}
	
	protected void expectExtendedGroupInfo(Port port, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
/**
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getExtendedPortInfo should never signal a port error", e);
		}
		
		final int PREFIX = 0;
		final int PORT_ID = 1;
		final int INFO = 2;
		final int PARTS_COUNT = 3;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(EXTENDED_PORT_INFO))
			throw new ProtocolException("unexpected reply, expected: " + EXTENDED_PORT_INFO);
		if (!parts[PORT_ID].equals(port.getID()))
			throw new ProtocolException("wrong port ID");
		// info is optional
		if (parts.length > INFO)
			port.setExtendedPortInfo(parts[INFO]);
*/			
	}
	
	public PortGroup getGroupInfo(String groupID, int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		
		// check whether the group has already been cached
		if (groupCache.containsKey(groupID))
			return groupCache.get(groupID);
		
		send(new Message(channel, Strings.join(SEPARATOR, GET_GROUP_INFO, groupID)));
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getGroupInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getGroupInfo should never signal a port error", e);
		}
		
		final int PREFIX = 0;
		final int GROUP_ID = 1;
		final int LABEL = 2;
		final int PARENT = 3;
		final int FLAGS = 4;
		final int PARTS_COUNT = 5;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(GROUP_INFO))
			throw new ProtocolException("unexpected reply, expected: " + GROUP_INFO);
		if (!parts[GROUP_ID].equals(groupID))
			throw new ProtocolException("wrong group ID");
		
		PortGroup result = new PortGroup(groupID, parts[LABEL], parts[PARENT], Strings.parseInt(parts[FLAGS], "flags", 0, Integer.MAX_VALUE));
		groupCache.put(groupID, result);
		
		if (!result.getParent().isEmpty()) {
			
			// load parent group information
			PortGroup parent = getGroupInfo(result.getParent(), channel);
			
			// assign parent (cycle check)
			result.setParentGroup(parent);
		}
		
		return result;
	}

	protected DeviceInfo expectExtendedDeviceInfo(int channel) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		Message message;
		try {
			message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getExtendedDeviceInfo should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getExtendedDeviceInfo should never signal a port error", e);
		} catch (Exception e) {
			e.printStackTrace();
			return new DeviceInfo("");
		}
		
		final int PREFIX = 0;
		final int INFO = 1;
		final int PARTS_COUNT = 2;
		
		String[] parts = Strings.split(message.getPayload(), SEPARATOR);
		if (parts.length > PARTS_COUNT)
			throw new ProtocolException("invalid number of message parts");
		if (!parts[PREFIX].equals(EXTENDED_DEVICE_INFO))
			throw new ProtocolException("unexpected reply, expected: " + EXTENDED_DEVICE_INFO);
		// info is required
		if (parts.length <= INFO)
			throw new ProtocolException("invalid number of message parts");
		return new DeviceInfo(parts[INFO]);
			
	}

	public DeviceInfo getExtendedDeviceInfo() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		
		int channel = this.getSynchronousChannel(false);
		
		send(new Message(channel, Strings.join(SEPARATOR, GET_EXTENDED_DEVICE_INFO)));
		return expectExtendedDeviceInfo(channel);
	}
	
	@Override
	public IDeviceCapabilities getDeviceCapabilities()
			throws TimeoutException, ProtocolException, DeviceException,
			InterruptedException, DisconnectedException {
		// has cached device capabilities?
		if (deviceCaps != null)
			return deviceCaps;
		
		IDeviceCapabilities result = super.getDeviceCapabilities();
		
		// additional query: extended device info
		this.getDevice().setDeviceInfo(getExtendedDeviceInfo());
		
		return result;
	}
	
	@Override
	protected BasicDeviceCapabilities parseDeviceCapabilities(int channel,
			Message capResult) throws ProtocolException, TimeoutException,
			InterruptedException, DisconnectedException, DeviceException {
		// TODO Auto-generated method stub
		return new ExtendedDeviceCapabilities(this, channel, capResult.getPayload());
	}

	public Collection<? extends Port> getAllPortInfos(int channel, String[] portIDs) throws TimeoutException, InterruptedException,
		DisconnectedException, DeviceException, ProtocolException {
		
		ArrayList<Port> result = new ArrayList<Port>(portIDs.length);

		// request all port information (basic infos plus extended infos)
		send(new Message(channel, Strings.join(SEPARATOR, GET_ALL_PORT_INFOS)));
		
		try {
			int counter = 0;
			while (counter < portIDs.length) {
				// the first message is the port info
				Message message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
				Port port = parsePortInfo(message);
				// the second message is the extended port info
				message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
				parseExtendedPortInfo(port, channel, message);
				
				counter++;
				
				result.add(port);
			}
		} catch (PortAccessDeniedException e) {
			throw new IllegalStateException("Programming error on device: getAllPortInfos should never signal port access denied", e);
		} catch (PortErrorException e) {
			throw new IllegalStateException("Programming error on device: getAllPortInfos should never signal a port error", e);
		}
		
		// go through ports, load group info
		for (Port port: result) {
			// check whether there's a group specified
			String group = port.getExtendedInfo("group", ""); 
			if (!group.isEmpty()) {
				// assign group
				port.setGroup(this.getGroupInfo(group, channel));
			}
		}
		
		return result;
	}

	public void getAllPortStates(List<Port> ports) throws DisconnectedException, TimeoutException, InterruptedException, DeviceException, ProtocolException {
		int channel = this.getSynchronousChannel(false);
		
		// request all port state (basic state plus extended state)
		send(new Message(channel, Strings.join(SEPARATOR, GET_ALL_PORT_STATES)));
		int counter = 0;
		while (counter < ports.size()) {
			Port port = ports.get(counter);

			// the first message is the port state
			Message message;
			try {
				message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
				if (port instanceof DigitalPort)
					parseDigitalPortState((DigitalPort)port, message);
				else
				if (port instanceof AnalogPort)
					parseAnalogPortState((AnalogPort)port, message);
				else
				if (port instanceof DialPort)
					parseDialPortPosition((DialPort)port, message);
				else
				if (port instanceof SelectPort)
					parseSelectPortPosition((SelectPort)port, message);
			
			} catch (PortAccessDeniedException pade) {
				// check port ID
				if (!pade.getPortID().equals(port.getID()))
					throw new ProtocolException("Received Access Denied for port " + pade.getPortID() + " while expecting state for port " + port.getID());
				// do not set the state of the port (after all, it's Access Denied)
			} catch (PortErrorException pee) {
				// the port itself handles port errors
				port.handlePortError(pee);
			}
			counter++;

			try {
				// the second message is the extended port state
				message = expect(channel, DEFAULT_TIMEOUT, ExpectationMode.IGNORE_REFRESHES);
				parseExtendedPortState(port, message);
			} catch (Exception e) {}
		}
	}

}
