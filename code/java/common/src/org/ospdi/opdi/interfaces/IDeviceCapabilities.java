package org.ospdi.opdi.interfaces;

import java.util.List;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.PortGroup;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.ProtocolException;

public interface IDeviceCapabilities {

	public abstract List<PortGroup> getPortGroups(String parentGroupID);

	public abstract List<Port> getPorts(String groupID);

	public abstract List<Port> getPorts();

	public abstract Port findPortByID(String portID);

	public abstract void getPortStates() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException;

}
