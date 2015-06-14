package org.ospdi.opdi.androPDI.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.androPDI.R;
import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.DigitalPort;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.Port.PortDirCaps;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.ProtocolException;

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

	protected DigitalPortViewAdapter(ShowDevicePorts showDevicePorts) {
		super();
		this.showDevicePorts = showDevicePorts;
	}

	public View getView(View convertView) {
		View cachedView;
		if (convertView == null) {
			
			LayoutInflater vi = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			// get layout property from UnitFormat definition
			String layoutName = dPort.getUnitFormat().getProperty("layout", "digital_port_row");
			// get layout identifier
			int layoutID = context.getResources().getIdentifier(layoutName, "layout", context.getPackageName());
			// inflate the identified layout
			cachedView = vi.inflate(layoutID, null);
		} else
			cachedView = convertView;
		
        tvToptext = (TextView) cachedView.findViewById(R.id.toptext);
        tvBottomtext = (TextView) cachedView.findViewById(R.id.bottomtext);
        ivPortIcon = (ImageView) cachedView.findViewById(R.id.port_icon);
        ivStateIcon = (ImageView) cachedView.findViewById(R.id.state_icon);

        if (tvToptext != null) tvToptext.setText(dPort.getName());

        if (tvBottomtext != null) tvBottomtext.setText("");

        showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
			@Override
			void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
				queryState();
				showDevicePorts.mHandler.post(new Runnable() {
					@Override
					public void run() {
						updateState();
					}
				});
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
	
	protected void queryState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException{
        // attempt to retrieve the values
        try {
            line = dPort.getLine();
			mode = dPort.getMode();
		} catch (PortAccessDeniedException e) {
			line = DigitalPort.PortLine.UNKNOWN;
			mode = DigitalPort.PortMode.UNKNOWN;
		}
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
					ProtocolException {
				// reload the port state
				dPort.refresh();
				queryState();
			}
		});
	}
	
	// must be called on the UI thread!
	public void updateState() {
		if (stateError) {
			if (ivPortIcon != null) ivPortIcon.setImageDrawable(context.getResources().getDrawable(R.drawable.digital_port_error));
			if (ivPortIcon != null) ivPortIcon.setOnClickListener(null);
			if (ivPortIcon != null) ivStateIcon.setImageDrawable(context.getResources().getDrawable(R.drawable.led_yellow));
			if (ivPortIcon != null) ivStateIcon.setOnClickListener(null);
			if (tvBottomtext != null) tvBottomtext.setText(dPort.getErrorMessage());
    		return;
		}

		// set the proper icon
		if (ivPortIcon != null) {
			// default icon
			Drawable portIcon = null;
			String iconName = dPort.getExtendedProperty("icon","");
			if (iconName != "") {
				// get icon identifier
				int iconID = context.getResources().getIdentifier(iconName, "drawable", context.getPackageName());
				if (iconID == 0)
					throw new IllegalArgumentException("Drawable resource not found: " + iconName);							
					
				portIcon = context.getResources().getDrawable(iconID);
			}
			
			if (portIcon == null) {
		    	switch(mode) {
		    	case OUTPUT: 
		    		portIcon = context.getResources().getDrawable(R.drawable.digital_port_output);
		    		tvBottomtext.setText("Mode: Output");
		    		break;
		    	case INPUT_FLOATING: 
		    		portIcon = context.getResources().getDrawable(R.drawable.digital_port_input); 
		    		tvBottomtext.setText("Mode: Input");
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
			}
			
			ivPortIcon.setImageDrawable(portIcon);
			
			Drawable stateIcon = null;
			if (!dPort.isReadonly() && (mode == DigitalPort.PortMode.OUTPUT)) {
	        	switch(line) {
	        	case UNKNOWN:
	        		// TODO icon for UNKNOWN
	        		stateIcon = context.getResources().getDrawable(R.drawable.switch_off);
	        	case LOW: 
	        		stateIcon = context.getResources().getDrawable(R.drawable.switch_off);
	        		if (ivStateIcon != null)
		        		ivStateIcon.setOnClickListener(new View.OnClickListener() {							
							@Override
							public void onClick(View v) {
								DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
									@Override
									void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
										// set line
										try {
											dPort.setLine(DigitalPort.PortLine.HIGH);
											line = dPort.getLine();
										} catch (PortAccessDeniedException e) {
											line = DigitalPort.PortLine.UNKNOWN;
										}
									}							
								});
							}
						});
	        		break;
	        	case HIGH: 
	        		stateIcon = context.getResources().getDrawable(R.drawable.switch_on);
	        		if (ivStateIcon != null)
		        		ivStateIcon.setOnClickListener(new View.OnClickListener() {							
							@Override
							public void onClick(View v) {
								DigitalPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DigitalPortViewAdapter.this) {
									@Override
									void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
										// set line
										try {
											dPort.setLine(DigitalPort.PortLine.LOW);
											line = dPort.getLine();
										} catch (PortAccessDeniedException e) {
											line = DigitalPort.PortLine.UNKNOWN;
										}
									}						
								});
							}
						});
	        		break;
	        	}
			} else {
				// an input mode is active
				// disable click listener
				if (ivStateIcon != null)
					ivStateIcon.setOnClickListener(null);				
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

			if (ivStateIcon != null)
				ivStateIcon.setImageDrawable(stateIcon);
		}
    			
		// context menu when clicking
		if (ivPortIcon != null)
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
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								dPort.refresh();
								queryState();
							}
						});
					}
				});

				// an output only port can't have its mode set
				boolean canSetMode = !dPort.isReadonly() && (dPort.getDirCaps() != PortDirCaps.OUTPUT);
				
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
				boolean canSetState = !dPort.isReadonly() && (dPort.getMode() == DigitalPort.PortMode.OUTPUT);
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
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set mode
								try {
									dPort.setMode(DigitalPort.PortMode.INPUT_FLOATING);
								} catch (PortAccessDeniedException e) {
								}
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
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set mode
								try {
									dPort.setMode(DigitalPort.PortMode.INPUT_PULLUP);
								} catch (PortAccessDeniedException e) {
								}
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
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
									// set mode
									try {
										dPort.setMode(DigitalPort.PortMode.INPUT_PULLDOWN);
									} catch (PortAccessDeniedException e) {
									}
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
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
									// set mode
									try {
										dPort.setMode(DigitalPort.PortMode.OUTPUT);
									} catch (PortAccessDeniedException e) {
									}
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
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
									// set line
									try {
										dPort.setLine(DigitalPort.PortLine.LOW);
									} catch (PortAccessDeniedException e) {
									}
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
								void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
									// set line
									try {
										dPort.setLine(DigitalPort.PortLine.HIGH);
									} catch (PortAccessDeniedException e) {
									}
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
