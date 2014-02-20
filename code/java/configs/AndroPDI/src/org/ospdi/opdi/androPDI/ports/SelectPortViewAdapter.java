package org.ospdi.opdi.androPDI.ports;

import java.util.concurrent.TimeoutException;

import org.ospdi.opdi.devices.DeviceException;
import org.ospdi.opdi.ports.Port;
import org.ospdi.opdi.ports.SelectPort;
import org.ospdi.opdi.protocol.DisconnectedException;
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
	boolean stateError;
	int position = -1;

	private View cachedView;
	
	protected SelectPortViewAdapter(ShowDevicePorts showDevicePorts) {
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

    	tvToptext.setText(sPort.getName());

    	tvBottomtext.setText("");

       	ivStateIcon.setVisibility(View.GONE);

		showDevicePorts.addPortAction(new PortAction(SelectPortViewAdapter.this) {
			@Override
			void perform() throws TimeoutException, InterruptedException, DisconnectedException, DeviceException, ProtocolException {
				queryState();
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
		position = sPort.getPosition();
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
		if (stateError) {
			// indicate an error
			if (ivPortIcon != null) {
				ivPortIcon.setImageDrawable(context.getResources().getDrawable(R.drawable.select_port_error));
				ivPortIcon.setOnClickListener(null);
			}
			return;
		}
		
		// set the proper icon
		Drawable portIcon = context.getResources().getDrawable(R.drawable.select_port);
		ivPortIcon.setImageDrawable(portIcon);
		// context menu when clicking
		ivPortIcon.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				handleClick();
			}
		});
			
		tvBottomtext.setText(sPort.getLabelAt(position));
	}		
	
	@Override
	public void createContextMenu(ContextMenu menu,
			ContextMenuInfo menuInfo) {
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
				item.setChecked(i == sPort.getPosition());
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
								// set mode
								sPort.setPosition(mItem.getNumericShortcut());
								position = sPort.getPosition();
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
}