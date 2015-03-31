package org.ospdi.opdi.devices;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.nio.charset.UnsupportedCharsetException;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.interfaces.IDeviceListener;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.protocol.AbstractProtocol;
import org.ospdi.opdi.protocol.AbstractProtocol.IAbortable;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.Message;
import org.ospdi.opdi.protocol.MessageException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.protocol.ProtocolFactory;
import org.ospdi.opdi.utils.ResourceFactory;
import org.ospdi.opdi.utils.Strings;


/** Defines a queued device that communicates via input and output streams.
 *  Implements the initial handshake protocol with such a device.
 * @author Leo
 *
 */
public abstract class IODevice extends MessageQueueDevice {
	
	/** Used for supplying the device with authentication credentials during the handshake.
	 * 
	 * @author Leo
	 *
	 */
	public interface ICredentialsCallback {
		/** The callback method must put the user name into the first component and the
		 * password into the second component. if save is true the credentials may be saved.
		 * May return false if the handshake should be cancelled.
		 * @param namePassword
		 */
		public boolean getCredentials(String[] namePassword, Boolean[] save);
	}
	
	@SuppressWarnings("serial")
	class DisagreementException extends Exception {}
	
	@SuppressWarnings("serial")
	class CancelledException extends Exception {}
	
	@SuppressWarnings("serial")
	class AuthenticationException extends Exception {}
	
	private IBasicProtocol protocol;
	private ConnectThread connThread;
	
	private String user;
	private String password;
	private int flags = 0;
	private String deviceSuppliedName;
	
	// default locale of the app is preset
	@SuppressWarnings("serial")
	private List<String> preferredLocales = new ArrayList<String>() {
		{ add(Locale.getDefault().toString()); }
	};
	
	@Override
	public boolean hasCredentials() {
		return user != null && !user.isEmpty();
	}

	public String getUser() {
		return user;
	}

	public void setUser(String user) {
		if (user != null && user.trim().isEmpty())
			this.user = null;
		else
			this.user = user;
	}

	public String getPassword() {
		return password;
	}

	public void setPassword(String password) {
		if (password != null && password.trim().isEmpty())
			this.password = null;
		else
			this.password = password;
	}

	public int getFlags() {
		return flags;
	}

	public void setFlags(int flags) {
		this.flags = flags;
	}

	@Override
	public String getDeviceName() {
		return deviceSuppliedName;
	}

	protected void setDeviceSuppliedName(String deviceSuppliedName) {
		this.deviceSuppliedName = deviceSuppliedName;
	}

	public List<String> getPreferredLocales() {
		return preferredLocales;
	}

	public void setPreferredLocales(List<String> preferredLocales) {
		this.preferredLocales = preferredLocales;
	}
	
	private String getPreferredLocalesString() {
		StringBuilder result = new StringBuilder();
		for (String l: preferredLocales) {
			result.append(l);
			result.append(',');
		}
		if (result.length() > 1)
			// trim last comma
			result.deleteCharAt(result.length() - 1);
		return result.toString();
	}
	
