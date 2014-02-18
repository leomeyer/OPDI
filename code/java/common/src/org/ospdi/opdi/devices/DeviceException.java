package org.ospdi.opdi.devices;

/** This class is used to indicate an error on the device while executing a protocol request or otherwise.
 * 
 * @author Leo
 *
 */
@SuppressWarnings("serial")
public class DeviceException extends Exception {

	public DeviceException() {
		// TODO Auto-generated constructor stub
	}

	public DeviceException(String detailMessage) {
		super(detailMessage);
		// TODO Auto-generated constructor stub
	}

	public DeviceException(Throwable throwable) {
		super(throwable);
		// TODO Auto-generated constructor stub
	}

	public DeviceException(String detailMessage, Throwable throwable) {
		super(detailMessage, throwable);
		// TODO Auto-generated constructor stub
	}

}
