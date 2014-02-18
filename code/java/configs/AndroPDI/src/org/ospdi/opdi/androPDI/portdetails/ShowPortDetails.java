package org.ospdi.opdi.androPDI.portdetails;

import org.ospdi.opdi.androPDI.DeviceManager;
import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.interfaces.IDeviceListener;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.androPDI.R;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewStub;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/** This class implements an activity to show a port's detail view.
*/
public class ShowPortDetails extends Activity implements IDeviceListener {

	private IDevice device;
	private StreamingPort port;
	private IViewController viewController;
	private View portView;
    private Handler mHandler = new Handler();

	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.port_details);

		final ViewStub stub = (ViewStub) findViewById(R.id.portdetails_viewstub);

        // get the selected device
        final String devId = getIntent().getStringExtra(AndroPDI.CURRENT_DEVICE_ID);
        device = DeviceManager.getInstance().findDeviceById(devId);
        if (device == null) {
        	Toast.makeText(this, R.string.devicecaps_no_device, Toast.LENGTH_SHORT).show();
        	finish();
        }
        // the device must support the basic protocol
        if (!(device.getProtocol() instanceof IBasicProtocol)) {
        	Toast.makeText(this, R.string.devicecaps_not_supported, Toast.LENGTH_SHORT).show();
        	finish();
        }

        // Start lengthy operation in a background thread
        new Thread(new Runnable() {
            public void run() {

            	// get device and port information
                try {
					
			        // get the selected port
			        String portID = getIntent().getStringExtra(AndroPDI.CURRENT_PORT_ID);
			        device = DeviceManager.getInstance().findDeviceById(devId);
			        Port p = device.getProtocol().findPortByID(portID);
			        if (p == null) {
	                    mHandler.post(new Runnable() {
							public void run() {
								Toast.makeText(ShowPortDetails.this, R.string.portdetails_no_port, Toast.LENGTH_SHORT).show();
							}});
			        	finish();
			        	return;
			        }
			        
			        if (!(p instanceof StreamingPort)) {
	                    mHandler.post(new Runnable() {
							public void run() {
								Toast.makeText(ShowPortDetails.this, R.string.portdetails_not_supported, Toast.LENGTH_SHORT).show();
							}});
			        	finish();
			        	return;
			        }
			        
			        port = (StreamingPort)p;
			        
			        // get the view controller
					viewController = ViewControllerFactory.getViewController(port.getDriverID());
					if (viewController == null) {
	                    mHandler.post(new Runnable() {
							public void run() {
								Toast.makeText(ShowPortDetails.this, R.string.portdetails_not_supported, Toast.LENGTH_SHORT).show();
							}});
			        	finish();
			        	return;
			        }
					
					// Bind the viewController to the port. This will also bind the channel on the device
					if (!viewController.bind(port)) {
			        	finish();
			        	return;
					}

			        // set port info
                    mHandler.post(new Runnable() {

						public void run() {
		                	TextView tvDevice = (TextView)findViewById(R.id.portdetails_device);
		                	tvDevice.setText(port.getName());
		                	TextView tvPort = (TextView)findViewById(R.id.portdetails_port);
		                	tvPort.setText(device.getDeviceName() + " (" + device.getLabel() + ")");
		                	tvPort.setVisibility(View.VISIBLE);
		                	
		                	// Create the view layout
		                	portView = viewController.inflateLayout(stub, mHandler);
		                	// expand port view
		                	portView.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
		                	registerForContextMenu(portView);
						}
                    });
				} catch (Exception e1) {
					Log.e(AndroPDI.MASTER_NAME, "Error", e1);
                	finish();
				}
            }
        }, "InitStreamingPortDetails").start();
        
    	// add connection listener to finish when the device is disconnected
    	if (device != null)
    		device.addConnectionListener(this);
      
    }
    
    @Override
    public void onStart() {
        super.onStart();
    }    
    
    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	// remove connection listener
    	if (device != null)
    		device.removeConnectionListener(this);
    	
    	// unbind the port
    	if (viewController != null)
    		viewController.unbind();
    }
    
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
    }
    
    @Override
	public boolean onKeyDown(int keycode, KeyEvent event) {
		if (keycode == KeyEvent.KEYCODE_MENU) {
			openContextMenu(portView);
		}
		return super.onKeyDown(keycode, event);
	}
    
    @Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {

		super.onCreateContextMenu(menu, v, menuInfo);
		  
    	// let the view controller create the menu
    	if (viewController != null)
    		viewController.createContextMenu(menu, getMenuInflater());
	}
    
    @Override
    public void onContextMenuClosed(Menu menu) {
    }
    
	@Override
	public void connectionOpened(IDevice device) {
		finish();
	}

	@Override
	public void connectionError(IDevice device, String message) {
		finish();
	}

	@Override
	public void connectionTerminating(IDevice device) {
		finish();
	}

	@Override
	public void connectionClosed(IDevice device) {
		finish();
	}

	@Override
	public void connectionInitiated(IDevice device) {
	}

	@Override
	public void connectionAborted(IDevice device) {
		finish();
	}

	@Override
	public void connectionFailed(IDevice device, String message) {
		finish();
	}
	
	@Override
	public boolean getCredentials(IDevice device, String[] namePassword, Boolean[] save) {
		// not supported here
		return false;
	}
	
	@Override
	public void receivedDebug(IDevice device, String message) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void receivedReconfigure(IDevice device) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void receivedRefresh(IDevice device, String[] portIDs) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void receivedError(IDevice device, String text) {
		// TODO Auto-generated method stub
		
	}

	void showError(String message) {
		Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
	}
	
}
