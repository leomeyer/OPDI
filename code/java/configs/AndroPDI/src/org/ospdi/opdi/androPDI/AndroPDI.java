package org.ospdi.opdi.androPDI;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.nio.charset.Charset;
import java.text.MessageFormat;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Map;

import jim.h.common.android.zxinglib.integrator.IntentIntegrator;
import jim.h.common.android.zxinglib.integrator.IntentResult;

import org.ospdi.opdi.androPDI.bluetooth.AddBluetoothDevice;
import org.ospdi.opdi.androPDI.bluetooth.BluetoothDevice;
import org.ospdi.opdi.androPDI.bluetooth.EditBluetoothDevice;
import org.ospdi.opdi.androPDI.gui.LoggingActivity;
import org.ospdi.opdi.androPDI.ports.ShowDevicePorts;
import org.ospdi.opdi.androPDI.tcpip.AddTCPIPDevice;
import org.ospdi.opdi.androPDI.tcpip.EditTCPIPDevice;
import org.ospdi.opdi.androPDI.tcpip.TCPIPDevice;
import org.ospdi.opdi.androPDI.utils.ResourceFactory;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.interfaces.IDevice.DeviceStatus;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/** This is the main activity for the AndroPDI client.
 * 
 * @author Leo
 *
 */
public class AndroPDI extends LoggingActivity implements DeviceManager.IDeviceStatusListener {
	
	/** This client's name as it is sent to slave devices. Also, the Log tag. */
	public static final String MASTER_NAME = "AndroPDI";
	/** This client's version number. */
	public static final String VERSION = "0.1";	// must be a number
	/** This client's version number as double. */
	public static final double VERSION_DOUBLE = Double.parseDouble(VERSION);
	
	/** Defines the character encodings that are supported by this client. */
	@SuppressWarnings("serial")
	public static final HashSet<Charset> SUPPORTED_CHARSETS = new HashSet<Charset>() {
		{ 
			add(Charset.forName("ASCII"));
			add(Charset.forName("ISO-8859-1"));
			add(Charset.forName("ISO-8859-11"));
			add(Charset.forName("UTF8"));		
		}
	};

    public static final int ADD_BLUETOOTH_DEVICE = 1;
    public static final int ADD_TCPIP_DEVICE = 2;
    public static final int EDIT_BLUETOOTH_DEVICE = 3;
    public static final int EDIT_TCPIP_DEVICE = 4;
	
    public static final String DEVICE_PSK = "DEVICE_PSK";

    public static final String BT_ADDRESS = "BT_ADDRESS";
    public static final String BT_NAME = "BT_NAME";
    public static final String BT_PIN = "BT_PIN";
    public static final String BT_SECURE = "BT_SECURE";

    public static final String TCPIP_NAME = "TCPIP_NAME";
    public static final String TCPIP_HOST = "TCPIP_HOST";
    public static final String TCPIP_PORT = "TCPIP_PORT";

    public static final String DEVICE_PREVIOUS_SERIALIZATION = "previous_serialization";
	public static final String DEVICE_SERIALIZATION = "device_serialization";
	public static final String CURRENT_PORT_ID = "current_port_id";
	public static final String CURRENT_DEVICE_ID = "current_device_id";
	private static final String EXTRA_SHORTCUT_DEVICE_ADDRESS = "org.ospdi.opdi.androPDI.shortcut.DEVICENAME";
	
	public static AndroPDI instance;
	
    protected DeviceManager deviceManager;
    private Handler handler = new Handler();
    
//    private WakeLock wakeLock; 		// the WakeLock is acquired while devices are connected
    
	private DeviceListAdapter deviceAdapter;
	private ListView lvDevices;
	private AndroPDIDevice selectedDevice;
    protected AndroPDIDevice shortcutDevice;
	private boolean dialogOk;

