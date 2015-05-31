package org.ospdi.opdi.units;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;
import java.util.TimeZone;

import org.ospdi.opdi.utils.Strings;

public class UnitFormat {
	
	String name;
	String label;
	String formatString = "%s";
	String valueString = "%.0f";
	int numerator = 1;
	int denominator = 1;
	String conversion;
	Map<String, String> config;
	
	public static UnitFormat DEFAULT = new UnitFormat("Default", "");
	
	public UnitFormat(String name, String formatDef) {
		// formatDef is a config string consisting of the following parts:
		//  label: User-friendly label for the format selection
		//  conversion: "unixSeconds": treat value as timestamp, format as DateTime
		//  formatString: How the value is displayed on the UI. Default: %s
		//  valueString: How the value is displayed in input fields. Default: %.0f
		//  numerator and denominator: Factor for calculation. Missing numeric components are assumed as 1.
		// Additionally specified properties can be queried using the getProperty method.
		
		this.name = name;
		this.label = name;
		
		config = Strings.getProperties(formatDef);
		if (config.containsKey("label"))
			label = config.get("label");
		if (config.containsKey("conversion"))
			conversion = config.get("conversion");
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
	
	protected String formatUnixSeconds(long value) {
		Date date = new Date(value * 1000);
		return new SimpleDateFormat(formatString).format(date);
	}
	
	protected String formatUnixSecondsLocal(long value) {
		Date date = new Date(value * 1000);
		// treat unix seconds as local time value
		SimpleDateFormat sdf = new SimpleDateFormat(formatString);
		sdf.setTimeZone(TimeZone.getTimeZone("UTC"));
		return sdf.format(date);
	}
	
	public String format(int value) {
		if ("unixSeconds".equals(conversion)) {
			return formatUnixSeconds(value);
		}
		if ("unixSecondsLocal".equals(conversion)) {
			return formatUnixSecondsLocal(value);
		}
		// calculate value; format the result
		if ((numerator != 1) || (denominator != 1)) {
			double val = value * numerator / (double)denominator;
			return String.format(formatString, val);
		} else
			return String.valueOf(value);
	}
	
	public String format(long value) {
		if ("unixSeconds".equals(conversion)) {
			return formatUnixSeconds(value);
		}
		if ("unixSecondsLocal".equals(conversion)) {
			return formatUnixSecondsLocal(value);
		}
		// calculate value; format the result
		if ((numerator != 1) || (denominator != 1)) {
			double val = value * numerator / (double)denominator;
			return String.format(formatString, val);
		} else
			return String.valueOf(value);
	}
}
