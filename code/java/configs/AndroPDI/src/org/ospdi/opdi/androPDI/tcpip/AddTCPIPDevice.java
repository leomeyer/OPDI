package org.ospdi.opdi.androPDI.tcpip;

import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.androPDI.R;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

/** This class implements an activity to add a TCP/IP device.
*/
public class AddTCPIPDevice extends Activity {
	
	private EditText etName;
	private EditText etAddress;
	private EditText etPort;
	private EditText etPSK;
    
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.add_tcpip_device);
        
        // Set result CANCELED in case the user backs out
        setResult(Activity.RESULT_CANCELED);
        etName = (EditText)findViewById(R.id.etDeviceName);
		etAddress = (EditText)findViewById(R.id.etDeviceAddress);
		etPort = (EditText)findViewById(R.id.etDevicePort);
		etPort.setText("" + TCPIPDevice.STANDARD_PORT);
		etPSK = (EditText)findViewById(R.id.etPSK);
        
		Bundle extras = getIntent().getExtras();
		if (extras != null) {
			String host = extras.getString(AndroPDI.TCPIP_HOST);
			if (host == null) host = "";
			String name = extras.getString(AndroPDI.TCPIP_NAME);
			if (name == null) name = host;
			String port = extras.getString(AndroPDI.TCPIP_PORT);
			if (port == null) port = "";
			String psk = extras.getString(AndroPDI.DEVICE_PSK);
			if (psk == null) psk = "";
			
			etAddress.setText(host);
			etName.setText(name);
			etPort.setText(port);
			etPSK.setText(psk);
		}
        
        Button btnAddDevice = (Button)findViewById(R.id.btnAddDevice);
        btnAddDevice.setOnClickListener(new View.OnClickListener() {			
			@Override
			public void onClick(View v) {
				
				// validate
				String address = etAddress.getText().toString().trim();
				if (address.isEmpty()) {
					// invalid address
					Toast.makeText(AddTCPIPDevice.this, R.string.tcpip_invalid_address, Toast.LENGTH_SHORT).show();
					return;
				}
				
				int port = 0;
				try {
					port = Integer.parseInt(etPort.getText().toString());
				} catch (NumberFormatException nfe) {
					// invalid port number
					Toast.makeText(AddTCPIPDevice.this, R.string.tcpip_invalid_port, Toast.LENGTH_SHORT).show();
					return;
				}
				if (port < 0 || port > 65535) {
					// invalid port number
					Toast.makeText(AddTCPIPDevice.this, R.string.tcpip_invalid_port, Toast.LENGTH_SHORT).show();
					return;
				}
				String psk = etPSK.getText().toString().trim();
				
				// create the device
				TCPIPDevice device = new TCPIPDevice(etName.getText().toString(), etAddress.getText().toString(), port, psk);
	            // Set result and finish this Activity
        		Intent result = new Intent();
        		result.putExtra(AndroPDI.DEVICE_SERIALIZATION, device.serialize());
	            setResult(AndroPDI.ADD_TCPIP_DEVICE, result);

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
    }
}
