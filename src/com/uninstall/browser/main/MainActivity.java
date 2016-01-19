package com.uninstall.browser.main;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;

public class MainActivity extends Activity{
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
	}
}
