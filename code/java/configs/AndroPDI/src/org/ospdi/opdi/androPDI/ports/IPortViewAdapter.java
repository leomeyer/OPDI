package org.ospdi.opdi.androPDI.ports;

import org.ospdi.opdi.ports.Port;

import android.content.Context;
import android.view.ContextMenu;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;

/** Defines the common functions for an object that connects ports and their views.
 * 
 * @author Leo
 *
 */
interface IPortViewAdapter {

	/** Returns the view associated with the port.
	 * @param convertView 
	 * 
	 * @return
	 */
	View getView(View convertView);

	/** Called to configure the port view.
	 * 
	 * @param context
	 * @param view
	 */
	public void configure(Port port, Context context);

	/** Called when the context menu should be created.
	 * 
	 * @param port
	 * @param menu
	 * @param menuInfo
	 */
	void createContextMenu(ContextMenu menu, ContextMenuInfo menuInfo);

	/** Called when the port row has been clicked.
	 * 
	 * @param port
	 */
	public void handleClick();
	
	/** Called by the action processor before an action is processed.
	 * 
	 */
	public void startPerformAction();
	
	/** Called by the action processor in case an error occurs.
	 *
	 * @param throwable
	 */
	public void setError(Throwable throwable);

	/** Called by the action processor in case a message should be displayed.
	 * 
	 * @param message
	 */
	public void showMessage(String message);

	/** UI update routine that is to be run on the UI thread.
	 * 
	 */
	public void updateState();
	
	/** Tells the adapter to refresh the state asynchronously.
	 * 
	 */
	public void refresh();
	
}