package com.android.attack;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("loader");
    }

    private static native String stringFromNative();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ((TextView) findViewById(R.id.sample_text)).setText(stringFromNative());
    }
}
