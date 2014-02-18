package org.ospdi.opdi.wizdroyd.model;

/** A Device describes a remote device.
 * 
 * @author Leo
 *
 */
public class Device {
	
	private String clazz;
	private String info;
	
	protected Device() {}

	public Device(String clazz, String info) {
		this.clazz = clazz;
		this.info = info;
	}

	/** The class that is used to represent this device. */
	public String getClazz() {
		return clazz;
	}

	/** The serialized information required to restore the device object. */
	public String getInfo() {
		return info;
	}
}
