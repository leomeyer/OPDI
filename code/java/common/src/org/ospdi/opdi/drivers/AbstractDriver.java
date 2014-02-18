package org.ospdi.opdi.drivers;

import java.util.Date;

import org.ospdi.opdi.interfaces.IDriver;

/** Implements generic functions for drivers.
 * 
 * @author Leo
 *
 */
public abstract class AbstractDriver implements IDriver {

	private long receiveTime;
	private boolean valid;
	
	/** Notifies that data has been received and whether it is valid or not.
	 * 
	 * @param valid
	 */
	public void dataReceived(boolean valid) {
		// remember receive time
		receiveTime = new Date().getTime();
		this.valid = valid;
	}
	
	public long getDataAge() {
		return new Date().getTime() - receiveTime;
	}


	@Override
	public boolean hasValidData() {
		return valid;
	}

	
}
