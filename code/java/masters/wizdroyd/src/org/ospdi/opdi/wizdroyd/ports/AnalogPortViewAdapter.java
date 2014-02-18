package org.ospdi.opdi.wizdroyd.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.AnalogPort;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.Port.PortDirCaps;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.ProtocolException;
import org.ospdi.opdi.wizdroyd.R;

import android.app.Dialog;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

/** A port view adapter for analog ports.
 * 
 * @author Leo
 *
 */
class AnalogPortViewAdapter implements IPortViewAdapter {

	private final ShowDevicePorts showDevicePorts;
	final static int MENU_OUTER = 0;
	final static int MENU_MODE = 1;
	final static int MENU_RESOLUTION = 2;
	final static int MENU_REFERENCE = 3;

	Context context;
	
	int menutype;
	private TextView tvToptext;
	private TextView tvMax;
	private TextView tvRef;
	private TextView tvCur;
	private SeekBar sbSeek;
	private ImageView ivPortIcon;
	
	AnalogPort aPort;

	// view state values
	boolean stateError;
    int value = -1;
    int maxValue;
    AnalogPort.PortMode mode;
    AnalogPort.Reference reference;
    AnalogPort.Resolution resolution;

	boolean ignoreNextValueSet;
	private View cachedView;

	protected AnalogPortViewAdapter(ShowDevicePorts showDevicePorts) {
		this.showDevicePorts = showDevicePorts;
	}

