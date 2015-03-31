package org.ospdi.opdi.androPDI.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.androPDI.R;
import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.DialPort;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.protocol.DisconnectedException;
import org.ospdi.opdi.protocol.PortAccessDeniedException;
import org.ospdi.opdi.protocol.ProtocolException;

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
	private TextView tvBottomtext;
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
		
		// get layout property from UnitFormat definition
		String layoutName = dPort.getUnitFormat().getProperty("layout", "dial_port_row");
		// get layout identifier
		int layoutID = context.getResources().getIdentifier(layoutName, "layout", context.getPackageName());
		// inflate the identified layout
		cachedView = vi.inflate(layoutID, null);
				
        tvToptext = (TextView) cachedView.findViewById(R.id.toptext);
        tvBottomtext = (TextView) cachedView.findViewById(R.id.bottomtext);
        tvMax = (TextView) cachedView.findViewById(R.id.dial_max_value);
        tvMin = (TextView) cachedView.findViewById(R.id.dial_min_value);
        tvCur = (TextView) cachedView.findViewById(R.id.dial_current_value);
        sbSeek = (SeekBar) cachedView.findViewById(R.id.seekBar);
        ivPortIcon = (ImageView) cachedView.findViewById(R.id.port_icon);

        if (tvToptext != null) tvToptext.setText(dPort.getName());
        if (tvBottomtext != null) tvBottomtext.setText(dPort.getType() + " " + dPort.getDirCaps());
        
        showDevicePorts.addPortAction(new PortAction(DialPortViewAdapter.this) {
			@Override
			void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
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

	protected void queryState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
        // attempt to retrieve the values
        position = dPort.getPosition();
        maxValue = dPort.getMaximum();
        minValue = dPort.getMinimum();
        step = dPort.getStep();
        stateError = dPort.hasError();
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
			// indicate an error
	        Drawable drawable = context.getResources().getDrawable(R.drawable.dial_port_error);
			if (ivPortIcon != null) ivPortIcon.setImageDrawable(drawable);
			if (ivPortIcon != null) ivPortIcon.setOnClickListener(null);		// no context menu

			if (tvBottomtext != null) tvBottomtext.setText(dPort.getErrorMessage());
			if (tvMax != null) tvMax.setText("");
			if (tvMin != null) tvMin.setText("");
			if (tvCur != null) tvCur.setText("");
			if (sbSeek != null) sbSeek.setEnabled(false);
			if (sbSeek != null) sbSeek.setOnSeekBarChangeListener(null);
			return;
		}
		
		// set the proper icon
		if (ivPortIcon != null) {
			// default icon
			Drawable drawable = context.getResources().getDrawable(R.drawable.dial_port);
			String iconName = dPort.getUnitFormat().getProperty("icon", "");
			if (iconName != "") {
				// get icon identifier
				int iconID = context.getResources().getIdentifier(iconName, "drawable", context.getPackageName());
				if (iconID == 0)
					throw new IllegalArgumentException("Drawable resource not found: " + iconName);							
					
				drawable = context.getResources().getDrawable(iconID);
			}
			
			ivPortIcon.setImageDrawable(drawable);

			
			// context menu when clicking
			ivPortIcon.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					handleClick();
				}
			});
		}
		
			
		if (tvBottomtext != null) tvBottomtext.setText("");
		if (tvMax != null) tvMax.setText("" + maxValue);
		if (tvMin != null) tvMin.setText("" + minValue);
		if (tvCur != null) tvCur.setText(dPort.getUnitFormat().format(position));

		if (sbSeek != null) sbSeek.setOnSeekBarChangeListener(null);
		if (sbSeek != null) sbSeek.setMax(maxValue - minValue);
		if (sbSeek != null) sbSeek.setProgress(position - minValue);
		if (sbSeek != null) sbSeek.setEnabled(!dPort.isReadonly());

		if (sbSeek != null) sbSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			boolean ignoreNextSet;
			
			private void setValue(final int val) {
				DialPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(DialPortViewAdapter.this) {
					@Override
					void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
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

		// show only if the port is not readonly
		if (dPort.isReadonly())
			return;
		
		MenuInflater inflater = this.showDevicePorts.getMenuInflater();
		
		try {
			// inflate the context menu 
			inflater.inflate(R.menu.dial_port_menu, menu);
			
			// selectively enable/disable menu items
			MenuItem item;

			item = menu.findItem(R.id.menuitem_port_reload);
			item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
				@Override
				public boolean onMenuItemClick(MenuItem item) {
					return showDevicePorts.addPortAction(new PortAction(DialPortViewAdapter.this) {
						@Override
						void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException, PortAccessDeniedException {
							dPort.refresh();
							queryState();
						}
					});
				}
			});
			
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
											ProtocolException, PortAccessDeniedException {
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

	@Override
	public void showMessage(String message) {
		showDevicePorts.receivedPortMessage(this.dPort, message);
		
	}

}