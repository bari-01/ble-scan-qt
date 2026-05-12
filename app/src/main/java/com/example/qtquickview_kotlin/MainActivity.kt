// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtquickview_kotlin

import android.os.Bundle
import android.util.Log
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.core.graphics.toColorInt
import com.example.qtquickview_kotlin.databinding.ActivityMainBinding
//! [qmlTypeImports]
import org.qtproject.example.qtquickview.QmlModule.Main
import org.qtproject.example.qtquickview.QmlModule.Second
//! [qmlTypeImports]
import org.qtproject.qt.android.QtQmlStatus
import org.qtproject.qt.android.QtQmlStatusChangeListener
import org.qtproject.qt.android.QtQuickView
import org.qtproject.qt.android.QtQuickViewContent


class MainActivity : AppCompatActivity(), QtQmlStatusChangeListener {
    private val tag = "myTag"
    private val colors: Colors = Colors()
    private lateinit var binding: ActivityMainBinding
    private var qmlButtonSignalListenerId = 0
    //! [qmlContent]
    private val firstQmlContent: Main = Main()
    private val secondQmlContent: Second = Second()
    //! [qmlContent]
    private val statusNames = hashMapOf(
        QtQmlStatus.READY to "READY",
        QtQmlStatus.LOADING to "LOADING",
        QtQmlStatus.ERROR to "ERROR",
        QtQmlStatus.NULL to "NULL",
    )
    //! [onCreate]
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //! [binding]
        binding = ActivityMainBinding.inflate(layoutInflater)
        val view = binding.root
        setContentView(view)
        //! [binding]

        binding.disconnectQmlListenerSwitch.setOnCheckedChangeListener { _, checked ->
            switchListener(
                checked
            )
        }

        //! [m_qtQuickView]
        val firstQtQuickView = QtQuickView(this)
        val secondQtQuickView = QtQuickView(this)
        //! [m_qtQuickView]

        // Set status change listener for m_qmlView
        // listener implemented below in OnStatusChanged
        //! [setStatusChangeListener]
        firstQmlContent.setStatusChangeListener(this)
        secondQmlContent.setStatusChangeListener(this)
        //! [setStatusChangeListener]

        //! [layoutParams]
        val params: ViewGroup.LayoutParams = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT
        )
        binding.firstQmlFrame.addView(firstQtQuickView, params)
        binding.secondQmlFrame.addView(secondQtQuickView, params)
        //! [layoutParams]
        //! [loadContent]
        firstQtQuickView.loadContent(firstQmlContent)
        secondQtQuickView.loadContent(secondQmlContent)
        //! [loadContent]

        binding.changeQmlColorButton.setOnClickListener { onClickListener() }
        binding.rotateQmlGridButton.setOnClickListener { rotateQmlGrid() }
    }
    //! [onCreate]

    //! [onClickListener]
    private fun onClickListener() {
        // Set the QML view root object property "colorStringFormat" value to
        // color from Colors.getColor()
        firstQmlContent.colorStringFormat = colors.getColor()
        updateColorDisplay()
    }

    private fun updateColorDisplay() {
        val qmlBackgroundColor = firstQmlContent.colorStringFormat
        // Display the QML View background color code
        binding.qmlViewBackgroundText.text = qmlBackgroundColor
        // Display the QML View background color in a view
        // if qmlBackgroundColor is not null
        if (qmlBackgroundColor != null) {
            binding.qmlColorBox.setBackgroundColor(qmlBackgroundColor.toColorInt())
        }
    }
    //! [onClickListener]

    private fun switchListener(isChecked: Boolean) {
        // Disconnect QML button signal listener if switch is On using the saved signal listener Id
        // and connect it again if switch is turned off
        if (isChecked) {
            qmlButtonSignalListenerId =
                firstQmlContent.connectOnClickedListener { _: String, _: Void? ->
                    binding.kotlinRelative.setBackgroundColor(
                        colors.getColor().toColorInt()
                    )
                }
        } else {
            //! [disconnect qml signal listener]
            firstQmlContent.disconnectSignalListener(qmlButtonSignalListenerId)
            //! [disconnect qml signal listener]
        }
    }

    //! [onStatusChanged]
    override fun onStatusChanged(status: QtQmlStatus?, content: QtQuickViewContent?) {
        Log.v(tag, "Status of QtQuickView: $status")

        // Show current QML View status in a textview
        binding.qmlStatusText.text = getString(R.string.qml_view_status, statusNames[status])

        updateColorDisplay()

        if (content == firstQmlContent) {
            // Connect signal listener to "onClicked" signal from main.qml
            // addSignalListener returns int which can be used later to identify the listener
            //! [qml signal listener]
            if ((status == QtQmlStatus.READY) && binding.disconnectQmlListenerSwitch.isChecked) {
                qmlButtonSignalListenerId =
                    firstQmlContent.connectOnClickedListener { _: String, _: Void? ->
                        Log.i(tag, "QML button clicked")
                        binding.kotlinRelative.setBackgroundColor(
                            colors.getColor().toColorInt()
                        )
                    }
            }
            //! [qml signal listener]
        }
    }
    //! [onStatusChanged]
    //! [gridRotate]
    private fun rotateQmlGrid() {
        val previousGridRotation = secondQmlContent.gridRotation
        if (previousGridRotation != null) {
            secondQmlContent.gridRotation = previousGridRotation + 45
        }
    }
    //! [gridRotate]
}
