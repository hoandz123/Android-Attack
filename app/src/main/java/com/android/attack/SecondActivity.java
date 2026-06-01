package com.android.attack;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Màn hình phụ: nếu overlay ImGui chặn touch, counter / nút / input dưới đây không phản hồi.
 */
public class SecondActivity extends Activity {

    private int tapCount;
    private int touchCount;
    private TextView counterView;
    private TextView touchLog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_second);

        counterView = findViewById(R.id.second_counter);
        touchLog = findViewById(R.id.second_touch_log);
        EditText input = findViewById(R.id.second_input);

        Button tapZone = findViewById(R.id.btn_tap_zone);
        Button action = findViewById(R.id.btn_second_input);
        Button back = findViewById(R.id.btn_back);

        tapZone.setOnClickListener(v -> {
            tapCount++;
            counterView.setText(getString(R.string.second_counter_fmt, tapCount));
        });

        action.setOnClickListener(v -> {
            String t = input.getText().toString().trim();
            Toast.makeText(this, getString(R.string.toast_second, t), Toast.LENGTH_SHORT).show();
        });

        back.setOnClickListener(v -> finish());
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            touchCount++;
            if (touchLog != null) {
                touchLog.setText(getString(R.string.touch_dispatch_fmt, touchCount,
                        event.getX(), event.getY()));
            }
        }
        return super.dispatchTouchEvent(event);
    }
}
