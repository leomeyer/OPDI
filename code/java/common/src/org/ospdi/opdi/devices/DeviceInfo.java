package org.ospdi.opdi.devices;

import java.util.HashMap;

import org.ospdi.opdi.utils.Strings;

public class DeviceInfo {
	
	String deviceInfo;
	HashMap<String, String> properties = new HashMap<String, String>();
	
	public DeviceInfo(String info) {
		this.deviceInfo = info;
		
		this.properties = Strings.getProperties(this.deviceInfo);
	}

	public String getStartGroup() {
		if (!properties.containsKey("startGroup"))
			return null;

		return properties.get("startGroup");
	}

}
