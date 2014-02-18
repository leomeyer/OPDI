package org.ospdi.opdi.drivers;

import org.ospdi.opdi.interfaces.IDriver;

/** A factory that creates driver instances from driver IDs.
 * 
 * @author Leo
 *
 */
public class DriverFactory {

	/** Returns a driver instance if it could be created, otherwise null.
	 * 
	 * @param driverID
	 * @return
	 */
	public static IDriver getDriverInstance(String driverID) {
		if (driverID.equals(Text_Driver.MAGIC))
			return new Text_Driver();
		else
		if (driverID.equals(BMP085_Driver.MAGIC))
			return new BMP085_Driver();
		else
		if (driverID.equals(NMEAGen_Driver.MAGIC))
			return new NMEAGen_Driver();
		else
		if (driverID.equals(CHR6dm_Driver.MAGIC))
			return new CHR6dm_Driver();

		return null;
	}
}