	public View getView() {
		if (cachedView != null)
			return cachedView;

		LayoutInflater vi = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		cachedView = vi.inflate(R.layout.analog_port_row, null);
		
        tvToptext = (TextView) cachedView.findViewById(R.id.toptext);
        tvMax = (TextView) cachedView.findViewById(R.id.analog_max_value);
        tvRef = (TextView) cachedView.findViewById(R.id.analog_ref_value);
        tvCur = (TextView) cachedView.findViewById(R.id.analog_current_value);
        sbSeek = (SeekBar) cachedView.findViewById(R.id.seekBar);
    	sbSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			
			private void setValue(int value) {
				final int val = value;
				showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
					@Override
					void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
						aPort.setValue(val);
						AnalogPortViewAdapter.this.value = aPort.getValue();
					}
				});
			}
			
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				try {
					if (aPort.getMode() == AnalogPort.PortMode.OUTPUT) {
						ignoreNextValueSet = true;
						setValue(seekBar.getProgress());
					}
				} catch (Exception e) {
					AnalogPortViewAdapter.this.showDevicePorts.showError(e.toString());
				}
			}
			
			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				if (fromUser) return;
				if (ignoreNextValueSet) {
					ignoreNextValueSet = false;
					return;
				}
				try {
					if (aPort.getMode() == AnalogPort.PortMode.OUTPUT) {
						ignoreNextValueSet = true;
						setValue(seekBar.getProgress());
					}
				} catch (Exception e) {
					AnalogPortViewAdapter.this.showDevicePorts.showError(e.toString());
				}
			}
    	});			
        
        ivPortIcon = (ImageView) cachedView.findViewById(R.id.port_icon);

        if (tvToptext != null) {
        	tvToptext.setText(aPort.getName());
        }
        
		showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
			@Override
			void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
				queryState();
			}
		});
		
		return cachedView;
	};
	
	void openMenu(int menu) {
		this.menutype = menu;
		this.showDevicePorts.portAndAdapter = this.showDevicePorts.new PortAndAdapter(aPort, this);
		this.showDevicePorts.mHandler.post(new Runnable() {
			@Override
			public void run() {
			    AnalogPortViewAdapter.this.showDevicePorts.registerForContextMenu(AnalogPortViewAdapter.this.showDevicePorts.ports_listview);
			    AnalogPortViewAdapter.this.showDevicePorts.openContextMenu(AnalogPortViewAdapter.this.showDevicePorts.ports_listview);
			    AnalogPortViewAdapter.this.showDevicePorts.unregisterForContextMenu(AnalogPortViewAdapter.this.showDevicePorts.ports_listview);
			}
		});
	}
	
	@Override
	public void handleClick() {
		openMenu(MENU_OUTER);
	}

	protected void queryState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
        // attempt to retrieve the values
        value = aPort.getValue();
        maxValue = aPort.getMaxValue();
        mode = aPort.getMode();
        resolution = aPort.getResolution();
        reference = aPort.getReference();
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

	// must be called on the UI thread!
	public void updateState() {
		if (stateError) {
			// indicate an error
            Drawable drawable = context.getResources().getDrawable(R.drawable.analog_port_error);
			ivPortIcon.setImageDrawable(drawable);
			ivPortIcon.setOnClickListener(null);		// no context menu

//				tt.setText("");
			tvMax.setText("");
			tvRef.setText("");
			tvCur.setText("");
			sbSeek.setEnabled(false);
			return;
		}

		// set the proper icon
		Drawable drawable = null;
    	switch(mode) {
    	case OUTPUT: drawable = context.getResources().getDrawable(R.drawable.analog_port_output); break;
    	case INPUT: drawable = context.getResources().getDrawable(R.drawable.analog_port_input); break;
    	default: drawable = context.getResources().getDrawable(R.drawable.analog_port); break;
    	}
		ivPortIcon.setImageDrawable(drawable);
		
		// context menu when clicking
		ivPortIcon.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				handleClick();
			}
		});
			
    	tvMax.setText("" + maxValue);

    	String refStr = "";
    	// show reference for input ports only
    	if (mode == AnalogPort.PortMode.INPUT)
        	switch (reference) {
        	case INTERNAL: refStr = "Int"; break;
        	case EXTERNAL: refStr = "Ext"; break;
        	}
    	
    	tvRef.setText(refStr);

    	tvCur.setText("" + value);

    	// setting max or value will cause setValue() if it's not suppressed!
    	ignoreNextValueSet = true;
    	sbSeek.setMax(maxValue);
    	ignoreNextValueSet = true;
   		sbSeek.setProgress(value);
    	sbSeek.setEnabled(mode == AnalogPort.PortMode.OUTPUT);
	}
	
	@Override
	public void configure(Port port, Context context) {
		this.aPort = (AnalogPort)port;
		this.context = context;
	}
	
	@Override
	public void refresh() {
		showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
			@Override
			void perform()
					throws TimeoutException,
					InterruptedException,
					DisconnectedException,
					DeviceException,
					ProtocolException {
				// reload the port state
				aPort.refresh();
				queryState();
			}
		});
	}

	@Override
	public void createContextMenu(ContextMenu menu,
			ContextMenuInfo menuInfo) {
		MenuInflater inflater = this.showDevicePorts.getMenuInflater();
		
		try {
			if (menutype == MENU_OUTER) {
				
				// 
				inflater.inflate(R.menu.analog_port_menu, menu);

				// selectively enable/disable menu items
				MenuItem item;
				
				final int portMaxValue = aPort.getMaxValue();
				
				boolean canSetValue = aPort.getMode() == AnalogPort.PortMode.OUTPUT;
				item = menu.findItem(R.id.menuitem_analog_set_value);
				item.setVisible(canSetValue);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						// open input dialog

						final Dialog dl = new Dialog(showDevicePorts);
						dl.setTitle("Analog port value");
						dl.setContentView(R.layout.dialog_analog_value);
						
						final EditText value = (EditText) dl.findViewById(R.id.edittext_value);
						try {
							value.setText("" + aPort.getValue());
						} catch (Exception e1) {
							// can't read the value
							return false;
						}
						
						// show soft keyboard
						value.setOnFocusChangeListener(new View.OnFocusChangeListener() {
							@Override
							public void onFocusChange(View v, boolean hasFocus) {
								value.post(new Runnable() {
									@Override
									public void run() {
										InputMethodManager imm = (InputMethodManager)showDevicePorts.getSystemService(Context.INPUT_METHOD_SERVICE);
										imm.showSoftInput(value, InputMethodManager.SHOW_IMPLICIT);
									}
								});
							}
						});
						value.requestFocus();
						value.selectAll();

						Button bOk = (Button) dl.findViewById(R.id.button_ok);
						bOk.setOnClickListener(new View.OnClickListener() {
							public void onClick(View v) {
								// close dialog
								dl.dismiss();
								
								// parse value
								final int val;
								try {
									val = Integer.parseInt(value.getText().toString());
								} catch (Exception e) {
									// no number entered
									return;
								}
								if ((val >= 0) && (val <= portMaxValue))
									// set value
									AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
										@Override
										void perform()
												throws TimeoutException,
												InterruptedException,
												DisconnectedException,
												DeviceException,
												ProtocolException {
											// set the port value
											aPort.setValue(val);
											AnalogPortViewAdapter.this.queryState();
										}
									});
							}
						});
						Button bCancel = (Button) dl.findViewById(R.id.button_cancel);
						bCancel.setOnClickListener(new View.OnClickListener() {
							public void onClick(View v) {
								dl.dismiss();
							}
						});
						
						dl.show();
						return true;
					}
				});
				
				boolean canSetDirection = aPort.getDirCaps() == PortDirCaps.BIDIRECTIONAL;
				item = menu.findItem(R.id.menuitem_analog_set_mode);
				item.setVisible(canSetDirection);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						AnalogPortViewAdapter.this.openMenu(MENU_MODE);
						return true;
					}
				});

				boolean canSetReference = mode == AnalogPort.PortMode.INPUT &&  
						(aPort.getFlags() & AnalogPort.ADC_CAN_CHANGE_REFERENCE) == AnalogPort.ADC_CAN_CHANGE_REFERENCE; 
				item = menu.findItem(R.id.menuitem_analog_set_reference);
				item.setVisible(canSetReference);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						AnalogPortViewAdapter.this.openMenu(MENU_REFERENCE);
						return true;
					}
				});

				boolean canSetResolution = (aPort.getFlags() & AnalogPort.ADC_CAN_CHANGE_RESOLUTION) == AnalogPort.ADC_CAN_CHANGE_RESOLUTION;
				// The remove item is only enabled when the device is not connected or connecting
				item = menu.findItem(R.id.menuitem_analog_set_resolution);
				item.setVisible(canSetResolution);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						AnalogPortViewAdapter.this.openMenu(MENU_RESOLUTION);
						return true;
					}
				});

			}
			else if (menutype == MENU_MODE) {
				// inflate the mode menu 
				inflater.inflate(R.menu.analog_mode_menu, menu);

				// selectively enable/disable menu items
				MenuItem item;
				
				item = menu.findItem(R.id.menuitem_analog_mode_input);
				item.setVisible(aPort.getDirCaps() != PortDirCaps.OUTPUT);
				item.setCheckable(true);
				item.setChecked(aPort.getMode() == AnalogPort.PortMode.INPUT);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set mode
								aPort.setMode(AnalogPort.PortMode.INPUT);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});

				item = menu.findItem(R.id.menuitem_analog_mode_output);
				item.setVisible(aPort.getDirCaps() != PortDirCaps.INPUT);
				item.setCheckable(true);
				item.setChecked(aPort.getMode() == AnalogPort.PortMode.OUTPUT);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set mode
								aPort.setMode(AnalogPort.PortMode.OUTPUT);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});
			}
			else if (menutype == MENU_RESOLUTION) {
				// inflate the line menu 
				inflater.inflate(R.menu.analog_resolution_menu, menu);

				// selectively enable/disable menu items
				MenuItem item;
				
				item = menu.findItem(R.id.menuitem_analog_res_8bit);
				item.setCheckable(true);
				item.setChecked(resolution == AnalogPort.Resolution.BITS_8);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set line
								aPort.setResolution(AnalogPort.Resolution.BITS_8);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});
				item = menu.findItem(R.id.menuitem_analog_res_9bit);
				item.setCheckable(true);
				item.setChecked(resolution == AnalogPort.Resolution.BITS_9);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set line
								aPort.setResolution(AnalogPort.Resolution.BITS_9);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});
				item = menu.findItem(R.id.menuitem_analog_res_10bit);
				item.setCheckable(true);
				item.setChecked(resolution == AnalogPort.Resolution.BITS_10);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set line
								aPort.setResolution(AnalogPort.Resolution.BITS_10);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});
				item = menu.findItem(R.id.menuitem_analog_res_11bit);
				item.setCheckable(true);
				item.setChecked(resolution == AnalogPort.Resolution.BITS_11);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set line
								aPort.setResolution(AnalogPort.Resolution.BITS_11);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});
				item = menu.findItem(R.id.menuitem_analog_res_12bit);
				item.setCheckable(true);
				item.setChecked(resolution == AnalogPort.Resolution.BITS_12);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set line
								aPort.setResolution(AnalogPort.Resolution.BITS_12);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});
			}
			else if (menutype == MENU_REFERENCE) {
				// inflate the line menu 
				inflater.inflate(R.menu.analog_reference_menu, menu);

				// selectively enable/disable menu items
				MenuItem item;
				
				item = menu.findItem(R.id.menuitem_analog_ref_internal);
				item.setCheckable(true);
				item.setChecked(reference == AnalogPort.Reference.INTERNAL);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set line
								aPort.setReference(AnalogPort.Reference.INTERNAL);
								AnalogPortViewAdapter.this.queryState();
							}
						});
					}
				});

				item = menu.findItem(R.id.menuitem_analog_ref_external);
				item.setCheckable(true);
				item.setChecked(reference == AnalogPort.Reference.EXTERNAL);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return AnalogPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(AnalogPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set reference
								aPort.setReference(AnalogPort.Reference.EXTERNAL);
								AnalogPortViewAdapter.this.queryState();
							}									
						});
					}
				});
			}
		} catch (Exception e) {
			this.showDevicePorts.showError("Can't show the context menu");
		}
	}

}