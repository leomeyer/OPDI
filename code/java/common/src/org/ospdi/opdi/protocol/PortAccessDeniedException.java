package org.ospdi.opdi.protocol;

@SuppressWarnings("serial")
public class PortAccessDeniedException extends Exception {
	
	protected String portID = "";	// avoid null
	
	public PortAccessDeniedException() {
		// TODO Auto-generated constructor stub
	}

	public PortAccessDeniedException(String portID, String message) {
		super(message);
		this.portID = portID;
	}

	public String getPortID() {
		return portID;
	}
}
