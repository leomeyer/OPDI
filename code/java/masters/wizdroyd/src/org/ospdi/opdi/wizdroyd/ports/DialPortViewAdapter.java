package org.ospdi.opdi.wizdroyd.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.DialPort;
import org.ospdi.opdi.ports.Port;
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
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

/** A port view adapter for dial ports.
 * 
 * @author Leo
 *
 */
class DialPortViewAdapter implements IPortViewAdapter {
	
	protected final ShowDevicePorts showDevicePorts;

	Context context;

	private TextView tvToptext;
	private TextView tvMax;
	private TextView tvMin;
	private TextView tvCur;
	private SeekBar sbSeek;
	private ImageView ivPortIcon;
	
	DialPort dPort;
	
	// cache values
    int position;
    int minValue;
    int maxValue;
    int step;
    boolean stateError;

	private View cachedView;
    
	protected DialPortViewAdapter(ShowDevicePorts showDevicePorts) {
		super();
		this.showDevicePorts = showDevicePorts;
	}

	public View getView() {
		if (cachedView != null)
			return cachedView;
		
		LayoutInflater vi = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		cachedView = vi.inflate(R.layout.dial_port_row, null);
				
        tvToptext = (TextView) cachedView.findViewById(R.id.toptext);
        tvMax = (TextView) cachedView.findViewById(R.id.dial_max_value);
        tvMin = (TextView) cachedView.findViewById(R.id.dial_min_value);
        tvCur = (TextView) cachedView.findViewById(R.id.dial_current_value);
        sbSeek = (SeekBar) cachedView.findViewById(R.id.seekBar);
        ivPortIcon = (ImageView) cachedView.findViewById(R.id.port_icon);

        tvToptext.setText(dPort.getName());

        showDevicePorts.addPortAction(new PortAction(DialPortViewAdapter.this) {
			@Override
			void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
				queryState();
			}
		});

        return cachedView;
	}
	
	public void openMenu() {
		this.showDevicePorts.portAndAdapter = this.showDevicePorts.new PortAndAdapter(dPort, this);
		this.showDevicePorts.mHandler.post(new Runnable() {
			@Override
			public void run() {
			    DialPortViewAdapter.this.showDevicePorts.registerForContextMenu(DialPortViewAdapter.this.showDevicePorts.ports_listview);
			    DialPortViewAdapter.this.showDevicePorts.openContextMenu(DialPortViewAdapter.this.showDevicePorts.ports_listview);
			    DialPortViewAdapter.this.showDevicePorts.unregisterForContextMenu(DialPortViewAdapter.this.showDevicePorts.ports_listview);
			}
		});
	}
	
	@Override
	public void handleClick() {
		openMenu();
	}

	protected void queryState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
        // attempt to retrieve the values
        position = dPort.getPosition();
        maxValue = dPort.getMaximum();
        minValue = dPort.getMinimum();
        step = dPort.getStep();
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
	public void configure(Port port, Context context) {
		this.dPort = (DialPort)port;
		this.context = context;
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
			// indicate an error
	        Drawable drawable = context.getResources().getDrawable(R.drawable.dial_port_error);
			ivPortIcon.setImageDrawable(drawable);
			ivPortIcon.setOnClickListener(null);		// no context menu

			tvMax.setText("");
			tvMin.setText("");
			tvCur.setText("");
			sbSeek.setEnabled(false);
    		sbSeek.setOnSeekBarChangeListener(null);
			return;
		}
		
		// set the proper icon
		Drawable drawable = context.getResources().getDrawable(R.drawable.dial_port);
		ivPortIcon.setImageDrawable(drawable);
		
		// context menu when clicking
		ivPortIcon.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				handleClick();
			}
		});
			
      	tvMax.setText("" + maxValue);

       	tvMin.setText("" + minValue);

       	tvCur.setText("" + position);

       	sbSeek.setMax(maxValue - minValue);
        sbSeek.setProgress(position - minValue);
        	
        sbSeek.setEnabled(true);

        sbSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			boolean ignoreNextSet;
			
			private void setValue(final int val) {
				DialPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DialPortViewAdapter.this) {
					@Override
					void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
						dPort.setPosition(val + dPort.getMinimum());
						position = dPort.getPosition();
					}
				});
			}
			
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				try {
					ignoreNextSet = true;
					setValue(seekBar.getProgress());
				} catch (Exception e) {
					DialPortViewAdapter.this.showDevicePorts.showError(e.toString());
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
				if (ignoreNextSet) {
					ignoreNextSet = false;
					return;
				}
				try {
					ignoreNextSet = true;
					setValue(seekBar.getProgress());
				} catch (Exception e) {
					DialPortViewAdapter.this.showDevicePorts.showError(e.toString());
				}
			}
		});
	}		
	
	@Override
	public void createContextMenu(ContextMenu menu,
			ContextMenuInfo menuInfo) {
		MenuInflater inflater = this.showDevicePorts.getMenuInflater();
		
		try {
			// inflate the context menu 
			inflater.inflate(R.menu.dial_port_menu, menu);
			
			// selectively enable/disable menu items
			MenuItem item;
			
			final int portMinValue = dPort.getMinimum();
			final int portMaxValue = dPort.getMaximum();
			
			item = menu.findItem(R.id.menuitem_dial_set_value);
			item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
				@Override
				public boolean onMenuItemClick(MenuItem item) {
					// open input dialog

					final Dialog dl = new Dialog(showDevicePorts);
					dl.setTitle("Dial port value");
					dl.setContentView(R.layout.dialog_analog_value);
					
					final EditText value = (EditText) dl.findViewById(R.id.edittext_value);
					try {
						value.setText("" + dPort.getPosition());
					} catch (Exception e1) {
						// can't read the value
						return false;
					}

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
							if ((val >= portMinValue) && (val <= portMaxValue))
								// set value
								DialPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DialPortViewAdapter.this) {
									@Override
									void perform()
											throws TimeoutException,
											InterruptedException,
											DisconnectedException,
											DeviceException,
											ProtocolException {
										// set the port value
										// this corrects to the nearest step
										dPort.setPosition(val);
										position = dPort.getPosition();
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
		} catch (Exception e) {
			this.showDevicePorts.showError("Can't show the context menu");
			menu.close();
		}
	}
}