cmake_minimum_required(VERSION 3.21)
project(ApertarUI LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Network Qml Quick QuickControls2)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(TIFF REQUIRED)

if(Qt6_VERSION VERSION_GREATER_EQUAL "6.8")
    qt_policy(SET QTP0004 NEW)
endif()

set(APERTAR_QML_FILES
    qml/Main.qml
    qml/ExternalMonitorWindow.qml
    qml/CameraPage.qml
    qml/TimelapseIntervalPicker.qml
    qml/TimelapseIntervalDialog.qml
    qml/TimelapseIntervals.js
    qml/StillExposureOverlay.qml
    qml/ClipBrowserPage.qml
    qml/ClipPlayerPage.qml
    qml/TopDropdownStat.qml
    qml/InfoLine.qml
    qml/InfoDataRow.qml
    qml/AudioLevelCard.qml
    qml/AudioMeterOverlay.qml
    qml/FalseColorLegend.qml
    qml/HorizontalLevelOverlay.qml
    qml/DisplayCalibrationPanel.qml
    qml/RecordButton.qml
    qml/ClipBrowserButton.qml
    qml/StatusChip.qml
    qml/DroppedFrameWarning.qml
    qml/SettingsPage.qml
    qml/SettingsMenuItem.qml
    qml/MonitoringRow.qml
    qml/SettingsChoice.qml
    qml/InlineDropdownField.qml
    qml/TogglePill.qml
    qml/SystemSectionCard.qml
    qml/AdjustNumberTile.qml
    qml/ActionTile.qml
    qml/GyroBubbleLevel.qml
    qml/GyroOrientationIndicator.qml
)

# A cross-build requires host Qt tools that exactly match the target Qt. The
# current WSL toolchain does not, while native Pi builds can safely use AOT.
if(CMAKE_CROSSCOMPILING)
    set(APERTAR_QML_AOT_DEFAULT OFF)
else()
    set(APERTAR_QML_AOT_DEFAULT ON)
endif()
option(APERTAR_QML_AOT "Compile QML ahead of time for faster startup" ${APERTAR_QML_AOT_DEFAULT})

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
    src/DisplayConfigBridge.cpp
    src/DisplayConfigBridge.hpp
    src/DisplayCalibration.cpp
    src/DisplayCalibration.hpp
    src/SystemActionBridge.cpp
    src/SystemActionBridge.hpp
    src/PowerButtonBridge.cpp
    src/PowerButtonBridge.hpp
    src/SleepManager.cpp
    src/SleepManager.hpp
    src/DisplayStandbyController.cpp
    src/DisplayStandbyController.hpp
    src/MediaStatusBridge.cpp
    src/MediaStatusBridge.hpp
    src/Ina219Bridge.cpp
    src/Ina219Bridge.hpp
    src/GyroConfigBridge.cpp
    src/GyroConfigBridge.hpp
    src/WifiBridge.cpp
    src/WifiBridge.hpp
    src/AudioDeviceBridge.cpp
    src/AudioDeviceBridge.hpp
    src/AudioMeterBridge.cpp
    src/AudioMeterBridge.hpp
    src/AudioMixerBridge.cpp
    src/AudioMixerBridge.hpp
    src/ExternalMonitorManager.cpp
    src/ExternalMonitorManager.hpp
    src/ExternalMonitorRuntimeBridge.cpp
    src/ExternalMonitorRuntimeBridge.hpp
    src/ApertarControlBridge.cpp
    src/ApertarControlBridge.hpp
    src/SettingsBridge.cpp
    src/SettingsBridge.hpp
    src/cliplistmodel.cpp
    src/cliplistmodel.h
    src/playbackcontroller.cpp
    src/playbackcontroller.h
    src/VideoPlayback.cpp
    src/VideoPlayback.hpp
    src/VideoPlaybackApi.h
    src/MlvReader.cpp
    src/MlvReader.hpp
    src/RawPreview.hpp
    src/cdngimageprovider.cpp
    src/cdngimageprovider.h
    src/dngdecoder.cpp
    third_party/liblj92/lj92.c
    src/dngdecoder.h
    src/tiffhelper.cpp
    src/tiffhelper.h
    src/rawframe.h
    resources.qrc

)

# This module is loaded only when browsing/playing a movie. Do not link
# it to ApertarUI: FFmpeg must stay out of the cold-boot dependency graph.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(VIDEO_PLAYBACK QUIET IMPORTED_TARGET libavcodec libavformat libavutil libswscale)
endif()
if(VIDEO_PLAYBACK_FOUND)
    add_library(apertar-video-libav MODULE src/VideoPlaybackLibav.cpp)
    set_target_properties(apertar-video-libav PROPERTIES PREFIX "" AUTOMOC OFF AUTORCC OFF)
    target_link_libraries(apertar-video-libav PRIVATE PkgConfig::VIDEO_PLAYBACK)
    target_compile_options(apertar-video-libav PRIVATE -Wall -Wextra -Wpedantic)
    add_dependencies(ApertarUI apertar-video-libav)
    install(TARGETS apertar-video-libav LIBRARY DESTINATION lib/apertar-ui)
else()
    message(STATUS "Video playback decoder disabled: install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev")
