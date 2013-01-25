package com.alex.android.ampsim;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

import com.alex.andoird.ampsim.jni.JNIWrapper;

public class Main extends Activity {

	static AssetManager assetManager;
	private Context mcontext = this;
	
	// buttons
	Button startButton, shutdownButton, runButton, reverbButton, quitButton;

	static {

		// load c code
		System.loadLibrary("native");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		setupButtonHandler();
	}

	/**
	 * gets references to buttons on view and implement onClick handler for them
	 */
	private void setupButtonHandler() {
		
		startButton = (Button) this.findViewById(R.id.Start);
		shutdownButton = (Button) this.findViewById(R.id.Shutdown);
		runButton = (Button) this.findViewById(R.id.Run);
		reverbButton = (Button) this.findViewById(R.id.Reverb);
		quitButton = (Button) this.findViewById(R.id.Quit);
		
		startButton.setOnClickListener(new OnClickListener() {

			public void onClick(View view) {

				JNIWrapper.createEngine();
				Toast.makeText(mcontext, "Latency: " + String.valueOf(JNIWrapper.getLatency()) + "ms", Toast.LENGTH_SHORT).show();
			}
		});

		shutdownButton.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {

				JNIWrapper.shutdown();
				JNIWrapper.createEngine();
			}
		});

		runButton.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {

				JNIWrapper.runEngine();
			}
		});

		reverbButton.setOnClickListener(new OnClickListener() {

			boolean enabled = false;

			public void onClick(View view) {
				enabled = !enabled;
				if (!JNIWrapper.enableReverb(enabled)) {
					enabled = !enabled;
				}

			}
		});
		
		quitButton.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
				JNIWrapper.shutdown();
				finish();
			}
		});
	}

	/**
	 * Called when the activity is about to be paused.
	 */
	@Override
	protected void onPause() {
		super.onPause();
	}

	/**
	 * Called when the activity is about to be destroyed.
	 */
	@Override
	protected void onDestroy() {
		JNIWrapper.shutdown();
		super.onDestroy();
	}
}
