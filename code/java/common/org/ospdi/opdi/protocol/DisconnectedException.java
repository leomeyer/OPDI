package org.ospdi.opdi.protocol;

/** This exception is thrown when a connected device disconnects via the protocol
 * while a message is expected.
 * 
 * @author Leo
 *
 */
@SuppressWarnings("serial")
public class DisconnectedException extends Exception {

	public DisconnectedException() {
		super();
		// TODO Auto-generated constructor stub
	}

	public DisconnectedException(String detailMessage, Throwable throwable) {
		super(detailMessage, throwable);
		// TODO Auto-generated constructor stub
	}

	public DisconnectedException(String detailMessage) {
		super(detailMessage);
		// TODO Auto-generated constructor stub
	}

	public DisconnectedException(Throwable throwable) {
		super(throwable);
		// TODO Auto-generated constructor stub
	}

}
