package org.ospdi.opdi.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.protocol.BasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.PortErrorException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.utils.Strings;

/** This class represents an analog dial for things like volume controls.
 * It may be represented by knobs or sliders.
 * A dial has a minimum, a maximum and a step width (increment).
 * It is always an output only port.
 * 
 * @author Leo
 *
 */
public class DialPort extends Port {

	static final String MAGIC = "DL";
	
	protected int position = -1;
	protected int minimum;
	protected int maximum;
	protected int step;
	
	protected DialPort(String id, String name, int minimum, int maximum, int step) {
		super(null, id, name, PortType.OTHER, PortDirCaps.OUTPUT);
		this.minimum = minimum;
		this.maximum = maximum;
		this.step = step;
	}
	
	/** Constructor for deserializing from wire form
	 * 
	 * @param protocol
	 * @param parts
	 * @throws ProtocolException
	 */
	public DialPort(IBasicProtocol protocol, String[] parts) throws ProtocolException {
		super(protocol, null, null, PortType.OTHER, null);
		
		final int ID_PART = 1;
		final int NAME_PART = 2;
		final int MIN_PART = 3;
		final int MAX_PART = 4;
		final int STEP_PART = 5;
		final int FLAGS_PART = 6;
		final int PART_COUNT = 7;
		
		checkSerialForm(parts, PART_COUNT, MAGIC);

		setID(parts[ID_PART]);
		setName(parts[NAME_PART]);
		setMinimum(Strings.parseInt(parts[MIN_PART], "Minimum", Integer.MIN_VALUE, Integer.MAX_VALUE));
		setMaximum(Strings.parseInt(parts[MAX_PART], "Maximum", Integer.MIN_VALUE, Integer.MAX_VALUE));
		setStep(Strings.parseInt(parts[STEP_PART], "Step", Integer.MIN_VALUE, Integer.MAX_VALUE));
		flags = Strings.parseInt(parts[FLAGS_PART], "flags", 0, Integer.MAX_VALUE);
	}
	
	public String serialize() {
		return Strings.join(':', MAGIC, getID(), getName(), minimum, maximum, step);
	}
	
	public int getMinimum() {
		return minimum;
	}
	
	public int getMaximum() {
		return maximum;
	}

	public int getStep() {
		return step;
	}
	
	protected void setMinimum(int minimum) {
		this.minimum = minimum;
	}

	protected void setMaximum(int maximum) {
		this.maximum = maximum;
	}

	protected void setStep(int step) {
		this.step = step;
	}

	@Override
	protected BasicProtocol getProtocol() {
		// return protocol as extended type
		return (BasicProtocol)super.getProtocol();
	}
	
	@Override
	public void refresh() {
		super.refresh();
		position = -1;
	}

	public void setPortPosition(IBasicProtocol protocol, int position) {
		if (protocol != getProtocol())
			throw new IllegalAccessError("Setting the port state is only allowed from its protocol implementation");
		this.position = position;
	}
	
	// Retrieve the position from the device
	public int getPosition() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		if (position < 0)
			getPortState();
		return position;
	}
		
	public void setPosition(int pos) throws ProtocolException, TimeoutException, InterruptedException, DisconnectedException, DeviceException, PortAccessDeniedException {
		// limit value range
		int val = pos;
		if (val < minimum) val = minimum;
		// adjust to nearest step
		int modulo = val % step;
		// round up or down, depending on what's nearest
		val = (modulo <= step / 2 ? val - modulo : val + step - modulo);
		if (val > maximum) val = maximum;		
		clearError();
		try {
			getProtocol().setPosition(this, val);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}
	
	@Override
	public void getPortState() throws TimeoutException, InterruptedException,
			DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		clearError();
		// Request port state
		try {
			getProtocol().getPosition(this);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}


	@Override	
	public boolean isReadonly() {
		return (flags & PORTFLAG_READONLY) == PORTFLAG_READONLY;
	}
}
