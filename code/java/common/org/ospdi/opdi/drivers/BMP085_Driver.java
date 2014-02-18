package org.ospdi.opdi.drivers;

import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.ports.StreamingPort.IStreamingPortListener;
import org.ospdi.opdi.utils.Strings;

/** Driver for Bosch Sensortec pressure sensor BMP085
 * 
 * @author Leo
 *
 */
public class BMP085_Driver extends AbstractDriver implements IStreamingPortListener {
	
	protected static final char SEPARATOR = ':';
	public static final String MAGIC = "BMP085";
	
	protected float temperature;
	protected float pressure;
	
	@Override
	public synchronized void attach(StreamingPort port) {
		// register this driver as a listener
		port.setStreamingPortListener(this);
	}

	@Override
	public synchronized void dataReceived(StreamingPort port, String data) {
		
		// extract pressure and temperature from the stream
		
		String[] parts = Strings.split(data, SEPARATOR);
		
		final int ID = 0;
		final int TEMP = 1;
		final int PRES = 2;
		final int PART_COUNT = 3;

		if (parts.length != PART_COUNT || !parts[ID].equals(MAGIC))
			super.dataReceived(false);
		else {
			try {
				temperature = Float.parseFloat(parts[TEMP]);
				pressure = Float.parseFloat(parts[PRES]);
			} catch (NumberFormatException e) {
				super.dataReceived(false);
			}			
		}
		super.dataReceived(true);
	}

	public synchronized float getTemperature() {
		return temperature;
	}

	public synchronized float getPressure() {
		return pressure;
	}

	@Override
	public void portUnbound(StreamingPort port) {
	}
}
