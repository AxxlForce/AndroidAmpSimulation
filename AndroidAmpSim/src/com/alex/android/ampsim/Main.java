package com.alex.android.ampsim;

import com.alex.andoird.ampsim.R;

import android.os.Bundle;
import android.app.Activity;
import android.content.res.AssetManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

public class Main extends Activity {

	static AssetManager assetManager;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		try {

			System.loadLibrary("native");

		} catch (Exception e) {

			Toast.makeText(this, "lib loaded!", Toast.LENGTH_SHORT).show();
		}
		Toast.makeText(this, "lib loaded!", Toast.LENGTH_SHORT).show();

		// initialize button click handlers
		((Button) findViewById(R.id.Run))
				.setOnClickListener(new OnClickListener() {
					public void onClick(View view) {

						// ignore the return value
						mainJob();
					}
				});
		
		((Button) findViewById(R.id.Stop))
		.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {

				// ignore the return value
				shutdown();
			}
		});
	}

	private void mainJob() {

		assetManager = getAssets();

		// initialize native audio system
		startAudioLoopback();

		// // initialize button click handlers
		// ((Button) findViewById(R.id.hello)).setOnClickListener(new
		// OnClickListener() {
		// public void onClick(View view) {
		// // ignore the return value
		// selectClip(CLIP_HELLO, 5);
		// }
		// });
		//
		// ((Button) findViewById(R.id.android)).setOnClickListener(new
		// OnClickListener() {
		// public void onClick(View view) {
		// // ignore the return value
		// selectClip(CLIP_ANDROID, 7);
		// }
		// });
		//
		// ((Button) findViewById(R.id.sawtooth)).setOnClickListener(new
		// OnClickListener() {
		// public void onClick(View view) {
		// // ignore the return value
		// selectClip(CLIP_SAWTOOTH, 1);
		// }
		// });
		//
		//
		// ((Button) findViewById(R.id.record)).setOnClickListener(new
		// OnClickListener() {
		// boolean created = false;
		// public void onClick(View view) {
		// if (!created) {
		// created = createAudioRecorder();
		// }
		// if (created) {
		// startRecording();
		// }
		// }
		// });
		//
		// ((Button) findViewById(R.id.playback)).setOnClickListener(new
		// OnClickListener() {
		// public void onClick(View view) {
		// // ignore the return value
		// selectClip(CLIP_PLAYBACK, 3);
		// }
		// });
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
	public static native void startAudioLoopback();
	public static native boolean enableReverb(boolean enabled);
	public static native void shutdown();
}
