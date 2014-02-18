package org.ospdi.opdi.androPDI.portdetails;

import org.ospdi.opdi.ports.StreamingPort;

import android.os.Handler;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewStub;

/** Specifies the functions of a view controller that forms the link between a driver and a GUI.
 * 
 * @author Leo
 *
 */
public interface IViewController {

	/** Binds the controller to the port. This also binds the channel on the device.
	 * May return false to indicate that the operation failed.
	 * @param port
	 */
	public boolean bind(StreamingPort port);

	
	/** Unbinds the port.
	 * 
	 */
	public void unbind();
	

	/** Inflates the view in the stub and initializes it.
	 * 
	 * @param stub
	 * @return
	 */
	public View inflateLayout(ViewStub stub, Handler handler);

	/** Creates the context menu. Returns true to indicate that the menu should be shown.
	 * 
	 * @param menu
	 * @return
	 */
	public boolean createContextMenu(Menu menu, MenuInflater inflater);

}
