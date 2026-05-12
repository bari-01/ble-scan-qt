// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtquickview_kotlin

import java.util.Stack

internal class Colors {
    private val recycle: Stack<Int> = Stack()
    private val colors: Stack<Int> = Stack()

    init {
        recycle.addAll(
            mutableListOf(
                -0xe34997, -0xffbeb6, -0xd8ec75,
                -0x4a3ef2, -0xc8c0da, -0x506c21,
                -0x7e8afb,
            )
        )
    }

    fun getColor(): String {
        if (colors.isEmpty()) {
            while (recycle.isNotEmpty()) {
                colors.push(recycle.pop())
            }
            colors.shuffle()
        }
        val color = colors.pop()
        recycle.push(color)
        return String.format("#%06X", 0xFFFFFF and color)
    }
}