endif()

add_executable(ApertarCfeAutomounter src/CfeAutomounter.cpp)
add_executable(ApertarStandbyUsbPower src/StandbyUsbPower.cpp)
add_executable(ApertarStandbyLowPower src/StandbyLowPower.cpp)
set_target_properties(ApertarStandbyLowPower PROPERTIES OUTPUT_NAME "apertar-standby-low-power" AUTOMOC OFF AUTORCC OFF)
target_compile_options(ApertarStandbyLowPower PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(ApertarStandbyUsbPower PROPERTIES OUTPUT_NAME "apertar-standby-usb" AUTOMOC OFF AUTORCC OFF)
target_compile_options(ApertarStandbyUsbPower PRIVATE -Wall -Wextra -Wpedantic)
add_executable(ApertarNetworkHelper src/NetworkHelper.cpp src/WifiBridge.cpp src/WifiBridge.hpp)
set_target_properties(ApertarNetworkHelper PROPERTIES OUTPUT_NAME "apertar-network-helper" AUTORCC OFF)
target_link_libraries(ApertarNetworkHelper PRIVATE Qt6::Core)
set_target_properties(ApertarCfeAutomounter PROPERTIES
    OUTPUT_NAME "apertar-cfe-automounter"
    AUTOMOC OFF
    AUTORCC OFF
)
target_link_libraries(ApertarCfeAutomounter PRIVATE Threads::Threads)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ApertarCfeAutomounter PRIVATE -Wall -Wextra -Wpedantic)
endif()

# QtNetwork links Debian's desktop proxy-discovery stack even though Apertar
# only communicates over local sockets. Keep that large dependency tree out of
# the camera UI's cold-start path while retaining direct network behaviour.
if(UNIX AND NOT APPLE)
    add_library(ApertarDirectProxy SHARED src/DirectProxyStub.c)
    set_target_properties(ApertarDirectProxy PROPERTIES
        OUTPUT_NAME "proxy"
        SOVERSION 1
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apertar-runtime"
    )
    target_link_options(ApertarDirectProxy PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/DirectProxyStub.map"
    )
endif()

if(APERTAR_QML_AOT)
    target_compile_definitions(ApertarUI PRIVATE APERTAR_QML_AOT_ENABLED=1)
    qt_add_qml_module(ApertarUI
        URI Apertar.UI
        VERSION 1.0
        RESOURCE_PREFIX /
        NO_RESOURCE_TARGET_PATH
        NO_PLUGIN
        NO_GENERATE_QMLTYPES
        NO_IMPORT_SCAN
        NO_LINT
        QML_FILES ${APERTAR_QML_FILES}
    )
else()
    target_compile_definitions(ApertarUI PRIVATE APERTAR_QML_AOT_ENABLED=0)
    target_sources(ApertarUI PRIVATE qml.qrc)
endif()

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

# Qt's screen-power API is private. Cross-builds without matching headers
# retain blanking/backlight support without adding broken include paths.
set(APERTAR_HAS_DISPLAY_POWER OFF)
if(TARGET Qt6::GuiPrivate)
    get_target_property(APERTAR_GUI_PRIVATE_INCLUDES Qt6::GuiPrivate INTERFACE_INCLUDE_DIRECTORIES)
    set(APERTAR_HAS_DISPLAY_POWER ON)
    foreach(directory IN LISTS APERTAR_GUI_PRIVATE_INCLUDES)
        # Qt wraps private include paths in conditional generator expressions.
        string(REGEX REPLACE "^.*>:" "" directory "${directory}")
        string(REGEX REPLACE ">$" "" directory "${directory}")
        if(directory AND NOT EXISTS "${directory}")
            set(APERTAR_HAS_DISPLAY_POWER OFF)
        endif()
    endforeach()
endif()
if(APERTAR_HAS_DISPLAY_POWER)
    target_link_libraries(ApertarUI PRIVATE Qt6::GuiPrivate)
    target_compile_definitions(ApertarUI PRIVATE APERTAR_HAS_DISPLAY_POWER=1)
else()
    target_compile_definitions(ApertarUI PRIVATE APERTAR_HAS_DISPLAY_POWER=0)
    message(STATUS "HDMI DPMS disabled: install matching qt6-base-private-dev headers to enable it")
endif()

# Debian's toolchain links PIE executables with immediate symbol binding by
# default. Deferring PLT binding avoids resolving the entire Qt stack before
# main() during cold boot; symbols are resolved on their first actual use.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(ApertarUI PRIVATE "LINKER:-z,lazy")
endif()

# Force host-side tools when cross-compiling from WSL.
if(EXISTS "/usr/lib/qt6/libexec/moc")
    set_target_properties(ApertarNetworkHelper PROPERTIES
        AUTOMOC_EXECUTABLE "/usr/lib/qt6/libexec/moc"
    )
    set_target_properties(ApertarUI PROPERTIES
        OUTPUT_NAME "Apertar-UI"
        AUTOMOC_EXECUTABLE "/usr/lib/qt6/libexec/moc"
        AUTORCC_EXECUTABLE "/usr/lib/qt6/libexec/rcc"
    )
endif()
