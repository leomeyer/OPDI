package org.ospdi.opdi.wizdroyd.ports;

import java.util.List;

import org.ospdi.opdi.ports.AnalogPort;
import org.ospdi.opdi.ports.DialPort;
import org.ospdi.opdi.ports.DigitalPort;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.SelectPort;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.wizdroyd.R;

import android.content.Context;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

/** Adapts a list of devices for display in a ListView.
 * 
 * @author Leo
 *
 */
class PortListAdapter extends ArrayAdapter<Port> {

	private final ShowDevicePorts showDevicePorts;
	private Context context;
    private List<Port> items;

    public PortListAdapter(ShowDevicePorts showDevicePorts, Context context, int textViewResourceId, List<Port> items) {
        super(context, textViewResourceId, items);
		this.showDevicePorts = showDevicePorts;
        this.context = context;
        this.items = items;
    }
    
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
    	
    	View result = null;		// do not reuse convert views because their types probably don't match!
    	
    	IPortViewAdapter adapter = null;
    	if (result == null) { 		
	    	Port port = items.get(position);
	    	
	    	// does the port have a view adapter assigned?
	    	if (port.getViewAdapter() != null) {
	    		
	    		// re-use existing view adapter for performance
	    		adapter = (IPortViewAdapter)port.getViewAdapter();
	    		// assume it is already configured
	    		return adapter.getView();
	    	}
	    	
            if (port instanceof DigitalPort) {
            	adapter = new DigitalPortViewAdapter(showDevicePorts);
            }
            else if (port instanceof AnalogPort) {
            	adapter = new AnalogPortViewAdapter(showDevicePorts);
            } 
            else if (port instanceof SelectPort) {
            	adapter = new SelectPortViewAdapter(showDevicePorts);
            } 
            else if (port instanceof DialPort) {
            	adapter = new DialPortViewAdapter(showDevicePorts);
            } 
            else if (port instanceof StreamingPort) {
            	adapter = new StreamingPortViewAdapter(showDevicePorts);
            } 
            
            // register adapter with the port
            port.setViewAdapter(adapter);
            
            if (adapter != null) {
            	// configure the port adapter
	            adapter.configure(port, context);
	            
	            // get the adapter view
	            if (result == null) {
	                result = adapter.getView();
	            }
            }	            
            else {
            	// no matching adapter found - create a simple default view
	            if (result == null) {
	                LayoutInflater vi = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	                result = vi.inflate(R.layout.default_port_row, null);
	            }
	            
	            TextView tt = (TextView) result.findViewById(R.id.toptext);
	            TextView bt = (TextView) result.findViewById(R.id.bottomtext);
	            if (tt != null) {
	            	tt.setText(port.getName());
	            }
	            if (bt != null){
	            	bt.setText(port.getType() + " " + port.getDirCaps());
	            }
            }
    	}
    	
        result.setBackgroundColor(Color.TRANSPARENT);            
        return result;
    }
    
    /** Clears all cached view adapters from the items list.
     * Must be done before the list is released.
     */
    public void invalidateViews() {
    	for (Port port: items) {
	    	port.setViewAdapter(null);
    	}
    }
}