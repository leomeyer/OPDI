package org.ospdi.opdi.wizdroyd.utils;

import android.content.Context;


public class ResourceFactory extends org.ospdi.opdi.utils.ResourceFactory {

	Context context;
	static ResourceFactory instance;
	
	public ResourceFactory(Context context) {
		this.context = context;
		
		instance = this;
	}
	
	public static ResourceFactory getInstance() {
		return instance;
	}
	
	@Override
	public String getString(String id) {
		
		// get android resource string
		int intId = context.getResources().getIdentifier(id, "string", context.getPackageName());
	    String value = intId == 0 ? "" : (String)context.getResources().getText(intId);

	    if (value == null || value.equals(""))
	    		return super.getString(id);
	    else
	    	return value;
	}

	public String getString(int id) {
		
	    String value = id == 0 ? "" : (String)context.getResources().getText(id);

	    return value;
	}
}
