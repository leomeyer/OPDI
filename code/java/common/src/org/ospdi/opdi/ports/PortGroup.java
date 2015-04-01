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
	
	@Override
	public int hashCode() {
		// port groups are identical when their IDs match
		return id.hashCode();
	}
	
	@Override
	public boolean equals(Object obj) {
		// port groups are identical when their IDs match
		if (obj instanceof PortGroup)
			return this.id.equals(((PortGroup)obj).id);
		return false;
	}

	@Override
	public String toString() {
		return label;
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

	public synchronized String getID() {
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
		this.parentGroup = parent;
		// cycle check
		PortGroup gParent = parent.parentGroup;
		while (gParent != null) {
			if (gParent.getID().equals(parent.getID()))
				throw new IllegalArgumentException("Invalid group hierarchy: cycle for " + parent.getID());
			gParent = gParent.parentGroup;
		}
	}

	public PortGroup getParentGroup() {
		return this.parentGroup;
	}

	public boolean hasParentGroup(String groupID) {
		PortGroup gParent = parentGroup;
		while (gParent != null) {
			if (gParent.getID().equals(groupID))
				return true;
			gParent = gParent.parentGroup;
		}
		return false;
	}
}
