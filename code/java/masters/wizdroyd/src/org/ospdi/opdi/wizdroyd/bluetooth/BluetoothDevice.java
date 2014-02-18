package org.ospdi.opdi.wizdroyd.bluetooth;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.text.MessageFormat;
import java.util.Set;
import java.util.UUID;

import org.ospdi.opdi.utils.Strings;
import org.ospdi.opdi.wizdroyd.R;
import org.ospdi.opdi.wizdroyd.Wizdroyd;
import org.ospdi.opdi.wizdroyd.WizdroydDevice;
import org.ospdi.opdi.wizdroyd.utils.ResourceFactory;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.util.Log;
import android.widget.Toast;

/** Implements IDevice for a Bluetooth device.
 * 
 * @author Leo
 *
 */
public class BluetoothDevice extends WizdroydDevice {
	
	private final static String MAGIC = "bt://";
	private final static char SEPARATOR = ';';
	
	private BluetoothAdapter bluetoothAdapter;
	private String name;
	private String address;
	private String pin;
	private String psk;
	private boolean secure;
	
	BluetoothSocket socket;
	private InputStream inputStream;
	private OutputStream outputStream;
	
	
	/** Deserializing constructor */
	public BluetoothDevice(String ser) {		
		if (!ser.startsWith(MAGIC))
			throw new IllegalArgumentException("Can't deserialize; magic is incorrect");
		
		// use default adapter
		bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
		
		// serialized form is:
		// bt://user;password;name;address;secure;encoding;flags;encryption;key
		
		final int USER = 0;
		final int PASSWORD = 1;
		final int NAME = 2;
		final int ADDRESS = 3;
		final int SECURE = 4;
		final int RESERVED = 5;
		final int FLAGS = 6;
//		final int ENCRYPTION_METHOD = 7;
		final int ENCRYPTION_KEY = 8;
		final int PIN = 9;
		final int PARTS_COUNT = 10;
		
		// evaluate the rest of the address
		String addr = ser.substring(MAGIC.length());
		String[] parts = Strings.split(addr, SEPARATOR);
		if (parts.length != PARTS_COUNT)
			throw new IllegalArgumentException("Can't deserialize; invalid part count");
		name = parts[NAME];
		setUser(parts[USER]);
		setPassword(parts[PASSWORD]);
		address = parts[ADDRESS];
		secure = Boolean.parseBoolean(parts[SECURE]);
		setFlags(Strings.parseInt(parts[FLAGS], "flags", Integer.MIN_VALUE, Integer.MAX_VALUE));
//		String em = parts[ENCRYPTION_METHOD];
//		if (!em.isEmpty())
//			setEncryption(Encryption.valueOf(em));
		String ek = parts[ENCRYPTION_KEY];
		if (!ek.isEmpty())
			psk = ek;
		pin = parts[PIN];
	}
	
	/** Standard constructor */
	public BluetoothDevice(BluetoothAdapter bluetoothAdapter, String name, String address, String psk, String pin, boolean secure) {
		this.bluetoothAdapter = bluetoothAdapter;
		this.name = name;
		if (this.name != null) this.name = this.name.trim();
		this.address = address.trim();
		this.psk = psk;
		this.pin = pin;
		this.secure = secure;
	}
	
	@Override
	public String serialize() {
		return MAGIC + Strings.join(SEPARATOR, getUser(), getPassword(), name, address, secure, null /*reserved*/, 
				getFlags(), getEncryption() == null ? "" : getEncryption(),
				getEncryptionKey() == null ? "" : getEncryptionKey(), pin);
	}

	@Override
	public boolean isSupported() {
		// Bluetooth devices are supported if a Bluetooth adapter is present
		return BluetoothAdapter.getDefaultAdapter() != null;
	}

	@Override
	public String getName() {
		// Fall back to address if there is no name
		if (name == null || name.isEmpty())
			return address;
		return name;
	}

	@Override
	public String getLabel() {
		return getName();
	}

	@Override
	public String getAddress() {
		// secure has "bts://" channel identifier
		return "bt" + (secure ? "s" : "") + "://" + address;
	}
	
