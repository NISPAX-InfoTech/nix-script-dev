package com.nixscript.ide;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.text.Spannable;
import android.text.style.ForegroundColorSpan;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.TextView;

public class MainActivity extends Activity {
    private EditText editor;
    private TextView output, lineNumbers, status;
    private LinearLayout outputPanel;
    private int bg = Color.rgb(48,48,48), bar = Color.rgb(36,36,36), text = Color.rgb(215,215,215);
    private boolean applyingHighlight = false;
    private final int keywordColor = Color.rgb(224, 151, 91);
    private final int stringColor = Color.rgb(143, 194, 125);
    private final int numberColor = Color.rgb(129, 183, 231);
    private final int functionColor = Color.rgb(190, 169, 224);
    private final int commentColor = Color.rgb(139, 139, 139);

    private void highlightCode(Editable e) {
        if (applyingHighlight) return;
        applyingHighlight = true;
        ForegroundColorSpan[] old = e.getSpans(0, e.length(), ForegroundColorSpan.class);
        for (ForegroundColorSpan span : old) e.removeSpan(span);
        String source = e.toString();
        applyPattern(e, source, "\\\"[^\\\"\\n]*\\\"|'[^'\\n]*'", stringColor);
        applyPattern(e, source, "//[^\\n]*", commentColor);
        applyPattern(e, source, "\\b(nix|fn|main|var|class|public|private|protected|static|return|if|elif|else|for|repeat|while|true|false|null|this|use|new|delete)\\b", keywordColor);
        applyPattern(e, source, "\\b(print|uin|sqrt|sq|pow|abs|floor|ceil|round|isUpper|isLower|toUpper|toLower|isSpace)\\b", functionColor);
        applyPattern(e, source, "\\b\\d+(?:\\.\\d+)?\\b", numberColor);
        applyingHighlight = false;
    }

