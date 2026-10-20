package `in`.nispax.nix

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.os.Bundle
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
        window.setSoftInputMode(android.view.WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE)
        editor = WebView(this).apply {
            setBackgroundColor(android.graphics.Color.rgb(48, 48, 48))
            isFocusable = true
            isFocusableInTouchMode = true
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            webChromeClient = WebChromeClient()
            webViewClient = WebViewClient()
            addJavascriptInterface(EditorBridge(), "Nix")
            loadUrl("file:///android_asset/editor.html")
        }
        setContentView(editor)
    }

    override fun onResume() {
        super.onResume()
        editor.postDelayed({
            editor.requestFocus()
            editor.evaluateJavascript("(function(){var e=document.getElementById('code');if(e){e.focus({preventScroll:true});}})();", null)
        }, 250)
    }

    inner class EditorBridge {
        @JavascriptInterface fun run(source: String, input: String): String = try {
            runNixScript(source, input)
        } catch (e: Throwable) { "NixScript error: ${e.message}" }
        @JavascriptInterface fun showKeyboard() {
            runOnUiThread {
                editor.requestFocus()
                val manager = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
                manager.showSoftInput(editor, InputMethodManager.SHOW_IMPLICIT)
            }
        }
        @JavascriptInterface fun toast(message: String) {
            runOnUiThread { Toast.makeText(this@MainActivity, message, Toast.LENGTH_SHORT).show() }
        }
    }
}
