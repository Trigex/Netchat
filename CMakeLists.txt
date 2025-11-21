cmake_minimum_required(VERSION 3.24)
project(Netchat VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===================================================================
#  Fetch and Configure Dyad.c for networking
# ===================================================================
include(FetchContent)
FetchContent_Declare(
    dyad
    GIT_REPOSITORY https://github.com/rxi/dyad.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(dyad)

# ===================================================================
#  Fetch and Compile Curses (Cross-Platform)
# ===================================================================
# Strategy:
# - Windows: Use PDCurses (Native CMake support, works in Command Prompt/PowerShell)
# - Unix:    Use GNU NCurses (Build via ExternalProject using Autotools)

if(WIN32)
    message(STATUS "Fetching PDCurses for Windows...")
    
    # PDCurses has a CMakeLists.txt, so we can use FetchContent directly
    FetchContent_Declare(
        pdcurses
        GIT_REPOSITORY https://github.com/wmcbrine/PDCurses.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(pdcurses)

    # Create a unified alias to match the Unix logic
    add_library(curs_lib ALIAS pdcurses)
    
    # PDCurses headers are in the root source dir
    set(CURS_INCLUDE_DIRS ${pdcurses_SOURCE_DIR})
else()
    message(STATUS "Fetching and Compiling GNU NCurses for Unix...")
    
    include(ExternalProject)
    include(ProcessorCount)
    ProcessorCount(N)
    
    set(NCURSES_INSTALL_DIR ${CMAKE_BINARY_DIR}/ncurses_install)
    
    # FIX 1: Point specifically to the 'ncurses' subfolder where the header actually lives
    set(NCURSES_INCLUDE_DIR "${NCURSES_INSTALL_DIR}/include" "${NCURSES_INSTALL_DIR}/include/ncurses")
    set(NCURSES_LIB_DIR     "${NCURSES_INSTALL_DIR}/lib")
    
    # Create directories to satisfy CMake validation
    file(MAKE_DIRECTORY ${NCURSES_INSTALL_DIR}/include)
    file(MAKE_DIRECTORY ${NCURSES_INSTALL_DIR}/include/ncurses)

    if(NOT N EQUAL 0)
        set(PARALLEL_JOBS -j${N})
    else()
        set(PARALLEL_JOBS "")
    endif()

    ExternalProject_Add(
        ncurses_build
        URL https://ftp.gnu.org/pub/gnu/ncurses/ncurses-6.4.tar.gz
        PREFIX ${CMAKE_BINARY_DIR}/ncurses_build
        INSTALL_DIR ${NCURSES_INSTALL_DIR}
        CONFIGURE_COMMAND <SOURCE_DIR>/configure 
                          --prefix=<INSTALL_DIR>
                          --disable-shared 
                          --enable-static 
                          --without-ada 
                          --without-manpages 
                          --without-progs 
                          --without-tests
                          --enable-sigwinch
                          --disable-db-install
                          --with-default-terminfo-dir=/usr/share/terminfo
                          --with-terminfo-dirs=/usr/share/terminfo:/lib/terminfo:/usr/local/share/terminfo
        BUILD_COMMAND make ${PARALLEL_JOBS}
        INSTALL_COMMAND make install
    )

    add_library(curs_lib STATIC IMPORTED)
    set_target_properties(curs_lib PROPERTIES
        IMPORTED_LOCATION "${NCURSES_LIB_DIR}/libncurses.a"
        INTERFACE_INCLUDE_DIRECTORIES "${NCURSES_INCLUDE_DIR}"
    )
    
    add_dependencies(curs_lib ncurses_build)
    
    # FIX 2: Ensure the variable passed to the main target includes both paths
    set(CURS_INCLUDE_DIRS ${NCURSES_INCLUDE_DIR})
endif()
# ===================================================================
#  Project Sources
# ===================================================================

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

# ===================================================================
#  Linking and Includes
# ===================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${dyad_SOURCE_DIR}/src"
    ${CURS_INCLUDE_DIRS}
)

# Link against our fetched curses library (curs_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE
    curs_lib
)

# Windows specific linking for sockets
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

# Create symlink to compile_commands.json (Unix helper)
if(UNIX)
    execute_process(
        COMMAND ln -sf ${CMAKE_BINARY_DIR}/compile_commands.json .
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        ERROR_QUIET
    )
endif()
