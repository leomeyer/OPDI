package org.ospdi.opdi.androPDI.gui;

import org.ospdi.opdi.androPDI.DeviceManager;
import org.ospdi.opdi.androPDI.R;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.ScrollView;
import android.widget.TextView;

/** An activity that implements basic logging functionality.
 * Expects that onCreate methods load a view that contains a ScrollView svLog and a TextView svLog.
 * 
 * @author leo.meyer
 *
 */
public abstract class LoggingActivity extends Activity {
	
	protected ScrollView svLog;
	protected TextView tvLog;
    
	DeviceManager deviceManager;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		super.onCreate(savedInstanceState);
        
		deviceManager = DeviceManager.getInstance();
	}
	
	@Override
	protected void onStart() {
		super.onStart();
		
        tvLog = (TextView)findViewById(R.id.tvLog);
        if (tvLog != null)
        	tvLog.setText(deviceManager.getLogText());
        svLog = (ScrollView)findViewById(R.id.svLog);
        if (svLog != null)
        	svLog.fullScroll(View.FOCUS_DOWN);
	}
	
	protected void addLogMessage(final String message) {
        if (tvLog != null) {
			tvLog.append(message);
			tvLog.append("\n");
        }
        if (svLog != null)
        	svLog.fullScroll(View.FOCUS_DOWN);
		
		deviceManager.addLogText(message + "\n");
	}
}
