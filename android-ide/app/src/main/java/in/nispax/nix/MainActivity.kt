package `in`.nispax.nix

import android.annotation.SuppressLint
import android.app.Activity
import android.os.Bundle
import android.webkit.JavascriptInterface
import android.webkit.WebChromeClient
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.Toast

class MainActivity : Activity() {
    private lateinit var editor: WebView
    companion object { init { System.loadLibrary("nixscript") } }
    private external fun runNixScript(source: String): String

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        editor = WebView(this).apply {
            setBackgroundColor(android.graphics.Color.rgb(48, 48, 48))
            settings.javaScriptEnabled = true
            webChromeClient = WebChromeClient()
            webViewClient = WebViewClient()
            addJavascriptInterface(EditorBridge(), "Nix")
            loadUrl("file:///android_asset/editor.html")
        }
        setContentView(editor)
    }

    inner class EditorBridge {
        @JavascriptInterface fun run(source: String): String = try { runNixScript(source) } catch (e: Throwable) { "NixScript error: ${e.message}" }
        @JavascriptInterface fun toast(message: String) { runOnUiThread { Toast.makeText(this@MainActivity, message, Toast.LENGTH_SHORT).show() } }
    }
}
