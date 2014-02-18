package org.ospdi.opdi.devices;

import java.util.HashMap;

public class DeviceErrors {

	protected static HashMap<Integer, String> errors = new HashMap<Integer, String>();
	
	static {
		errors.put(-1, "");		// special case: error on master

		errors.put(0, "STATUS_OK");
		errors.put(1, "DISCONNECTED");
		errors.put(2, "TIMEOUT");
		errors.put(3, "CANCELLED");
		errors.put(4, "ERROR_MALFORMED_MESSAGE");
		errors.put(5, "ERROR_CONVERSION");
		errors.put(6, "ERROR_MSGBUF_OVERFLOW");
		errors.put(7, "ERROR_DEST_OVERFLOW");
		errors.put(8, "ERROR_STRINGS_OVERFLOW");
		errors.put(9, "ERROR_PARTS_OVERFLOW");
		errors.put(10, "PROTOCOL_ERROR");
		errors.put(11, "PROTOCOL_NOT_SUPPORTED");
		errors.put(12, "ENCRYPTION_NOT_SUPPORTED");
		errors.put(13, "ENCRYPTION_REQUIRED");
		errors.put(14, "ENCRYPTION_ERROR");
		errors.put(15, "AUTH_NOT_SUPPORTED");
		errors.put(16, "AUTHENTICATION_EXPECTED");
		errors.put(17, "AUTHENTICATION_FAILED");
		errors.put(18, "DEVICE_ERROR");
		errors.put(19, "TOO_MANY_PORTS");
		errors.put(20, "PORTTYPE_UNKNOWN");
		errors.put(21, "PORT_UNKNOWN");
		errors.put(22, "WRONG_PORT_TYPE");
		errors.put(23, "TOO_MANY_BINDINGS");
		errors.put(24, "NO_BINDING");
		errors.put(25, "CHANNEL_INVALID");
		errors.put(26, "POSITION_INVALID");
		errors.put(27, "NETWORK_ERROR");
		errors.put(28, "TERMINATOR_IN_PAYLOAD");
	}
	
	
	public static String getErrorText(int error) {
		String text = errors.get(error);
		
		if (text == null)
			return "Unknown error number: " + error;
		
		return text;
	}
}
