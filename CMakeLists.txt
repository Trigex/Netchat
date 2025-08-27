cmake_minimum_required(VERSION 3.24)
project(Netchat VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)

# ===================================================================
#  Fetch and Configure Dyad.c for networking
# ===================================================================
include(FetchContent)
FetchContent_Declare(
        dyad
        GIT_REPOSITORY https://github.com/rxi/dyad.git
        GIT_TAG        master # The master branch is the only option here
)
FetchContent_MakeAvailable(dyad)

# ncurses for client
find_package(Curses REQUIRED)

# Recursively find all .cpp and .h files under src/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.c
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/src/*.hpp
        ${dyad_SOURCE_DIR}/src/*.c
)

# Include project sources and also include the dyad source
add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${dyad_SOURCE_DIR}/src/dyad.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
)

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${dyad_SOURCE_DIR}/src"
        ${CURSES_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CURSES_LIBRARIES}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    # Create a symbolic link from the source directory to the generated file
    execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()


if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()
