package com.android.attack.nativedex;

import android.app.Activity;
import android.content.Context;
import android.text.Editable;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;

/** IME → ImGui (composition + hardware shortcut). minSdk 26. */
public final class KeyboardInputBridge {

    private static final String TAG = "ATTACK_Keyboard";
    private static final int INPUT_TEXT =
            InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
    private static final int META_CHORD =
            KeyEvent.META_CTRL_ON | KeyEvent.META_CTRL_LEFT_ON | KeyEvent.META_CTRL_RIGHT_ON
                    | KeyEvent.META_ALT_ON | KeyEvent.META_ALT_LEFT_ON | KeyEvent.META_ALT_RIGHT_ON;

    private static volatile Activity attachedActivity;
    private static ImeSink imeSink;
    private static boolean imeVisible;
    private static boolean imeMultiline;
    private static boolean ctrlDown;
    private static boolean altDown;

    private KeyboardInputBridge() {}

    public static void install(Activity activity) {
        if (activity == null) return;
        if (attachedActivity == activity && imeSink != null && imeSink.getParent() != null) return;
        if (attachedActivity != null) uninstall(attachedActivity);
        attachedActivity = activity;
        try {
            ViewGroup root = (ViewGroup) activity.getWindow().getDecorView();
            imeSink = new ImeSink(activity);
            imeSink.setInputType(INPUT_TEXT);
            imeSink.setAlpha(0f);
            imeSink.setFocusable(true);
            imeSink.setFocusableInTouchMode(true);
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN) {
                imeSink.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            }
            root.addView(imeSink, new FrameLayout.LayoutParams(1, 1));
            Log.i(TAG, "install ok");
        } catch (Throwable t) {
            Log.e(TAG, "install", t);
            uninstall(activity);
        }
    }

    public static void uninstall(Activity activity) {
        if (activity != null && attachedActivity != activity) return;
        syncIme(false);
        ctrlDown = altDown = false;
        if (imeSink != null) {
            ViewGroup p = (ViewGroup) imeSink.getParent();
            if (p != null) p.removeView(imeSink);
            imeSink = null;
        }
        attachedActivity = null;
    }

    public static void syncIme(boolean want) {
        syncIme(want, false);
    }

    public static void syncIme(boolean want, boolean multiline) {
        Activity a = attachedActivity;
        if (a == null || imeSink == null) return;
        final boolean modeChanged = multiline != imeMultiline;
        imeMultiline = multiline;
        if (!want) {
            if (!imeVisible) return;
            a.runOnUiThread(() -> applyImeOnUi(false));
            return;
        }
        if (imeVisible && !modeChanged) return;
        a.runOnUiThread(() -> {
            if (imeVisible && modeChanged) applyImeOnUi(false);
            applyImeOnUi(true);
        });
    }

    private static void applyImeInputType() {
        if (imeSink == null) return;
        int type = INPUT_TEXT;
        if (imeMultiline) type |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
        imeSink.setInputType(type);
    }

    private static void applyImeOnUi(boolean want) {
        if (attachedActivity == null || imeSink == null) return;
        try {
            InputMethodManager imm = (InputMethodManager) attachedActivity.getSystemService(
                    Context.INPUT_METHOD_SERVICE);
            if (imm == null) return;
            if (want) {
                applyImeInputType();
                imeSink.requestFocus();
                imm.showSoftInput(imeSink, InputMethodManager.SHOW_IMPLICIT);
                imeVisible = true;
            } else {
                imm.hideSoftInputFromWindow(imeSink.getWindowToken(), 0);
                imeSink.clearFocus();
                imeVisible = false;
            }
        } catch (Throwable t) {
            Log.w(TAG, "syncIme", t);
        }
    }

    static void feedKeyEvent(KeyEvent event) {
        if (event == null) return;
        int action = event.getAction();
        if (action != KeyEvent.ACTION_DOWN && action != KeyEvent.ACTION_UP) return;
        int code = event.getKeyCode();
        boolean down = action == KeyEvent.ACTION_DOWN;
        if (code == KeyEvent.KEYCODE_CTRL_LEFT || code == KeyEvent.KEYCODE_CTRL_RIGHT) {
            ctrlDown = down;
        } else if (code == KeyEvent.KEYCODE_ALT_LEFT || code == KeyEvent.KEYCODE_ALT_RIGHT) {
            altDown = down;
        }
        int meta = event.getMetaState();
        if (imeVisible && !allowWhileIme(code, meta)) return;
        int unicode = 0;
        if (down && !isModifier(code) && (meta & META_CHORD) == 0 && !ctrlDown && !altDown) {
            unicode = event.getUnicodeChar(meta);
        }
        nativeOnKey(code, action, meta, unicode);
    }

    private static boolean isLetterOrDigit(int code) {
        return (code >= KeyEvent.KEYCODE_0 && code <= KeyEvent.KEYCODE_9)
                || (code >= KeyEvent.KEYCODE_A && code <= KeyEvent.KEYCODE_Z);
    }

    private static boolean isModifier(int code) {
        return code == KeyEvent.KEYCODE_SHIFT_LEFT || code == KeyEvent.KEYCODE_SHIFT_RIGHT
                || code == KeyEvent.KEYCODE_CTRL_LEFT || code == KeyEvent.KEYCODE_CTRL_RIGHT
                || code == KeyEvent.KEYCODE_ALT_LEFT || code == KeyEvent.KEYCODE_ALT_RIGHT
                || code == KeyEvent.KEYCODE_META_LEFT || code == KeyEvent.KEYCODE_META_RIGHT;
    }

    private static boolean allowWhileIme(int code, int meta) {
        if (isModifier(code)) return true;
        if ((ctrlDown || altDown || (meta & META_CHORD) != 0) && isLetterOrDigit(code)) {
            return true;
        }
        switch (code) {
            case KeyEvent.KEYCODE_DEL:
            case KeyEvent.KEYCODE_FORWARD_DEL:
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_TAB:
            case KeyEvent.KEYCODE_ESCAPE:
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_MOVE_HOME:
            case KeyEvent.KEYCODE_MOVE_END:
            case KeyEvent.KEYCODE_PAGE_UP:
            case KeyEvent.KEYCODE_PAGE_DOWN:
                return true;
            default:
                return false;
        }
    }

    private static final class ImeSink extends EditText {
        private final Editable imeBuffer = Editable.Factory.getInstance().newEditable("");

        ImeSink(Context context) {
            super(context);
        }

        @Override
        public InputConnection onCreateInputConnection(EditorInfo out) {
            if (out != null) {
                int type = INPUT_TEXT;
                if (imeMultiline) type |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
                out.inputType = type;
                out.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI
                        | (imeMultiline ? EditorInfo.IME_FLAG_NO_ENTER_ACTION : 0);
            }
            return new ImGuiIc(this, imeBuffer);
        }
    }

    private static final class ImGuiIc extends BaseInputConnection {
        private int composingChars;
        private final Editable buffer;

        ImGuiIc(ImeSink v, Editable buffer) {
            super(v, false);
            this.buffer = buffer;
        }

        @Override
        public Editable getEditable() {
            buffer.clear();
            buffer.clearSpans();
            return buffer;
        }

        @Override
        public boolean setComposingText(CharSequence text, int newCursorPosition) {
            String s = text != null ? text.toString() : "";
            nativeOnReplaceTail(composingChars, s);
            composingChars = s.length();
            return true;
        }

        @Override
        public boolean finishComposingText() {
            composingChars = 0;
            return true;
        }

        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
            if (text == null) return true;
            String s = text.toString();
            if (s.isEmpty()) return true;
            if (composingChars > 0) {
                nativeOnReplaceTail(composingChars, s);
                composingChars = 0;
            } else {
                nativeOnTextUtf8(s);
            }
            return true;
        }

        @Override
        public boolean deleteSurroundingText(int beforeLength, int afterLength) {
            if (beforeLength > 0) {
                nativeOnReplaceTail(beforeLength, "");
                composingChars = Math.max(0, composingChars - beforeLength);
            }
            for (int i = 0; i < afterLength; i++) {
                nativeOnKey(KeyEvent.KEYCODE_FORWARD_DEL, KeyEvent.ACTION_DOWN, 0, 0);
                nativeOnKey(KeyEvent.KEYCODE_FORWARD_DEL, KeyEvent.ACTION_UP, 0, 0);
            }
            return true;
        }
    }

    private static native void nativeOnKey(int keyCode, int action, int metaState, int unicodeChar);

    private static native void nativeOnTextUtf8(String text);

    private static native void nativeOnReplaceTail(int deleteChars, String insertUtf8);
}
