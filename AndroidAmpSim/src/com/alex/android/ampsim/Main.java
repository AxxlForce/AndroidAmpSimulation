package com.alex.android.ampsim;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

import com.alex.andoird.ampsim.R;

public class Main extends Activity {

	static AssetManager assetManager;
	private Context mcontext = this;
	
	static{
		
		// load c code
		System.loadLibrary("native");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		handleUI();
	}

	private void handleUI() {
		// initialize button click handlers
		((Button) findViewById(R.id.Start))
				.setOnClickListener(new OnClickListener() {
					public void onClick(View view) {

						createEngine();
						Toast.makeText(
								mcontext,
								"Latency: " + String.valueOf(getLatency())
										+ "ms", Toast.LENGTH_SHORT).show();
					}
				});

		((Button) findViewById(R.id.Shutdown))
				.setOnClickListener(new OnClickListener() {
					public void onClick(View view) {

						shutdown();
						createEngine();
					}
				});

		((Button) findViewById(R.id.Run))
				.setOnClickListener(new OnClickListener() {
					public void onClick(View view) {

						runEngine();
					}
				});

		((Button) findViewById(R.id.Reverb))
				.setOnClickListener(new OnClickListener() {

					boolean enabled = false;

					public void onClick(View view) {
						enabled = !enabled;
						if (!enableReverb(enabled)) {
							enabled = !enabled;
						}

					}
				});

		((Button) findViewById(R.id.Quit))
				.setOnClickListener(new OnClickListener() {
					public void onClick(View view) {
						shutdown();
						finish();
					}
				});
	}

	/** Called when the activity is about to be destroyed. */
	@Override
	protected void onPause() {
		super.onPause();
	}

	/** Called when the activity is about to be destroyed. */
	@Override
	protected void onDestroy() {
		shutdown();
		super.onDestroy();
	}

	/** Native methods, implemented in jni folder */
	public static native void createEngine();

	public static native void runEngine();

	public static native float getLatency();

	public static native boolean enableReverb(boolean enabled);

	public static native void shutdown();
}
