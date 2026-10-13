# mcs_add_test(<nome> <bibliotecas...>): cria executável de teste Qt Test e registra no CTest.
function(mcs_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Qt6::Test mcs_warnings ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(WIN32)
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
        # windeployqt do app copia Qt6Core*.dll para a mesma pasta dos testes, e esse Qt
        # procura plugins ao lado de si (só qwindows). Aponta para os plugins da instalação.
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
            "QT_PLUGIN_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}")
    endif()
endfunction()
mcs_add_test(tst_toolchain mcs_media)
mcs_add_test(tst_mainwindow mcs_ui)
mcs_add_test(tst_rational mcs_core)

add_library(mcs_test_support STATIC support/synth_media.h support/synth_media.cpp)
target_include_directories(mcs_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mcs_test_support SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(mcs_test_support PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(mcs_test_support PUBLIC Qt6::Core ${FFMPEG_LIBRARIES} PRIVATE mcs_warnings)

mcs_add_test(tst_probe mcs_media mcs_test_support)
mcs_add_test(tst_decoder mcs_media mcs_test_support)
mcs_add_test(tst_mediapool mcs_ui mcs_test_support)
mcs_add_test(tst_editpage mcs_ui mcs_test_support)
mcs_add_test(tst_timeline mcs_core)
mcs_add_test(tst_audiodecoder mcs_media mcs_test_support)
mcs_add_test(tst_timelinevideo mcs_playback mcs_test_support)
mcs_add_test(tst_playback mcs_playback mcs_test_support)
