package org.ospdi.opdi.androPDI.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.DigitalPort;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.Port.PortDirCaps;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.androPDI.R;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

/** A port view adapter for digital ports.
 * 
 * @author Leo
 *
 */
class DigitalPortViewAdapter implements IPortViewAdapter {

	private final ShowDevicePorts showDevicePorts;
	final static int MENU_OUTER = 0;
	final static int MENU_MODE = 1;
	final static int MENU_LINE = 2;
	
	Context context;

	int menutype;
	private TextView tvToptext;
	private TextView tvBottomtext;
	private ImageView ivPortIcon;
	private ImageView ivStateIcon;

	DigitalPort dPort;
	
	// view state values
	boolean stateError;
	DigitalPort.PortLine line;
	DigitalPort.PortMode mode;
	private View cachedView;

	protected DigitalPortViewAdapter(ShowDevicePorts showDevicePorts) {
		super();
		this.showDevicePorts = showDevicePorts;
	}

	public View getView() {
		if (cachedView != null)
			return cachedView;
		
		LayoutInflater vi = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		cachedView = vi.inflate(R.layout.digital_port_row, null);
		
        tvToptext = (TextView) cachedView.findViewById(R.id.toptext);
        tvBottomtext = (TextView) cachedView.findViewById(R.id.bottomtext);
        ivPortIcon = (ImageView) cachedView.findViewById(R.id.port_icon);
        ivStateIcon = (ImageView) cachedView.findViewById(R.id.state_icon);

        tvToptext.setText(dPort.getName());

        tvBottomtext.setText(dPort.getType() + " " + dPort.getDirCaps());

        showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
			@Override
			void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
				queryState();
			}
		});
        
        return cachedView;
	}
	
	void openMenu(int menu) {
		this.menutype = menu;
		this.showDevicePorts.portAndAdapter = this.showDevicePorts.new PortAndAdapter(dPort, this);
		this.showDevicePorts.mHandler.post(new Runnable() {
			@Override
			public void run() {
			    DigitalPortViewAdapter.this.showDevicePorts.registerForContextMenu(DigitalPortViewAdapter.this.showDevicePorts.ports_listview);
			    DigitalPortViewAdapter.this.showDevicePorts.openContextMenu(DigitalPortViewAdapter.this.showDevicePorts.ports_listview);
			    DigitalPortViewAdapter.this.showDevicePorts.unregisterForContextMenu(DigitalPortViewAdapter.this.showDevicePorts.ports_listview);
			}
		});
	}
	
	protected void queryState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
        // attempt to retrieve the values
        line = dPort.getLine();
        mode = dPort.getMode();
        stateError = dPort.hasError();
        if (stateError) {
        	this.showMessage("Error: " + dPort.getErrorMessage());
        }
	}
	
	@Override
	public void startPerformAction() {
		// reset error state
		stateError = false;
	}
	
	@Override
	public void setError(Throwable t) {
		// 
		stateError = true;
	}
	
	@Override
	public void refresh() {
		showDevicePorts.addPortAction(new PortAction(this) {
			@Override
			void perform()
					throws TimeoutException,
					InterruptedException,
					DisconnectedException,
					DeviceException,
					ProtocolException, PortAccessDeniedException {
				// reload the port state
				dPort.refresh();
				queryState();
			}
		});
	}
	
	// must be called on the UI thread!
	public void updateState() {
		if (stateError) {
			ivPortIcon.setImageDrawable(context.getResources().getDrawable(R.drawable.digital_port_error));
			ivPortIcon.setOnClickListener(null);
    		ivStateIcon.setImageDrawable(context.getResources().getDrawable(R.drawable.led_yellow));
    		ivStateIcon.setOnClickListener(null);
    		tvBottomtext.setText(dPort.getErrorMessage());
    		return;
		}

		// set the proper icon
		Drawable portIcon = null;
    	switch(mode) {
    	case OUTPUT: 
    		portIcon = context.getResources().getDrawable(R.drawable.digital_port_output);
    		tvBottomtext.setText("Mode: Output");
    		break;
    	case INPUT_FLOATING: 
    		portIcon = context.getResources().getDrawable(R.drawable.digital_port_input); 
    		tvBottomtext.setText("Mode: Floating input");
    		break;
    	case INPUT_PULLUP: 
    		portIcon = context.getResources().getDrawable(R.drawable.digital_port_input); 
            tvBottomtext.setText("Mode: Input, pullup on");
    		break;
    	case INPUT_PULLDOWN: 
    		portIcon = context.getResources().getDrawable(R.drawable.digital_port_input); 
            tvBottomtext.setText("Mode: Input, pulldown on");
    		break;
    	default:
    		portIcon = context.getResources().getDrawable(R.drawable.digital_port); 
            tvBottomtext.setText("Mode: Unknown");
    		break;
    	}
		ivPortIcon.setImageDrawable(portIcon);
		
		Drawable stateIcon = null;
		if (mode == DigitalPort.PortMode.OUTPUT) {
        	switch(line) {
        	case LOW: 
        		stateIcon = context.getResources().getDrawable(R.drawable.switch_off);
        		ivStateIcon.setOnClickListener(new View.OnClickListener() {							
					@Override
					public void onClick(View v) {
						DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
								// set line
								dPort.setLine(DigitalPort.PortLine.HIGH);
								line = dPort.getLine();
							}							
						});
					}
				});
        		break;
        	case HIGH: 
        		stateIcon = context.getResources().getDrawable(R.drawable.switch_on);
        		ivStateIcon.setOnClickListener(new View.OnClickListener() {							
					@Override
					public void onClick(View v) {
						DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
								// set mode
								dPort.setLine(DigitalPort.PortLine.LOW);
								line = dPort.getLine();
							}						
						});
					}
				});
        		break;
        	}
		} else {
			// an input mode is active
        	switch(line) {
        	case LOW: 
        		stateIcon = context.getResources().getDrawable(R.drawable.led_red);
        		break;
        	case HIGH: 
        		stateIcon = context.getResources().getDrawable(R.drawable.led_green);
        		break;
        	default: 
        		stateIcon = context.getResources().getDrawable(R.drawable.led_yellow);
        		break;            		
        	}
		}
    	ivStateIcon.setImageDrawable(stateIcon);
    			
		// context menu when clicking
		ivPortIcon.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				handleClick();
			}
		});
	}
		
		
	@Override
	public void configure(Port port, Context context) {
		this.dPort = (DigitalPort)port;
		this.context = context;
	}
	
	@Override
	public void handleClick() {
		openMenu(MENU_OUTER);
	}
	
	
	@Override
	public void createContextMenu(ContextMenu menu,
			ContextMenuInfo menuInfo) {
		MenuInflater inflater = this.showDevicePorts.getMenuInflater();
		
		try {
			if (menutype == MENU_OUTER) {
				// inflate the outer menu 
				inflater.inflate(R.menu.digital_port_menu, menu);

				// selectively enable/disable menu items
				MenuItem item;

				item = menu.findItem(R.id.menuitem_port_reload);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
								dPort.refresh();
								queryState();
							}
						});
					}
				});

				// an output only port can't have its mode set
				boolean canSetMode = dPort.getDirCaps() != PortDirCaps.OUTPUT;
				item = menu.findItem(R.id.menuitem_digital_set_mode);
				item.setEnabled(canSetMode);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					// open mode menu
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						// open mode context menu
						DigitalPortViewAdapter.this.openMenu(MENU_MODE);
						return true;
					}
				});
				
				// A port set to output can have its state set
				boolean canSetState = dPort.getMode() == DigitalPort.PortMode.OUTPUT;
				item = menu.findItem(R.id.menuitem_digital_set_state);
				item.setEnabled(canSetState);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					// open mode menu
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						// open mode context menu
						DigitalPortViewAdapter.this.openMenu(MENU_LINE);
						return true;
					}
				});
			}
			else if (menutype == MENU_MODE) {
				// inflate the mode menu 
				inflater.inflate(R.menu.digital_mode_menu, menu);

				// selectively enable/disable menu items
				MenuItem item;
				
				item = menu.findItem(R.id.menuitem_digital_mode_input_floating);
				item.setVisible(dPort.getDirCaps() != PortDirCaps.OUTPUT);
				item.setCheckable(true);
				item.setChecked(dPort.getMode() == DigitalPort.PortMode.INPUT_FLOATING);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
								// set mode
								dPort.setMode(DigitalPort.PortMode.INPUT_FLOATING);
								queryState();
							}
						});
					}
				});

				item = menu.findItem(R.id.menuitem_digital_mode_input_pullup);
				item.setVisible(dPort.getDirCaps() != PortDirCaps.OUTPUT && dPort.hasPullup());
				item.setCheckable(true);
				item.setChecked(dPort.getMode() == DigitalPort.PortMode.INPUT_PULLUP);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
								// set mode
								dPort.setMode(DigitalPort.PortMode.INPUT_PULLUP);
								queryState();
							}
						});
					}
				});

				item = menu.findItem(R.id.menuitem_digital_mode_input_pulldown);
				item.setVisible(dPort.getDirCaps() != PortDirCaps.OUTPUT && dPort.hasPulldown());
				item.setCheckable(true);
				item.setChecked(dPort.getMode() == DigitalPort.PortMode.INPUT_PULLDOWN);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
						@Override
						public boolean onMenuItemClick(MenuItem item) {
							return DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
								@Override
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
									// set mode
									dPort.setMode(DigitalPort.PortMode.INPUT_PULLDOWN);
									queryState();
								}
							});
						}
					});
					
					item = menu.findItem(R.id.menuitem_digital_mode_output);
					item.setVisible(dPort.getDirCaps() != PortDirCaps.INPUT);
					item.setCheckable(true);
					item.setChecked(dPort.getMode() == DigitalPort.PortMode.OUTPUT);
					item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
						@Override
						public boolean onMenuItemClick(MenuItem item) {
							return DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
								@Override
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
									// set mode
									dPort.setMode(DigitalPort.PortMode.OUTPUT);
									queryState();
								}
							});
						}
					});						
				}
				else if (menutype == MENU_LINE) {
					// inflate the line menu 
					inflater.inflate(R.menu.digital_line_menu, menu);
	
					// selectively enable/disable menu items
					MenuItem item;
					
					item = menu.findItem(R.id.menuitem_digital_line_low);
					item.setCheckable(true);
					item.setChecked(dPort.getLine() == DigitalPort.PortLine.LOW);
					item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
						@Override
						public boolean onMenuItemClick(MenuItem item) {
							return DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
								@Override
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
									// set line
									dPort.setLine(DigitalPort.PortLine.LOW);
									queryState();
								}
							});
						}
					});

					item = menu.findItem(R.id.menuitem_digital_line_high);
					item.setCheckable(true);
					item.setChecked(dPort.getLine() == DigitalPort.PortLine.HIGH);
					item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
						@Override
						public boolean onMenuItemClick(MenuItem item) {
							return DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
								@Override
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
									// set line
									dPort.setLine(DigitalPort.PortLine.HIGH);
									queryState();
								}
							});
						}
					});
				}
		} catch (Exception e) {
			this.showDevicePorts.showError("Can't show the context menu");
			menu.close();
		}
	}

	@Override
	public void showMessage(String message) {
		showDevicePorts.receivedPortMessage(this.dPort, message);
	}

}
