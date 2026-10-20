package `in`.nispax.nix

import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.os.Bundle
import android.text.InputType
import android.view.WindowManager
import android.widget.EditText
import android.webkit.JavascriptInterface
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference
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

    // Called from native code on the WebView bridge thread. Wait here while
    // the Android UI thread shows an input dialog; this keeps the UI responsive.
    private fun requestNixInput(programOutput: String): String {
        val completed = CountDownLatch(1)
        val response = AtomicReference("")

        runOnUiThread {
            val inputBox = EditText(this@MainActivity).apply {
                inputType = InputType.TYPE_CLASS_TEXT
                singleLine = true
                hint = "Enter input"
            }
            val dialog = AlertDialog.Builder(this@MainActivity)
                .setTitle("NixScript input")
                .setMessage(if (programOutput.isBlank()) "Your program is waiting for input." else programOutput)
                .setView(inputBox)
                .setPositiveButton("Submit", null)
                .setNegativeButton("Cancel") { _, _ ->
                    response.set("")
                    completed.countDown()
                }
                .create()

            dialog.setOnShowListener {
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener {
                    response.set(inputBox.text.toString())
                    completed.countDown()
                    dialog.dismiss()
                }
                inputBox.requestFocus()
                dialog.window?.setSoftInputMode(
                    WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE or
                        WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE
                )
            }
            dialog.setOnCancelListener {
                response.set("")
                completed.countDown()
            }
            dialog.show()
        }

        try {
            if (!completed.await(10, TimeUnit.MINUTES)) return ""
        } catch (interrupted: InterruptedException) {
            Thread.currentThread().interrupt()
            return ""
        }
        return response.get()
    }

    inner class EditorBridge {
        @JavascriptInterface fun run(source: String): String = try { runNixScript(source) } catch (e: Throwable) { "NixScript error: ${e.message}" }
        @JavascriptInterface fun toast(message: String) { runOnUiThread { Toast.makeText(this@MainActivity, message, Toast.LENGTH_SHORT).show() } }
    }
}
