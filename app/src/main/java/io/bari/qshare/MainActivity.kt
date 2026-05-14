package io.bari.qshare

import android.os.Bundle
import android.util.Log
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import io.bari.qshare.databinding.ActivityMainBinding
import org.qtproject.example.qtquickview.QmlModule.Main
import org.qtproject.qt.android.QtQmlStatus
import org.qtproject.qt.android.QtQmlStatusChangeListener
import org.qtproject.qt.android.QtQuickView
import org.qtproject.qt.android.QtQuickViewContent

class MainActivity : AppCompatActivity(), QtQmlStatusChangeListener {
    private val TAG = "QshareMainActivity"
    private lateinit var binding: ActivityMainBinding
    private val qmlContent: Main = Main()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val qtQuickView = QtQuickView(this)
        val params = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT
        )
        binding.qmlFrame.addView(qtQuickView, params)

        qmlContent.setStatusChangeListener(this)
        qtQuickView.loadContent(qmlContent)
    }

    override fun onStart() {
        super.onStart()
    }

    override fun onStatusChanged(status: QtQmlStatus?, content: QtQuickViewContent?) {
        Log.d(TAG, "QML Status changed: $status")
    }
}
