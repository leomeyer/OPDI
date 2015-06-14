package org.ospdi.opdi.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.PortErrorException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.utils.Strings;


/** Represents a digital port on a device. This port may be of type input, output, or bidirectional.
 * 
 * @author Leo
 *
 */
public class DigitalPort extends Port {

	/** The mode a digital port may be in.*/
	public enum PortMode {
		/** The port is in input mode and floating. */
		INPUT_FLOATING,
		/** The port is in input mode and the pull down resistor is active. */
		INPUT_PULLUP,
		/** The port is in input mode and the pull down resistor is active. */
		INPUT_PULLDOWN,
		/** The port is in output mode. */
		OUTPUT,
		/** UNKNOWN must be the last one */
		UNKNOWN		
	}	

	/** The states a digital line may be in. */
	public enum PortLine {		
		/** Indicates that the port is low. */
		LOW,
		/** Indicates that the port is high. */
		HIGH,
		/** UNKNOWN must be the last one */
		UNKNOWN		
	}	
	
	static final String MAGIC = "DP";
	
	public static final int FLAG_HAS_PULLUP = 1;
	public static final int FLAG_HAS_PULLDOWN = 2;

	// State section
	protected PortMode mode;
	protected PortLine line;
	
	protected DigitalPort(String id, String name, PortDirCaps portDirection, int flags) {
		super(null, id, name, PortType.DIGITAL, portDirection);
		this.flags = flags;
		mode = PortMode.INPUT_FLOATING;
	}
	
	/** Constructor for deserializing from wire form
	 * 
	 * @param protocol
	 * @param parts
	 * @throws ProtocolException
	 */
	public DigitalPort(IBasicProtocol protocol, String[] parts) throws ProtocolException {
		super(protocol, null, null, PortType.DIGITAL, null);
		
		final int ID_PART = 1;
		final int NAME_PART = 2;
		final int DIR_PART = 3;
		final int FLAGS_PART = 4;
		final int PART_COUNT = 5;
		
		checkSerialForm(parts, PART_COUNT, MAGIC);

		setID(parts[ID_PART]);
		setName(parts[NAME_PART]);
		setDirCaps(PortDirCaps.values()[Strings.parseInt(parts[DIR_PART], "PortDirCaps", 0, PortDirCaps.values().length - 1)]);
		flags = Strings.parseInt(parts[FLAGS_PART], "flags", 0, Integer.MAX_VALUE);
	}
	
	public String serialize() {
		return Strings.join(':', MAGIC, getID(), getName(), getDirCaps().ordinal(), flags);
	}
	
	public boolean hasPullup() {
		return (flags & FLAG_HAS_PULLUP) == FLAG_HAS_PULLUP;
	}

	public boolean hasPulldown() {
		return (flags & FLAG_HAS_PULLDOWN) == FLAG_HAS_PULLDOWN;
	}
	
	// Retrieve all port state from the device
	public void getPortState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		clearError();
		// Request port state
		try {
			getProtocol().getPortState(this);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}
	
	public void setPortState(IBasicProtocol protocol, PortMode mode) throws ProtocolException {
		if (protocol != getProtocol())
			throw new IllegalAccessError("Setting the port state is only allowed from its protocol implementation");
		try {
			checkMode(mode);
		} catch (Exception e) {
			throw new ProtocolException(e.getMessage());
		}
		this.mode = mode;
	}

	public void setPortLine(IBasicProtocol protocol, PortLine line) {
		if (protocol != getProtocol())
			throw new IllegalAccessError("Setting the port state is only allowed from its protocol implementation");
		this.line = line;
	}
	
	private void checkMode(PortMode portMode) {
		// configure for output?
		if (portMode == PortMode.OUTPUT)
			if (getDirCaps() == PortDirCaps.INPUT)
				throw new IllegalArgumentException("Can't configure input only digital port for output: ID = " + getID());
		// configure for input?
		if (portMode == PortMode.INPUT_FLOATING || portMode == PortMode.INPUT_PULLUP || portMode == PortMode.INPUT_PULLDOWN)
			if (getDirCaps() == PortDirCaps.OUTPUT)
				throw new IllegalArgumentException("Can't configure output only digital port for input: ID = " + getID());
		// check pullup/pulldown
		if (portMode == PortMode.INPUT_PULLUP && !hasPullup())
			throw new IllegalArgumentException("Digital port has no pullup: ID = " + getID());
		if (portMode == PortMode.INPUT_PULLDOWN && !hasPulldown())
			throw new IllegalArgumentException("Digital port has no pullup: ID = " + getID());
	}

	/** Configures the port in the given mode. Throws an IllegalArgumentException if the mode
	 * is not supported.
	 * @param portMode
	 * @throws PortErrorException 
	 * @throws PortAccessDeniedException 
	 */
	public void setMode(PortMode portMode)  throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		checkMode(portMode);
		clearError();
		// set the mode
		try {
			getProtocol().setPortMode(this, portMode);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}
	
	public PortMode getMode() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		if (mode == null)
			getPortState();
		return mode;
	}
	
	/** Sets the port to the given state. Throws an IllegalArgumentException if the state
	 * is not supported.
	 * @param portState
	 */
	public void setLine(PortLine portLine)  throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		if (getDirCaps() == PortDirCaps.INPUT)
			throw new IllegalArgumentException("Can't set state on input only digital port: ID = " + getID());
		if (mode != PortMode.OUTPUT)
			throw new IllegalArgumentException("Can't set state on digital port configured as input: ID = " + getID());
		clearError();
		// set the line state
		try {
			getProtocol().setPortLine(this, portLine);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}
	
	public PortLine getLine() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		if (line == null)
			// get state from the device
			getPortState();
		return line;
	}
	
	@Override
	public String toString() {
		return "DigitalPort id=" + getID() + "; name='" + getName() + "'; type=" + getType() + "; dir_caps=" + getDirCaps() + "; hasPullup=" + hasPullup() + "; hasPulldown=" + hasPulldown() + "; mode=" + mode;
	}

	@Override
	public void refresh() {
		super.refresh();
		mode = null;
		line = null;
	}

	@Override	
	public boolean isReadonly() {
		return (flags & PORTFLAG_READONLY) == PORTFLAG_READONLY;
	}
}
