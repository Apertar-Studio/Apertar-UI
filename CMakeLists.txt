cmake_minimum_required(VERSION 3.21)
project(ApertarUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Network Qml Quick QuickControls2)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(TIFF REQUIRED)

add_executable(ApertarUI
    src/main.cpp
    src/PreviewFrameInfo.hpp
    src/ApertarPreviewSocketClient.cpp
    src/ApertarPreviewSocketClient.hpp
    src/CameraPreviewItem.cpp
    src/CameraPreviewItem.hpp
    src/PreviewWindowRenderer.cpp
    src/PreviewWindowRenderer.hpp
    src/SystemStatsBridge.cpp
    src/SystemStatsBridge.hpp
    src/DeviceInfoBridge.cpp
    src/DeviceInfoBridge.hpp
    src/SystemActionBridge.cpp
    src/SystemActionBridge.hpp
    src/SleepManager.cpp
    src/SleepManager.hpp
    src/MediaStatusBridge.cpp
    src/MediaStatusBridge.hpp
    src/Ina219Bridge.cpp
    src/Ina219Bridge.hpp
    src/ApertarControlBridge.cpp
    src/ApertarControlBridge.hpp
    src/SettingsBridge.cpp
    src/SettingsBridge.hpp
    src/cliplistmodel.cpp
    src/cliplistmodel.h
    src/playbackcontroller.cpp
    src/playbackcontroller.h
    src/cdngimageprovider.cpp
    src/cdngimageprovider.h
    src/dngdecoder.cpp
    src/dngdecoder.h
    src/tiffhelper.cpp
    src/tiffhelper.h
    src/rawframe.h
    resources.qrc

)

target_include_directories(ApertarUI PRIVATE
    ${CMAKE_SYSROOT}/usr/include
    ${CMAKE_SYSROOT}/usr/include/libcamera
)

target_link_libraries(ApertarUI PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    OpenGL::GL
    EGL
    GLESv2
    TIFF::TIFF
)

# Force host-side tools when cross-compiling from WSL.
if(EXISTS "/usr/lib/qt6/libexec/moc")
    set_target_properties(ApertarUI PROPERTIES
        OUTPUT_NAME "Apertar-UI"
        AUTOMOC_EXECUTABLE "/usr/lib/qt6/libexec/moc"
        AUTORCC_EXECUTABLE "/usr/lib/qt6/libexec/rcc"
    )
endif()