    BroadcastReceiver screenReceiver = new BroadcastReceiver() {
    	@Override
    	public void onReceive(Context context, Intent intent) {
    		// disconnect when the screen is turned off
    		if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF))
    			disconnectAllDevices();
    	}
    };

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        instance = this;
        
        // detect when the screen is turned off
        IntentFilter filter = new IntentFilter(Intent.ACTION_SCREEN_OFF);
        registerReceiver(screenReceiver, filter);        

        // use localized resource factory
        org.ospdi.opdi.utils.ResourceFactory.instance = new org.ospdi.opdi.androPDI.utils.ResourceFactory(this);
        
        deviceManager = DeviceManager.getInstance();
        
        // register for connected devices: this instance is now the listener
        deviceManager.registerListener(this);
        
        if (deviceManager.isExternalStorageNotReadable()) {
	        Toast.makeText(this, "External media is not readable nor writable. Settings can neither be read nor saved.", Toast.LENGTH_SHORT).show();            
        } else if (!deviceManager.isExternalStorageWriteable()) {
	        Toast.makeText(this, "External media is not writable. Settings cannot be saved.", Toast.LENGTH_SHORT).show();            
        }

        // the device adapter adapts the loaded devices for displaying
        deviceAdapter = new DeviceListAdapter(this, android.R.layout.simple_list_item_1, deviceManager.getDevices());
        
        // setup devices list view
        lvDevices = (ListView)findViewById(R.id.lvDevices);
        lvDevices.setAdapter(deviceAdapter);
        // set device click listener
        lvDevices.setOnItemClickListener(new OnItemClickListener() {
        	@Override
        	public void onItemClick(AdapterView<?> av, View view, int position, long id) {
        		// wrong device index
        		if (position >= deviceManager.getDevices().size()) return;
        		// special management list item?
        		if (deviceManager.getDevices().get(position) == null)
        			// open device management menu
        			openDeviceManagement();
        		else {
        			// device has been selected
    				final AndroPDIDevice selDevice = deviceManager.getDevices().get(position);
        			if (selDevice.isConnected())
        				// show capabilities for connected device
        				showDevicePorts(selDevice);
        			else if (selDevice.getStatus() != DeviceStatus.CONNECTING) {
        				// not connected
        				connectToDevice(selDevice, false);
        			}
        		}
        	}
		});
        // set device long click listener
        lvDevices.setOnItemLongClickListener(new OnItemLongClickListener() {
        	@Override
        	public boolean onItemLongClick(AdapterView<?> av, View view, int position, long id) {
        		// wrong device index
        		if (position >= deviceManager.getDevices().size()) return false;
        		// special management list item?
        		if (deviceManager.getDevices().get(position) == null)
        			// no menu for this item
        			return false;
        		else
        			// device has been selected
        			openDeviceMenu(deviceManager.getDevices().get(position));
        		return true;
        	}
		});
        
        // started from shortcut?
        String shortcutDeviceAddress = getIntent().getStringExtra(EXTRA_SHORTCUT_DEVICE_ADDRESS);
        if (shortcutDeviceAddress != null) {
        	// find the device
        	AndroPDIDevice device = findDeviceByAddress(shortcutDeviceAddress);
        	if (device == null) {
        		Toast.makeText(this, "The device with the shortcut address: " + shortcutDeviceAddress + " could not be found, please remove the shortcut", Toast.LENGTH_SHORT).show();
        		return;
        	}

        	// connect the device and display the device ports
        	selectedDevice = device;
        	shortcutDevice = device;
        	connectSelectedDevice();        	
        }
    }

	private void connectToDevice(AndroPDIDevice selDevice, boolean noConfirmation) {
		
		if (selDevice == null)
			return;

		selectedDevice = selDevice;
		
		// Ask the user if they want to connect
		
		// get connection message from the device
		String message = selDevice.getConnectionMessage(noConfirmation);

		// no message? connect right away
		if (message == null) {
        	AndroPDI.this.selectedDevice = selDevice;
        	AndroPDI.this.connectSelectedDevice();
        }
		else {
			// show message dialog
	        new AlertDialog.Builder(AndroPDI.this)
	        .setIcon(android.R.drawable.ic_dialog_alert)
	        .setTitle(R.string.connect_device)
	        .setMessage(message)
	        .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
	            @Override
	            public void onClick(DialogInterface dialog, int which) {
	            	AndroPDI.this.connectSelectedDevice();
	            }
	        })
	        .setNegativeButton(R.string.no, null)
	        .show();
		}
	}
    
	@Override
	protected void onPause() {
		super.onPause();
		
        try {
			deviceManager.saveSettings();
		} catch (Exception e) {
	        Toast.makeText(this, "The settings could not be saved: " + e.getMessage(), Toast.LENGTH_SHORT).show();            
		}		
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		if (isFinishing())
			disconnectAllDevices();

		unregisterReceiver(screenReceiver);
	}
	
	@Override
	protected void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		
		// outState.putString(LOGTEXT, tvLog.getText().toString());
	}

	protected void showDevicePorts(AndroPDIDevice device) {
		if (device.getStatus() != DeviceStatus.CONNECTED) return;
		
		// Create an intent to start the showDevicePorts activity
		Intent intent = new Intent(this, ShowDevicePorts.class);
		intent.putExtra(CURRENT_DEVICE_ID, device.getId());
		startActivity(intent);
	}

	private void openDeviceMenu(AndroPDIDevice iDevice) {
		selectedDevice = iDevice;
	    registerForContextMenu(lvDevices);
	    openContextMenu(lvDevices);
	    unregisterForContextMenu(lvDevices);
	}

	private void openDeviceManagement() {
		selectedDevice = null;
	    registerForContextMenu(lvDevices);
	    openContextMenu(lvDevices);
	    unregisterForContextMenu(lvDevices);
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		if (!super.onCreateOptionsMenu(menu))
			return false;
				
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.manage_devices_menu, menu);
		inflater.inflate(R.menu.options_menu, menu);
		
		return true;
	}
	
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		MenuInflater inflater = getMenuInflater();
		super.onCreateContextMenu(menu, v, menuInfo);
		  
		if (v == lvDevices) {
			// Management item selected?
			if (selectedDevice == null)
				inflater.inflate(R.menu.manage_devices_menu, menu);
			else {
				// normal device selected
				inflater.inflate(R.menu.device_menu, menu);

				// selectively enable/disable menu items
				MenuItem item;
				
				boolean canConnect = (selectedDevice.getStatus() != DeviceStatus.CONNECTING) && (selectedDevice.getStatus() != DeviceStatus.CONNECTED);
				item = menu.findItem(R.id.menuitem_connect_device);
				item.setVisible(canConnect);
				
				boolean canAbort = (selectedDevice.getStatus() == DeviceStatus.CONNECTING);
				item = menu.findItem(R.id.menuitem_abort_connect_device);
				item.setVisible(canAbort);

				boolean canDisconnect = (selectedDevice.getStatus() == DeviceStatus.CONNECTED);
				item = menu.findItem(R.id.menuitem_disconnect_device);
				item.setVisible(canDisconnect);

				item = menu.findItem(R.id.menuitem_forget_credentials);
				item.setVisible(selectedDevice.hasCredentials());

				// These items are only enabled when the device is not connected or connecting
				item = menu.findItem(R.id.menuitem_edit_device);
				item.setEnabled(canConnect);
				item = menu.findItem(R.id.menuitem_remove_device);
				item.setEnabled(canConnect);
			}
		}
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		return onContextItemSelected(item);
	}
	
	@Override
	public boolean onContextItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menuitem_scan_qr_code:
			scanQRCode();
			return true;
		case R.id.menuitem_add_tcpip_device:
			addTCPIPDevice();
			return true;
		case R.id.menuitem_add_bluetooth_device:
			addBluetoothDevice();
			return true;
		case R.id.menuitem_remove_devices:
			removeAllDevices();
			return true;
		case R.id.menuitem_forget_credentials:
			forgetCredentials();
			return true;
		case R.id.menuitem_edit_device:
			editSelectedDevice();
			return true;
		case R.id.menuitem_remove_device:
			removeSelectedDevice();
			return true;
		case R.id.menuitem_install_device_shortcut:
			installShortcutForSelectedDevice();
			return true;
		case R.id.menuitem_connect_device:
			connectToDevice(selectedDevice, true);
			return true;
		case R.id.menuitem_disconnect_device:
			disconnectSelectedDevice();
			return true;
		case R.id.menuitem_connect_all_devices:
			connectAllDevices();
			return true;
		case R.id.menuitem_disconnect_all_devices:
			disconnectAllDevices();
			return true;
		case R.id.menuitem_abort_connect_device:
			abortConnectSelectedDevice();
			return true;
		case R.id.menuitem_help:
			showHelp();
			return true;
		case R.id.menuitem_quit:
			quitApplication();
			return true;
		default:
			return super.onContextItemSelected(item);
		}
	}

	private void quitApplication() {
		// perform a clean exit
		disconnectAllDevices();
		finish();
	}

	private void showHelp() {
		// Create an intent to start the help activity
		Intent intent = new Intent(this, HelpActivity.class);
		startActivity(intent);
	}

	private void forgetCredentials() {
		if (selectedDevice != null) {
			selectedDevice.setUser(null);
			selectedDevice.setPassword(null);
		}
	}

	private synchronized void disconnectAllDevices() {
		deviceManager.disconnectAllDevices();
	}

	private synchronized void connectAllDevices() {
		deviceManager.connectAllDevices(this);
	}

	private void abortConnectSelectedDevice() {
		if (selectedDevice == null) return;
		
		selectedDevice.abortConnect();		
	}

	private void disconnectSelectedDevice() {
		if (selectedDevice == null) return;
		
		selectedDevice.disconnect(false);
		deviceStatusChanged();
	}
	
	@Override
	public void deviceConnected(AndroPDIDevice device) {
		// shortcut device connected?
		if (shortcutDevice == device) {
			// automatically switch to port view
			showDevicePorts(device);
		}
		
		shortcutDevice = null;
	}

	private void deviceStatusChanged() {
		// Should be called when the status of one or more devices changes.
		// This updates the list and does power related work.
		
		// refresh list
		deviceAdapter.notifyDataSetChanged();
		deviceAdapter.notifyDataSetInvalidated();
		
/*		
		// If any of the devices is currently connected, acquire or keep the WakeLock.
		boolean hasConnection = false;
		for (AndroPDIDevice device: deviceManager.getDevices())
			if (device != null && device.isConnected()) {
				hasConnection = true;
				break;
			}
		// are there active connections?
		if (hasConnection) {
			// wakeLock needs to be created?
			if (wakeLock == null) {
				PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
				wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, "AndroPDI WakeLock");
			}
			// if the wakeLock is not active, acquire it
			if (!wakeLock.isHeld()) {
				wakeLock.acquire();
				// Toast.makeText(this, "WakeLock acquired.", Toast.LENGTH_SHORT).show();			
			}
		} else {
			// no active connections
			// release an acquired wakeLock
			if (wakeLock != null)
				if (wakeLock.isHeld()) {
					wakeLock.release();
					// 	Toast.makeText(this, "WakeLock released.", Toast.LENGTH_SHORT).show();			
				}
		}
*/
	}

	private void connectSelectedDevice() {
		if (selectedDevice == null) return;
		
		// prepare the connection
		if (!selectedDevice.prepare())
			return;
		
		deviceManager.connect(selectedDevice, this);
	}
	
	private void editSelectedDevice() {
		if (selectedDevice == null) return;

		// edit device
        editDevice(selectedDevice);
	}	

	private void removeSelectedDevice() {
		if (selectedDevice == null) return;
		// Ask the user if they are sure
        new AlertDialog.Builder(this)
        .setIcon(android.R.drawable.ic_dialog_alert)
        .setTitle(R.string.remove_a_device)
        .setMessage(MessageFormat.format(ResourceFactory.getInstance().getString(R.string.really_remove_device), selectedDevice.getName()))
        .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            	// delete device
            	removeDevice(selectedDevice);
            }
        })
        .setNegativeButton(R.string.no, null)
        .show();
	}

	private void installShortcutForSelectedDevice() {
		if (selectedDevice == null) return;
		Intent shortcutIntent = new Intent(getApplicationContext(), AndroPDI.class);
	    shortcutIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	    shortcutIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
	    shortcutIntent.putExtra(EXTRA_SHORTCUT_DEVICE_ADDRESS, selectedDevice.getAddress());

	    Intent addIntent = new Intent();
	    addIntent.putExtra("duplicate", false);
	    addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
	    addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, selectedDevice.getLabel());
	    addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE, Intent.ShortcutIconResource.fromContext(getApplicationContext(), R.drawable.device));
	    addIntent.setAction("com.android.launcher.action.UNINSTALL_SHORTCUT");
	    getApplicationContext().sendBroadcast(addIntent);
	    addIntent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
	    getApplicationContext().sendBroadcast(addIntent);
	    
	    Toast.makeText(this, "The shortcut has been created.", Toast.LENGTH_SHORT).show();
	}

	private void addBluetoothDevice() {
		// Create an intent to start the AddBluetoothDevice activity
		Intent intent = new Intent(this, AddBluetoothDevice.class);
		startActivityForResult(intent, ADD_BLUETOOTH_DEVICE);
	}

	private void addTCPIPDevice() {
		// Create an intent to start the AddTCPIPDevice activity
		Intent intent = new Intent(this, AddTCPIPDevice.class);
		startActivityForResult(intent, ADD_TCPIP_DEVICE);
	}
	
	private void scanQRCode() {
        // set the last parameter to true to open front light if available
        IntentIntegrator.initiateScan(AndroPDI.this, R.layout.capture,
                R.id.viewfinder_view, R.id.preview_view, true);
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        
        switch (requestCode) {
        	// result from QR scan?
            case IntentIntegrator.REQUEST_CODE: {
                IntentResult scanResult = IntentIntegrator.parseActivityResult(requestCode,
                        resultCode, data);
                if (scanResult == null) {
                    return;
                }
                final String result = scanResult.getContents();
                if (result != null) {
                	addDeviceFromURI(result);
                }
                break;
            }
            // starting a Bluetooth device caused enable prompt
            case AddBluetoothDevice.REQUEST_ENABLE_BT: {
                // When the request to enable Bluetooth returns
                if (resultCode == Activity.RESULT_OK) {
                    // Bluetooth is now enabled
                	// try again to connect
                	connectSelectedDevice();
                } else {
                    // User did not enable Bluetooth or an error occurred
                    Toast.makeText(this, R.string.bt_not_enabled_leaving, Toast.LENGTH_SHORT).show();
                }
                break;
            }
            case ADD_BLUETOOTH_DEVICE: {
                if (resultCode == Activity.RESULT_CANCELED)
                	return;
            	String ser = data.getStringExtra(AndroPDI.DEVICE_SERIALIZATION);
				BluetoothDevice device = new BluetoothDevice(ser);
				addDevice(device);
				break;
            }
            case EDIT_BLUETOOTH_DEVICE: {
                if (resultCode == Activity.RESULT_CANCELED)
                	return;
            	String ser = data.getStringExtra(AndroPDI.DEVICE_SERIALIZATION);
            	String prevSer = data.getStringExtra(AndroPDI.DEVICE_PREVIOUS_SERIALIZATION);
				BluetoothDevice device = new BluetoothDevice(ser);
				replaceDevice(prevSer, device);
				break;
            }
            case ADD_TCPIP_DEVICE: {
                if (resultCode == Activity.RESULT_CANCELED)
                	return;
            	String ser = data.getStringExtra(AndroPDI.DEVICE_SERIALIZATION);
				TCPIPDevice device = new TCPIPDevice(ser);
				addDevice(device);
				break;
            }
            case EDIT_TCPIP_DEVICE: {
                if (resultCode == Activity.RESULT_CANCELED)
                	return;
            	String ser = data.getStringExtra(AndroPDI.DEVICE_SERIALIZATION);
            	String prevSer = data.getStringExtra(AndroPDI.DEVICE_PREVIOUS_SERIALIZATION);
            	TCPIPDevice device = new TCPIPDevice(ser);
				replaceDevice(prevSer, device);
				break;
            }
            default:
        }
	}
	
	private void addDeviceFromURI(String uri) {
		// parse the uri
		if (!uri.startsWith("opdi_")) {
			Toast.makeText(this, R.string.invalid_opdi_url, Toast.LENGTH_SHORT).show();  
		}
		
		// Bluetooth or Bluetooth secure?
		if (uri.startsWith("opdi_bt://") || uri.startsWith("opdi_bts://")) {
			
			// URI schema: opdi_bt[s]://<address in format AA:BB:CC:DD:EE:FF>[?parameters]
			// with parameters in form key=value, separated by &
			// possible keys: name, pin
			// use default adapter
			BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
			if (bluetoothAdapter == null) {
				Toast.makeText(this, R.string.sorry_bluetooth_not_supported, Toast.LENGTH_SHORT).show();
				return;
			}
			boolean secure = uri.startsWith("opdi_bts://");
			String remainder = uri.substring("opdi_bt://".length() + (secure ? 1 : 0));
			// split remainder
			String[] parts = remainder.split("\\?");
			// there is at least one part
			String address = parts[0];
			String name = address;		// default
			String psk = "";
			String pin = "";
			if (parts.length > 1) {
				// parse URI parameters
			    Map<String, String> parameters = new LinkedHashMap<String, String>();
			    String[] pairs = parts[1].split("&");
			    for (String pair : pairs) {
			        int idx = pair.indexOf("=");
			        try {
						parameters.put(URLDecoder.decode(pair.substring(0, idx), "UTF-8"), URLDecoder.decode(pair.substring(idx + 1), "UTF-8"));
					} catch (UnsupportedEncodingException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
			    }
			    if (parameters.containsKey("name")) {
			    	name = parameters.get("name");
			    }
			    if (parameters.containsKey("psk")) {
			    	psk = parameters.get("psk");
			    }
			    if (parameters.containsKey("pin")) {
			    	pin = parameters.get("pin");
			    }
			}
			// validate address
			if (!BluetoothAdapter.checkBluetoothAddress(address)) {
				// invalid address
				Toast.makeText(this, R.string.bluetooth_invalid_address, Toast.LENGTH_SHORT).show();
				return;
			}
			
			// Create an intent to start the AddBluetoothDevice activity
			Intent intent = new Intent(this, AddBluetoothDevice.class);
			intent.putExtra(BT_ADDRESS, address);
			intent.putExtra(BT_NAME, name);
			intent.putExtra(DEVICE_PSK, psk);
			intent.putExtra(BT_PIN, pin);
			intent.putExtra(BT_SECURE, secure);
			startActivityForResult(intent, ADD_BLUETOOTH_DEVICE);
		}
		// TCP/IP?
		if (uri.startsWith("opdi_tcp://")) {
			
			// URI schema: opdi_tcp://<host/IP>:<port>[?parameters]
			// with parameters in form key=value, separated by &
			// possible keys: name
			// use default adapter
			String remainder = uri.substring("opdi_tcp://".length());
			// split remainder
			String[] parts = remainder.split("\\?");
			// there is at least one part
			String host = parts[0];
			String port = "" + TCPIPDevice.STANDARD_PORT;
			String psk = "";
			String adrparts[] = host.split(":");
			host = adrparts[0];	// host/IP
			if (adrparts.length > 1)
				port = adrparts[1];
			String name = host;		// default
			if (parts.length > 1) {
				// parse URI parameters
			    Map<String, String> parameters = new LinkedHashMap<String, String>();
			    String[] pairs = parts[1].split("&");
			    for (String pair : pairs) {
			        int idx = pair.indexOf("=");
			        try {
						parameters.put(URLDecoder.decode(pair.substring(0, idx), "UTF-8"), URLDecoder.decode(pair.substring(idx + 1), "UTF-8"));
					} catch (UnsupportedEncodingException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
			    }
			    if (parameters.containsKey("name")) {
			    	name = parameters.get("name");
			    }
			    if (parameters.containsKey("psk")) {
			    	psk = parameters.get("psk");
			    }
			}

			// Create an intent to start the AddTCPIPDevice activity
			Intent intent = new Intent(this, AddTCPIPDevice.class);
			intent.putExtra(TCPIP_NAME, name);
			intent.putExtra(TCPIP_HOST, host);
			intent.putExtra(TCPIP_PORT, port);
			intent.putExtra(DEVICE_PSK, psk);
			startActivityForResult(intent, ADD_TCPIP_DEVICE);
		}
		
	}

	public synchronized boolean addDevice(AndroPDIDevice device) {
		
		// check whether this device already exists
		for (AndroPDIDevice aDevice: deviceManager.getDevices()) {
			if (aDevice == null)
				continue;
			if (aDevice.serialize().equals(device.serialize())) {
				
				Toast.makeText(this, R.string.device_already_exists, Toast.LENGTH_SHORT).show();
				return false;
			}
		}
		
		deviceManager.addDevice(device);
		devicesChanged();

		Toast.makeText(this, R.string.device_added, Toast.LENGTH_LONG).show();

		return true;
	}

	private void devicesChanged() {
		// Should be called when the number of devices has changed.
		deviceAdapter.notifyDataSetChanged();
		deviceStatusChanged();
	}
	
	private synchronized void editDevice(AndroPDIDevice device) {
		if (device instanceof BluetoothDevice) {
			// Create an intent to start the EditBluetoothDevice activity
			Intent intent = new Intent(this, EditBluetoothDevice.class);
			intent.putExtra(DEVICE_SERIALIZATION, device.serialize());
			startActivityForResult(intent, EDIT_BLUETOOTH_DEVICE);
		}
		if (device instanceof TCPIPDevice) {
			// Create an intent to start the EditTCPIPDevice activity
			Intent intent = new Intent(this, EditTCPIPDevice.class);
			intent.putExtra(DEVICE_SERIALIZATION, device.serialize());
			startActivityForResult(intent, EDIT_TCPIP_DEVICE);
		}
	}

	private synchronized void removeDevice(AndroPDIDevice device) {
		
		deviceManager.removeDevice(device);
		
		// TODO remove it from all configurations
		
		devicesChanged();
	}

	private void removeAllDevices() {
		// Ask the user if they are sure
        new AlertDialog.Builder(this)
        .setIcon(android.R.drawable.ic_dialog_alert)
        .setTitle(R.string.remove_devices)
        .setMessage(R.string.really_remove_devices)
        .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            	// delete all devices
            	for (AndroPDIDevice device: deviceManager.getDevices()) {
            		removeDevice(device);
            	}
            }
        })
        .setNegativeButton(R.string.no, null)
        .show();
	}
	
	@Override
	public synchronized boolean getCredentials(AndroPDIDevice device, String[] namePassword, Boolean[] save) {
		
		final String[] fNamePassword = namePassword;
		final Boolean[] fSave = save;
		final IDevice dev = device;
		
		// run on UI thread
		final Thread runnable = new Thread() {
			
			public void run() {
				final Thread thisThread = this;
				
				dialogOk = false;
				// show a credentials dialog
				final Dialog dl = new Dialog(AndroPDI.this);
				dl.setTitle(R.string.authentication_required);
				dl.setContentView(R.layout.dialog_authenticate);
				
				// name as specified
				TextView tvName = (TextView) dl.findViewById(R.id.textview_name);
				tvName.setText(dev.getName());
				// address as specified
				TextView tvAddress = (TextView) dl.findViewById(R.id.textview_deviceaddress);
				tvAddress.setText(dev.getDisplayAddress());
				// device name as retrieved from device
				TextView tvDeviceName = (TextView) dl.findViewById(R.id.textview_devicename);
				tvDeviceName.setText(dev.getDeviceName());
				ImageView icon = (ImageView) dl.findViewById(R.id.icon);
				icon.setImageResource(dev.getImageResource());
				
				final EditText username = (EditText) dl.findViewById(R.id.edittext_username);
				username.setText("");
				if (fNamePassword[0] != null)
					username.setText(fNamePassword[0]);
					
				final EditText password = (EditText) dl.findViewById(R.id.edittext_password);
				password.setText("");
				if (fNamePassword[1] != null)
					username.setText(fNamePassword[1]);
				
				final CheckBox cbSave = (CheckBox) dl.findViewById(R.id.checkbox_save);
				cbSave.setChecked(fSave[0] != null && fSave[0]);

				Button bOk = (Button) dl.findViewById(R.id.button_ok);
				bOk.setOnClickListener(new View.OnClickListener() {
					public void onClick(View v) {
						dialogOk = true;

						fNamePassword[0] = username.getText().toString().trim();
						fNamePassword[1] = password.getText().toString().trim();
						fSave[0] = cbSave.isChecked();
						
						dl.dismiss();
						synchronized (thisThread) {
							thisThread.notify();
						}
					}
				});
				Button bCancel = (Button) dl.findViewById(R.id.button_cancel);
				bCancel.setOnClickListener(new View.OnClickListener() {
					public void onClick(View v) {
						dialogOk = false;
						dl.dismiss();
						synchronized (thisThread) {
							thisThread.notify();
						}
					}
				});
				
				dl.show();				
			}
		};
		
		// wait for dialog to finish
		synchronized (runnable) {
			handler.post(runnable);
			try {
				runnable.wait();
			} catch (InterruptedException e) {
				return false;
			}
		}
		
		return dialogOk;
	}

	/** Returns the device with the given address. null if the address is null or not found.
	 * 
	 * @param devAddress
	 * @return
	 */
	public synchronized AndroPDIDevice findDeviceByAddress(String devAddress) {
		return deviceManager.findDeviceByAddress(devAddress);
	}
	
	public synchronized AndroPDIDevice findDeviceById(String devId) {
		return deviceManager.findDeviceById(devId);
	}
	
	public boolean replaceDevice(String originalDeviceSerialization,
			AndroPDIDevice newDevice) {
		if (deviceManager.replaceDevice(originalDeviceSerialization, newDevice)) {
			notifyDevicesChanged((String)getResources().getText(R.string.device_settings_changed), false);
			return true;
		}
		
		// device not found
		return false;
	}

	@Override
	public void notifyDevicesChanged(final String message, final boolean error) {
		// If this activity is at the end of its life, do not show messages.
		if (isFinishing()) return;
		Runnable runnable = new Runnable() {
			@Override
			public void run() {
				// update device status
				deviceStatusChanged();
				if (message != null) {
					Toast.makeText(AndroPDI.this, message, (error ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT)).show();
					addLogMessage(message);   
				}
			}
		};
		runOnUiThread(runnable);
		// give the UI time to update
		Thread.yield();	
	}
	
	@Override
	public void receivedDebug(AndroPDIDevice device, String message) {
		if (isFinishing()) return;
		final String msg = MessageFormat.format(ResourceFactory.getInstance().getString(R.string.device_debug), device.getDeviceName(), message);
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				Toast.makeText(AndroPDI.this, msg, Toast.LENGTH_SHORT).show();
				addLogMessage(msg);
			}
		});
	}
	
	@Override
	public void receivedError(AndroPDIDevice device, String text) {
		notifyDevicesChanged(MessageFormat.format(ResourceFactory.getInstance().getString(R.string.device_error), device.getDeviceName(), text), true);
	}
	
}