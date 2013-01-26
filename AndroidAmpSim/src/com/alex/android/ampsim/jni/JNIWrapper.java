package com.alex.android.ampsim.jni;

public class JNIWrapper {
	
	/**
	 * Native methods, implemented in jni/native.c
	 */
	public static native void createEngine();

	public static native void runEngine();

	public static native float getLatency();

	public static native boolean enableReverb(boolean enabled);

	public static native void shutdown();
}
