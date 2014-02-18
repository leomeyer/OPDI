package org.ospdi.opdi.devices;

import java.io.IOException;
import java.nio.charset.Charset;
import java.security.InvalidKeyException;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.Random;
import java.util.Set;
import java.util.concurrent.TimeoutException;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.spec.SecretKeySpec;

import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.interfaces.IDeviceListener;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.protocol.AbstractProtocol.IAbortable;
import org.ospdi.opdi.protocol.Message;
import org.ospdi.opdi.protocol.MessageException;
import org.ospdi.opdi.utils.ResourceFactory;

/** This class defines the functions of a device that communicates via messages that are put into
 * input and output queues. It provides an IBasicProtocol object that is connected to these queues.
 * 
 * @author Leo
 *
 */
public abstract class MessageQueueDevice implements IDevice {
	
	/** Defines the possible encryption methods.
	 * 
	 * @author Leo
	 *
	 */
	public enum Encryption {
		AES
	}

	protected static final int DEFAULT_BUFFER_SIZE = 256;
	protected static final Charset DEFAULT_ENCODING = Charset.forName("ISO-8859-1");
	
	protected static final int TERMINATION_TIMEOUT = 3000;

	// time in ms of the last message send as measured by System.currentTimeMillis()
	protected volatile long lastSendTimeMS;

	/**
     * This thread runs during a connection with a remote device.
     * It handles all incoming and outgoing transmissions.
     */
    private class MessageProcessor extends Thread {

    	private IBasicProtocol protocol;
        private boolean stop = false;
        private boolean done = false;
        // flag to ensure that remaining messages are sent before stopping
        public volatile boolean hasMessagesToSend = false;        

        protected MessageProcessor(IBasicProtocol protocol) {
			super();
			this.protocol = protocol;
		}

