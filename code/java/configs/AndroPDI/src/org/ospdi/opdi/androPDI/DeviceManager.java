package org.ospdi.opdi.androPDI;

import java.io.IOException;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;

import org.ospdi.opdi.androPDI.model.Settings;
import org.ospdi.opdi.androPDI.utils.ResourceFactory;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.interfaces.IDevice.DeviceStatus;
import org.ospdi.opdi.interfaces.IDeviceListener;
import org.ospdi.opdi.androPDI.R;

import android.os.Environment;

/** This class manages a list of devices.
 * A device listener can be associated with each device which gets notified about device status changes.
 * @author leo.meyer
 *
 */
public class DeviceManager implements IDeviceListener {
	
	public interface IDeviceStatusListener {
		/** Notifies attached device listeners that one or more devices have changed their status.
		 * 
		 * @param message
		 */
		void notifyDevicesChanged(String message, boolean error);
		
		/** Supplies necessary credentials.
		 */
		boolean getCredentials(AndroPDIDevice device, String[] namePassword, Boolean[] save);

		/** Notifies the listener that a debug message from the device has been received. 
		 */
		void receivedDebug(AndroPDIDevice device, String message);

		/** Notifies the listener that an error occurred on the device.
		 */
		void receivedError(AndroPDIDevice device, String text);
	}
	
    protected static final String SETTINGS_FILENAME = "/androPDI.xml";
    
	protected static DeviceManager instance;

	protected Settings settings;
	protected ArrayList<AndroPDIDevice> devices;
	
    protected boolean mExternalStorageWriteable = false;
    protected boolean mExternalStorageNotReadable = false;

	private StringBuilder logText;
	
	/** Creates a new device manager.
	 * 
	 */
	protected DeviceManager() {
        // populate device list
        devices = new ArrayList<AndroPDIDevice>();
        // add "Manage devices..." item (interpreted by DeviceAdapter)
        devices.add(null);

        logText = new StringBuilder();
        
        settings = new Settings();

        // check external storage state
        String state = Environment.getExternalStorageState();

        if (Environment.MEDIA_MOUNTED.equals(state)) {
            // We can read and write the media
        	mExternalStorageWriteable = true;
        } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
            // We can only read the media
            mExternalStorageWriteable = false;
            mExternalStorageNotReadable = false;            
        } else {
            // Something else is wrong. It may be one of many other states, but all we need
            // to know is we can neither read nor write
            mExternalStorageWriteable = false;
            mExternalStorageNotReadable = true;            
        }

        // load settings if possible
        if (!mExternalStorageNotReadable) {
            settings = Settings.load(getSettingsFilename());
        }
        
