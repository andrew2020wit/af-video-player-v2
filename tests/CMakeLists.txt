function(afvp_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE FALSE MACOSX_BUNDLE FALSE)
    target_link_libraries(${name} PRIVATE afvp_core afvp_export Qt6::Test)
    target_compile_definitions(${name} PRIVATE AFVP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
    add_test(NAME ${name} COMMAND ${name})
    afvp_copy_runtime_dlls(${name})

    if(WIN32)
        # Qt DLLs are not deployed next to test executables in the build tree.
        set_tests_properties(${name} PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
    endif()
endfunction()

afvp_add_test(tst_subtitleparser)
afvp_add_test(tst_subtitlemerge)
afvp_add_test(tst_wordlookup)
afvp_add_test(tst_languages)
afvp_add_test(tst_storage)
afvp_add_test(tst_exportnaming)
afvp_add_test(tst_autoexport)
afvp_add_test(tst_subtitlesearch)
afvp_add_test(tst_mediafiles)
afvp_add_test(tst_historygroups)
afvp_add_test(tst_misc)
afvp_add_test(tst_export)

# QML has no compile-time checks: a handler of a missing signal or an unknown property only fails when the window
# loads. qmllint checks the app's QML with the arguments of its af-video-player_qmllint target; any warning fails.
add_test(NAME qmllint
    COMMAND Qt6::qmllint --max-warnings 0 @${PROJECT_BINARY_DIR}/src/app/.rcc/qmllint/af-video-player.rsp)
