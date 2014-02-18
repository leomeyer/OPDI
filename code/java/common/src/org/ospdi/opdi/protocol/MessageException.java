package org.ospdi.opdi.protocol;

@SuppressWarnings("serial")
public class MessageException extends Exception {

	public MessageException() {
		super();
	}

	public MessageException(String detailMessage, Throwable throwable) {
		super(detailMessage, throwable);
	}

	public MessageException(String detailMessage) {
		super(detailMessage);
	}

	public MessageException(Throwable throwable) {
		super(throwable);
	}
	
}
