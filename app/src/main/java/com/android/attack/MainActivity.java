package com.android.attack;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.MotionEvent;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("loader");
    }

    private int counter;
    private int touchCount;
    private TextView touchLog;
    private EditText inputText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        touchLog = findViewById(R.id.touch_log);
        inputText = findViewById(R.id.input_text);

        Button btnSubmit = findViewById(R.id.btn_submit);
        Button btnCounter = findViewById(R.id.btn_counter);
        Button btnClear = findViewById(R.id.btn_clear);
        Button btnSecond = findViewById(R.id.btn_second);

        btnSubmit.setOnClickListener(v -> {
            String text = inputText.getText().toString().trim();
            Toast.makeText(this, getString(R.string.toast_submit, text), Toast.LENGTH_SHORT).show();
        });

        btnCounter.setOnClickListener(v -> {
            counter++;
            btnCounter.setText(getString(R.string.btn_counter_fmt, counter));
        });

        btnClear.setOnClickListener(v -> {
            counter = 0;
            touchCount = 0;
            btnCounter.setText(getString(R.string.btn_counter));
            inputText.setText("");
            updateTouchLog(getString(R.string.touch_cleared));
        });

        btnSecond.setOnClickListener(v ->
                startActivity(new Intent(this, SecondActivity.class)));
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            touchCount++;
            updateTouchLog(getString(R.string.touch_dispatch_fmt, touchCount,
                    event.getX(), event.getY()));
        }
        return super.dispatchTouchEvent(event);
    }

    private void updateTouchLog(String line) {
        if (touchLog != null) {
            touchLog.setText(line);
        }
    }
}
