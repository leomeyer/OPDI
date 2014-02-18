package org.ospdi.opdi.androPDI;

import java.lang.ref.WeakReference;

import org.ospdi.opdi.androPDI.DeviceManager.IDeviceStatusListener;
import org.ospdi.opdi.devices.IODevice;

public abstract class AndroPDIDevice extends IODevice {

	protected static int idCounter = 1; 
	protected String id;		// temporary identifier

	// a device can have only one active listener
	// which is set when the device is being connected
	WeakReference<IDeviceStatusListener> deviceListener;
	
	public AndroPDIDevice() {
		
		// create temporary ID
		id = "dev" + (idCounter++);
	}

	public String getId() {
		return id;
	}
	
	public IDeviceStatusListener getListener() {
		if (deviceListener == null)
			return null;
		
		if (deviceListener.get() == null)
			deviceListener = null;
		
		return deviceListener.get();
	}

	public void setListener(IDeviceStatusListener listener) {
		deviceListener = new WeakReference<IDeviceStatusListener>(listener);
	}
	
	/** Is called before connect() to give the device the chance to prepare IO, e. g. enable Bluetooth.
	 * Returns true if the connect can proceed.
	 */
	public abstract boolean prepare();

	
}
