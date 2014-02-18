package org.ospdi.opdi.androPDI.portdetails;

import org.ospdi.opdi.drivers.BMP085_Driver;
import org.ospdi.opdi.drivers.CHR6dm_Driver;
import org.ospdi.opdi.drivers.NMEAGen_Driver;

/** A factory that creates view controllers for streaming port drivers.
 * 
 * @author Leo
 *
 */
public class ViewControllerFactory {

	public static IViewController getViewController(String driverID) {
		if (driverID.equals(BMP085_Driver.MAGIC))
			return new BMP085_ViewController();
		else
		if (driverID.equals(NMEAGen_Driver.MAGIC))
			return new NMEAGen_ViewController();
		else
		if (driverID.equals(CHR6dm_Driver.MAGIC))
			return new CHR6dm_ViewController();
		return null;
	}
}