    private void applyPattern(Editable e, String source, String regex, int color) {
        Matcher m = Pattern.compile(regex).matcher(source);
        while (m.find()) e.setSpan(new ForegroundColorSpan(color), m.start(), m.end(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    }

    private void moveCursor(String direction) {
        int start = Math.max(0, editor.getSelectionStart());
        int end = Math.max(start, editor.getSelectionEnd());
        int pos = end;
        String source = editor.getText().toString();
        if (direction.equals("left")) pos = Math.max(0, start - 1);
        else if (direction.equals("right")) pos = Math.min(source.length(), end + 1);
        else {
            int lineStart = source.lastIndexOf('\n', Math.max(0, pos - 1)) + 1;
            int column = pos - lineStart;
            int lineEnd = source.indexOf('\n', pos);
            if (lineEnd < 0) lineEnd = source.length();
            if (direction.equals("up") && lineStart > 0) {
                int previousEnd = lineStart - 1;
                int previousStart = source.lastIndexOf('\n', Math.max(0, previousEnd - 1)) + 1;
                pos = Math.min(previousStart + column, previousEnd);
            } else if (direction.equals("down") && lineEnd < source.length()) {
                int nextStart = lineEnd + 1;
                int nextEnd = source.indexOf('\n', nextStart);
                if (nextEnd < 0) nextEnd = source.length();
                pos = Math.min(nextStart + column, nextEnd);
            }
        }
        editor.setSelection(Math.max(0, Math.min(pos, source.length())));
        editor.requestFocus();
    }

    static { System.loadLibrary("nixscript"); }
    private static native String runNixScript(String source);

    private int dp(float n) { return (int)(n * getResources().getDisplayMetrics().density + 0.5f); }
    private LinearLayout column() { LinearLayout v = new LinearLayout(this); v.setOrientation(LinearLayout.VERTICAL); return v; }
    private TextView label(String s, float size, int color) {
        TextView v = new TextView(this); v.setText(s); v.setTextSize(size); v.setTextColor(color); return v;
    }
    private Button action(String s) {
        Button b = new Button(this); b.setText(s); b.setTextSize(15); b.setTextColor(Color.WHITE);
        b.setAllCaps(false); b.setBackgroundColor(Color.TRANSPARENT); return b;
    }
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(bar);
        getWindow().setNavigationBarColor(bar);
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);

        LinearLayout root = column();
        root.setBackgroundColor(bg);
        root.setFocusableInTouchMode(true);

        LinearLayout header = new LinearLayout(this);
        header.setGravity(Gravity.CENTER_VERTICAL);
        header.setPadding(dp(18), dp(4), dp(12), dp(4));
        header.setBackgroundColor(bar);
        TextView brand = label("NixScript", 25, Color.WHITE);
        brand.setTypeface(Typeface.DEFAULT, Typeface.NORMAL);
        header.addView(brand, new LinearLayout.LayoutParams(0, -2, 1));
        Button run = action("RUN");
        Button menu = action("MENU");
        header.addView(run, new LinearLayout.LayoutParams(-2, dp(54)));
        header.addView(menu, new LinearLayout.LayoutParams(-2, dp(54)));
        root.addView(header, new LinearLayout.LayoutParams(-1, dp(68)));

        status = label("Ready", 13, Color.LTGRAY);
        status.setPadding(dp(18), dp(5), dp(18), dp(5));
        root.addView(status);

        LinearLayout editorRow = new LinearLayout(this);
        editorRow.setGravity(Gravity.TOP);
        editorRow.setBackgroundColor(bg);
        lineNumbers = label("1", 16, Color.rgb(145,145,145));
        lineNumbers.setTypeface(Typeface.MONOSPACE);
        lineNumbers.setGravity(Gravity.TOP | Gravity.RIGHT);
        lineNumbers.setPadding(0, dp(7), dp(8), 0);
        editorRow.addView(lineNumbers, new LinearLayout.LayoutParams(dp(42), -1));

        editor = new EditText(this);
        editor.setGravity(Gravity.TOP | Gravity.START);
        editor.setTypeface(Typeface.MONOSPACE);
        editor.setTextSize(16);
        editor.setTextColor(text);
        editor.setHintTextColor(Color.GRAY);
        editor.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        editor.setSingleLine(false);
        editor.setHorizontallyScrolling(true);
        editor.setPadding(dp(8), dp(7), dp(8), dp(24));
        editor.setBackgroundColor(Color.rgb(48,48,48));
        editor.setText("nix main() {\\n    print(\\\"Hello from NixScript!\\\");\\n    var answer = 40 + 2;\\n    print(answer);\\n}\\n");
        editorRow.addView(editor, new LinearLayout.LayoutParams(0, -1, 1));
        root.addView(editorRow, new LinearLayout.LayoutParams(-1, 0, 1));

        editor.addTextChangedListener(new TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {
                int lines = Math.max(1, s.toString().split("\\\\n", -1).length);
                StringBuilder nums = new StringBuilder();
                for (int i = 1; i <= lines; i++) { if (i > 1) nums.append('\n'); nums.append(i); }
                lineNumbers.setText(nums.toString());
                status.setText("Edited");
            }
            @Override public void afterTextChanged(Editable s) { highlightCode(s); }
        });

        editor.setOnKeyListener((v, keyCode, event) -> {
            if (event.getAction() != android.view.KeyEvent.ACTION_DOWN) return false;
            if (keyCode == android.view.KeyEvent.KEYCODE_ENTER) {
                int cursor = Math.max(0, editor.getSelectionStart());
                int selectionEnd = Math.max(cursor, editor.getSelectionEnd());
                String source = editor.getText().toString();
                int lineStart = source.lastIndexOf('\n', Math.max(0, cursor - 1)) + 1;
                String before = source.substring(lineStart, cursor);
                String indent = before.substring(0, before.length() - before.stripLeading().length());
                if (before.trim().endsWith("{")) indent += "    ";
                editor.getText().replace(cursor, selectionEnd, "\\n" + indent);
                return true;
            }
            return false;
        });

