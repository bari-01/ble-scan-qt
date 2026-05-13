# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
cmake_minimum_required(VERSION 3.16)

project(qtquickview VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
        Quick
        Core
    Gui
    Qml
    Bluetooth
)

# Force CMake to look at the host system path ignoring the Android toolchain root.
# Note: This allows finding the package, but will likely cause a linker error
# when building for Android because the host library is for Linux (x86_64).
#find_package(simpleble REQUIRED CONFIG
#    PATHS "/usr/local/lib64/cmake/simpleble"
#    NO_CMAKE_FIND_ROOT_PATH
#)

#include(FetchContent)
#FetchContent_Declare(
#        simpleble
#        SOURCE_DIR "/home/a/cpp/simpleble/simpleble"
#)
# SimpleBLE requires some options to be set for Android/Static builds
#set(SIMPLEBLE_LIBRARY_TYPE STATIC CACHE INTERNAL "")
#set(SIMPLEBLE_PLAIN_ONLY ON CACHE INTERNAL "")

#FetchContent_MakeAvailable(simpleble)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(qtquickview
    main.cpp
        BluetoothManager.cpp
        BluetoothManager.h
        HotspotBridge.cpp
        HotspotBridge.h
        TransportManager.cpp
        TransportManager.h
)

# Add includes for SimpleBLE's internal dependencies so main.cpp can initialize it
#target_include_directories(qtquickview PRIVATE
#    "/home/a/cpp/simpleble/dependencies/internal/include"
#)

qt_add_qml_module(qtquickview
    URI qmlModule
    QML_FILES Main.qml
)

target_link_libraries(qtquickview
    PRIVATE Qt6::Quick
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Bluetooth
)
#target_link_libraries(qtquickview PRIVATE
#        simpleble::simpleble
#)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_command(TARGET qtquickview POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json"
        COMMENT "Copying compile_commands.json to source directory"
        VERBATIM
    )
endif()

install(TARGETS qtquickview
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)








