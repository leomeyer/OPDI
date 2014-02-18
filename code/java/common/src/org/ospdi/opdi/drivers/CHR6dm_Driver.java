package org.ospdi.opdi.drivers;

import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.ports.StreamingPort.IStreamingPortListener;
import org.ospdi.opdi.protocol.DisconnectedException;

/** Driver for Attitude and Heading Reference System (AHRS) CHR-6dm
 * 
 * @author Leo
 *
 */
public class CHR6dm_Driver extends AbstractDriver implements IStreamingPortListener {
	
	public static final String MAGIC = "CHR-6dm";
	
	public static final int NO_ERROR = 0;
	public static final int ERROR_NODATA = 1;
	public static final int ERROR_EODATA = 2;
	public static final int ERROR_CHKSUM = 3;
	public static final int ERROR_CONVERSION = 4;
	
	public class CHR6dmInfo {
		
		public static final int YAW = 32768;
		public static final int PITCH = 16384;
		public static final int ROLL = 8192;
		public static final int YAW_RATE = 4096;
		public static final int PITCH_RATE = 2048;
		public static final int ROLL_RATE = 1024; 
		public static final int MX = 512;
		public static final int MY = 256;
		public static final int MZ = 128;
		public static final int GX = 64;
		public static final int GY = 32;
		public static final int GZ = 16;
		public static final int AX = 8;
		public static final int AY = 4;
		public static final int AZ = 2;
		
		public int sensors;
		public double yaw;
		public double pitch;
		public double roll;
		public double yaw_rate;
		public double pitch_rate;
		public double roll_rate;
		public double mx;
		public double my;
		public double mz;
		public double gz;
		public double gy;
		public double gx;
		public double az;
		public double ay;
		public double ax;
		
		public boolean hasSensor(int sensor) {
			return (sensors & sensor) != 0;
		}
	}
	
	private StreamingPort port;
	private CHR6dmInfo info = new CHR6dmInfo();
	private int error;

	@Override
	public synchronized void attach(StreamingPort port) {
		// register this driver as a listener
		port.setStreamingPortListener(this);
		this.port = port;
	}
	
	private short getNextPart(StringBuilder sb) {
		if (sb.length() < 6)
			throw new IllegalArgumentException("insufficient string length");
		String s = sb.substring(0, 6);
		sb.delete(0, 6);
		return Short.parseShort(s.trim());		
	}

	@Override
	public synchronized void dataReceived(StreamingPort port, String data) {
		error = NO_ERROR;
		
		try {
			StringBuilder sb = new StringBuilder(data);
			final String errPrefix = "Error: ";
			if (sb.substring(0, errPrefix.length()).equals(errPrefix)) {
				// parse error number
				error = Integer.parseInt(sb.substring(errPrefix.length() + 1).trim());
				super.dataReceived(false);
			} else {
				
				// the string can be dissected into substrings of length 6 each.
				info.sensors = getNextPart(sb);
				// all scale factors taken from data sheet
				if (info.hasSensor(CHR6dmInfo.YAW))
					info.yaw = getNextPart(sb) * 0.0109863;
				if (info.hasSensor(CHR6dmInfo.PITCH))
					info.pitch = getNextPart(sb) * 0.0109863;
				if (info.hasSensor(CHR6dmInfo.ROLL))
					info.roll = getNextPart(sb) * 0.0109863;
				if (info.hasSensor(CHR6dmInfo.YAW_RATE))
					info.yaw_rate = getNextPart(sb) * 0.0137329;
				if (info.hasSensor(CHR6dmInfo.PITCH_RATE))
					info.pitch_rate = getNextPart(sb) * 0.0137329;
				if (info.hasSensor(CHR6dmInfo.ROLL_RATE))
					info.roll_rate = getNextPart(sb) * 0.0137329;
				if (info.hasSensor(CHR6dmInfo.MZ))
					info.mz = getNextPart(sb) * 0.061035;
				if (info.hasSensor(CHR6dmInfo.MY))
					info.my = getNextPart(sb) * 0.061035;
				if (info.hasSensor(CHR6dmInfo.MX))
					info.mx = getNextPart(sb) * 0.061035;
				if (info.hasSensor(CHR6dmInfo.GZ))
					info.gz = getNextPart(sb) * 0.01812;
				if (info.hasSensor(CHR6dmInfo.GY))
					info.gy = getNextPart(sb) * 0.01812;
				if (info.hasSensor(CHR6dmInfo.GX))
					info.gx = getNextPart(sb) * 0.01812;
				if (info.hasSensor(CHR6dmInfo.AZ))
					info.az = getNextPart(sb) * 0.15387;
				if (info.hasSensor(CHR6dmInfo.AY))
					info.ay = getNextPart(sb) * 0.15387;
				if (info.hasSensor(CHR6dmInfo.AX))
					info.ax = getNextPart(sb) * 0.15387;
				super.dataReceived(true);
			}
			
		} catch (Exception e) {
			error = ERROR_CONVERSION;
			super.dataReceived(false);
		}
	}

	public synchronized CHR6dmInfo getInfo() {
		return info;
	}
	
	public synchronized int getError() {
		return error;
	}
	
	/** Causes the device to have the sensor calibrate itself.
	 * @throws DisconnectedException 
	 * 
	 */
	public void calibrate() throws DisconnectedException {
		port.sendData("cal");
	}

	@Override
	public void portUnbound(StreamingPort port) {
	}	
}