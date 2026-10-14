package com.nixscript.ide;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.InputType;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class MainActivity extends Activity {
    private EditText editor;
    private TextView output;

    static {
        System.loadLibrary("nixscript");
    }

    private static native String runNixScript(String source);

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(Color.rgb(17, 19, 24));
        getWindow().setNavigationBarColor(Color.rgb(17, 19, 24));

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(18, 18, 18, 18);
        root.setBackgroundColor(Color.rgb(17, 19, 24));

        TextView title = new TextView(this);
        title.setText("NixScript IDE");
        title.setTextColor(Color.rgb(120, 190, 255));
        title.setTextSize(24);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        root.addView(title, new LinearLayout.LayoutParams(-1, -2));

        TextView subtitle = new TextView(this);
        subtitle.setText("Write .ns code • runs on-device");
        subtitle.setTextColor(Color.LTGRAY);
        subtitle.setPadding(0, 4, 0, 14);
        root.addView(subtitle, new LinearLayout.LayoutParams(-1, -2));

        editor = new EditText(this);
        editor.setGravity(android.view.Gravity.TOP | android.view.Gravity.START);
        editor.setTypeface(Typeface.MONOSPACE);
        editor.setTextSize(14);
        editor.setTextColor(Color.rgb(235, 238, 245));
        editor.setHintTextColor(Color.GRAY);
        editor.setHint("Write NixScript here...");
        editor.setInputType(InputType.TYPE_CLASS_TEXT |
                InputType.TYPE_TEXT_FLAG_MULTI_LINE |
                InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        editor.setSingleLine(false);
        editor.setPadding(12, 12, 12, 12);
        editor.setBackgroundColor(Color.rgb(29, 32, 40));
        editor.setText("nix main() {\n    print(\"Hello from NixScript on Android!\");\n    var answer = 40 + 2;\n    print(answer);\n}");
        LinearLayout.LayoutParams editorParams = new LinearLayout.LayoutParams(-1, 0, 1f);
        root.addView(editor, editorParams);

        Button run = new Button(this);
        run.setText("▶  Run NixScript");
        run.setOnClickListener(v -> output.setText(runNixScript(editor.getText().toString())));
        root.addView(run, new LinearLayout.LayoutParams(-1, -2));

        TextView outputTitle = new TextView(this);
        outputTitle.setText("OUTPUT");
        outputTitle.setTextColor(Color.rgb(120, 190, 255));
        outputTitle.setTypeface(Typeface.DEFAULT_BOLD);
        outputTitle.setPadding(0, 10, 0, 6);
        root.addView(outputTitle, new LinearLayout.LayoutParams(-1, -2));

        output = new TextView(this);
        output.setTypeface(Typeface.MONOSPACE);
        output.setTextSize(13);
        output.setTextColor(Color.rgb(210, 220, 210));
        output.setText("Run your program to see output here.");
        output.setPadding(12, 12, 12, 12);
        output.setBackgroundColor(Color.rgb(29, 32, 40));
        ScrollView outputScroll = new ScrollView(this);
        outputScroll.addView(output);
        root.addView(outputScroll, new LinearLayout.LayoutParams(-1, 130));

        setContentView(root);
    }
}
