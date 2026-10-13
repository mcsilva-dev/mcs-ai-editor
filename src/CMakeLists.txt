add_library(mcs_media STATIC
    media/ffmpeg_info.h
    media/ffmpeg_info.cpp
)
target_include_directories(mcs_media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mcs_media SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(mcs_media PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(mcs_media PUBLIC Qt6::Core ${FFMPEG_LIBRARIES} PRIVATE mcs_warnings)
