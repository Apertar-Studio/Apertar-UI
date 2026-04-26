set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TARGET_TRIPLE aarch64-linux-gnu)
set(CMAKE_SYSROOT "$ENV{HOME}/sysroots/pi5")

set(CMAKE_C_COMPILER   /usr/bin/${TARGET_TRIPLE}-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/${TARGET_TRIPLE}-g++)
set(CMAKE_ASM_COMPILER /usr/bin/${TARGET_TRIPLE}-gcc)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

set(PI_SYSROOT_LIBRARY_DIRS
    "${CMAKE_SYSROOT}/lib/aarch64-linux-gnu"
    "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu"
    "${CMAKE_SYSROOT}/usr/local/lib/aarch64-linux-gnu"
)

foreach(_libdir IN LISTS PI_SYSROOT_LIBRARY_DIRS)
    string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -L${_libdir} -Wl,-rpath-link,${_libdir}")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " -L${_libdir} -Wl,-rpath-link,${_libdir}")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_INIT " -L${_libdir} -Wl,-rpath-link,${_libdir}")
endforeach()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_LIBDIR}
    "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig:${CMAKE_SYSROOT}/usr/lib/pkgconfig"
)

# Host Qt tools must come from WSL, not the Pi sysroot
set(QT_HOST_PATH "/usr")
set(QT_FORCE_FIND_TOOLS ON)

# Target Qt packages must come from the Pi sysroot
set(CMAKE_PREFIX_PATH
    "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/cmake"
    "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/cmake/Qt6"
)

# Extra hint so Qt finds target-side packages, but keeps host tools on host
set(QT_ADDITIONAL_PACKAGES_PREFIX_PATH
    "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/cmake"
)
