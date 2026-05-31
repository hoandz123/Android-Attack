package com.android.attack;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("MyLibName");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        int pad = (int) (16 * getResources().getDisplayMetrics().density);
        root.setPadding(pad, pad, pad, pad);
        root.setBackgroundColor(Color.parseColor("#FF1A1A2E"));
        TextView title = new TextView(this);
        title.setText("Android-Attack Base Layer");
        title.setTextColor(Color.WHITE);
        title.setTextSize(22f);
        title.setPadding(0, 0, 0, pad);
        root.addView(title);
        final TextView status = new TextView(this);
        status.setText("Tap buttons — touches outside ImGui window should reach here.");
        status.setTextColor(Color.parseColor("#FFCCCCCC"));
        status.setPadding(0, 0, 0, pad);
        root.addView(status);
        Button btnA = new Button(this);
        btnA.setText("Button A");
        btnA.setOnClickListener(v -> { status.setText("Button A clicked"); Toast.makeText(MainActivity.this, "Button A", Toast.LENGTH_SHORT).show(); });
        root.addView(btnA, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        Button btnB = new Button(this);
        btnB.setText("Button B");
        btnB.setOnClickListener(v -> { status.setText("Button B clicked"); Toast.makeText(MainActivity.this, "Button B", Toast.LENGTH_SHORT).show(); });
        root.addView(btnB, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        Button btnC = new Button(this);
        btnC.setText("Button C");
        btnC.setOnClickListener(v -> { status.setText("Button C clicked"); Toast.makeText(MainActivity.this, "Button C", Toast.LENGTH_SHORT).show(); });
        root.addView(btnC, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        setContentView(root);
    }
}
