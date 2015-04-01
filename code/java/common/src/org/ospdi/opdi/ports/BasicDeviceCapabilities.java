package org.ospdi.opdi.ports;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.utils.Strings;


/** This class describes the capabilities of an OPDI device. It contains convenience methods to access ports.
 * 
 * @author Leo
 *
 */
public class BasicDeviceCapabilities {
	
	private static final String MAGIC = "BDC";

	private List<Port> ports = new ArrayList<Port>();
	
	public static BasicDeviceCapabilities makeInstance(String name, String address) {
		return new BasicDeviceCapabilities();
	}
	
	private BasicDeviceCapabilities() {}

	public BasicDeviceCapabilities(IBasicProtocol protocol, int channel, String serialForm) throws ProtocolException, TimeoutException, InterruptedException, DisconnectedException, DeviceException {
		
		final int PORTS_PART = 1;
		final int PART_COUNT = 2;
		
		// decode the serial representation
		String[] parts = Strings.split(serialForm, ':');
		if (parts.length != PART_COUNT) 
			throw new ProtocolException("BasicDeviceCapabilities message invalid");
		if (!parts[0].equals(MAGIC))
			throw new ProtocolException("BasicDeviceCapabilities message invalid: incorrect magic: " + parts[0]);
		// split port IDs by comma
		String[] portIDs = Strings.split(parts[PORTS_PART], ',');
		// get port info for each port
		for (String portID: portIDs) {
			if (!portID.isEmpty())
				ports.add(protocol.getPortInfo(portID, channel));
		}
	}
	
	public String toString() {
		String result = "BasicDeviceCapabilities\n";
		if (ports.size() == 0)
			result += "  No ports\n";
		else
			for (Port port: ports) {
				result += "  " + port.toString() + "\n";
			}
		return result;
	}

	public Port findPortByID(String portID) {
		for (Port port: ports) {
			if (port.getID().equals(portID)) return port;
		}
		// not found
		return null;
	}

	public List<Port> getPorts() {
		return ports;
	}

	public List<Port> getPorts(String groupID) {
		// filter ports by parent group ID
		List<Port> result = new ArrayList<Port>();
		for (Port port: ports) {
			if (port.isInGroup(groupID))
				result.add(port);
		}
		return result;
	}

	public List<PortGroup> getPortGroups(String parentGroupID) {
		// TODO parentGroupID
		
		Set<PortGroup> result = new HashSet<PortGroup>();
		
		for (Port port: ports) { 
			if (port.getGroup() != null) {
				result.add(port.getGroup());
				// add all parent groups way up the hierarchy
				PortGroup parent = port.getGroup().getParentGroup();
				while (parent != null) {
					result.add(parent);
					parent = parent.getParentGroup();
				}
			}
		}
		
		return new ArrayList<PortGroup>(result);
	}
}
