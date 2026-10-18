qt_add_executable(af-video-player
    main.cpp
)

set_target_properties(af-video-player PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.andrew2020wit.af-video-player
    MACOSX_BUNDLE_BUNDLE_NAME "AF Video Player"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    OUTPUT_NAME af-video-player
)

target_compile_definitions(af-video-player PRIVATE AFVP_VERSION="${PROJECT_VERSION}"
    AFVP_HOMEPAGE_URL="${PROJECT_HOMEPAGE_URL}")

if(MSVC)
    # Qt headers used by the generated QML type registration code trigger "unreachable code".
    target_compile_options(af-video-player PRIVATE /wd4702)
endif()

qt_add_qml_module(af-video-player
    URI AfVideoPlayer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/StartPage.qml
        qml/PlayerPage.qml
        qml/ControlBar.qml
        qml/SubtitleList.qml
        qml/SubtitleRow.qml
        qml/Osd.qml
        qml/SubtitleOverlay.qml
        qml/SubtitlesDialog.qml
        qml/ExportDialogs.qml
        qml/BatchExportWindow.qml
        qml/SettingsPage.qml
        qml/LanguagePicker.qml
        qml/UrlTemplateField.qml
        qml/NumberField.qml
    SOURCES
        AppController.cpp AppController.h
        BatchExporter.cpp BatchExporter.h
        FramePreviewProvider.cpp FramePreviewProvider.h
        FullScreenPlacement.cpp FullScreenPlacement.h
        HandCursorFilter.cpp HandCursorFilter.h
        MpvObject.cpp MpvObject.h
        SingleInstance.cpp SingleInstance.h
        SleepInhibitor.cpp SleepInhibitor.h
        SubtitleListModel.cpp SubtitleListModel.h
)

target_link_libraries(af-video-player PRIVATE
    afvp_core
    afvp_export
    Mpv::Mpv
    Qt6::Gui
    Qt6::Network
    Qt6::OpenGL
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

if(APPLE)
    # Power assertions of SleepInhibitor.
    target_link_libraries(af-video-player PRIVATE "-framework IOKit" "-framework CoreFoundation")
elseif(UNIX)
    # org.freedesktop.ScreenSaver of SleepInhibitor.
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    target_link_libraries(af-video-player PRIVATE Qt6::DBus)
endif()

afvp_copy_runtime_dlls(af-video-player)

# The application icon (I-84), generated from packaging/icons/af-video-player.svg by scripts/make-icons.ps1.
set(AFVP_ICON_DIR ${PROJECT_SOURCE_DIR}/packaging/icons)

if(WIN32)
    # IDI_ICON1 is also the window icon: Qt takes it from the executable.
    file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/af-video-player.rc
        CONTENT "IDI_ICON1 ICON \"${AFVP_ICON_DIR}/af-video-player.ico\"\n")
    target_sources(af-video-player PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/af-video-player.rc)
elseif(APPLE)
    set_source_files_properties(${AFVP_ICON_DIR}/af-video-player.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    target_sources(af-video-player PRIVATE ${AFVP_ICON_DIR}/af-video-player.icns)
    set_target_properties(af-video-player PROPERTIES MACOSX_BUNDLE_ICON_FILE af-video-player.icns)
else()
    qt_add_resources(af-video-player icons
        PREFIX /icons
        BASE ${AFVP_ICON_DIR}/png
        FILES
            ${AFVP_ICON_DIR}/png/16.png
            ${AFVP_ICON_DIR}/png/24.png
            ${AFVP_ICON_DIR}/png/32.png
            ${AFVP_ICON_DIR}/png/48.png
            ${AFVP_ICON_DIR}/png/64.png
            ${AFVP_ICON_DIR}/png/128.png
            ${AFVP_ICON_DIR}/png/256.png
    )
endif()

include(GNUInstallDirs)

install(TARGETS af-video-player
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Qt libraries, plugins and QML modules: windeployqt / macdeployqt. On Linux the AppImage is assembled by
# linuxdeploy (scripts/package-linux.sh), and distribution packages use the system Qt.
if(WIN32 OR APPLE)
    qt_generate_deploy_qml_app_script(
        TARGET af-video-player
        OUTPUT_SCRIPT deployScript
        NO_TRANSLATIONS
    )
    install(SCRIPT ${deployScript})
endif()

if(UNIX AND NOT APPLE)
    install(FILES ${PROJECT_SOURCE_DIR}/packaging/linux/af-video-player.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES ${PROJECT_SOURCE_DIR}/packaging/icons/af-video-player.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()

if(WIN32)
    install(FILES ${AFVP_RUNTIME_DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})

    # MSVC runtime, so that the portable build works without the Visual C++ Redistributable
    # (the universal CRT is part of Windows 10 and later).
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR})
    include(InstallRequiredSystemLibraries)
endif()

set(AFVP_LICENSE_FILES ${PROJECT_SOURCE_DIR}/LICENSE ${PROJECT_SOURCE_DIR}/THIRD-PARTY.md)

if(WIN32 OR APPLE)
    install(FILES ${AFVP_LICENSE_FILES} DESTINATION .)
else()
    install(FILES ${AFVP_LICENSE_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/af-video-player)
endif()
