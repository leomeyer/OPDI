package org.ospdi.opdi.androPDI.ports.editors;

import org.joda.time.LocalDate;
import org.joda.time.LocalDateTime;
import org.joda.time.LocalTime;
import org.joda.time.format.DateTimeFormat;
import org.ospdi.opdi.androPDI.R;

import android.app.DatePickerDialog;
import android.app.DatePickerDialog.OnDateSetListener;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.TimePickerDialog;
import android.app.TimePickerDialog.OnTimeSetListener;
import android.os.Bundle;
import android.text.format.DateFormat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.DatePicker;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.TimePicker;

public class DialPortDateTimeEditor extends DialogFragment {
	
	public interface DismissedListener {
		void dismissed(LocalDateTime dateTime);
	}

	class TimePickerDialogFragment extends DialogFragment implements OnTimeSetListener {
		
		LocalDateTime time;
		TextView textView;
		
	    public TimePickerDialogFragment(LocalDateTime time, TextView textView) {
			super();
			this.time = time;
			this.textView = textView;
		}

		@Override
	    public Dialog onCreateDialog(Bundle savedInstanceState) {
	        // Use the current time as the default values for the picker
	        int hour = time.getHourOfDay();
	        int minute = time.getMinuteOfHour();

	        // Create a new instance of TimePickerDialog and return it
	        return new TimePickerDialog(getActivity(), this, hour, minute, 
	                DateFormat.is24HourFormat(getActivity()));
	    }

	    public void onTimeSet(TimePicker view, int hourOfDay, int minute) {
	        time = new LocalDateTime(1970, 1, 1, hourOfDay, minute, 0);
	        textView.setText(DateTimeFormat.mediumTime().print(time));
	    }
	}
	
	class DatePickerDialogFragment extends DialogFragment implements OnDateSetListener {
		
		LocalDateTime date;
		TextView textView;
		
	    public DatePickerDialogFragment(LocalDateTime date, TextView textView) {
			super();
			this.date = date;
			this.textView = textView;
		}

		@Override
	    public Dialog onCreateDialog(Bundle savedInstanceState) {
	        // Use the current time as the default values for the picker
	        int year = date.getYear();
	        int month = date.getMonthOfYear();
	        int day = date.getDayOfMonth();

	        // Create a new instance of TimePickerDialog and return it
	        return new DatePickerDialog(getActivity(), this, year, month, day);
	    }

		@Override
		public void onDateSet(DatePicker view, int year, int monthOfYear, int dayOfMonth) {
			date = new LocalDateTime(year, monthOfYear, dayOfMonth, 0, 0, 0);
			textView.setText(DateTimeFormat.mediumDate().print(date));
		}
	}
	
	LocalDateTime initialDate;
	DismissedListener dismissedListener;
	
	TextView tvDate;
	TextView tvTime;
	Spinner spSelect;
	Button btnDate;
	Button btnTime;
	Button btnOk;
	Button btnCancel;
	TimePickerDialogFragment timePicker;
	DatePickerDialogFragment datePicker;
	
	public DialPortDateTimeEditor(LocalDateTime date, DismissedListener dismissedListener) {
		super();
		this.initialDate = date;
		this.dismissedListener = dismissedListener;
	}
	
	protected void setEventTime(int selectedPosition) {

		LocalDateTime dateTime = null;
		
		switch (selectedPosition) {
/*
		<item>Select&#8230;</item>
 		<item>In 30 seconds</item>
        <item>In one minute</item>
        <item>In five minutes</item>
        <item>In 15 minutes</item>
        <item>In one hour</item>
        <item>In two hours</item>
        <item>In six hours</item>
        <item>In one day</item>
 */
		case 1:	// 30 seconds
			dateTime = new LocalDateTime().plusSeconds(30); break;
		case 2:	// one minute
			dateTime = new LocalDateTime().plusMinutes(1); break;
		case 3:	// five minutes
			dateTime = new LocalDateTime().plusMinutes(5); break;
		case 4:	// 15 minutes
			dateTime = new LocalDateTime().plusMinutes(15); break;
		case 5:	// 30 minutes
			dateTime = new LocalDateTime().plusMinutes(30); break;
		case 6:	// one hour
			dateTime = new LocalDateTime().plusHours(1); break;
		case 7:	// two hours
			dateTime = new LocalDateTime().plusHours(2); break;
		case 8:	// six hours
			dateTime = new LocalDateTime().plusHours(6); break;
		case 9:	// one day
			dateTime = new LocalDateTime().plusDays(1); break;
		default:
			return;
		}
		
		tvDate.setText(DateTimeFormat.mediumDate().print(dateTime));
        tvTime.setText(DateTimeFormat.mediumTime().print(dateTime));
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View view = inflater.inflate(R.layout.date_time_editor, container);
        tvDate = (TextView)view.findViewById(R.id.date_time_editor_date);
        tvDate.setText(DateTimeFormat.mediumDate().print(initialDate));
        tvTime = (TextView)view.findViewById(R.id.date_time_editor_time);
        tvTime.setText(DateTimeFormat.mediumTime().print(initialDate));
        btnDate = (Button)view.findViewById(R.id.date_time_editor_select_date_button);
        btnTime = (Button)view.findViewById(R.id.date_time_editor_select_time_button);
        spSelect = (Spinner)view.findViewById(R.id.date_time_editor_spinner);
        btnOk = (Button)view.findViewById(R.id.date_time_editor_ok_button);
        btnCancel = (Button)view.findViewById(R.id.date_time_editor_cancel_button);

        getDialog().setTitle("Select date and time");
        
        btnTime.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				timePicker = new TimePickerDialogFragment(initialDate, tvTime);
			    timePicker.show(getFragmentManager(), "timePicker");
			}
		});
        
        btnDate.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				datePicker = new DatePickerDialogFragment(initialDate, tvDate);
			    datePicker.show(getFragmentManager(), "datePicker");
			}
		});
        
    	spSelect.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
				setEventTime(position);
			}

			@Override
			public void onNothingSelected(AdapterView<?> parent) {
			}
		});      
    	
        btnOk.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				// parse date
				LocalDate date = null;
				LocalTime time = null;
					date = DateTimeFormat.mediumDate().parseLocalDate(tvDate.getText().toString());
					String t = tvTime.getText().toString();
					/*
					// add seconds to time if they're missing
					if (t.split(":").length <= 2)
						t += ":00";
					*/
					time = DateTimeFormat.mediumTime().parseLocalTime(t);
				LocalDateTime result = date.toLocalDateTime(time);
				dismiss();
				dismissedListener.dismissed(result);
			}
		});

        btnCancel.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				dismiss();
			}
		});

        return view;
	}
	

}
