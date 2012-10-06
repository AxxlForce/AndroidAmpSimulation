package com.alex.testing;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends Activity {

	static {
		System.loadLibrary("native");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.activity_main, menu);
		return true;
	}

	public void Button_onClick(View view) {

		TextView textView = (TextView) findViewById(R.id.textView1);
		long result = invokeNativeFunction();
		textView.setText(String.valueOf(result));
	}

	native private long invokeNativeFunction();
}
