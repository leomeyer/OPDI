package org.ospdi.opdi.units;

import java.util.Map;

import org.ospdi.opdi.utils.Strings;

public class UnitFormat {
	
	String name;
	String label;
	String formatString = "%s";
	String valueString = "%.0f";
	int numerator = 1;
	int denominator = 1;
	Map<String, String> config;
	
	public static UnitFormat DEFAULT = new UnitFormat("Default", "");
	
	public UnitFormat(String name, String formatDef) {
		// formatDef is a config string consisting of the following parts:
		//  label: User-friendly label for the format selection
		//  formatString: How the value is displayed on the UI. Default: %s
		//  valueString: How the value is displayed in input fields. Default: %.0f
		//  numerator and denominator: Factor for calculation. Missing numeric components are assumed as 1.
		// Additionally specified properties can be queried using the getProperty method.
		
		this.name = name;
		this.label = name;
		
		config = Strings.getProperties(formatDef);
		if (config.containsKey("label"))
			label = config.get("label");
		if (config.containsKey("formatString"))
			formatString = config.get("formatString");
		if (config.containsKey("valueString"))
			valueString = config.get("valueString");
		if (config.containsKey("numerator"))
			numerator = Strings.parseInt(config.get("numerator"), "Unit numerator: " + name, 1, Integer.MAX_VALUE);
		if (config.containsKey("denominator"))
			denominator = Strings.parseInt(config.get("denominator"), "Unit denominator: " + name, 1, Integer.MAX_VALUE);
	}
	
	public String getProperty(String property, String defaultValue) {
		if (config.containsKey(property))
			return config.get(property);
		
		return defaultValue;
	}
	
	public String format(int value) {
		// calculate value; format the result
		double val = value * numerator / denominator;
		return String.format(formatString, val);
	}
}