	@Override
	protected String getMasterName() {
		return Wizdroyd.MASTER_NAME;
	}
	
	@Override
	protected Set<Charset> getMastersSupportedCharsets() {
		return Wizdroyd.SUPPORTED_CHARSETS;
	}
	
	@Override
	public void logDebug(String message) {
		Log.d(Wizdroyd.MASTER_NAME, message);
	}

	public String getBluetoothAddress() {
		return address;
	}

	public String getPIN() {
		return pin;
	}

	public boolean isSecure() {
		return secure;
	}
	
	@Override
	protected String getEncryptionKey() {
		return psk;
	}

	@Override
	public String getDisplayAddress() {
		// Bluetooth devices may have only an address, but also a name which is different from the device name
		return getAddress() + (name != null && !name.isEmpty() && !name.equals(getDeviceName()) ? " (" + name + ")" : "");
	}

	@Override
	public int getImageResource() {
		return R.drawable.bluetooth64;
	}

	@Override
	public boolean prepare() {
		if (bluetoothAdapter == null) {
			Toast.makeText(Wizdroyd.instance.getApplicationContext(), R.string.bluetooth_not_supported, Toast.LENGTH_SHORT).show();
			return false;
		}
		
		// check whether bluetooth is enabled
		if (!bluetoothAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            Wizdroyd.instance.startActivityForResult(enableIntent, AddBluetoothDevice.REQUEST_ENABLE_BT);
            return false;
		}
		
		// check address
		if (!BluetoothAdapter.checkBluetoothAddress(address)) {
			Toast.makeText(Wizdroyd.instance.getApplicationContext(), R.string.bluetooth_invalid_address, Toast.LENGTH_SHORT).show();
			return false;
		}
		
		return true;
	}

    @Override
	protected void tryConnect() throws IOException {    	
    	socket = null;
    	
		// Always cancel discovery because it will slow down a connection
		bluetoothAdapter.cancelDiscovery();
		
		// Get the BluetoothDevice object
		android.bluetooth.BluetoothDevice mmDevice = bluetoothAdapter.getRemoteDevice(address);
		
		// Attempt to connect to the device
		BluetoothSocket tmp = null;

//	    // Unique UUID for this application
	    final UUID MY_UUID_SECURE =
	        UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
	    final UUID MY_UUID_INSECURE =
	        UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
		
		// Get a BluetoothSocket for a connection with the
		// given BluetoothDevice		
		try {
		    if (secure) {
		    	//Method m;
				//m = mmDevice.getClass().getMethod("createRfcommSocket", new Class[] {int.class});
		        //tmp = (BluetoothSocket) m.invoke(mmDevice, 1);
		        tmp = mmDevice.createRfcommSocketToServiceRecord(MY_UUID_SECURE);
		    } else {
		    	//Method m = mmDevice.getClass().getMethod("createInsecureRfcommSocket", new Class[] {int.class});
		        //tmp = (BluetoothSocket) m.invoke(mmDevice, 1);
				tmp = mmDevice.createInsecureRfcommSocketToServiceRecord(MY_UUID_INSECURE);
		    }
		} catch (SecurityException e) {
			throw new IOException("SecurityException", e);
//		} catch (NoSuchMethodException e) {
//			throw new IOException("NoSuchMethodException", e);
//		} catch (IllegalAccessException e) {
//			throw new IOException("IllegalAccessException", e);
//		} catch (InvocationTargetException e) {
//			throw new IOException("Error connecting to device", e);
		}
		    
		tmp.connect();
	    
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
			if (socket != null)
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
		if (pin == null || pin.equals(""))
			return MessageFormat.format(ResourceFactory.getInstance().getString(R.string.really_connect_device), getName());
		else
			return MessageFormat.format(ResourceFactory.getInstance().getString(R.string.really_connect_bluetooth_pin), getName(), pin);
	}


	@Override
	public boolean tryToUseEncryption() {
		// encryption may be used if a pre-shared key has been specified
		return psk != null && !psk.isEmpty();
	}
}