		public void run() {
            // Read and write messages while not disconnected
			byte[] message = new byte[getBufferSize()];
			byte[] buffer = new byte[getBufferSize()];
	        int bytesProcessed = 0;
            while (!stop || hasMessagesToSend) {
                try {
                    // Read from the InputStream
                	synchronized(inQueue) {
                    	if ((encryption == null ? hasBytes() > 0 : has_block())) {
                    		// fill buffer with available bytes
                    		int bytes = (encryption == null ? read(buffer) : read_block(buffer));
                    		// need to increase message?
                    		if (bytesProcessed + bytes > message.length) {
                    			byte[] msg = message;
                    			message = new byte[message.length + getBufferSize()];
                    			System.arraycopy(msg, 0, message, 0, bytesProcessed);
                    		}
                    		// append received bytes to message until terminator character
                    		int terminatorPos = -1;
                    		int bufferEnd = 0;
                    		for (; bufferEnd < bytes; bytesProcessed++, bufferEnd++) {
                    			message[bytesProcessed] = buffer[bufferEnd];
                    			if (buffer[bufferEnd] == Message.TERMINATOR) {
                    				terminatorPos = bytesProcessed;
                    				break;
                    			}
                    		}
                    		
                    		// at least one complete message?
                    		while (terminatorPos > -1) {
                    			int msgLen = terminatorPos;
                    			// copy current part
                    			byte[] part = new byte[msgLen];
                    			System.arraycopy(message, 0, part, 0, msgLen);
                            	// try to decode the message
                            	Message msg;
								try {
									msg = Message.decode(part, getEncoding());
                            		logDebug("Message received: " + msg.toString());
                            		// message is valid
	                        		// let the protocol dispatch the message in case it contains streaming data
	                        		if (!protocol.dispatch(msg))
	                        			// add the message to the input queue
	                        			inQueue.add(msg);
								} catch (MessageException e) {
                            		logDebug("Invalid message: " + e.getMessage());
                            		logDebug("Invalid message content: " + (bytesProcessed == 0 ? "<empty>" : new String(part, getEncoding())));
                            	}
                            	
                        		// start over with a new message
                        		bytesProcessed = 0;
                        		terminatorPos = -1;
                        		message = new byte[getBufferSize()];
                            	// there may be remaining characters in buffer after the terminator
                            	if (bytes > ++bufferEnd) {
                            		// only if encryption is off - in this case we discard the remaining buffer because
                            		// we deal with a certain block size
                            		if (encryption == null) {
	                            		for (; bufferEnd < bytes; bytesProcessed++, bufferEnd++) {
	                            			message[bytesProcessed] = buffer[bufferEnd];
	                            			if (buffer[bufferEnd] == Message.TERMINATOR) {
	                            				terminatorPos = bytesProcessed;
	                            				break;
	                            			}
	                            		}
                            		}
                            	}
                    		}
                    	}                		
                	}
                	// are there messages to send?
                	synchronized(outQueue) {
                		if (!outQueue.isEmpty()) {
                			Message m = outQueue.remove();
                			
                			sendSynchronous(m);
                        	
                        	hasMessagesToSend = !outQueue.isEmpty();
                		} else
                			hasMessagesToSend = false;
                	}
                	Thread.sleep(1);
                } catch (IOException e) {
                	if (!stop) {
	                	String msg = MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.CONNECTION_FAILURE), e.getMessage());
	                	setDeviceError(msg);
                	}
                    return;
                } catch (MessageException e) {
                	if (!stop) {
	                	String msg = MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.INVALID_MESSAGE), e.getMessage());
	                	setDeviceError(msg);
                	}
                    return;
                } catch (InterruptedException e) {
                	if (!stop) {
	                	String msg = "Thread interrupted";
	                	setDeviceError(msg);
                	}
                	Thread.currentThread().interrupt();
                    return;
				} catch (DeviceException e) {
                	if (!stop) {
                		String msg = MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.DEVICE_ERROR), e.getMessage());
	                	setDeviceError(msg);
                	}
				} finally {
					done = true;
				}                
            }
        }

		public void stopProcessing() {
        	stop = true;
        }
    }
	
	protected int bufferSize = DEFAULT_BUFFER_SIZE;
	protected Charset encoding = DEFAULT_ENCODING;

	protected static final int AES_BLOCKSIZE = 16;
	
	protected Encryption encryption;
	protected SecretKeySpec skeySpec; 
	protected Cipher cipher;	
	protected Random random;
	
	protected DeviceStatus status = DeviceStatus.DISCONNECTED;
	protected String statusInfo;
	
	MessageProcessor msgThread;
	Queue<Message> inQueue = new LinkedList<Message>();
	Queue<Message> outQueue = new LinkedList<Message>();
	
	List<IDeviceListener> listeners = new ArrayList<IDeviceListener>();
	
	@Override
	public void sendMessages(Message... messages) {
		synchronized(outQueue) {
			outQueue.addAll(Arrays.asList(messages));
			msgThread.hasMessagesToSend = true;
		}
	}

	@Override
	public Queue<Message> getInputMessages() {
		return inQueue;
	}

	protected void clearQueues() {
		synchronized(inQueue) {
			inQueue.clear();
		}
		synchronized(outQueue) {
			outQueue.clear();
		}
	}
	
	@Override
	public long getLastSendTimeMS() {
		return lastSendTimeMS;
	}

	@Override
	public synchronized DeviceStatus getStatus() {
		return status;
	}
	
	protected synchronized void setStatus(DeviceStatus status, String message) {
		this.status = status;
		statusInfo = message;
	}

	protected synchronized void setStatus(DeviceStatus status) {
		this.status = status;
		statusInfo = null;
	}

	@Override
	public synchronized String getStatusText() {
		String resName = "Unknown Status";
		switch (status) {
		case CONNECTED: return MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.CONNECTED_OPTIONAL_ENCRYPTION), getDeviceName(), (getEncryption() != null ? MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.SECURED_WITH_ENCRYPTION), getEncryption().name()) : ""));
		case DISCONNECTED : resName = ResourceFactory.DISCONNECTED; break;
		case CONNECTING : resName = ResourceFactory.CONNECTING; break;
		case DISCONNECTING: resName = ResourceFactory.DISCONNECTING; break;
		case ERROR : resName = ResourceFactory.ERROR; break;
		}
		return ResourceFactory.instance.getString(resName) + (statusInfo != null ? " (" + statusInfo + ")" : "");
	}		

	@Override
	public boolean usesEncryption() {
		return isConnected() && getEncryption() != null;
	}

	/** Start processing the messages.
	 */
	protected void startMessageProcessing(IBasicProtocol protocol) {		
		// Start the thread that handles message IO with the connected device
		msgThread = new MessageProcessor(protocol);
		msgThread.start();
		notifyOpened();
	}

	protected void stopProcessing() {
		synchronized(inQueue) {
			synchronized(outQueue) {
				if (msgThread != null) {
					msgThread.stopProcessing();
				}
			}
		}
	}

	@Override
	public synchronized boolean isConnected() {
		return getStatus() == DeviceStatus.CONNECTED;
	}
	
	@Override
	public synchronized boolean isConnecting() {
		return getStatus() == DeviceStatus.CONNECTING;
	}
	
	@Override
	public void disconnect(boolean onError) {
		if (!onError && !isConnected()) return;
		
		if (msgThread != null) {
			// terminate the connection asynchronously
			// to give it time to finish the protocol
			Thread terminator = new Thread() {
				public void run() {
					setStatus(DeviceStatus.DISCONNECTING);
					notifyTerminating();
					// tell the message processor to stop
					stopProcessing();
					int counter = 0;
					while (counter++ < TERMINATION_TIMEOUT && !msgThread.done) {
						try {
							Thread.sleep(1);
						} catch (InterruptedException e) {
							Thread.currentThread().interrupt();
						}
					}
					// time is up or thread is done				
					close();
					setStatus(DeviceStatus.DISCONNECTED);
					notifyClosed();
				};
			};
			
			terminator.start();
		}
	}

	private void notifyOpened() {
		synchronized(listeners) {
			// create temporary copy
			IDeviceListener[] ls = (IDeviceListener[]) listeners.toArray(new IDeviceListener[listeners.size()]);
			for (IDeviceListener l: ls)
				l.connectionOpened(this);
		}				
	}

	private void notifyTerminating() {
		synchronized(listeners) {
			// create temporary copy
			IDeviceListener[] ls = (IDeviceListener[]) listeners.toArray(new IDeviceListener[listeners.size()]);
			for (IDeviceListener listener: ls)
				listener.connectionTerminating(this);
		}		
	}	

	private void notifyClosed() {
		synchronized(listeners) {
			// create temporary copy
			IDeviceListener[] ls = (IDeviceListener[]) listeners.toArray(new IDeviceListener[listeners.size()]);
			for (IDeviceListener listener: ls)
				listener.connectionClosed(this);
		}		
	}

	private void notifyError(String message) {
		synchronized(listeners) {
			// create temporary copy
			IDeviceListener[] ls = (IDeviceListener[]) listeners.toArray(new IDeviceListener[listeners.size()]);
			for (IDeviceListener listener: ls)
				listener.connectionError(this, message);
		}		
	}
	
	@Override
	public void addConnectionListener(IDeviceListener listener) {
		synchronized(listeners) {
			if (!listeners.contains(listener))
				listeners.add(listener);
		}
	}

	@Override
	public void removeConnectionListener(IDeviceListener listener) {
		synchronized(listeners) {
			listeners.remove(listener);
		}
	}

	protected synchronized int getBufferSize() {
		return bufferSize;
	}

	protected synchronized void setBufferSize(int bufferSize) {
		this.bufferSize = bufferSize;
	}

	protected synchronized Charset getEncoding() {
		return encoding;
	}

	protected synchronized void setEncoding(Charset encoding) {
		if (!getMastersSupportedCharsets().contains(encoding))
			throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCODING_NOT_SUPPORTED) + encoding);
		this.encoding = encoding;
	}
	
	protected synchronized void clearEncryption() {
		this.encryption = null;
	}

	/** May return null in case there is no encryption.
	 * 
	 * @return
	 */
	protected synchronized Encryption getEncryption() {
		return encryption;
	}

	protected synchronized void setEncryption(Encryption encryption) throws DeviceException {
		random = new Random();
		if (encryption == null)
			clearEncryption();
		else
			switch (encryption) {
			case AES:
				if (getEncryptionKey() == null || getEncryptionKey().isEmpty())
					throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_KEY_INVALID));
				
				skeySpec = new SecretKeySpec(getEncryptionKey().getBytes(), "AES");
	
				// Instantiate the cipher
				try {
					cipher = Cipher.getInstance("AES/ECB/NoPadding");
				} catch (Exception e) {
					throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_INITIALIZING) + e);
				}
				this.encryption = encryption;
				break;
			default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
			}		
	}

	protected abstract String getEncryptionKey();

	/** Returns the number of bytes that could be read by read().
	 * 
	 * @return
	 */
	protected abstract int hasBytes() throws IOException; 
	
	/** Reads the received bytes into the buffer.
	 * 
	 * @param buffer
	 * @return
	 * @throws IOException
	 */
	protected abstract int read(byte[] buffer) throws IOException;

	/** Reads the next byte.
	 * 
	 * @param buffer
	 * @return
	 * @throws IOException
	 */
	protected abstract byte read() throws IOException;
	
	/** Writes the bytes out.
	 * 
	 * @param buffer
	 * @throws IOException
	 */
	protected abstract void write(byte[] buffer) throws IOException;
	
	/** Is used to supply the master's supported encodings to the device.
	 * 
	 * @return
	 */
	protected abstract Set<Charset> getMastersSupportedCharsets();
	
	/** Is used to supply the master's name to the device.
	 * 
	 * @return
	 */
	protected abstract String getMasterName();
	
	// Returns true if there are enough bytes for a block to be read and decoded
	private boolean has_block() throws IOException {
		switch (encryption) {
		case AES: return hasBytes() >= AES_BLOCKSIZE; 
		default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
		}
	}
	
	// read and decrypt an encrypted block from the wire
	private int read_block(byte[] buffer) throws IOException, DeviceException {
		try {
			switch (encryption) {
			case AES:
				byte[] bytes = new byte[AES_BLOCKSIZE];
				// read the block
				for (int i = 0; i < AES_BLOCKSIZE; i++)
					bytes[i] = read();

				// decrypt the block
				cipher.init(Cipher.DECRYPT_MODE, skeySpec);
				byte[] out = cipher.doFinal(bytes);
				System.arraycopy(out, 0, buffer, 0, AES_BLOCKSIZE);
				return AES_BLOCKSIZE;
			default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
			}
		} catch (BadPaddingException e) {
			throw new DeviceException("Error decrypting block: " + e);
		} catch (InvalidKeyException e) {
			throw new DeviceException("Error decrypting block: " + e);
		} catch (IllegalBlockSizeException e) {
			throw new DeviceException("Error decrypting block: " + e);
		}		
	}
	
	private void write_blocks(byte[] bytes) throws IOException, DeviceException {
		try {
			switch (encryption) {
			case AES:
				cipher.init(Cipher.ENCRYPT_MODE, skeySpec);
				byte[] block = new byte[AES_BLOCKSIZE];
				
				int b = 0;
				while (bytes.length > b * AES_BLOCKSIZE) {
					for (int i = 0; i < AES_BLOCKSIZE; i++) {
						// source position
						int pos = b * AES_BLOCKSIZE + i;
						if (pos < bytes.length)
							block[i] = bytes[pos];
						else
							// pad with random byte; terminator may not occur
							do { 
								block[i] = (byte)random.nextInt(); 
							} while (block[i] == Message.TERMINATOR);
							
					}
					
					// encrypt the block
					byte[] toSend = cipher.doFinal(block);
					write(toSend);
					// next block
					b++;
				}
				break;
			default: throw new IllegalArgumentException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + encryption);
			}
		} catch (BadPaddingException e) {
			throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_BLOCK) + e);
		} catch (InvalidKeyException e) {
			throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_BLOCK) + e);
		} catch (IllegalBlockSizeException e) {
			throw new DeviceException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_ERROR_BLOCK) + e);
		}
	}
	
	
	/** Closes allocated resources. May fail silently.
	 */
	protected void close() {
		// signal to stop
		stopProcessing();
	}

	/** Sends a message immediately.
	 * 
	 * @param message
	 * @throws IOException
	 * @throws DeviceException 
	 */
	protected void sendSynchronous(Message message) throws IOException, DeviceException, MessageException {
		byte[] bytes = message.encode(getEncoding()); 
    	// write the bytes
		if (encryption == null)
			write(bytes);
		else
			write_blocks(bytes);
		
		lastSendTimeMS = System.currentTimeMillis();
    	logDebug("Message sent: " + message.toString());
	}	

	/** Waits for a message until the timeout expires, the operation is aborted or a valid message
	 * is actually received.
	 * @param timeout
	 * @param abortable
	 * @return
	 * @throws IOException 
	 * @throws InterruptedException 
	 * @throws AbortedException 
	 * @throws TimeoutException 
	 * @throws DeviceException 
	 */
	protected Message receiveHandshakeMessage(int timeout, IAbortable abortable) throws IOException, InterruptedException, TimeoutException, DeviceException {
		byte[] message = new byte[getBufferSize()];
		byte[] buffer = new byte[getBufferSize()];
        int bytesReceived = 0;
		int counter = 0;
		while (counter++ < timeout && (abortable == null || !abortable.isAborted())) {
        	if ((encryption == null ? hasBytes() > 0 : has_block())) {
        		int bytes = (encryption == null ? read(buffer) : read_block(buffer));
        		// need to increase buffer?
        		if (bytesReceived + bytes > message.length) {
        			byte[] msg = message;
        			message = new byte[message.length + getBufferSize()];
        			System.arraycopy(msg, 0, message, 0, bytesReceived);
        		}
        		// append received bytes to buffer
        		int terminatorPos = -1;
        		for (int i = 0; i < bytes; bytesReceived++, i++) {
        			message[bytesReceived] = buffer[i];
        			if (buffer[i] == Message.TERMINATOR)
        				terminatorPos = bytesReceived;
        		}
        		
        		// at least one complete message?
        		while (terminatorPos > -1) {
        			int msgLen = terminatorPos;
        			// copy current part
        			byte[] part = new byte[msgLen];
        			System.arraycopy(message, 0, part, 0, msgLen);
                	// try to decode the message
                	Message msg;
					try {
						msg = Message.decode(part, getEncoding());
                		logDebug("Message received: " + msg.toString());
                		// this discards remaining bytes in buffer but this shouldn't be a problem 
                		// because we're expecting handshake messages which are always synchronous
                		return msg;
					} catch (MessageException e) {
                		logDebug("Invalid message: " + e.getMessage());
                		logDebug("Invalid message content: " + (bytesReceived == 0 ? "<empty>" : new String(part, getEncoding())));
                	}
                	// there may be remaining characters in buffer after the terminator
                	if (buffer.length > terminatorPos + 1) {
                		// start over with the remainder that's in the buffer
                		message = new byte[getBufferSize()];
                		bytesReceived = 0;
                		int i = terminatorPos + 1;
                		terminatorPos = -1;
                		for (; i < bytes; bytesReceived++, i++) {
                			message[bytesReceived] = buffer[i];
                			if (buffer[i] == Message.TERMINATOR)
                				terminatorPos = i;
                		}
                	}
        		}
        	}                		
        	Thread.sleep(1);
        }
		if (abortable != null && abortable.isAborted())
			throw new InterruptedException();
		throw new TimeoutException("Timeout waiting for handshake message");
	}
	
	public boolean hasCredentials() {
		return false;
	}


    private void setDeviceError(String msg) {
    	// close the connection
		close();
		setStatus(DeviceStatus.ERROR, msg);
		notifyError(msg);
	}

    @Override
    public void receivedDebug(String message) {
		synchronized(listeners) {
			// create temporary copy
			IDeviceListener[] ls = (IDeviceListener[]) listeners.toArray(new IDeviceListener[listeners.size()]);
			for (IDeviceListener l: ls)
				l.receivedDebug(this, message);
		}				
    }

	@Override
	public void receivedReconfigure() {
		synchronized(listeners) {
			// create temporary copy
			IDeviceListener[] ls = (IDeviceListener[]) listeners.toArray(new IDeviceListener[listeners.size()]);
			for (IDeviceListener l: ls)
				l.receivedReconfigure(this);
		}				
	}

	@Override
	public void receivedRefresh(String[] portIDs) {
		synchronized(listeners) {
			// create temporary copy
			IDeviceListener[] ls = (IDeviceListener[]) listeners.toArray(new IDeviceListener[listeners.size()]);
			for (IDeviceListener l: ls)
				l.receivedRefresh(this, portIDs);
		}				
	}
	
	@Override
	public void setError(int error, String text) {
		// map error message from int to text
		String message = ResourceFactory.instance.getString(DeviceErrors.getErrorText(error));
		if (text != null)
			message += (message.equals("") ? "" : ": ") + text;
		setDeviceError(message);
	}
}
