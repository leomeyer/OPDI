package org.ospdi.opdi.ports;

import java.util.HashMap;
import java.util.Map;

import org.ospdi.opdi.utils.Strings;

public class PortGroup {

	protected String id;
	protected String label;
	protected String parent;
	protected int flags;
	protected String extendedInfo;
	
	protected PortGroup parentGroup;	
	protected Map<String, String> extendedProperties = new HashMap<String, String>(); 
	

	public PortGroup(String id, String label, String parent, int flags) {
		super();
		this.id = id;
		this.label = label;
		this.parent = parent;
		this.flags = flags;
	}

	public void setExtendedPortInfo(String info) {
		this.extendedInfo = info;
		
		// extract detailed information from extended info
		extendedProperties = Strings.getProperties(info);
	}
	
	public String getExtendedProperty(String property, String defaultValue) {
		if (extendedProperties.containsKey(property)) {
			return extendedProperties.get(property);
		}
		return defaultValue;
	}

	public synchronized String getId() {
		return id;
	}

	public synchronized String getLabel() {
		return label;
	}

	public synchronized String getParent() {
		return parent;
	}

	public synchronized int getFlags() {
		return flags;
	}

	public void setParentGroup(PortGroup parent) {

		// cycle check
		PortGroup gParent = parent.parentGroup;
		while (gParent != null) {
			if (gParent.getId().equals(parent.getId()))
				throw new IllegalArgumentException("Invalid group hierarchy: cycle for " + parent.getId());
			gParent = gParent.parentGroup;
		}
		
		this.parentGroup = parent;
	}
	
}
