package com.alex.testing;

import android.os.Bundle;
import android.app.Activity;
import android.content.res.AssetManager;
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
	
	private void handleAudio()
	{
		 assetManager = getAssets();

	        // initialize native audio system

	        createEngine();
	        createBufferQueueAudioPlayer();

	        // initialize button click handlers

	        ((Button) findViewById(R.id.hello)).setOnClickListener(new OnClickListener() {
	            public void onClick(View view) {
	                // ignore the return value
	                selectClip(CLIP_HELLO, 5);
	            }
	        });

	        ((Button) findViewById(R.id.android)).setOnClickListener(new OnClickListener() {
	            public void onClick(View view) {
	                // ignore the return value
	                selectClip(CLIP_ANDROID, 7);
	            }
	        });

	        ((Button) findViewById(R.id.sawtooth)).setOnClickListener(new OnClickListener() {
	            public void onClick(View view) {
	                // ignore the return value
	                selectClip(CLIP_SAWTOOTH, 1);
	            }
	        });

	        ((Button) findViewById(R.id.reverb)).setOnClickListener(new OnClickListener() {
	            boolean enabled = false;
	            public void onClick(View view) {
	                enabled = !enabled;
	                if (!enableReverb(enabled)) {
	                    enabled = !enabled;
	                }
	            }
	        });

	        ((Button) findViewById(R.id.embedded_soundtrack)).setOnClickListener(new OnClickListener() {
	            boolean created = false;
	            public void onClick(View view) {
	                if (!created) {
	                    created = createAssetAudioPlayer(assetManager, "background.mp3");
	                }
	                if (created) {
	                    isPlayingAsset = !isPlayingAsset;
	                    setPlayingAssetAudioPlayer(isPlayingAsset);
	                }
	            }
	        });

	        ((Button) findViewById(R.id.uri_soundtrack)).setOnClickListener(new OnClickListener() {
	            boolean created = false;
	            public void onClick(View view) {
	                if (!created) {
	                    created = createUriAudioPlayer(URI);
	                }
	                if (created) {
	                    isPlayingUri = !isPlayingUri;
	                    setPlayingUriAudioPlayer(isPlayingUri);
	                }
	             }
	        });

	        ((Button) findViewById(R.id.record)).setOnClickListener(new OnClickListener() {
	            boolean created = false;
	            public void onClick(View view) {
	                if (!created) {
	                    created = createAudioRecorder();
	                }
	                if (created) {
	                    startRecording();
	                }
	            }
	        });

	        ((Button) findViewById(R.id.playback)).setOnClickListener(new OnClickListener() {
	            public void onClick(View view) {
	                // ignore the return value
	                selectClip(CLIP_PLAYBACK, 3);
	            }
	        });
	}

	/** Called when the activity is about to be destroyed. */
    @Override
    protected void onDestroy()
    {
        shutdown();
        super.onDestroy();
    }
    
	  /** Native methods, implemented in jni folder */
    public static native void createEngine();
    public static native void createBufferQueueAudioPlayer();
    public static native boolean createAssetAudioPlayer(AssetManager assetManager, String filename);
    public static native void setPlayingAssetAudioPlayer(boolean isPlaying);
    public static native boolean createUriAudioPlayer(String uri);
    public static native void setPlayingUriAudioPlayer(boolean isPlaying);
    public static native boolean selectClip(int which, int count);
    public static native boolean enableReverb(boolean enabled);
    public static native boolean createAudioRecorder();
    public static native void startRecording();
    public static native void shutdown();
}
