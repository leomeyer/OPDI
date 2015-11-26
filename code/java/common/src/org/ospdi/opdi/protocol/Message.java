package org.ospdi.opdi.protocol;

import java.nio.charset.Charset;

/** Represents a message that is sent to or from a device.
 *
 * @author Leo
 *
 */
public class Message {
	
	public final static byte TERMINATOR = '\n';

	private int channel;
	private String payload;
	private int checksum;

	/** Creates a message.
	 * 
	 * @param payload
	 */
	public Message(int channel, String payload) {
		super();
		this.channel = channel;
		this.payload = payload;
	}
	
	private Message(int channel, String payload, int checksum) {
		super();
		this.channel = channel;
		this.payload = payload;
		this.checksum = checksum;
	}
	
	public static int calcChecksum(byte[] message) {
		int myChecksum = 0;
		boolean colonFound = false; 
		// add everything before the last colon
		for (int i = message.length - 1; i >= 0; i--)
			if (colonFound)
				// add unsigned byte values
				myChecksum += message[i] & 0xFF;
			else
				colonFound = message[i] == ':';
		// checksum is limited to 16 bits
		return myChecksum & 0xFFFF;
	}
	
	/** Tries to decode a message from its serial form.
	 * 
	 * @return
	 */
	public static Message decode(byte[] serialForm, Charset encoding) throws MessageException {
		String message = new String(serialForm, encoding);
		// split at ":"
		String[] parts = message.split(":");
		// valid form?
		if (parts.length < 3) 
			throw new MessageException("Message part number too low");
		// last part must be checksum
		try {
			int checksum = Integer.parseInt(parts[parts.length - 1].trim(), 16);
			StringBuilder content = new StringBuilder();
			for (int i = 0; i < parts.length - 1; i++) {
				content.append(parts[i]);
				// include intermediate ":"s
				if (i < parts.length - 2)
					content.append(':');
			}
			// calculate content checksum
			int calcCheck = calcChecksum(serialForm);
			// checksums not equal?
			if (calcCheck != checksum) {
				throw new MessageException(String.format("Message checksum invalid: %04x, expected: %04x", calcCheck, checksum));
			}
			// checksum is ok
			// the first part is the channel
			try {
				// trim the channel number (in case of padding characters)
				int pid = Integer.parseInt(parts[0].trim());
				// the payload doesn't contain the channel
				return new Message(pid, content.toString().substring(parts[0].length() + 1), checksum);
			} catch (NumberFormatException nfe) {
				throw new MessageException("Message channel number invalid");
				
			}
		} catch (NumberFormatException nfe) {
			throw new MessageException("Message checksum invalid");
		}
	}
	
	private String addLeadingZeroes(String str, int digits) {
		String result = str;
		while (result.length() < digits)
			result = "0" + result;
		return result;
	}
	
	/** Returns the serial form of a message that contains payload and checksum.
	 * 
	 * @return
	 * @throws MessageException 
	 */
	public byte[] encode(Charset encoding) throws MessageException {
		// message content is channel plus payload
		StringBuilder content = new StringBuilder();
		content.append(channel);
		content.append(':');
		content.append(payload);
		// calculate message checksum over the bytes to transfer
		checksum = 0;
		byte[] bytes = content.toString().getBytes(encoding);
		for (int i = 0; i < bytes.length; i++) {
			if (bytes[i] == TERMINATOR)
				throw new MessageException("Message terminator may not appear in payload");
			// add unsigned bytes for payload
			checksum += bytes[i] & 0xFF;
		}
		// A message is terminated by the checksum and a \n
		content.append(":");
		content.append(addLeadingZeroes(Integer.toHexString(checksum & 0xffff), 4));
		content.append((char)TERMINATOR);
		bytes = content.toString().getBytes(encoding);
		return bytes;
	}

	public String getPayload() {
		return payload;
	}

	public int getChannel() {
		return channel;		
	}
	
	@Override
	public String toString() {
		return String.format("%d:%s:%04x", channel, payload, checksum); 
	}
}
