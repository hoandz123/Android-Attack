package com.android.attack;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

import com.android.attack.loader.NativeLoader;

public class MainActivity extends Activity {
    static {
        NativeLoader.class.getName();
    }

    private static native String stringFromNative();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (!NativeLoader.loadFromDir(getApplicationInfo().nativeLibraryDir, "attack")) {
            throw new UnsatisfiedLinkError("loader failed to load libattack.so");
        }
        setContentView(R.layout.activity_main);
        ((TextView) findViewById(R.id.sample_text)).setText(stringFromNative());
    }
}
