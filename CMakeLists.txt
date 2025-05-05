cmake_minimum_required(VERSION 3.11)
project(Snake CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SFML as static libraries")
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.5.1
)

FetchContent_MakeAvailable(SFML)


add_executable(Snake main.cpp)


target_link_libraries(Snake PRIVATE sfml-graphics sfml-window sfml-system)


foreach(ASSET_FILE ${ASSET_FILES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ASSET_FILE}")
        add_custom_command(TARGET Snake POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${ASSET_FILE}"
                $<TARGET_FILE_DIR:Snake>
                COMMENT "Copying ${ASSET_FILE} to build directory"
        )
    else()
        message(WARNING "Asset file not found: ${CMAKE_CURRENT_SOURCE_DIR}/${ASSET_FILE}")
    endif()
endforeach()