	@Override
	public void connect(IDeviceListener listener) {
		if (isConnected()) return;
		// connection attempt in progress?
		if (isConnecting()) {
			abortConnect();
		}
		
		// all connections begin without encryption
		clearEncryption();
		
		// wrap the listener
		final IDeviceListener outerListener = listener;
		
		// start a thread to obtain a socket
		connThread = new ConnectThread(new IDeviceListener() {
			
			@Override
			public synchronized void connectionOpened(IDevice device) {
				// clear remaining messages
				clearQueues();
				
				// add the listener
				addConnectionListener(outerListener);
				
				// start the messaging thread
				startMessageProcessing(getProtocol());

				// start the protocol
				getProtocol().initiate();
				
				setStatus(DeviceStatus.CONNECTED, null);
			}
			
			@Override
			public synchronized void connectionInitiated(IDevice device) {
				setStatus(DeviceStatus.CONNECTING, null);
				outerListener.connectionInitiated(IODevice.this);
			}
			
			@Override
			public synchronized void connectionAborted(IDevice device) {
				setStatus(DeviceStatus.DISCONNECTED, null);
				outerListener.connectionAborted(IODevice.this);
			}

			@Override
			public synchronized void connectionFailed(IDevice device, String message) {
				setStatus(DeviceStatus.DISCONNECTED, message);
				outerListener.connectionFailed(device, message);
			}

			@Override
			public void connectionTerminating(IDevice device) {
				setStatus(DeviceStatus.DISCONNECTING, null);
				outerListener.connectionTerminating(device);
			}
			
			@Override
			public void connectionClosed(IDevice device) {
				setStatus(DeviceStatus.DISCONNECTED, null);
				outerListener.connectionClosed(device);
			}

			@Override
			public void connectionError(IDevice device, String message) {
				setStatus(DeviceStatus.ERROR, message);
				outerListener.connectionError(device, message);
			}
			
			@Override
			public boolean getCredentials(IDevice device, String[] namePassword, Boolean[] save) {
				return outerListener.getCredentials(device, namePassword, save);
			}
			
			@Override
			public void receivedDebug(IDevice device, String message) {
				outerListener.receivedDebug(device, message);
			}
			
			@Override
			public void receivedReconfigure(IDevice device) {
				outerListener.receivedReconfigure(device);
			}
			
			@Override
			public void receivedRefresh(IDevice device, String[] portIDs) {
				outerListener.receivedRefresh(device, portIDs);
			}
			
			@Override
			public void receivedError(IDevice device, String text) {
				outerListener.receivedError(device, text);
			}
		});
		connThread.start();
	}
	
	public void abortConnect() {
		// not yet connected?
		if (connThread != null && !connThread.done) {
			connThread.abort();
		} else
			close();
	}
	
	@Override
	public void disconnect(boolean onError) {
		getProtocol().disconnect();
		super.disconnect(onError);
	}
	
	public synchronized IBasicProtocol getProtocol() {
		return protocol;
	}
	
	@Override
	protected int hasBytes() throws IOException {
		synchronized(getInputStream()) {
			return getInputStream().available();
		}
	}

	@Override
	protected int read(byte[] buffer) throws IOException {
		synchronized(getInputStream()) {
			return getInputStream().read(buffer);
		}
	}
	
	@Override
	protected byte read() throws IOException {
		return (byte)getInputStream().read();
	}

	@Override
	protected void write(byte[] buffer) throws IOException {
		synchronized(getOutputStream()) {
			getOutputStream().write(buffer);
			getOutputStream().flush();
		}
	}
	
