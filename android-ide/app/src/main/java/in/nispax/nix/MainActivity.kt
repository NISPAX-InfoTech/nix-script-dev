package `in`.nispax.nix

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.graphics.Color
import android.os.Bundle
import android.view.View
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
        // Keep this WebView inside the system-bar-safe content area; do not inherit
        // fullscreen/layout-behind-bars flags from an OEM theme or previous UI state.
        @Suppress("DEPRECATION")
        window.decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_VISIBLE
        window.statusBarColor = Color.rgb(32, 33, 36)
        window.navigationBarColor = Color.rgb(17, 19, 22)
        window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)

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