        // add devices from settings
        for (AndroPDIDevice device: settings.getDeviceObjects()) {        	
       		devices.add(device);
        }
	}
	
    private static String getSettingsFilename() {
    	return Environment.getExternalStorageDirectory().getAbsolutePath() + SETTINGS_FILENAME;    	
    }

	public static DeviceManager getInstance() {
		if (instance == null) {
			instance = new DeviceManager();
		}
		return instance;
	}

	public boolean isExternalStorageWriteable() {
		return mExternalStorageWriteable;
	}

	public boolean isExternalStorageNotReadable() {
		return mExternalStorageNotReadable;
	}

	public List<AndroPDIDevice> getDevices() {
		return devices;
	}

	public void saveSettings() throws IOException {
		// Store settings if possible
		if (!mExternalStorageWriteable) {
			throw new IllegalStateException("The external storage is not writable");
		}
		
        Settings settings = new Settings();
        
        // add devices
        for (IDevice device: devices)
        	settings.addDevice(device);
        
        // write to file; will throw an exception if something goes wrong
		settings.saveTo(getSettingsFilename());
		
		// remember new settings
		this.settings = settings;
	}

	public void disconnectAllDevices() {
		for (IDevice device: devices) {
			if (device == null) continue;
			if (device.getStatus() == DeviceStatus.CONNECTING)
				device.abortConnect();
			else if (device.isConnected())
				device.disconnect(false);
		}
	}

	public void connectAllDevices(IDeviceStatusListener deviceListener) {
		for (AndroPDIDevice device: devices) {
			if (device == null) continue;
			if (device.getStatus() != DeviceStatus.CONNECTED)
				this.connect(device, deviceListener);
		}
	}

	public void addDevice(AndroPDIDevice device) {
		devices.add(device);
	}

	public AndroPDIDevice findDeviceByAddress(String devAddress) {
		for (AndroPDIDevice device: devices)
			if (device != null && device.getAddress().equals(devAddress))
				return device;
		
		return null;
	}
	
	public AndroPDIDevice findDeviceById(String devId) {
		for (AndroPDIDevice device: devices)
			if (device != null && device.getId().equals(devId))
				return device;
		
		return null;
	}
	
	public boolean replaceDevice(String originalDeviceSerialization,
			AndroPDIDevice newDevice) {
		
		// check whether this device already exists
		int pos = 0;
		for (AndroPDIDevice aDevice: devices) {
			if (aDevice == null)
				continue;
			pos++;
			if (aDevice.serialize().equals(originalDeviceSerialization)) {
				// found
				devices.remove(pos);
				devices.add(pos, newDevice);
				return true;
			}
		}
		
		// device not found
		return false;
	}

	public void removeDevice(IDevice device) {
		devices.remove(device);
	}

	protected void notifyDevices(AndroPDIDevice device, String message) {
		notifyDevices(device, message, false);
	}
	
	protected void notifyDevicesOfError(AndroPDIDevice device, String message) {
		notifyDevices(device, message, true);
	}
	
	protected void notifyDevices(AndroPDIDevice device, String message, boolean error) {
		// get listener for device
		if (device.getListener() == null)
			return;		// no listener
		
		device.getListener().notifyDevicesChanged(message, error);
	}
	
	@Override
	public synchronized void connectionInitiated(IDevice device) {
		notifyDevices((AndroPDIDevice)device, null);
	}

	@Override
	public synchronized void connectionAborted(IDevice device) {
		notifyDevices((AndroPDIDevice)device, MessageFormat.format(ResourceFactory.getInstance().getString(R.string.connection_attempt_to_0_aborted), device.getLabel()));
	}

	@Override
	public synchronized void connectionOpened(IDevice device) {
		notifyDevices((AndroPDIDevice)device, MessageFormat.format(ResourceFactory.getInstance().getString(R.string.connection_established_with_0_), device.getLabel()));
	}

	@Override
	public synchronized void connectionFailed(IDevice device, String message) {
		notifyDevicesOfError((AndroPDIDevice)device, MessageFormat.format(ResourceFactory.getInstance().getString(R.string.connection_to_0_failed_1_), device.getLabel(),
				message));
	}

	@Override
	public synchronized void connectionError(IDevice device, String message) {
		notifyDevicesOfError((AndroPDIDevice)device, MessageFormat.format(ResourceFactory.getInstance().getString(R.string.error_on_0_1_), device.getLabel(), message));
	}
	
	@Override
	public synchronized void connectionTerminating(IDevice device) {
		notifyDevices((AndroPDIDevice)device, null);
	}

	@Override
	public synchronized void connectionClosed(IDevice device) {
		notifyDevices((AndroPDIDevice)device, MessageFormat.format(ResourceFactory.getInstance().getString(R.string.connection_to_0_closed_), device.getLabel()));
	}

	@Override
	public synchronized boolean getCredentials(IDevice dev, String[] namePassword, Boolean[] save) {
		AndroPDIDevice device = (AndroPDIDevice)dev;
		if (device.getListener() == null)
			return false;		// no listener
		
		return device.getListener().getCredentials(device, namePassword, save);
	}
	
	@Override
	public synchronized void receivedDebug(IDevice dev, String message) {
		AndroPDIDevice device = (AndroPDIDevice)dev;
		if (device.getListener() == null)
			return;		// no listener

		device.getListener().receivedDebug(device, message);		
	}
	
	@Override
	public synchronized void receivedReconfigure(IDevice device) {
	}
	
	@Override
	public synchronized void receivedRefresh(IDevice device, String[] portIDs) {
	}
	
	@Override
	public void receivedError(IDevice dev, String text) {
		AndroPDIDevice device = (AndroPDIDevice)dev;
		if (device.getListener() == null)
			return;		// no listener

		device.getListener().receivedError(device, text);		
	}

	public synchronized void connect(AndroPDIDevice device, IDeviceStatusListener deviceListener) {
		// remember the listener for this device
		device.setListener(deviceListener);
		device.connect(this);	
	}
	
	public synchronized void registerListener(IDeviceStatusListener deviceListener) {
		// register this listener for all connected devices
		for (AndroPDIDevice device: devices)
			if (device != null && device.isConnected())
				device.setListener(deviceListener);
	}
	
	public synchronized void registerListenerForDeviceId(IDeviceStatusListener deviceListener, String id) {
		// register this listener for the given device
		for (AndroPDIDevice device: devices)
			if (device != null && device.getId() == id)
				device.setListener(deviceListener);
	}	
	
	public synchronized void addLogText(String logText) {
		this.logText.append(logText);
	}

	public synchronized CharSequence getLogText() {
		return logText.toString();
	}
}
