package org.ospdi.opdi.androPDI.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.ProtocolException;

abstract class PortAction {

	protected IPortViewAdapter adapter;
	protected Port port;
	
	public PortAction(IPortViewAdapter adapter) {
		this.adapter = adapter;
	}

	public PortAction(Port port) {
		this.port = port;
	}

	protected PortAction() {}

	/** Defines the operations to perform asynchronously.
	 * 
	 * @throws TimeoutException
	 * @throws InterruptedException
	 * @throws DisconnectedException
	 * @throws DeviceException
	 * @throws ProtocolException
	 * @throws PortAccessDeniedException 
	 */
	abstract void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException;

	/** Defines the operations to be done on the UI thread after perform() has been run.
	 * By default calls the updateState() method of the adapter. This causes a UI refresh.
	 */
	public void runOnUIThread() {
		if (adapter != null)
			adapter.updateState();
	}
	
	public String getName() {
		return toString();
	}

	public void startPerformAction() {
		if (adapter != null)
			adapter.startPerformAction();
	}

	public void setError(Throwable t) {
		if (adapter != null)
			adapter.setError(t);
	}
}