package org.ospdi.opdi.protocol;

/** This exception can be thrown in case of a protocol error.
 * 
 * @author Leo
 *
 */
@SuppressWarnings("serial")
public class ProtocolException extends Exception {

	public ProtocolException() {
		super();
	}

	public ProtocolException(String detailMessage, Throwable throwable) {
		super(detailMessage, throwable);
	}

	public ProtocolException(String detailMessage) {
		super(detailMessage);
	}

	public ProtocolException(Throwable throwable) {
		super(throwable);
	}
}
