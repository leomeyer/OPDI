package org.ospdi.opdi.androPDI.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.SelectPort;
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

/** A port view adapter for select ports.
 * 
 * @author Leo
 *
 */
class SelectPortViewAdapter implements IPortViewAdapter {
	
	private final ShowDevicePorts showDevicePorts;
	
	Context context;
	
	private TextView tvToptext;
	private TextView tvBottomtext;
	private ImageView ivPortIcon;
	private ImageView ivStateIcon;
	
	SelectPort sPort;

	// view state values
	int position = -1;

	protected SelectPortViewAdapter(ShowDevicePorts showDevicePorts) {
		super();
		this.showDevicePorts = showDevicePorts;
	}

	public View getView(View convertView) {
		View cachedView;
		if (convertView == null) {
			LayoutInflater vi = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			// get layout property from UnitFormat definition
			String layoutName = sPort.getUnitFormat().getProperty("layout", "digital_port_row");
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

    	tvToptext.setText(sPort.getName());

    	tvBottomtext.setText("");

       	ivStateIcon.setVisibility(View.GONE);

		showDevicePorts.addPortAction(new PortAction(SelectPortViewAdapter.this) {
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

	public void openMenu() {
		this.showDevicePorts.portAndAdapter = this.showDevicePorts.new PortAndAdapter(sPort, this);
		this.showDevicePorts.mHandler.post(new Runnable() {
			@Override
			public void run() {
			    SelectPortViewAdapter.this.showDevicePorts.registerForContextMenu(SelectPortViewAdapter.this.showDevicePorts.ports_listview);
			    SelectPortViewAdapter.this.showDevicePorts.openContextMenu(SelectPortViewAdapter.this.showDevicePorts.ports_listview);
			    SelectPortViewAdapter.this.showDevicePorts.unregisterForContextMenu(SelectPortViewAdapter.this.showDevicePorts.ports_listview);
			}
		});
	}
	
	@Override
	public void handleClick() {
		openMenu();
	}
	
	protected void queryState() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
		try {
			position = sPort.getPosition();
		} catch (PortAccessDeniedException e) {
			position = -1;
		}
	}
	
	@Override
	public void startPerformAction() {
	}
	
	@Override
	public void setError(Throwable t) {
	}
	
	@Override
	public void configure(Port port, Context context) {
		this.sPort = (SelectPort)port;
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
				sPort.refresh();
				queryState();
			}
		});
	}
	
	// must be called on the UI thread!
	public void updateState() {
		if (sPort.hasError()) {
			// indicate an error
			if (ivPortIcon != null) {
				ivPortIcon.setImageDrawable(context.getResources().getDrawable(R.drawable.select_port_error));
				ivPortIcon.setOnClickListener(null);
			}
			tvBottomtext.setText(sPort.getErrorMessage());
		} else {
			// set the proper icon
			Drawable portIcon = context.getResources().getDrawable(R.drawable.select_port);
			String iconName = sPort.getUnitFormat().getProperty("icon", 
					sPort.getExtendedProperty("icon", "select_port"));
			
			if (!iconName.equals("")) {
				// get icon identifier
				int iconID = context.getResources().getIdentifier(iconName, "drawable", context.getPackageName());
				if (iconID == 0)
					throw new IllegalArgumentException("Drawable resource not found: " + iconName);							
					
				portIcon = context.getResources().getDrawable(iconID);
			}
			ivPortIcon.setImageDrawable(portIcon);
			try {
				tvBottomtext.setText(sPort.getLabelAt(position));
			} catch (Throwable t) {
				// text can't be set
				tvBottomtext.setText("");
			}
		}
		// context menu when clicking
		ivPortIcon.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				handleClick();
			}
		});
	}
	
	@Override
	public void createContextMenu(ContextMenu menu,
			ContextMenuInfo menuInfo) {
		
		// show only if the port is not readonly
		if (sPort.isReadonly())
			return;
		
		MenuInflater inflater = this.showDevicePorts.getMenuInflater();
		
		try {
			// inflate the context menu 
			inflater.inflate(R.menu.select_port_menu, menu);

			// add menu items dynamically

			MenuItem reloadItem = menu.findItem(R.id.menuitem_port_reload);
			if (reloadItem != null) {
				reloadItem.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						return showDevicePorts.addPortAction(new PortAction(SelectPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								sPort.refresh();
								queryState();
							}
						});
					}
				});
			}
			
			MenuItem item;

			for (int i = 0; i < sPort.getPosCount(); i++) {
				
				item = menu.add(sPort.getLabelAt(i));
				item.setCheckable(true);
				try {
					item.setChecked(i == sPort.getPosition());
				} catch (PortAccessDeniedException pade) {}
				// store index in numeric shortcut
				item.setNumericShortcut((char)i);
				item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
					// open mode menu
					@Override
					public boolean onMenuItemClick(MenuItem item) {
						final MenuItem mItem = item;
						
						return SelectPortViewAdapter.this.showDevicePorts.addPortAction(new PortAction(SelectPortViewAdapter.this) {
							@Override
							void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
								// set position
								try {
									if (sPort.setPosition(mItem.getNumericShortcut()))
										position = sPort.getPosition();
									else
										// error state; no position is selected
										position = -1;
								} catch (PortAccessDeniedException e) {
									// error state; no position is selected
									position = -1;
								}
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
		showDevicePorts.receivedPortMessage(this.sPort, message);
	}

}