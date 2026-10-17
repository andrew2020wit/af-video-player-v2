qt_add_library(afvp_core STATIC
    AudioTracks.cpp AudioTracks.h
    AutoExport.cpp AutoExport.h
    CueText.cpp CueText.h
    ExportNaming.cpp ExportNaming.h
    History.cpp History.h
    HistoryGroups.cpp HistoryGroups.h
    JsonFile.cpp JsonFile.h
    KeyMapping.cpp KeyMapping.h
    Languages.cpp Languages.h
    MediaFiles.cpp MediaFiles.h
    Settings.cpp Settings.h
    SubtitleCue.h
    SubtitleMerge.cpp SubtitleMerge.h
    SubtitleParser.cpp SubtitleParser.h
    SubtitleSearch.cpp SubtitleSearch.h
    TextDecoding.cpp TextDecoding.h
    TimeFormat.cpp TimeFormat.h
    WordLookup.cpp WordLookup.h
)

target_include_directories(afvp_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(afvp_core PUBLIC Qt6::Core)
