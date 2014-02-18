package org.ospdi.opdi.drivers;

import org.iu.gps.GPSInfo;
import org.iu.gps.NMEA;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.ports.StreamingPort.IStreamingPortListener;

/** Driver for generic NMEA GPS
 * 
 * @author Leo
 *
 */
public class NMEAGen_Driver extends AbstractDriver implements IStreamingPortListener {

	public static final Object MAGIC = "NMEAGen";
	
	private GPSInfo info = new GPSInfo();
	
	@Override
	public synchronized void attach(StreamingPort port) {
		// register this driver as a listener
		port.setStreamingPortListener(this);
	}

	@Override
	public synchronized void dataReceived(StreamingPort port, String data) {
		
		// extract NMEA records
		GPSInfo info = new GPSInfo();
		try {
			if (!NMEA.parse(data + "\n", info))		
				super.dataReceived(false);
			else {
				// parse into member field
				NMEA.parse(data + "\n", this.info);
				super.dataReceived(true);
			}
		} catch (Exception e) {
			super.dataReceived(false);
		}
	}

	public synchronized GPSInfo getInfo() {
		return info;
	}


	@Override
	public void portUnbound(StreamingPort port) {
	}
}