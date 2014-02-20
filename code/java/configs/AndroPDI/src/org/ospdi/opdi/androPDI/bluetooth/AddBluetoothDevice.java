package org.ospdi.opdi.androPDI.bluetooth;

import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.androPDI.R;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Toast;

/** This class implements an activity to add a Bluetooth device. 
 * Partly adapted from the Android BluetoothChat sample.
 * */
public class AddBluetoothDevice extends Activity {

    private BluetoothAdapter mBluetoothAdapter;

    // Intent request codes
//    private static final int REQUEST_CONNECT_DEVICE_SECURE = 1;
//    private static final int REQUEST_CONNECT_DEVICE_INSECURE = 2;
    public static final int REQUEST_ENABLE_BT = 15433622;
    public static final int REQUEST_LIST_DEVICES = 9885567;
    
	private EditText etAddress;
	private EditText etName;
	private EditText etPSK;
	private EditText etPIN;
	private CheckBox cbSecure;    
    
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.add_bluetooth_device);
        
        // Set result CANCELED in case the user backs out
        setResult(Activity.RESULT_CANCELED);

        // Get local Bluetooth adapter
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        // If the adapter is null, then Bluetooth is not supported
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, R.string.bluetooth_not_supported, Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
		etAddress = (EditText)findViewById(R.id.etDeviceAddress);
		etName = (EditText)findViewById(R.id.etDeviceName);
		etPSK = (EditText)findViewById(R.id.etPSK);
		etPIN = (EditText)findViewById(R.id.etDevicePIN);
		cbSecure = (CheckBox)findViewById(R.id.cbSecure);
		
		Bundle extras = getIntent().getExtras();
		if (extras != null) {
			String address = extras.getString(AndroPDI.BT_ADDRESS);
			if (address == null) address = "";
			String name = extras.getString(AndroPDI.BT_NAME);
			if (name == null) name = address;
			String psk = extras.getString(AndroPDI.DEVICE_PSK);
			if (psk == null) psk = "";
			String pin = extras.getString(AndroPDI.BT_PIN);
			if (pin == null) pin = "";
			boolean secure = extras.getBoolean(AndroPDI.BT_SECURE, false);
			
			etAddress.setText(address);
			etName.setText(name);
			etPSK.setText(psk);
			etPIN.setText(pin);
			cbSecure.setChecked(secure);
		}
        
        // register button click handlers
        Button btnListDevices = (Button)findViewById(R.id.btnListDevices);
        btnListDevices.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				// start activity to list and select devices
	            Intent listIntent = new Intent(AddBluetoothDevice.this, DeviceListActivity.class);
	            startActivityForResult(listIntent, REQUEST_LIST_DEVICES);
			}
		});
        
        Button btnAddDevice = (Button)findViewById(R.id.btnAddDevice);
        btnAddDevice.setOnClickListener(new View.OnClickListener() {			
			@Override
			public void onClick(View v) {
				
				// validate
				String address = etAddress.getText().toString().trim().toUpperCase();
				// check bluetooth address
				if (!BluetoothAdapter.checkBluetoothAddress(address)) {
					// invalid address
					Toast.makeText(AddBluetoothDevice.this, R.string.bluetooth_invalid_address, Toast.LENGTH_SHORT).show();
					return;
				}
				
				// create the device
				BluetoothDevice device = new BluetoothDevice(mBluetoothAdapter, etName.getText().toString(), address, etPSK.getText().toString().trim(), etPIN.getText().toString(), cbSecure.isChecked());
	            // Set result and finish this Activity
        		Intent result = new Intent();
        		result.putExtra(AndroPDI.DEVICE_SERIALIZATION, device.serialize());
	            setResult(AndroPDI.ADD_BLUETOOTH_DEVICE, result);
	            
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

        // If BT is not on, request that it be enabled.
        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        } 
    }    
    
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        case REQUEST_ENABLE_BT:
            // When the request to enable Bluetooth returns
            if (resultCode == Activity.RESULT_OK) {
                // Bluetooth is now enabled
            } else {
                // User did not enable Bluetooth or an error occurred
                Toast.makeText(this, R.string.bt_not_enabled_leaving, Toast.LENGTH_SHORT).show();
                finish();
            }
            break;
        case REQUEST_LIST_DEVICES:
        	// has a device been selected?
        	if (resultCode == Activity.RESULT_OK) {
        		// get the device information into the text fields
        		String address = data.getStringExtra(DeviceListActivity.EXTRA_DEVICE_ADDRESS);
        		if (address != null)
        			etAddress.setText(address);
        		else
        			etAddress.setText("");

        		String name = data.getStringExtra(DeviceListActivity.EXTRA_DEVICE_NAME);
        		if (name != null)
        			etName.setText(name);
        		else
        			etName.setText("");
        	}
        	break;
        }
    }
}
