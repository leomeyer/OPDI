package org.ospdi.opdi.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.PortErrorException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.utils.Strings;


/** Represents a digital port on a device. This class also serves as a proxy that communicates 
 * with the device via the specified protocol. Caches certain values. Normally the device
 * does not change these values of its own accord. If it does (for example, change the resolution)
 * it should notify the client to call the refresh() method of this class which will cause
 * the cache to be cleared, so that the current values are reloaded from the device.
 * 
 * @author Leo
 *
 */
public class AnalogPort extends Port {
	
	/** The mode an analog port may be in. */
	public enum PortMode {
		/** The port is in input mode. */
		INPUT,
		/** The port is in output mode. */
		OUTPUT
	}	
		
	/** Defines the resolution of an analog port. */
	public enum Resolution {
		BITS_8,
		BITS_9,
		BITS_10,
		BITS_11,
		BITS_12
	}
	
	/** Defines the reference voltage source of an analog port. */
	public enum Reference {
		INTERNAL,
		EXTERNAL
	}
	
	static final String MAGIC = "AP";
		
	// These flags define the capabilities of a port, not its actual settings.
	public static int ADC_CAN_CHANGE_RESOLUTION = 1;
	public static int ADC_RESOLUTION_8BITS = 2;
	public static int ADC_RESOLUTION_9BITS = 4;
	public static int ADC_RESOLUTION_10BITS = 8;
	public static int ADC_RESOLUTION_11BITS = 16;
	public static int ADC_RESOLUTION_12BITS = 32;
	public static int ADC_CAN_CHANGE_REFERENCE = 512;
	public static int ADC_REFERENCE_INTERNAL = 1024;
	public static int ADC_REFERENCE_EXTERNAL = 2048;
	
	/** Flag combination that includes all resolutions. */
	public static int ADC_ALL_RESOLUTIONS = ADC_RESOLUTION_8BITS | ADC_RESOLUTION_9BITS | ADC_RESOLUTION_10BITS | ADC_RESOLUTION_11BITS | ADC_RESOLUTION_12BITS;   
	
	protected int flags;

	// state cache
	protected int value;
	protected PortMode mode;
	protected Resolution resolution;
	protected Reference reference;

	protected AnalogPort(String id, String name, PortDirCaps dirCaps, int flags) {
		super(null, id, name, PortType.ANALOG, dirCaps);
		this.flags = flags;
		value = -1;
	}
	
	/** Constructor for deserializing from wire form
	 * 
	 * @param protocol
	 * @param parts
	 * @throws ProtocolException
	 */
	public AnalogPort(IBasicProtocol protocol, String[] parts) throws ProtocolException {
		super(protocol, null, null, PortType.ANALOG, null);
		
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

	public int getFlags() {
		return flags;
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

	public void setPortState(IBasicProtocol protocol, PortMode mode, Reference ref, Resolution res) {
		if (protocol != getProtocol())
			throw new IllegalAccessError("Setting the port state is only allowed from its protocol implementation");
		this.mode = mode;
		resolution = res;
		reference = ref;
	}

	public void setPortValue(IBasicProtocol protocol, int value) {
		if (protocol != getProtocol())
			throw new IllegalAccessError("Setting the port state is only allowed from its protocol implementation");
		this.value = value;
	}

	/** Configures the port in the given mode. Throws an IllegalArgumentException if the mode
	 * is not supported.
	 * @param portMode
	 * @throws PortAccessDeniedException 
	 */
	public PortMode setMode(PortMode portMode) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		// configure for output?
		if (portMode == PortMode.OUTPUT) {
			if (getDirCaps() == PortDirCaps.INPUT)
				throw new IllegalArgumentException("Can't configure input only analog port for output: ID = " + getID());
		// configure for input?
		} 
		else if (portMode == PortMode.INPUT) {
			if (getDirCaps() == PortDirCaps.OUTPUT)
				throw new IllegalArgumentException("Can't configure output only analog port for input: ID = " + getID());
		} else {
			throw new IllegalArgumentException("An analog port supports only input and output modes, but not: " + portMode);			
		}
		clearError();
		// set the mode
		try {
			getProtocol().setPortMode(this, portMode);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
		return mode;
	}
	
	public PortMode getMode() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		// use cached value
		if (mode == null)
			getPortState();
		return mode;
	}
		
	private void checkResolution(int resFlag, Resolution res) {
		if ((flags & resFlag) != resFlag)
			throw new IllegalArgumentException("Resolution not supported for analog port: " + getID() + ", res=" + res);
	}
	
	protected void checkResolution(Resolution res) {
		// check whether resolution is possible
		switch (resolution) {
			case BITS_8: checkResolution(ADC_RESOLUTION_8BITS, resolution); break;
			case BITS_9: checkResolution(ADC_RESOLUTION_9BITS, resolution); break;
			case BITS_10: checkResolution(ADC_RESOLUTION_10BITS, resolution); break;
			case BITS_11: checkResolution(ADC_RESOLUTION_11BITS, resolution); break;
			case BITS_12: checkResolution(ADC_RESOLUTION_12BITS, resolution); break;
			default: throw new IllegalArgumentException("check not implemented for resolution: " + resolution);
		}		
	}
	
	public Resolution setResolution(Resolution resolution) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		checkResolution(resolution);
		clearError();
		try {
			getProtocol().setPortResolution(this, resolution);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
		return this.resolution;
	}

	public Resolution getResolution() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		// use cached value
		if (resolution == null)
			getPortState();
		return resolution;
	}

