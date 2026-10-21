package `in`.nispax.nix

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.view.WindowInsets
import android.view.WindowManager
import android.view.inputmethod.InputMethodManager
import android.webkit.JavascriptInterface
import android.webkit.WebChromeClient
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.Toast

class MainActivity : Activity() {
    private lateinit var editor: WebView
    companion object { init { System.loadLibrary("nixscript") } }
    private external fun runNixScript(source: String, input: String): String

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.statusBarColor = Color.rgb(32, 33, 36)
        window.navigationBarColor = Color.rgb(17, 19, 22)
        window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)

        // Android 15 enforces edge-to-edge for targetSdk 35. Apply the insets ourselves
        // so the editor header never renders underneath the status icons and the
        // symbol toolbar remains inside the visible area above the IME.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false)
        }

        editor = WebView(this).apply {
            setBackgroundColor(Color.rgb(32, 33, 36))
            isFocusable = true
            isFocusableInTouchMode = true
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            settings.allowFileAccess = true
            webChromeClient = WebChromeClient()
            webViewClient = WebViewClient()
            addJavascriptInterface(EditorBridge(), "Nix")
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                setOnApplyWindowInsetsListener { view, insets ->
                    val bars = insets.getInsets(WindowInsets.Type.systemBars())
                    val ime = insets.getInsets(WindowInsets.Type.ime())
                    view.setPadding(0, bars.top, 0, maxOf(bars.bottom, ime.bottom))
                    insets
                }
            }
            loadUrl("file:///android_asset/editor.html")
        }
        setContentView(editor)
        editor.requestApplyInsets()
    }

    override fun onResume() {
        super.onResume()
        // Deliberately do not focus the editor on startup: that opened Gboard over
        // the splash screen and covered the bottom editing toolbar.
    }

    inner class EditorBridge {
        @JavascriptInterface
        fun run(source: String, input: String): String = try {
            runNixScript(source, input)
        } catch (e: Throwable) {
            "NixScript error: " + (e.message ?: "unknown error")
        }

        @JavascriptInterface
        fun showKeyboard() {
            runOnUiThread {
                editor.requestFocus()
                val manager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
                manager.showSoftInput(editor, InputMethodManager.SHOW_IMPLICIT)
            }
        }

        @JavascriptInterface
        fun hideKeyboard() {
            runOnUiThread {
                val manager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
                manager.hideSoftInputFromWindow(editor.windowToken, 0)
            }
        }

        @JavascriptInterface
        fun toast(message: String) {
            runOnUiThread {
                Toast.makeText(this@MainActivity, message, Toast.LENGTH_SHORT).show()
            }
        }
    }
}