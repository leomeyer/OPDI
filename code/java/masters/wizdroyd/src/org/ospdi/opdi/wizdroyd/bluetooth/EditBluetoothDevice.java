package org.ospdi.opdi.wizdroyd.bluetooth;

import org.ospdi.opdi.wizdroyd.R;
import org.ospdi.opdi.wizdroyd.Wizdroyd;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Toast;

/** This class implements an activity to edit a Bluetooth device's settings. 
 * */
public class EditBluetoothDevice extends Activity {

    private BluetoothAdapter mBluetoothAdapter;

	private EditText etAddress;
	private EditText etName;
	private EditText etPSK;
	private EditText etPIN;
	private CheckBox cbSecure;    
    
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.edit_bluetooth_device);

        // Get local Bluetooth adapter
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        // If the adapter is null, then Bluetooth is not supported
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, R.string.bluetooth_not_supported, Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
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
		BluetoothDevice device = null;
		
		try {
			device = new BluetoothDevice(originalDeviceSerialization);
		}
		catch (Exception e) {
			// can't deserialize - invalid data
			finish();
			return;
		}

		etAddress = (EditText)findViewById(R.id.etDeviceAddress);
		etName = (EditText)findViewById(R.id.etDeviceName);
		etPSK = (EditText)findViewById(R.id.etPSK);
		etPIN = (EditText)findViewById(R.id.etDevicePIN);
		cbSecure = (CheckBox)findViewById(R.id.cbSecure);
		
		etAddress.setText(device.getBluetoothAddress());
		etName.setText(device.getName());
		etPSK.setText(device.getEncryptionKey());
		etPIN.setText(device.getPIN());
		cbSecure.setChecked(device.isSecure());
        
        // register button click handlers
        Button btnSave = (Button)findViewById(R.id.btnSave);
        btnSave.setOnClickListener(new View.OnClickListener() {			
			@Override
			public void onClick(View v) {
				
				// validate
				String address = etAddress.getText().toString().trim().toUpperCase();
				// check bluetooth address
				if (!BluetoothAdapter.checkBluetoothAddress(address)) {
					// invalid address
					Toast.makeText(EditBluetoothDevice.this, R.string.bluetooth_invalid_address, Toast.LENGTH_SHORT).show();
					return;
				}
				
				// make a new device object
				BluetoothDevice resultDevice = new BluetoothDevice(mBluetoothAdapter, etName.getText().toString(), address, 
						etPSK.getText().toString(), etPIN.getText().toString(), cbSecure.isChecked());
				
				// create the device
				BluetoothDevice device = new BluetoothDevice(mBluetoothAdapter, etName.getText().toString(), address, etPSK.getText().toString(), etPIN.getText().toString(), cbSecure.isChecked());
	            // Set result and finish this Activity
        		Intent result = new Intent();
        		result.putExtra(Wizdroyd.DEVICE_SERIALIZATION, device.serialize());
        		result.putExtra(Wizdroyd.DEVICE_PREVIOUS_SERIALIZATION, originalDeviceSerialization);
	            setResult(Wizdroyd.EDIT_BLUETOOTH_DEVICE, result);
	            
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
