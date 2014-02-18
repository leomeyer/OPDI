package org.ospdi.opdi.wizdroyd.tcpip;

import org.ospdi.opdi.wizdroyd.R;
import org.ospdi.opdi.wizdroyd.Wizdroyd;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

/** This class implements an activity to edit a Bluetooth device's settings. 
 * */
public class EditTCPIPDevice extends Activity {

	private EditText etName;
	private EditText etAddress;
	private EditText etPort;
	private EditText etPSK;

	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.edit_tcpip_device);
        
        // Set result CANCELED in case the user backs out
        setResult(Activity.RESULT_CANCELED);

        Bundle extras = getIntent().getExtras();
		if (extras == null) {
			// no data? return
			finish();
			return;
		}

		final String originalDeviceSerialization = extras.getString(Wizdroyd.DEVICE_SERIALIZATION);
		if (originalDeviceSerialization == null) {
			finish();
			return;
		}
		
		// deserialize device from data
		TCPIPDevice device = null;
		
		try {
			device = new TCPIPDevice(originalDeviceSerialization);
		}
		catch (Exception e) {
			// can't deserialize - invalid data
			finish();
			return;
		}

        etName = (EditText)findViewById(R.id.etDeviceName);
		etAddress = (EditText)findViewById(R.id.etDeviceAddress);
		etPort = (EditText)findViewById(R.id.etDevicePort);
		etPSK = (EditText)findViewById(R.id.etPSK);
		
		etName.setText(device.getName());
		etAddress.setText(device.getTCPIPAddress());
		etPort.setText("" + device.getPort());
		etPSK.setText(device.getEncryptionKey() == null ? "" : device.getEncryptionKey());
        
        // register button click handlers
        Button btnSave = (Button)findViewById(R.id.btnSave);
        btnSave.setOnClickListener(new View.OnClickListener() {			
			@Override
			public void onClick(View v) {
				// make a new device object
				TCPIPDevice resultDevice = new TCPIPDevice(etName.getText().toString(), etAddress.getText().toString(), 
						Integer.parseInt(etPort.getText().toString()), etPSK.getText().toString());
				
	            // Set result and finish this Activity
        		Intent result = new Intent();
        		result.putExtra(Wizdroyd.DEVICE_SERIALIZATION, resultDevice.serialize());
        		result.putExtra(Wizdroyd.DEVICE_PREVIOUS_SERIALIZATION, originalDeviceSerialization);
	            setResult(Wizdroyd.EDIT_TCPIP_DEVICE, result);
	            
	            finish();
			}
		});
        
        Button btnCancel = (Button)findViewById(R.id.btnCancel);
        btnCancel.setOnClickListener(new View.OnClickListener() {	
        	@Override
        	public void onClick(View v) {
        		setResult(Activity.RESULT_CANCELED);
        		finish();
        	}
        });
    }
    
    @Override
    public void onStart() {
        super.onStart();
    }    
    
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        }
    }
}
