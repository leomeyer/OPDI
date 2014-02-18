package org.ospdi.opdi.ports;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.utils.Strings;


/** Represents a select port on a device.
 * 
 * @author Leo
 *
 */
public class SelectPort extends Port {
	
	static final String MAGIC = "SLP";
	
	int posCount;
	int position = -1;
	List<String> labels = new ArrayList<String>();
	
	protected SelectPort(String id, String name, PortDirCaps portDirection, int flags) {
		super(null, id, name, PortType.SELECT, portDirection);
	}
	
	/** Constructor for deserializing from wire form
	 * 
	 * @param protocol
	 * @param parts
	 * @throws ProtocolException
	 * @throws DeviceException 
	 * @throws DisconnectedException 
	 * @throws InterruptedException 
	 * @throws TimeoutException 
	 */
	public SelectPort(IBasicProtocol protocol, String[] parts) throws ProtocolException, TimeoutException, InterruptedException, DisconnectedException, DeviceException {
		super(protocol, null, null, PortType.SELECT, PortDirCaps.OUTPUT);
		
		final int ID_PART = 1;
		final int NAME_PART = 2;
		final int POS_PART = 3;
//		final int FLAGS_PART = 4;
		final int PART_COUNT = 5;
		
		checkSerialForm(parts, PART_COUNT, MAGIC);

		setID(parts[ID_PART]);
		setName(parts[NAME_PART]);
		posCount = Strings.parseInt(parts[POS_PART], "position count", 0, Integer.MAX_VALUE);
		
		// query port labels
		for (int i = 0; i < posCount; i++)
			labels.add(protocol.getLabel(this, i));
	}
	
	public String serialize() {
		return Strings.join(':', MAGIC, getID(), getName(), posCount);
	}

	public int getPosCount() {
		return posCount;
	}

	public String getLabelAt(int pos) {
		if ((pos < 0) || (pos >= labels.size()))
			throw new IllegalArgumentException("The given position is not valid or the label has not yet been added");
		return labels.get(pos);
	}
	
	public void setPortPosition(IBasicProtocol protocol, int position) {
		if (protocol != getProtocol())
			throw new IllegalAccessError("Setting the port state is only allowed from its protocol implementation");
		this.position = position;
	}
	
	// Retrieve the position from the device
	public int getPosition() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		if (position < 0)
			getPortState();
		return position;
	}
	
	public void setPosition(int pos) throws ProtocolException, TimeoutException, InterruptedException, DisconnectedException, DeviceException {
		getProtocol().setPosition(this, pos);
	}
	
	@Override
	public String toString() {
		return "SelectPort id=" + getID() + "; name='" + getName() + "'; type=" + getType() + "; dir_caps=" + getDirCaps() + "; posCount=" + posCount;
	}

	@Override
	public void refresh() {
		position = -1;
	}
	
	@Override
	public void getPortState() throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException {
		// Request port state
		getProtocol().getPosition(this);
	}
}
