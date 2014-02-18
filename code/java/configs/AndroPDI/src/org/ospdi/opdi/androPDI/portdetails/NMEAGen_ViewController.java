package org.ospdi.opdi.androPDI.portdetails;

import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.drivers.NMEAGen_Driver;
import org.ospdi.opdi.interfaces.IDriver;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.androPDI.R;

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
public class NMEAGen_ViewController implements IViewController {
	
	public enum PositionFormat {
		DEGREES,
		DEGREES_MIN,
		DEGREES_MIN_SEC
	}
	
	public enum HeightFormat {
		METRES,
		FEET
	}
	
	private static final int VIEW_ID = R.layout.nmeagen_portview;
	private static final int SLEEP_DELAY = 500;
	private PositionFormat posFormat = PositionFormat.DEGREES_MIN_SEC;
	private HeightFormat heightFormat = HeightFormat.METRES;
	
	StreamingPort port;
	NMEAGen_Driver driver;
	View view;
	TextView tvLat;
	TextView tvLon;
	TextView tvHeight;
	boolean bound;
	UpdateThread uThread;
    private Handler mHandler;
    
    private String formatPos(double pos) {
    	switch (posFormat) {
    	case DEGREES: return String.format("%.6f°", pos);
    	case DEGREES_MIN: {
  	      // Convert to Degree Minutes Representation
  	      double LatDeg = Math.floor(pos);
  	      double LatMin = (pos - LatDeg) * 60;
  	      return String.format("%.0f° %.4f'", LatDeg, LatMin);
    	}
    	case DEGREES_MIN_SEC: {
  	      // Convert to Degree Minutes Seconds Representation
  	      double LatDeg = Math.floor(pos);
  	      double LatMin = Math.floor((pos - LatDeg) * 60);
	      double LatSec = ((((pos - LatDeg) - (LatMin / 60)) * 60 * 60) * 100) / 100;
  	      return String.format("%.0f° %.0f' %.2f''", LatDeg, LatMin, LatSec);
    	}
    	default: return String.format("%.6f", pos);
    	}
    }

    private String formatHeight(double height) {
    	switch (heightFormat) {
    	case METRES: return String.format("%.2f m", height);
    	case FEET: return String.format("%.2f ft", height * 3.2808399);
    	default: return String.format("%.2f", height);
    	}
    }
    
	@Override
	public boolean bind(StreamingPort port) {
		this.port = port;
		// get driver
		IDriver driver = port.getDriver();
		if (driver == null) return false;
		if (!(driver instanceof NMEAGen_Driver)) return false;
		this.driver = (NMEAGen_Driver) driver;
		
		// bind the port
		// the driver will now receive data
		try {
			port.bind();
		} catch (Exception e) {
			Log.e(AndroPDI.MASTER_NAME, "Could not bind the port: " + port.getID(), e);
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
		tvLat = (TextView) view.findViewById(R.id.nmeagen_lat);
		tvLon = (TextView) view.findViewById(R.id.nmeagen_lon);
		tvHeight = (TextView) view.findViewById(R.id.nmeagen_height);

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
	    inflater.inflate(R.menu.nmeagen_menu, menu);
	    
	    bindMenuAction(menu, R.id.nmeagen_menu_position_degrees, new Runnable() {
	    	@Override
	    	public void run() {
	    		posFormat = PositionFormat.DEGREES;
	    	}
	    });
	    
	    bindMenuAction(menu, R.id.nmeagen_menu_position_degrees_minutes, new Runnable() {
	    	@Override
	    	public void run() {
	    		posFormat = PositionFormat.DEGREES_MIN;
	    	}
	    });
	    
	    bindMenuAction(menu, R.id.nmeagen_menu_position_degrees_min_sec, new Runnable() {
	    	@Override
	    	public void run() {
	    		posFormat = PositionFormat.DEGREES_MIN_SEC;
	    	}
	    });
	    
	    bindMenuAction(menu, R.id.nmeagen_menu_height_meters, new Runnable() {
	    	@Override
	    	public void run() {
	    		heightFormat = HeightFormat.METRES;
	    	}
	    });
	    
	    bindMenuAction(menu, R.id.nmeagen_menu_height_feet, new Runnable() {
	    	@Override
	    	public void run() {
	    		heightFormat = HeightFormat.FEET;
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
			Log.e(AndroPDI.MASTER_NAME, "Could not unbind the port: " + port.getID(), e);
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
							tvLat.setText("---");
							tvLon.setText("---");
							tvHeight.setText("---");
							
						} else { 
							// data is valid

							// add values
							tvLat.setText(formatPos(driver.getInfo().latitude / 100.0));
							tvLon.setText(formatPos(driver.getInfo().longitude / 100.0));
							tvHeight.setText(formatHeight(driver.getInfo().height));
//							tvLat.setText(formatPos(47.637634)); // driver.getInfo().latitude / 100.0));
//							tvLon.setText(formatPos(9.330025)); //driver.getInfo().longitude / 100.0));
//							tvHeight.setText(formatHeight(395.23)); // driver.getInfo().height));
							
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