	private void checkReference(int refFlag, Reference ref) {
		if ((flags & refFlag) != refFlag)
			throw new IllegalArgumentException("Reference not supported for analog port: " + getID() + ", ref=" + ref);
	}

	protected void checkReference(Reference ref) {
		switch (ref) {
		case INTERNAL: checkReference(ADC_REFERENCE_INTERNAL, ref); break;
		case EXTERNAL: checkReference(ADC_REFERENCE_EXTERNAL, ref); break;
		default: throw new IllegalArgumentException("check not implemented for reference: " + ref);
		}
	}
	public Reference setReference(Reference reference) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		checkReference(reference);
		clearError();
		try {
			getProtocol().setPortReference(this, reference);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
		return this.reference;
	}

	public Reference getReference() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		// use cached value
		if (reference == null)
			getPortState();			
		return reference;
	}

	/** Sets the port to the given value. Throws an IllegalArgumentException if setting the value
	 * is not supported.
	 * @param value
	 * @throws PortAccessDeniedException 
	 */
	public void setValue(int value) throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		if (getDirCaps() == PortDirCaps.INPUT)
			throw new IllegalArgumentException("Can't set value on input only analog port: ID = " + getID());
		if (mode != PortMode.OUTPUT)
			throw new IllegalArgumentException("Can't set value on analog port configured as input: ID = " + getID());
		// limit value range
		int val = value;
		if (val < 0) val = 0;
		int maximum = getMaxValue();
		if (val > maximum) val = maximum;
		clearError();
		try {
			getProtocol().setPortValue(this, value);
		} catch (PortErrorException e) {
			handlePortError(e);
		}
	}
	
	/** Returns the current value of the analog port.
	 * 
	 * @return
	 * @throws PortAccessDeniedException 
	 */
	public int getValue() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		// use cached value
		if (value < 0)
			getPortState();			
		return value;
	}
	
	@Override
	public String toString() {
		return "AnalogPort id=" + getID() + "; name='" + getName()  + "'; type=" + getType() + "; dir_caps=" + getDirCaps() 
			+ "; flags=" + flags;
	}

	public int getMaxValue() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
		Resolution res = getResolution();
		int maximum;
		switch (res) {
		case BITS_8: maximum = 255; break;		
		case BITS_9: maximum = 511; break;
		case BITS_10: maximum = 1023; break;
		case BITS_11: maximum = 2047; break;
		case BITS_12: maximum = 4095; break;
		default:
			throw new IllegalArgumentException("Unknown resolution: " + res);
		}
		return maximum;
	}
	
	@Override
	public void refresh() {
		super.refresh();
		value = -1;
		mode = null;
		resolution = null;
		reference = null;
	}
}
