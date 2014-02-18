package org.ospdi.opdi.interfaces;

import org.ospdi.opdi.ports.StreamingPort;

/** Specifies the functions of a device driver based on a streaming port.
 * 
 * @author Leo
 *
 */
public interface IDriver {

	/** Attaches this driver to the specified port.
	 * May perform initialization functions.
	 * @param port
	 */
	public void attach(StreamingPort port);
	
	/** Returns true if the data is valid.
	 * 
	 * @return
	 */
	public boolean hasValidData();
	
	/** Returns the age of the data in milliseconds, i. e. the difference between
	 * the last receive time and now.
	 * @return
	 */
	public long getDataAge();
}
