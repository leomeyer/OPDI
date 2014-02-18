package org.ospdi.opdi.utils;

import java.util.HashMap;

public class ResourceFactory {

	public static ResourceFactory instance = new ResourceFactory();
	
	public static final String DISCONNECTED = "opdi_disconnected";
	public static final String CONNECTING = "opdi_connecting";
	public static final String CONNECTED = "opdi_connected";
	public static final String DISCONNECTING = "opdi_disconnecting";
	public static final String ERROR = "opdi_error";
	public static final String IO_FAILURE = "opdi_io_failure";
	public static final String OPERATION_WAS_ABORTED = "opdi_operation_aborted";
	public static final String DEVICE_DID_NOT_RESPOND = "opdi_device_did_not_respond";
	public static final String DEVICE_CLOSED_CONNECTION = "opdi_device_closed_connection";
	public static final String AUTHENTICATION_FAILED = "opdi_authentication_failed";
	public static final String PROTOCOL_ERROR = "opdi_protocol_error";
	public static final String CONNECTED_OPTIONAL_ENCRYPTION = "opdi_connection_optional_encryption";
	public static final String SECURED_WITH_ENCRYPTION = "opdi_secured_with_encryption_0";
	// translate -->
	public static final String INVALID_HANDSHAKE_MESSAGE = "opdi_invalid_handshake_message";
	public static final String AGREEMENT_EXPECTED = "opdi_agreement_expected";
	public static final String UNEXPECTED_HANDSHAKE = "opdi_unexpected_handshake";
	public static final String HANDSHAKE_VERSION = "opdi_handshake_version";
	public static final String HANDSHAKE_VERSION_NOT_SUPPORTED = "opdi_handshake_version_unsupported";
	public static final String ENCODING_NOT_SUPPORTED = "opdi_charset_unsupported";
	public static final String ENCRYPTION_NOT_SUPPORTED = "opdi_encryption_unsupported";
	public static final String FLAGS_INVALID = "opdi_flags_invalid";
	public static final String ENCRYPTION_REQUIRED = "opdi_encryption_required";
	public static final String ENCRYPTION_DISALLOWED = "opdi_encryption_disallowed";
	public static final String NO_SUPPORTED_PROTOCOL = "opdi_no_supported_protocol";
	public static final String DEVICE_CANCELLED_BECAUSE = "opdi_device_cancelled_because";
	public static final String DEVICE_CANCELLED = "opdi_device_cancelled";
	public static final String ENCRYPTION_KEY_INVALID = "opdi_encryption_key_invalid";
	public static final String ENCRYPTION_ERROR_INITIALIZING = "opdi_encryption_error_init";
	public static final String ENCRYPTION_ERROR_BLOCK = "opdi_encryption_error_block";
	public static final String TIMEOUT_WAITING_FOR_MESSAGE = "opdi_timeout_waiting_for_message";
	public static final String CONNECTION_FAILURE = "opdi_connection_failure";
	public static final String INVALID_MESSAGE = "opdi_invalid_message";
	public static final String DEVICE_ERROR = "opdi_device_error";
	
	
	protected final HashMap<String, String> stringMap = new HashMap<String, String>();
	
	protected ResourceFactory() {
		stringMap.put(DISCONNECTED, "DISCONNECTED");
		stringMap.put(CONNECTING, "CONNECTING...");
		stringMap.put(CONNECTED, "CONNECTED");
		stringMap.put(DISCONNECTING, "DISCONNECTING...");
		stringMap.put(ERROR, "ERROR");
		stringMap.put(IO_FAILURE, "I/O failure");
		stringMap.put(OPERATION_WAS_ABORTED, "The operation was aborted");
		stringMap.put(DEVICE_DID_NOT_RESPOND, "The device did not respond");
		stringMap.put(DEVICE_CLOSED_CONNECTION, "The device closed the connection");
		stringMap.put(AUTHENTICATION_FAILED, "Authentication failed");
		stringMap.put(PROTOCOL_ERROR, "Protocol error");
		stringMap.put(CONNECTED_OPTIONAL_ENCRYPTION, "Connected: {0}{1}");
		stringMap.put(SECURED_WITH_ENCRYPTION, " (secured with {0})");
		stringMap.put(INVALID_HANDSHAKE_MESSAGE, "The device did not send a valid handshake message");
		stringMap.put(AGREEMENT_EXPECTED, "Agreement expected, but received: ");
		stringMap.put(UNEXPECTED_HANDSHAKE, "The device did not return the expected handshake: ");
		stringMap.put(HANDSHAKE_VERSION, "Handhake message version");
		stringMap.put(HANDSHAKE_VERSION_NOT_SUPPORTED, "The handshake message version '{0}' is not supported by this master version");
		stringMap.put(ENCODING_NOT_SUPPORTED, "Unsupported encoding: ");
		stringMap.put(ENCRYPTION_NOT_SUPPORTED, "Unsupported encryption: "); 
		stringMap.put(FLAGS_INVALID, "Invalid flags: ");
		stringMap.put(ENCRYPTION_REQUIRED, "The device did not specify an encryption method even though it requires encryption");
		stringMap.put(ENCRYPTION_DISALLOWED, "The device specified an encryption method even though it does not allow encryption");
		stringMap.put(NO_SUPPORTED_PROTOCOL, "The device did not specify a supported communication protocol");
		stringMap.put(DEVICE_CANCELLED_BECAUSE, "The device canceled the handshake because: {0}");
		stringMap.put(DEVICE_CANCELLED, "The device canceled the handshake (no reason specified)");
		stringMap.put(ENCRYPTION_KEY_INVALID, "Encryption key may not be null or empty");
		stringMap.put(ENCRYPTION_ERROR_INITIALIZING, "Error initializing encryption engine: ");
		stringMap.put(ENCRYPTION_ERROR_BLOCK, "Error encrypting block: ");
		stringMap.put(TIMEOUT_WAITING_FOR_MESSAGE, "Timeout waiting for a message from the device");
		stringMap.put(CONNECTION_FAILURE, "Connection failure: {0}");
		stringMap.put(INVALID_MESSAGE, "Invalid message: {0}");
		stringMap.put(DEVICE_ERROR, "Error: {0}");
	}
	
	public String getString(String id) {
		if (stringMap.containsKey(id)) {
			return stringMap.get(id);
		}
		
		// fallback: return ID
		return id;
	}
}
