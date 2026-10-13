add_library(mcs_core STATIC
    core/rational.h
    core/rational.cpp
    core/media_info.h
)
target_include_directories(mcs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mcs_core PUBLIC Qt6::Core PRIVATE mcs_warnings)

add_library(mcs_media STATIC
    media/ffmpeg_info.h
    media/ffmpeg_info.cpp
    media/ffmpeg_util.h
    media/ffmpeg_util.cpp
    media/probe.h
    media/probe.cpp
    media/frame_decoder.h
    media/frame_decoder.cpp
)
target_include_directories(mcs_media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mcs_media SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(mcs_media PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(mcs_media PUBLIC mcs_core Qt6::Gui ${FFMPEG_LIBRARIES} PRIVATE mcs_warnings)

add_library(mcs_ui STATIC
    ui/theme.h
    ui/theme.cpp
    ui/main_window.h
    ui/main_window.cpp
)
target_include_directories(mcs_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(mcs_ui PUBLIC MCS_VERSION="${PROJECT_VERSION}")
target_link_libraries(mcs_ui PUBLIC Qt6::Widgets PRIVATE mcs_warnings)

qt_add_executable(mcs_ai_editor WIN32 MACOSX_BUNDLE app/main.cpp)
target_link_libraries(mcs_ai_editor PRIVATE mcs_ui mcs_media mcs_warnings)

# Review Focus 2: o .exe na pasta de build deve rodar sem Qt no PATH.
if(WIN32)
    add_custom_command(TARGET mcs_ai_editor POST_BUILD
        COMMAND "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}/windeployqt.exe"
                --no-translations --no-compiler-runtime --verbose 0
                "$<TARGET_FILE:mcs_ai_editor>"
        VERBATIM)
endif()

add_test(NAME app_starts COMMAND mcs_ai_editor --smoke)
# No Windows usa a plataforma normal: windeployqt copia só o plugin qwindows, não o offscreen.
if(WIN32)
    # PATH só com System32: o teste prova o deploy (windeployqt), não um Qt achado no PATH.
    set_tests_properties(app_starts PROPERTIES ENVIRONMENT "PATH=$ENV{SystemRoot}\\System32")
else()
    set_tests_properties(app_starts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
