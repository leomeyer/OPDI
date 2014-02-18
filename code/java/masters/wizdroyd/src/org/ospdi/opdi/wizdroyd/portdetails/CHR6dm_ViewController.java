package org.ospdi.opdi.wizdroyd.portdetails;

import org.ospdi.opdi.drivers.CHR6dm_Driver;
import org.ospdi.opdi.interfaces.IDriver;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.wizdroyd.R;
import org.ospdi.opdi.wizdroyd.Wizdroyd;

import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewStub;
import android.widget.TextView;

/** View controller for an NMEA GPS
 * 
 * @author Leo
 *
 */
public class CHR6dm_ViewController implements IViewController {
	
	private static final int VIEW_ID = R.layout.chr6dm_portview;
	private static final int SLEEP_DELAY = 500;
	
	StreamingPort port;
	CHR6dm_Driver driver;
	View view;
	TextView tvBearing;
	TextView tvPitch;
	TextView tvRoll;
	CompassView compassView;
	boolean bound;
	UpdateThread uThread;
    private Handler mHandler;
    
    private float calibYaw;
    private float calibPitch;
    private float calibRoll;
    
    private String formatAngle(double angle) {
    	String str = String.format("%.1f°", angle);
    	// correct odd negative zero problem
    	if (str.equals("-0,0°")) return "0,0°";
    	return str;
    }

	@Override
	public boolean bind(StreamingPort port) {
		this.port = port;
		// get driver
		IDriver driver = port.getDriver();
		if (driver == null) return false;
		if (!(driver instanceof CHR6dm_Driver)) return false;
		this.driver = (CHR6dm_Driver)driver;
		
		// bind the port
		// the driver will now receive data
		try {
			port.bind();
		} catch (Exception e) {
			Log.e(Wizdroyd.MASTER_NAME, "Could not bind the port: " + port.getID(), e);
			return false;
		}
		bound = true;
		
		return true;
	}

	@Override
	public View inflateLayout(ViewStub stub, Handler handler) {
		mHandler = handler;
		stub.setLayoutResource(VIEW_ID);
		view = stub.inflate();
		view.setMinimumWidth(200);
		view.setMinimumHeight(200);
		compassView = (CompassView) view.findViewById(R.id.compassView);
		tvBearing = (TextView) view.findViewById(R.id.chr6dm_bearing);
		tvPitch = (TextView) view.findViewById(R.id.chr6dm_pitch);
		tvRoll = (TextView) view.findViewById(R.id.chr6dm_roll);

		// start the update thread
		uThread = new UpdateThread();
		uThread.start();
		
		return view;
	}
	
	private void bindMenuAction(Menu menu, int id, Runnable action) {
		MenuItem mi = (MenuItem) menu.findItem(id);
		if (mi == null) return;
		final Runnable mAction = action;
		mi.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {			
			@Override
			public boolean onMenuItemClick(MenuItem item) {
				mAction.run();
				return true;
			}
		});
	}
	
	@Override
	public boolean createContextMenu(Menu menu, MenuInflater inflater) {
	    inflater.inflate(R.menu.chr6dm_menu, menu);
	    
	    bindMenuAction(menu, R.id.chr6dm_calibrate, new Runnable() {
	    	@Override
	    	public void run() {
				calibYaw = (float)driver.getInfo().yaw;
				calibPitch = (float)driver.getInfo().pitch;
				calibRoll = (float)driver.getInfo().roll;
	    	}
	    });
	    
	    return true;
	}

	@Override
	public synchronized void unbind() {
		// unbind the port
		try {
			port.unbind();
		} catch (Exception e) {
			Log.e(Wizdroyd.MASTER_NAME, "Could not unbind the port: " + port.getID(), e);
		}
		bound = false;
	}

	/** The thread that performs regular view updates.
	 * 
	 * @author Leo
	 *
	 */
	private class UpdateThread extends Thread {

		@Override
		public void run() {

			while (bound) {
				mHandler.post(new Runnable() {

					public void run() {
						int maxAge = 5000;
						long age = driver.getDataAge();
						if (age <= 0) age = 1;
						if (age > maxAge) age = maxAge;
						if (!driver.hasValidData() || age >= maxAge) {
							tvBearing.setText("---");
							tvPitch.setText("---");
							tvRoll.setText("---");
							
						} else { 
							// data is valid

							if (compassView != null) {
								compassView.setBearing((float)driver.getInfo().yaw - calibYaw);
								compassView.setPitch((float)driver.getInfo().pitch - calibPitch);
								compassView.setRoll((float)driver.getInfo().roll - calibRoll);
								compassView.invalidate();
							}
							
							// add values
							tvBearing.setText(formatAngle(driver.getInfo().yaw - calibYaw));
							tvPitch.setText(formatAngle(driver.getInfo().pitch - calibPitch));
							tvRoll.setText(formatAngle(driver.getInfo().roll - calibRoll));							
						}
					}
                });
				
				try {
					Thread.sleep(SLEEP_DELAY);
				} catch (InterruptedException e) {
					interrupt();
					return;
				}
			}
		}
	}
	
}