        Button runWide = action("▶  RUN NIXSCRIPT");
        runWide.setTextSize(17);
        runWide.setBackgroundColor(Color.rgb(88,88,90));
        runWide.setPadding(0, dp(8), 0, dp(8));
        LinearLayout.LayoutParams runParams = new LinearLayout.LayoutParams(-1, dp(54));
        runParams.setMargins(dp(18), dp(8), dp(18), dp(8));
        root.addView(runWide, runParams);

        TextView outputTitle = label("OUTPUT", 15, Color.rgb(120,190,255));
        outputTitle.setTypeface(Typeface.DEFAULT_BOLD);
        outputTitle.setPadding(dp(18), dp(4), dp(18), dp(4));
        root.addView(outputTitle);

        outputPanel = column();
        outputPanel.setPadding(dp(10), dp(8), dp(10), dp(8));
        outputPanel.setBackgroundColor(Color.rgb(35,37,43));
        output = label("Run your program to see output here.", 14, Color.rgb(210,220,210));
        outputPanel.addView(output);
        LinearLayout.LayoutParams outParams = new LinearLayout.LayoutParams(-1, -2);
        outParams.setMargins(dp(18), 0, dp(18), dp(10));
        root.addView(outputPanel, outParams);

        LinearLayout symbols = new LinearLayout(this);
        symbols.setOrientation(LinearLayout.VERTICAL);
        symbols.setBackgroundColor(bar);
        String[][] rows = {{"Tab","{}","\\\"\\\"",";","()","⇧","↻"},{"=","|","&",",","←","↓","→"}};
        for (String[] row : rows) {
            LinearLayout symbolRow = new LinearLayout(this);
            for (String symbol : row) {
                Button b = action(symbol);
                b.setTextColor(Color.LTGRAY);
                b.setTextSize(15);
                symbolRow.addView(b, new LinearLayout.LayoutParams(0, dp(38), 1));
                b.setOnClickListener(v -> {
                    if (symbol.equals("⇧")) { moveCursor("up"); return; }
                    if (symbol.equals("↓")) { moveCursor("down"); return; }
                    if (symbol.equals("←")) { moveCursor("left"); return; }
                    if (symbol.equals("→")) { moveCursor("right"); return; }
                    if (symbol.equals("↻")) { highlightCode(editor.getText()); return; }
                    String insert = symbol.equals("Tab") ? "    " : symbol.equals("&") ? "&" : symbol;
                    int start = Math.max(0, editor.getSelectionStart());
                    int end = Math.max(start, editor.getSelectionEnd());
                    editor.getText().replace(start, end, insert);
                    editor.requestFocus();
                });
            }
            symbols.addView(symbolRow);
        }
        root.addView(symbols);

        run.setOnClickListener(v -> execute());
        runWide.setOnClickListener(v -> execute());
        menu.setOnClickListener(v -> {
            PopupMenu popup = new PopupMenu(this, menu);
            popup.getMenu().add("New file").setOnMenuItemClickListener(item -> { editor.setText("nix main() {\\n    \\n}\\n"); status.setText("New file"); return true; });
            popup.getMenu().add("Clear output").setOnMenuItemClickListener(item -> { output.setText(""); return true; });
            popup.getMenu().add("About NixScript").setOnMenuItemClickListener(item -> { output.setText("NixScript IDE 0.1.1\\nWrite and run .ns programs on your device."); return true; });
            popup.show();
        });

        setContentView(root);
    }

    private void execute() {
        status.setText("Running…");
        String result = runNixScript(editor.getText().toString());
        output.setText(result == null || result.isEmpty() ? "Program finished successfully." : result);
        status.setText("Finished");
        ((View)outputPanel.getParent()).requestLayout();
    }
}
