package org.ospdi.opdi.androPDI.ports;

import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.androPDI.DeviceManager;
import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.androPDI.AndroPDIDevice;
import org.ospdi.opdi.androPDI.gui.LoggingActivity;
import org.ospdi.opdi.androPDI.portdetails.ShowPortDetails;
import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.interfaces.IDevice;
import org.ospdi.opdi.interfaces.IDeviceListener;
import org.ospdi.opdi.ports.BasicDeviceCapabilities;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.Port.PortType;
import org.ospdi.opdi.ports.PortGroup;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.utils.ResourceFactory;
import org.ospdi.opdi.androPDI.R;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

/** This class implements an activity to show the ports of a connected device.
*/
public class ShowDevicePorts extends LoggingActivity implements IDeviceListener {

	private static final int COLOR_DEFAULT = Color.BLUE;
	private static final int COLOR_ERROR = Color.RED;
	
	private ShowDevicePorts instance;
	private ProgressBar mProgress;
    private AndroPDIDevice device;
    ListView ports_listview;
    private List<Port> portList;
    private PortListAdapter portListAdapter;
    PortAndAdapter portAndAdapter;

    private Spinner groupSelect;
    List<PortGroup> portGroups; 
    private String currentGroup = "";	// default: no group/all ports

    Handler mHandler = new Handler();

    /** A queue of operations to be performed sequentially.
	 * 
	 */
	protected BlockingQueue<PortAction> queue = new ArrayBlockingQueue<PortAction>(10);
	private TextView tvName; 
	private TextView tvInfo;
	
	protected Thread processorThread;
	
	protected boolean dispatchBlocked;
	
	class PortGroupSelectAdapter extends ArrayAdapter<PortGroup> {

        public PortGroupSelectAdapter(Context context, List<PortGroup> objects) {
            super(context, R.layout.port_group_select_row, objects);
        }

        @Override //don't override if you don't want the default spinner to be a two line view
        public View getView(int position, View convertView, ViewGroup parent) {
            return initView(position, convertView);
        }

        @Override
        public View getDropDownView(int position, View convertView,
                                    ViewGroup parent) {
            return initView(position, convertView);
        }

        private View initView(int position, View convertView) {
            if(convertView == null)
                convertView = View.inflate(getContext(),
                                           R.layout.port_group_select_row,
                                           null);
            TextView tvText1 = (TextView)convertView.findViewById(R.id.text1);
            TextView tvText2 = (TextView)convertView.findViewById(R.id.text2);
            tvText1.setText(getItem(position).getLabel());
            tvText2.setText(getItem(position).getID());
            return convertView;
        }
    }	

	/** Reloads the device capabilities and reconfigures all ports.
	 */
	class ReconfigureOperation extends PortAction {
		
		public ReconfigureOperation() {
			super();
		}
		
