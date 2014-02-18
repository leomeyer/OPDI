package org.ospdi.opdi.androPDI.model;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.List;

import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.androPDI.AndroPDIDevice;
import org.ospdi.opdi.interfaces.IDevice;

import android.util.Log;

import com.thoughtworks.xstream.XStream;
import com.thoughtworks.xstream.io.xml.DomDriver;


public class Settings {

	private List<Device> devices = new ArrayList<Device>();
	
	/** Try to load settings from the specified filename. */
	public static Settings load(String filename) {
		try {
			FileInputStream fis = new FileInputStream(filename);
			XStream xstream = new XStream(new DomDriver());
			// return the deserialized settings
			return (Settings)xstream.fromXML(fis);
		} catch (FileNotFoundException e) {
			// The file is not there - don't read the settings
			return new Settings();
		}
	}
	
	/** Store settings under the specified filename. 
	 * @throws IOException */
	public void saveTo(String filename) throws IOException {
		XStream xstream = new XStream();
		String xml = xstream.toXML(this);
		FileOutputStream fos = new FileOutputStream(filename);
		fos.write(xml.getBytes());
		fos.close();
	}

	/** The list of devices in this Settings object. */
	public List<Device> getDevices() {
		return devices;
	}
	
	/** Adds the device to the Settings object. */
	public void addDevice(IDevice device) {
		if (device == null) return;
		// add class name and serialized form
		devices.add(new Device(device.getClass().getName(), device.serialize()));
	}
	
	/** Returns a list of deserialized device objects. */
	public List<AndroPDIDevice> getDeviceObjects() {
		List<AndroPDIDevice> result = new ArrayList<AndroPDIDevice>(devices.size());
		for (Device device: devices) {
			// instantiate device class
			try {
				@SuppressWarnings("rawtypes")
				Class clazz = Class.forName(device.getClazz());
				@SuppressWarnings({ "unchecked", "rawtypes" })
				Constructor constr = clazz.getConstructor(String.class);
				// construct a deserialized object
				IDevice idev = (IDevice)constr.newInstance(device.getInfo());
				result.add((AndroPDIDevice)idev);
			} catch (Exception e) {
				// ignore this device in case of an error
				Log.e(AndroPDI.MASTER_NAME, "Error deserializing device info: " + device.getInfo(), e);
			}
		}
		return result;
	}
}
