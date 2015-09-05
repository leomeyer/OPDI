package org.ospdi.opdi.protocol;

@SuppressWarnings("serial")
public class PortErrorException extends Exception {

	protected String portID = "";	// avoid null
	
	public PortErrorException() {
		// TODO Auto-generated constructor stub
	}

	public PortErrorException(String portID, String message) {
		super(message);
		this.portID = portID;
	}

	public String getPortID() {
		return portID;
	}
}