		@Override
		void perform() throws TimeoutException, ProtocolException, DeviceException, InterruptedException, DisconnectedException, PortAccessDeniedException {
            IBasicProtocol protocol = device.getProtocol();
            final BasicDeviceCapabilities bdc;
            
        	// query device capabilities
			bdc = protocol.getDeviceCapabilities();
				
	        // get the state of all ports
            // this avoids too much flickering of the GUI
	        for (Port port: bdc.getPorts()) {
				port.getPortState();
				// clear previous view adapter
				port.setViewAdapter(null);
	        }
	        
	        // build group list
	    	portGroups = bdc.getPortGroups(currentGroup);
	    	// insert "All groups" item
	    	portGroups.add(0, new PortGroup("" /* empty ID means all ports */, "All ports", "", 0));
	    	
	    	final List<PortGroup> groups = portGroups; 
	    	
            // update port information
            mHandler.post(new Runnable() {
				public void run() {
					PortGroupSelectAdapter adapter = new PortGroupSelectAdapter(instance, groups);
			    	
			    	groupSelect.setAdapter(adapter);
			    	
			    	// select current group
			    	int selection = 0;
			    	for (PortGroup group: groups) {
			    		if (group.getID().equals(currentGroup))
			    			break;
			    		selection++;
			    	}
			    	groupSelect.setOnItemSelectedListener(null);
			    	if (selection < groups.size())
			    		groupSelect.setSelection(selection, false);
			    	
			    	groupSelect.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			    		@Override
			    		public void onItemSelected(AdapterView<?> parent,
			    				View view, int position, long id) {
			    			String oldGroup = currentGroup; 
			    			currentGroup = groups.get(position).getID();
			    			if (!currentGroup.equals(oldGroup))
				    			// reconfigure to filter the ports
				    	        queue.add(new ReconfigureOperation());
			    		}
			    		
			    		@Override
			    		public void onNothingSelected(AdapterView<?> parent) {
			    			
			    		}
			    	});

			    	portList = bdc.getPorts(currentGroup);
			        portListAdapter = new PortListAdapter(ShowDevicePorts.this, ShowDevicePorts.this, android.R.layout.simple_list_item_1, portList);
			        ports_listview.setAdapter(portListAdapter);
				}
            });
		}
	}

	/** This class performs the operations that are put in the queue asynchronously.
	 */
	class QueueProcessor implements Runnable {
		@Override
        public void run() {
            try {
                while(!Thread.currentThread().isInterrupted()) {
                    // attempt to take the next work item off the queue
                    // if we consume quicker than the producer then take
                    // will block until there is work to do.
                    final PortAction op = queue.take();
                    
                    // Log.d(AndroPDI.MASTER_NAME, "Processing action: " + op.getName());

                    // show the progress bar
                    mHandler.post(new Runnable() {
                        public void run() {
                            dispatchBlocked = true;
                            
                            //tvName.setVisibility(View.GONE);
                        	mProgress.setVisibility(View.VISIBLE);
                            mProgress.setProgress(0);
                        }
                    });
                    
                    // give the UI time to repaint
                    Thread.yield();

                    // do the work
                    try {
                    	op.startPerformAction();
                    	
						op.perform();

                    	// notify port view
	                    mHandler.post(new Runnable() {
	                        public void run() {
	                        	ShowDevicePorts.this.onActionCompleted(op);
	                        }
	                    });
                    } catch (PortAccessDeniedException e) {
                    	// display a message
                    	op.adapter.showMessage(ResourceFactory.instance.getString(ResourceFactory.PORT_ACCESS_DENIED) + 
                    			(e.getMessage() != null ? " " + e.getMessage() : ""));
                    } catch (DisconnectedException de) {
                    	// if disconnected, exit the thread and the view immediately
                    	ShowDevicePorts.this.finish();
                    	break;
                    } catch (final Throwable t) {
                    	t.printStackTrace();
                    	
						// notify adapter: an error occurred
						op.setError(t);
						
                    	// notify port view
	                    mHandler.post(new Runnable() {
	                        public void run() {
	                        	ShowDevicePorts.this.onActionError(op, t);
	                        }
	                    });				
					}

                    mHandler.post(new Runnable() {
                        public void run() {
                        	op.runOnUIThread();

                        	//tvName.setVisibility(View.VISIBLE);
                            // hide the progress bar
                        	mProgress.setVisibility(View.GONE);
                        	
                            dispatchBlocked = false;
                        }
                    });				
                }
            } catch (InterruptedException e) {
                // no need to re-set interrupted as we are outside loop
            }
        }
	}
		
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.show_device_ports);
        this.instance = this;
        
        // get the selected device
        String devId = getIntent().getStringExtra(AndroPDI.CURRENT_DEVICE_ID);
        device = DeviceManager.getInstance().findDeviceById(devId);
        if (device == null) {
        	Toast.makeText(this, R.string.devicecaps_no_device, Toast.LENGTH_SHORT).show();
        	finish();
        	return;
        }
        // the device must be connected
        if (!device.isConnected()) {
        	Toast.makeText(this, R.string.device_not_connected, Toast.LENGTH_SHORT).show();
        	finish();
        	return;
        }

        // the device must support the basic protocol
        if (!(device.getProtocol() instanceof IBasicProtocol)) {
        	Toast.makeText(this, R.string.devicecaps_not_supported, Toast.LENGTH_SHORT).show();
        	finish();
        	return;
        }
        
    	tvName = (TextView)findViewById(R.id.devicecaps_name);
    	tvName.setText(device.getDeviceName());
    	tvName.setVisibility(View.VISIBLE);
    	tvName.setTextColor(COLOR_DEFAULT);

    	tvInfo = (TextView)findViewById(R.id.devicecaps_textview);
    	tvInfo.setText(device.getDisplayAddress());

    	mProgress = (ProgressBar)findViewById(R.id.progress);
    	mProgress.setVisibility(View.GONE);
    	
    	groupSelect = (Spinner)findViewById(R.id.portGroupSelect);
        
    	ports_listview = (ListView)findViewById(R.id.devicecaps_listview);
        
        ports_listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        		// wrong port index
        		if (position >= portList.size()) return;
        		// streaming port clicked?
        		if (portList.get(position).getType() == PortType.STREAMING)
    				// show streaming port details
    				showPortDetails(portList.get(position));
        		// default port click handler: show context menu
        		else {
        			Port p = portList.get(position);
    				// show select port menu
        			if (p.getViewAdapter() instanceof IPortViewAdapter)
        				((IPortViewAdapter)p.getViewAdapter()).handleClick();
        		}
			}
		});
        
        // register this activity as a listener for the device
        device.addConnectionListener(this);
        
        // start asynchronous processing
        processorThread = new Thread(new QueueProcessor(), "QueueProcessor");
        processorThread.setDaemon(true); // don't hold the VM open for this thread
        processorThread.start();
        
        // query the device capabilities
        queue.add(new ReconfigureOperation());
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
    	if (dispatchBlocked)
    		return true;
    	
    	return super.dispatchTouchEvent(ev);
    }
    
	private void showPortDetails(Port port) {
		// Create an intent to start the ShowPortDetails activity
		Intent intent = new Intent(this, ShowPortDetails.class);
		intent.putExtra(AndroPDI.CURRENT_DEVICE_ID, device.getId());
		intent.putExtra(AndroPDI.CURRENT_PORT_ID, port.getID());
		startActivity(intent);			
	}

    @Override
    public void onStart() {
        super.onStart();
    }    
    
    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	
    	if (portList != null) {
	    	// unbind all streaming ports that may have been autobound
	    	for (Port port: portList) {
	    		if (port instanceof StreamingPort) {
	    			addPortAction(new PortAction(port) {
	    				@Override
	    				void perform() throws TimeoutException,
	    						InterruptedException, DisconnectedException,
	    						DeviceException, ProtocolException, PortAccessDeniedException {
	    					((StreamingPort)port).unbind();
	    				}
	    			});
	    		}
	    	}
    	}
    	// unregister listener
    	device.removeConnectionListener(this);
    }
    
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
    }
    
    @Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {

		super.onCreateContextMenu(menu, v, menuInfo);
		  
		// context menu for port item created?
		if (portAndAdapter != null && v == ports_listview) {
			// let the adapter create the menu
			portAndAdapter.createContextMenu(menu, menuInfo);
		}
	}
    
    @Override
    public void onContextMenuClosed(Menu menu) {
    }

    
    /** Queues the specified action for asynchronous processing.
     * 
     * @param portAction
     * @return
     */
	public synchronized boolean addPortAction(PortAction portAction) {
		queue.add(portAction);
		
		return true;
	}

	// must be called on the UI thread!
	protected void onActionError(PortAction action, Throwable t) {
    	Toast.makeText(ShowDevicePorts.this, t.getMessage(), Toast.LENGTH_LONG).show();
    	tvInfo.setText(t.getMessage());
    	
    	// indicate device problem
    	tvName.setTextColor(COLOR_ERROR);
	}
	
	// must be called on the UI thread!
	protected void onActionCompleted(PortAction action) {
    	// reset device name color
		tvName.setTextColor(COLOR_DEFAULT);
    	tvInfo.setText(device.getDisplayAddress());
   	}
	
	protected void stopProcessor() {
		if (processorThread != null)
			processorThread.interrupt();
	}

	@Override
	public void connectionAborted(IDevice device) {
		stopProcessor();
		finish();
	}
	
	@Override
	public void connectionClosed(IDevice device) {
		// interrupt pending or current operations and exit
		stopProcessor();
		finish();
	}
	
	@Override
	public void connectionError(IDevice device, String message) {
		// interrupt pending or current operations and exit
		stopProcessor();
		showError(message);
		finish();
	}

	@Override
	public boolean getCredentials(IDevice device, String[] namePassword, Boolean[] save) {
		// not supported here
		return false;
	}
	
	@Override
	public void connectionFailed(IDevice device, String message) {
		// interrupt pending or current operations and exit
		stopProcessor();
		showError(message);
		finish();
	}
	
	@Override
	public void connectionInitiated(IDevice device) {
	}
	
	@Override
	public void connectionOpened(IDevice device) {
	}
	
	@Override
	public void connectionTerminating(IDevice device) {
	}
	
	@Override
	public void receivedDebug(IDevice device, String message) {
		// nothing to do here (main activity handles all debug messages)
	}
	
	@Override
	public void receivedReconfigure(IDevice device) {
        // query the device capabilities
        queue.add(new ReconfigureOperation());
	}
	
	@Override
	public void receivedRefresh(IDevice device, String[] portIDs) {
		// ports not specified?
		if (portIDs == null || portIDs.length == 0) {
			// refresh all ports
			try {
				for (Port port: device.getProtocol().getDeviceCapabilities().getPorts()) {						
					if (port.getViewAdapter() != null) {
						IPortViewAdapter adapter = (IPortViewAdapter)port.getViewAdapter();
						adapter.refresh();
					}
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		else
			// refresh all specified ports
			for (String portID: portIDs) {
				try {
					Port port = device.getProtocol().getDeviceCapabilities().findPortByID(portID);
					if (port.getViewAdapter() != null) {
						IPortViewAdapter adapter = (IPortViewAdapter)port.getViewAdapter();
						adapter.refresh();
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
	}
	
	@Override
	public void receivedError(IDevice device, String text) {
		// error on device or in connection; close the view
		finish();
	}	
	
	void showError(final String message) {
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				Toast.makeText(ShowDevicePorts.this, message, Toast.LENGTH_SHORT).show();
			}
		});
	}

	/** Wraps a port and an adapter together.
	 * 
	 * @author Leo
	 *
	 */
	class PortAndAdapter {
		Port port;
		IPortViewAdapter adapter;
		
		protected PortAndAdapter(Port port, IPortViewAdapter adapter) {
			super();
			this.port = port;
			this.adapter = adapter;
		}
		
		void createContextMenu(ContextMenu menu, ContextMenuInfo menuInfo) {
			adapter.createContextMenu(menu, menuInfo);
		}
	}

	public void receivedPortMessage(Port port, String message) {
		final String msg = this.device.getDeviceName() + "/" + port.getName() + ": " + message;
		mHandler.post(new Runnable() {
			@Override
			public void run() {
				Toast.makeText(ShowDevicePorts.this, msg, Toast.LENGTH_LONG).show();
				addLogMessage(msg);
			}
		});
	}
}
