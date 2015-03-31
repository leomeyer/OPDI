package org.ospdi.opdi.androPDI.units;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import org.ospdi.opdi.androPDI.utils.ResourceFactory;
import org.ospdi.opdi.units.UnitFormat;
import org.ospdi.opdi.utils.Strings;

import android.content.Context;

public class Units {
	
	String name;
	
	static Map<String, List<UnitFormat>> unitFormatCache = new HashMap<String, List<UnitFormat>>(); 

	public static List<UnitFormat> loadUnits(Context context, String name) {
		// get string identifier
		int stringID = context.getResources().getIdentifier(name, "string", context.getPackageName());
		
		String unitDef = ResourceFactory.getInstance().getString(stringID);
		if (unitDef == name)
			throw new IllegalArgumentException("Units not or incorrectly defined for: " + name);

		Map<String, String> config = Strings.getProperties(unitDef);
		// build sorted list of specified formats
		TreeMap<String, String> sortedFormats = new TreeMap<String, String>();
		for (String key: config.keySet()) {
			sortedFormats.put(config.get(key), key);
		}
		
		// go through formats and build the list
		List<UnitFormat> formatList = new ArrayList<UnitFormat>();
		for (String key: sortedFormats.keySet()) {
			int formatID = context.getResources().getIdentifier(sortedFormats.get(key), "string", context.getPackageName());
			String formatDef = ResourceFactory.getInstance().getString(formatID);
			if (formatDef == key)
				throw new IllegalArgumentException("Unit format not or incorrectly defined: " + name);

			formatList.add(new UnitFormat(name, formatDef));
		}
		unitFormatCache.put(name, formatList);
		return formatList;
	}

	public static UnitFormat getDefaultFormat(Context context, String unit) {
		if (unit == null)
			return UnitFormat.DEFAULT;
		List<UnitFormat> formatList = null;
		if (unitFormatCache.containsKey(unit)) {
			formatList = unitFormatCache.get(unit);
		} else {
			// initialize format list from resources
			formatList = loadUnits(context, unit);
		}
		if (formatList.size() == 0)
			return UnitFormat.DEFAULT;
		return formatList.get(0);
	}
}
