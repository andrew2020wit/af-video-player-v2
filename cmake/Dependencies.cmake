# Finds libmpv and FFmpeg and exposes them as imported targets:
#   Mpv::Mpv, FFmpeg::avformat, FFmpeg::avcodec, FFmpeg::avutil, FFmpeg::swscale
#
# Windows: prebuilt binaries in AFVP_DEPS_DIR (see scripts/fetch-deps-windows.ps1).
# Linux and macOS: system packages found through pkg-config.

if(WIN32)
    set(AFVP_DEPS_DIR "${CMAKE_SOURCE_DIR}/deps" CACHE PATH "Directory with prebuilt libmpv and FFmpeg")

    if(NOT EXISTS "${AFVP_DEPS_DIR}/mpv/mpv.lib")
        message(FATAL_ERROR "libmpv not found in ${AFVP_DEPS_DIR}/mpv: run scripts/fetch-deps-windows.ps1")
    endif()

    file(GLOB mpvDll "${AFVP_DEPS_DIR}/mpv/libmpv-*.dll")
    add_library(Mpv::Mpv SHARED IMPORTED GLOBAL)
    set_target_properties(Mpv::Mpv PROPERTIES
        IMPORTED_IMPLIB "${AFVP_DEPS_DIR}/mpv/mpv.lib"
        IMPORTED_LOCATION "${mpvDll}"
        INTERFACE_INCLUDE_DIRECTORIES "${AFVP_DEPS_DIR}/mpv/include")

    foreach(ffmpegLibrary IN ITEMS avformat avcodec avutil swscale)
        file(GLOB ffmpegDll "${AFVP_DEPS_DIR}/ffmpeg/bin/${ffmpegLibrary}-*.dll")
        add_library(FFmpeg::${ffmpegLibrary} SHARED IMPORTED GLOBAL)
        set_target_properties(FFmpeg::${ffmpegLibrary} PROPERTIES
            IMPORTED_IMPLIB "${AFVP_DEPS_DIR}/ffmpeg/lib/${ffmpegLibrary}.lib"
            IMPORTED_LOCATION "${ffmpegDll}"
            INTERFACE_INCLUDE_DIRECTORIES "${AFVP_DEPS_DIR}/ffmpeg/include")
    endforeach()

    # DLLs the program needs at run time: libmpv, the FFmpeg libraries in use and their dependency swresample.
    file(GLOB ffmpegDlls
        "${AFVP_DEPS_DIR}/ffmpeg/bin/avformat-*.dll"
        "${AFVP_DEPS_DIR}/ffmpeg/bin/avcodec-*.dll"
        "${AFVP_DEPS_DIR}/ffmpeg/bin/avutil-*.dll"
        "${AFVP_DEPS_DIR}/ffmpeg/bin/swscale-*.dll"
        "${AFVP_DEPS_DIR}/ffmpeg/bin/swresample-*.dll")
    set(AFVP_RUNTIME_DLLS ${mpvDll} ${ffmpegDlls})
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AFVP_MPV REQUIRED IMPORTED_TARGET GLOBAL mpv)
    add_library(Mpv::Mpv ALIAS PkgConfig::AFVP_MPV)

    foreach(ffmpegLibrary IN ITEMS avformat avcodec avutil swscale)
        pkg_check_modules(AFVP_${ffmpegLibrary} REQUIRED IMPORTED_TARGET GLOBAL lib${ffmpegLibrary})
        add_library(FFmpeg::${ffmpegLibrary} ALIAS PkgConfig::AFVP_${ffmpegLibrary})
    endforeach()

    set(AFVP_RUNTIME_DLLS "")
endif()

# Copies libmpv and FFmpeg DLLs next to a target's executable (Windows only).
function(afvp_copy_runtime_dlls target)
    if(AFVP_RUNTIME_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${AFVP_RUNTIME_DLLS} $<TARGET_FILE_DIR:${target}>)
    endif()
endfunction()
