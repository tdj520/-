package com.example.ndkdemo2;

public class Watcher {
    // Used to load the 'native-lib' library on application startup.

    static {
        System.loadLibrary("native-lib");
    }
    public native void createWatcher(String userId);
    public native void connectMonitor();
}
