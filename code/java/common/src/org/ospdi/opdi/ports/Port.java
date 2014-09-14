package org.ospdi.opdi.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.PortErrorException;
import org.ospdi.opdi.protocol.ProtocolException;


/** Defines the abstract properties and functions of a port.
 * 
 * @author Leo
 *
 */
public abstract class Port {
	
	/** Defines different types of ports. */
	public enum PortType {
		/** A digital port with two states (low and high). */
		DIGITAL,
		/** An analog port. */
		ANALOG,
		/** A select port. */
		SELECT,
		/** A USART. */
		STREAMING,
		/** Another port type. */
		OTHER
	}	
	
	/** Defines the possible directions (capabilities) of ports. */
	public enum PortDirCaps {
		/** A port that can only be used for input. */
		INPUT,		
		/** A port that can only be used for output. */
		OUTPUT,		
		/** A port that can be configured for input or output. */
		BIDIRECTIONAL
	}

	protected IBasicProtocol protocol;
	protected String id;
	protected String name;
	protected PortType type;
	protected PortDirCaps dirCaps;
	protected Object viewAdapter;
	protected boolean hasError;
	protected String errorMessage;
	
	/** Only to be used by subclasses
	 * 
	 * @param protocol
	 * @param id
	 * @param name
	 * @param type
	 */
	protected Port(IBasicProtocol protocol, String id, String name, PortType type, PortDirCaps dirCaps) {
		super();
		this.protocol = protocol;
		this.id = id;
		this.name = name;
		this.type = type;
		this.dirCaps = dirCaps;
	}	

	public synchronized boolean hasError() {
		return hasError;
	}

	public synchronized String getErrorMessage() {
		if (errorMessage == null)
			return "";
		return errorMessage;
	}
	
	/** Returns the protocol.
	 * 
	 * @return
	 */
	protected IBasicProtocol getProtocol() {
		return protocol;
	}

	/** Returns the unique ID of this port.
	 * 
	 * @return
	 */
	public String getID() {
		return id;
	}

	/** Returns the name of this port.
	 * 
	 * @return
	 */
	public String getName() {
		return name;
	}
		
	/** Returns the type of this port.
	 * 
	 * @return
	 */
	public PortType getType() {
		return type;
	}

	/** Returns the possible direction of this port.
	 * 
	 * @return
	 */
	public PortDirCaps getDirCaps() {
		return dirCaps;
	}

	protected void setProtocol(IBasicProtocol protocol) {
		this.protocol = protocol;
	}

	protected void setID(String id) {
		this.id = id;
	}

	protected void setName(String name) {
		this.name = name;
	}

	protected void setType(PortType type) {
		this.type = type;
	}

	protected void setDirCaps(PortDirCaps dir) {
		dirCaps = dir;
	}
	
	/** A convenience method for subclasses. Throws a ProtocolException if the part count doesn't match
	 * or if the first part is not equal to the magic.
	 * @param parts
	 * @param count
	 * @param magic
	 * @throws ProtocolException
	 */
	protected void checkSerialForm(String[] parts, int count, String magic) throws ProtocolException {
		if (parts.length != count) 
			throw new ProtocolException("Serial form invalid");
		if (!parts[0].equals(magic))
			throw new ProtocolException("Serial form invalid: wrong magic");		
	}

	/** Returns a serialized description of this port.
	 * 
	 * @return
	 */
	public abstract String serialize();
	
	/** Clear cached state values.
	 * 
	 */
	public void refresh() {
		clearError();
	}

	@Override
	public String toString() {
		return "Port id=" + getID() + "; name='" + getName() + "'; type=" + getType() + "; dir_caps=" + getDirCaps();
	}

	/** Stores an arbitrary view object.
	 * 
	 * @param viewAdapter
	 */
	public void setViewAdapter(Object viewAdapter) {
		this.viewAdapter = viewAdapter;
	}

	/** Returns a stored view object.
	 * 
	 * @return
	 */
	public Object getViewAdapter() {
		return viewAdapter;
	}

	/** Fetches the state of the port from the device.
	 * 
	 */
	public abstract void getPortState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException;
	
	/** Should be called internally before port handling methods are called.
	 * 
	 */
	protected void clearError() {
		hasError = false;
		errorMessage = null;
	}
	
	protected void handlePortError(PortErrorException e) {
		hasError = true;
		errorMessage = e.getMessage();
	}
}
