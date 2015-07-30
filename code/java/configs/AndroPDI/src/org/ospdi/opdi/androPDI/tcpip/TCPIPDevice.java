package org.ospdi.opdi.androPDI.tcpip;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.charset.Charset;
import java.text.MessageFormat;
import java.util.Set;

import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.androPDI.AndroPDIDevice;
import org.ospdi.opdi.androPDI.utils.ResourceFactory;
import org.ospdi.opdi.utils.Strings;
import org.ospdi.opdi.androPDI.R;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;
import android.widget.Toast;

/** Implements IDevice for a TCP/IP device.
 * 
 * @author Leo
 *
 */
public class TCPIPDevice extends AndroPDIDevice {
	
	protected final static String MAGIC = "tcpip://";
	protected final static char SEPARATOR = ':';
	
	protected final static int TIMEOUT = 10000;	// milliseconds
	
	public static int STANDARD_PORT = 13110;
	
	protected String name;
	protected String address;
	protected String psk;
	protected int port = STANDARD_PORT;
	
	Socket socket;
	protected InputStream inputStream;
	protected OutputStream outputStream;
	
	/** Deserializing constructor */
	public TCPIPDevice(String ser) {
		// deserialize information
		if (!ser.startsWith(MAGIC))
			throw new IllegalArgumentException("Can't deserialize; magic is incorrect");
		
//		Log.d(AndroPDI.MASTER_NAME, "deserialize: " + ser);
		
		// serialized form is:
		// tcpip://user:password:name:host:port
		final int USER = 0;
		final int PASSWORD = 1;
		final int NAME = 2;
		final int HOST = 3;
		final int PORT = 4;
		final int ENCRYPTION_KEY = 5;
		final int PARTS_COUNT = 6;
		
		// evaluate the rest of the address
		String addr = ser.substring(MAGIC.length());
		String[] parts = Strings.split(addr, SEPARATOR);
		
		if (parts.length != PARTS_COUNT)
			throw new IllegalArgumentException("Invalid serial form");
		setUser(parts[USER].trim());
		setPassword(parts[PASSWORD].trim());
		name = parts[NAME];
		address = parts[HOST].trim();
		// default address fallback
		if (address.isEmpty())
			address = "localhost";
		try {
			port = Integer.parseInt(parts[PORT]);
		} catch (NumberFormatException nfe) {
			throw new IllegalArgumentException("Illegal port number: " + parts[1]);
		}
		if (port < 1 || port > 65535)
			throw new IllegalArgumentException("Port number not in valid range: " + port);
		String ek = parts[ENCRYPTION_KEY];
		if (!ek.isEmpty())
			psk = ek;
	}
	
	/** Standard constructor */
	public TCPIPDevice(String name, String address, int port, String psk) {
		this.name = name;
		this.address = address.trim();
		this.port = port;
		this.psk = psk;
	}
	
	@Override
	public String serialize() {
		return MAGIC + Strings.join(SEPARATOR, getUser(), getPassword(), name, address, port, psk);
	}

	protected boolean isNetworkAvailable() {
	    ConnectivityManager connectivityManager 
	          = (ConnectivityManager) AndroPDI.instance.getSystemService(AndroPDI.CONNECTIVITY_SERVICE);
	    NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
	    return activeNetworkInfo != null && activeNetworkInfo.isConnected();
	}
	
	@Override
	public boolean prepare() {
		// check whether network connection is available
		if (!isNetworkAvailable()) {
			AndroPDI.instance.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					Toast.makeText(AndroPDI.instance, ResourceFactory.getInstance().getString(R.string.network_not_available), Toast.LENGTH_SHORT).show();
				}
			});
			return false;
		}
		
		// if there is an encryption
		
		return true;
	}

	@Override
	public boolean isSupported() {
		// TCPIP devices are always supported
		return true;
	}

	@Override
	public String getName() {
		return name;
	}

	@Override
	public String getLabel() {
		if (name == null || name.equals(""))
			return address + ":" + port;

		return name;
	}

	@Override
	public String getAddress() {
		return "tcpip://" + address + ":" + port;
	}
		
	@Override
	protected String getMasterName() {
		return AndroPDI.MASTER_NAME;
	}
	
	@Override
	protected Set<Charset> getMastersSupportedCharsets() {
		return AndroPDI.SUPPORTED_CHARSETS;
	}
	
	@Override
	public void logDebug(String message) {
		Log.d(AndroPDI.MASTER_NAME, message);
	}

	public String getTCPIPAddress() {
		return address;
	}

	public int getPort() {
		return port;
	}
	
	@Override
	protected String getEncryptionKey() {
		return psk;
	}

	@Override
	public String getDisplayAddress() {
		// TCP/IP devices may have only an address, but also a name which is different from the device name
		return getAddress() + (name != null && !name.isEmpty() && !name.equals(getDeviceName()) ? " (" + name + ")" : "");
	}

	@Override
	public int getImageResource() {
		return R.drawable.tcpip;
	}	

    @Override
	protected void tryConnect() throws IOException {    	
    	socket = null;
    	
    	// make a socket
		SocketAddress addr = new InetSocketAddress(address, port);
		Socket tmp = new Socket();
		tmp.setTcpNoDelay(true);

		// construct the socket
	    tmp.connect(addr, TIMEOUT);
	    
	    // everything ok by now, remember the socket
	    socket = tmp;
	}

	@Override
	protected InputStream getInputStream() throws IOException {
		if (inputStream == null)
			inputStream = socket.getInputStream();
		return inputStream;
	}

	@Override
	protected OutputStream getOutputStream() throws IOException {
		if (outputStream == null)
			outputStream = socket.getOutputStream();
		return outputStream;
	}
	
	@Override
	protected void close() {
		super.close();
		try {
			if (socket != null && !socket.isClosed())
				socket.close();
		} catch (IOException e) {
			// fail silently
		}
		inputStream = null;
		outputStream = null;
	}
	
	@Override
	public String getConnectionMessage(boolean noConfirmation) {
		if (noConfirmation)
			return null;
		return MessageFormat.format(ResourceFactory.getInstance().getString(R.string.really_connect_device), getName());
	}

	@Override
	public boolean tryToUseEncryption() {
		// encryption may be used if a pre-shared key has been specified
		return psk != null && !psk.isEmpty();
	}
}