package org.ospdi.opdi.units;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;

import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;
import org.joda.time.LocalDateTime;
import org.joda.time.format.DateTimeFormat;
import org.ospdi.opdi.utils.Strings;

public class UnitFormat {
	
	String name;
	String label;
	String formatString = "%s";
	String valueString = "%.0f";
	int numerator = 1;
	int denominator = 1;
	String conversion;
	String editor;
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
		if (config.containsKey("editor"))
			editor = config.get("editor");
	}
	
	public String getProperty(String property, String defaultValue) {
		if (config.containsKey(property))
			return config.get(property);
		
		return defaultValue;
	}
	
	public boolean hasEditor() {
		return editor != null;
	}
	
	protected String formatUnixSeconds(long value) {
		Date date = new Date(value * 1000);
		return new SimpleDateFormat(formatString).format(date);
	}
	
	protected String formatUnixSecondsLocal(long value) {
		return DateTimeFormat.mediumDateTime().print(convertToLocalDate(value));
	}
/*	
	public DateTime convertToDate(long value) {
		if ("unixSeconds".equals(conversion)) {
			return new DateTime(value, DateTimeZone.UTC);
		}
		else
		if ("unixSecondsLocal".equals(conversion)) {
			return new DateTime(value);
		}
		else
			throw new RuntimeException("UnitFormat: Conversion not supported: " + conversion);
	}
*/	
	public LocalDateTime convertToLocalDate(long value) {
		if ("unixSeconds".equals(conversion)) {
			return new LocalDateTime(value * 1000, DateTimeZone.UTC);
		}
		else
		if ("unixSecondsLocal".equals(conversion)) {
			return new LocalDateTime(value * 1000);
		}
		else
			throw new RuntimeException("UnitFormat: Conversion not supported: " + conversion);
	}
/*	
	public long convertFromDate(DateTime date) {
		if ("unixSeconds".equals(conversion)) {
			return date.getMillis() / 1000;
		}
		else
		if ("unixSecondsLocal".equals(conversion)) {
			return date.getMillis() / 1000;
		}
		else
			throw new RuntimeException("UnitFormat: Conversion not supported: " + conversion);
	}	
*/
	public long convertFromLocalDate(LocalDateTime date) {
		if ("unixSeconds".equals(conversion)) {
			DateTime utc = date.toDateTime(DateTimeZone.UTC);
			return utc.getMillis() / 1000;
		}
		else
		if ("unixSecondsLocal".equals(conversion)) {
			DateTime utc = date.toDateTime();
			return utc.getMillis() / 1000;
		}
		else
			throw new RuntimeException("UnitFormat: Conversion not supported: " + conversion);
	}
	
	public String format(int value) {
		try {
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
				return String.format(formatString, value);
		} catch (Exception e) {
			return e.getClass().getSimpleName() + " " + e.getMessage();
		}
	}
	
	public String format(long value) {
		try {
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
				return String.format(formatString, value);
		} catch (Exception e) {
			return e.getClass().getSimpleName() + " " + e.getMessage();
		}
	}
}