	private String[] getHandshakeMessage(int partCount) throws IOException, InterruptedException, TimeoutException, DisconnectedException, DeviceException, ProtocolException {
		Message m = receiveHandshakeMessage(AbstractProtocol.HANDSHAKE_TIMEOUT, new IAbortable() {
			@Override
			public boolean isAborted() {
				return !isConnecting();
			}
		});
		if (!isConnecting()) return null;

		if (m.getChannel() != 0)
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.INVALID_HANDSHAKE_MESSAGE));
		
		// split the message
		String[] parts = Strings.split(m.getPayload(), AbstractProtocol.SEPARATOR);

		// devices may disconnect or send error messages at any time
		if (parts[0].equals(AbstractProtocol.DISCONNECT))
			throw new DisconnectedException();
		else if (parts[0].equals(AbstractProtocol.ERROR))
			throw new DeviceException((parts.length > 1 ? parts[1] : ""));
		else if (parts[0].equals(AbstractProtocol.DISAGREEMENT)) {
			String msg = ("" + (parts.length > 1 ? parts[1] : "") + (parts.length > 2 ? parts[2] : "")).trim();
			if (msg.length() > 1)
				throw new ProtocolException(MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.DEVICE_CANCELLED_BECAUSE), msg));
			else
				throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.DEVICE_CANCELLED));
		}

		if (parts.length != partCount)
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.INVALID_HANDSHAKE_MESSAGE));
		
		return parts;
	}
	
	private void expectAgreement() throws IOException, InterruptedException, TimeoutException, DisconnectedException, DeviceException, ProtocolException, DisagreementException {
		// expect an agreement
		final int REPLY = 0;
		String[] reply = getHandshakeMessage(1);
		if (reply[REPLY].equals(AbstractProtocol.DISAGREEMENT))
			throw new DisagreementException();
		if (!reply[REPLY].equals(AbstractProtocol.AGREEMENT))
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.AGREEMENT_EXPECTED) + reply[REPLY]);				
	}
	
	/** Returns the protocol implementation that is to be used for this device.
	 * Implements the standard handshake and uses the ProtocolFactory.
	 * Uses synchronous communication with the device.
	 * 
	 * @return
	 * @throws TimeoutException 
	 * @throws AbortedException 
	 * @throws InterruptedException 
	 * @throws IOException 
	 * @throws ProtocolException 
	 * @throws DisconnectedException 
	 * @throws DeviceException 
	 * @throws CancelledException 
	 * @throws AuthenticationException 
	 * @throws MessageException 
	 */
	protected IBasicProtocol handshake(ICredentialsCallback credCallback) throws IOException, InterruptedException, TimeoutException, ProtocolException, DisconnectedException, DeviceException, CancelledException, AuthenticationException, MessageException {

		String supportedEncryptions = (this.tryToUseEncryption() ? Strings.join(',', Encryption.AES.toString()) : "");
		// send handshake message
		Message handshake = new Message(0, Strings.join(AbstractProtocol.SEPARATOR, AbstractProtocol.HANDSHAKE, AbstractProtocol.HANDSHAKE_VERSION, flags, supportedEncryptions));
		
		////////////////////////////////////////////////////////////
		///// Send: Handshake
		////////////////////////////////////////////////////////////
		
		sendSynchronous(handshake);
		
		////////////////////////////////////////////////////////////
		///// Receive: Handshake Reply
		////////////////////////////////////////////////////////////
		
		final int HSHAKE = 0;
		final int VERSION = 1;
		final int ENCODING = 2;
		final int ENCRYPTION = 3;
		final int FLAGS = 4;
		final int PROTOCOLS = 5;
		final int PART_COUNT = 6;

		String[] parts = getHandshakeMessage(PART_COUNT);
		
		if (!parts[HSHAKE].equals(AbstractProtocol.HANDSHAKE)) {
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.UNEXPECTED_HANDSHAKE) + AbstractProtocol.HANDSHAKE);
		}
				
		// check version
		if (AbstractProtocol.HANDSHAKE_VERSION_DOUBLE < Strings.parseDouble(parts[VERSION], ResourceFactory.instance.getString(ResourceFactory.HANDSHAKE_VERSION), Double.MIN_VALUE, Double.MAX_VALUE)) {
			// send error message to the device
			sendSynchronous(new Message(0, AbstractProtocol.DISAGREEMENT));
			throw new ProtocolException(MessageFormat.format(ResourceFactory.instance.getString(ResourceFactory.HANDSHAKE_VERSION_NOT_SUPPORTED), parts[VERSION]));
		}

		// encoding name supplied?
		if (!parts[ENCODING].isEmpty()) {
			// find encoding
			try {
				Charset encoding = Charset.forName(parts[ENCODING]);
				if (!getMastersSupportedCharsets().contains(encoding))
					throw new UnsupportedCharsetException(parts[ENCODING]);
				
				// encoding is valid
				// encoding to use for future messages
				setEncoding(encoding);
			} catch (Exception e) {				
				// send error message to the device
				sendSynchronous(new Message(0, AbstractProtocol.DISAGREEMENT));
				
				throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.ENCODING_NOT_SUPPORTED) + parts[ENCODING]);
			}
		}
		
		// encryption specified?
		if (!parts[ENCRYPTION].isEmpty()) {
			// find encryption
			try {
				Encryption encryption = Encryption.valueOf(parts[ENCRYPTION]);
				// encryption to use for future messages
				setEncryption(encryption);
			} catch (DeviceException e) {
				throw e;			
			} catch (Exception e) {
				throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_NOT_SUPPORTED) + parts[ENCRYPTION]);
			}
		}
		
		int deviceFlags = 0;
		try {
			deviceFlags = Strings.parseInt(parts[FLAGS], "flags", Integer.MIN_VALUE, Integer.MAX_VALUE);
		} catch (Exception e) {
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.FLAGS_INVALID) + parts[FLAGS]);
		}
		
		// check flags
		if ((flags & FLAG_ENCRYPTION_REQUIRED) == FLAG_ENCRYPTION_REQUIRED) {
			if (getEncryption() == null)
				// the device did not specify an encryption method
				throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_REQUIRED));
		}
		if ((flags & FLAG_ENCRYPTION_NOT_ALLOWED) == FLAG_ENCRYPTION_NOT_ALLOWED) {
			if (getEncryption() != null)
				// the device did specify an encryption method but it's not allowed
				throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.ENCRYPTION_DISALLOWED));
		}
		
		// determine the protocol
		String[] protos = Strings.split(parts[PROTOCOLS], ',');
		
		IBasicProtocol prot = null;		
		// try each protocol indicator (preferred protocols should come first)
		for (String proto: protos) {
			IBasicProtocol protocol = ProtocolFactory.getProtocol(this, proto);
			if (protocol != null) {
				prot = protocol;
				break;
			}
		}
		
		// protocol found?
		if (prot == null) {
			// no match for supported protocols found
			sendSynchronous(new Message(0, Strings.join(AbstractProtocol.SEPARATOR, AbstractProtocol.DISAGREEMENT)));
			// no possible protocols found
			throw new ProtocolException(ResourceFactory.instance.getString(ResourceFactory.NO_SUPPORTED_PROTOCOL));
		}

		////////////////////////////////////////////////////////////
		///// Send: Protocol Select
		////////////////////////////////////////////////////////////
		
		String preferredLanguages = getPreferredLocalesString();
		sendSynchronous(new Message(0, Strings.join(AbstractProtocol.SEPARATOR, prot.getMagic(), preferredLanguages, getMasterName())));
		
		////////////////////////////////////////////////////////////
		///// Receive: Slave Name
		////////////////////////////////////////////////////////////
		
		final int DEVICE_NAME = 1;

		if (getEncryption() != null) {
			// If this particular message fails there may be something wrong with the encryption method.
			// Possibly the master's key is wrong or an encryption error has been detected on the device.
			// We can't receive any error messages from the device though, because it can't send something we
			// understand because encryption is broken. Therefore we handle a timeout specially here.
			try 
			{
				parts = getHandshakeMessage(2);
			}
			catch (TimeoutException te) {
				throw new DeviceException("Timeout received. Maybe there is something wrong with the encryption key?");
			}
		}
		else
			parts = getHandshakeMessage(2);
		
		if (!parts[0].equals(AbstractProtocol.AGREEMENT)) {
			return null;
		}
		
		// set the name of the device
		setDeviceSuppliedName(parts[DEVICE_NAME]);
		
		// authentication required?
		if ((deviceFlags & FLAG_AUTHENTICATION_REQUIRED) == FLAG_AUTHENTICATION_REQUIRED) {
			
			String user = getUser();
			String password = getPassword();
			// assume that password should be saved if username and password is there
			Boolean[] save = new Boolean[] { Boolean.valueOf(user != null && password != null)};
			// incomplete credentials?
			if (user == null || password == null) {
				// get credentials from callback
				String[] namePassword = new String[] { user, password };
				if (!credCallback.getCredentials(namePassword, save)) {
					// cancelled
					throw new CancelledException();
				}
				
				user = namePassword[0];
				password = namePassword[1];				
			}
			
			////////////////////////////////////////////////////////////
			///// Send: Authentication
			////////////////////////////////////////////////////////////
			
			Message auth = new Message(0, Strings.join(AbstractProtocol.SEPARATOR, AbstractProtocol.AUTHENTICATE, user, password));
			sendSynchronous(auth);

			try {

				////////////////////////////////////////////////////////////
				///// Receive: Confirmation
				////////////////////////////////////////////////////////////
				
				expectAgreement();
			} catch (DisagreementException e) {
				// on error clear password
				setPassword(null);
				
				sendSynchronous(new Message(0, AbstractProtocol.DISCONNECT));
				throw new AuthenticationException();
			}
			
			// authentication ok; should we save?
			if (save[0]) {
				setUser(user);
				setPassword(password);
			}
		}

		////////////////////////////////////////////////////////////
		///// Handshake completed
		////////////////////////////////////////////////////////////
		
		return prot;
	}

	/** Attempt to connect to the device.
	 * 
	 * @throws IOException
	 */
	protected abstract void tryConnect() throws IOException;

	/** Returns the input stream for the device.
	 * 
	 * @return
	 * @throws IOException
	 */
	protected abstract InputStream getInputStream() throws IOException;

	/** Returns the output stream for the device.
	 * 
	 * @return
	 * @throws IOException
	 */
	protected abstract OutputStream getOutputStream() throws IOException;

	/**
     * This thread runs while attempting to make an outgoing connection
     * with a device. It runs straight through; the connection either
     * succeeds or fails.
     * If a connection attempt succeeds, it calls the handshake() method
     * which has to negotiate and return the protocol that is to be
     * used for this device.
     */
	class ConnectThread extends Thread {
		
		IDeviceListener csListener;
		boolean aborted;
        boolean done;
		
		ConnectThread(IDeviceListener csListener) {
			this.csListener = csListener;
		}
		
		@Override
		public void run() {
			try {
				csListener.connectionInitiated(IODevice.this);
				
				// Attempt to connect to the device
				try {
					tryConnect();

					if (aborted) {
						close();
						return;
					}
					
					// get the protocol
					protocol = handshake(new ICredentialsCallback() {
						// wrap the listener into an ICredentialsCallback
						@Override
						public boolean getCredentials(String[] namePassword, Boolean[] save) {
							return csListener.getCredentials(IODevice.this, namePassword, save);
						}
					});
					
					if (protocol == null)
						throw new DisagreementException();
					
				} catch (IOException e) {
					close();
				    if (aborted) return;
					csListener.connectionFailed(IODevice.this, ResourceFactory.instance.getString(ResourceFactory.IO_FAILURE) + ": " + e.getMessage());
					return;
				} catch (InterruptedException e) {
					close();
				    if (aborted) return;
					csListener.connectionFailed(IODevice.this, ResourceFactory.instance.getString(ResourceFactory.OPERATION_WAS_ABORTED));
					Thread.currentThread().interrupt();
					return;
				} catch (TimeoutException e) {
					close();
				    if (aborted) return;
					csListener.connectionFailed(IODevice.this, ResourceFactory.instance.getString(ResourceFactory.DEVICE_DID_NOT_RESPOND));
					return;
				} catch (ProtocolException e) {
					// abort the connection
					try {
						sendSynchronous(new Message(0, AbstractProtocol.DISCONNECT));
					} catch (Exception e1) {
					}
					close();
				    if (aborted) return;
					csListener.connectionFailed(IODevice.this, ResourceFactory.instance.getString(ResourceFactory.PROTOCOL_ERROR) + (e.getMessage() != null ? ": " + e.getMessage() : ""));
					return;
				} catch (DisconnectedException e) {
					close();
				    if (aborted) return;
					csListener.connectionFailed(IODevice.this, ResourceFactory.instance.getString(ResourceFactory.DEVICE_CLOSED_CONNECTION));
					return;
				} catch (DeviceException e) {
					close();
				    if (aborted) return;
					csListener.connectionError(IODevice.this, e.getMessage());
					return;
				} catch (AuthenticationException e) {
					close();
				    if (aborted) return;
					csListener.connectionFailed(IODevice.this, ResourceFactory.instance.getString(ResourceFactory.AUTHENTICATION_FAILED));
				    return;				
				} catch (CancelledException e) {
					close();
				    if (aborted) return;
					csListener.connectionAborted(IODevice.this);
				    return;				
				} catch (Exception e) {
					close();
				    if (aborted) return;
					csListener.connectionFailed(IODevice.this, e.toString());
				    return;
				}
				
				setStatus(DeviceStatus.CONNECTED);
				csListener.connectionOpened(IODevice.this);
			} finally {
				done = true;
			}
		}
		
		void abort() {
			aborted = true;
        	csListener.connectionAborted(IODevice.this);			
		}
	}
}
