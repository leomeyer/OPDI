package org.ospdi.opdi.androPDI;

import org.ospdi.opdi.androPDI.R;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.webkit.WebView;

public class HelpActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_help);
		
		WebView browser = (WebView)findViewById(R.id.webView);
//		browser.getSettings().setJavaScriptEnabled(true);
		
		String language = getResources().getConfiguration().locale.getLanguage();
		
		// load language specific help
		try {
			browser.loadUrl("file:///android_asset/about-" + language + ".html");
		}
		catch (Exception e) {
			// fallback to default
			browser.loadUrl("file:///android_asset/about.html");
		}
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.help, menu);
		return true;
	}

}
