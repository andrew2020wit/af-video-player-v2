qt_add_library(afvp_export STATIC
    FrameGrabber.cpp FrameGrabber.h
    SubtitleExtractor.cpp SubtitleExtractor.h
)

target_include_directories(afvp_export PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(afvp_export
    PUBLIC afvp_core Qt6::Core
    PRIVATE FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale
)
