package org.ospdi.opdi.drivers;

import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.ports.StreamingPort.IStreamingPortListener;

/** This class receives text messages from a device.
 * 
 * @author leo.meyer
 *
 */
public class Text_Driver extends AbstractDriver implements IStreamingPortListener {
	
	public static final String MAGIC = "Text";
	
	protected String text;

	public synchronized String getText() {
		return text;
	}

	@Override
	public synchronized void attach(StreamingPort port) {
		// register this driver as a listener
		port.setStreamingPortListener(this);
	}

	@Override
	public synchronized void dataReceived(StreamingPort port, String data) {
		text = data;
		
		super.dataReceived(true);
	}

	@Override
	public void portUnbound(StreamingPort port) {
	}
}
