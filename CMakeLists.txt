cmake_minimum_required(VERSION 3.13)

set(PLATFORM "SWITCH" CACHE STRING "Platform for build target.")

if (${PLATFORM} STREQUAL "SWITCH")
    set(PLATFORM_SWITCH ON PARENT_SCOPE BOOL "For borealis build.")
elseif (${PLATFORM} STREQUAL "DESKTOP")
    set(PLATFORM_DESKTOP ON PARENT_SCOPE BOOL "For borealis build.")
else ()
    message(FATAL_ERROR "Please set build target. Example: -DPLATFORM=SWITCH or -DPLATFORM=DESKTOP")
endif ()

# ui library
set(BOREALIS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/borealis)
set(BOREALIS_LIBRARY ${BOREALIS_DIR}/library)

include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)

# NintendoSwitch
cmake_dependent_option(BUILTIN_NSP "Built in NSP forwarder" OFF "PLATFORM_SWITCH" OFF)

# mpv related
# If your system does not support OpenGL(ES), you can use software rendering, but it will affect performance.
option(MPV_SW_RENDER "Using CPU to draw videos" OFF)
if (MPV_SW_RENDER)
    list(APPEND APP_PLATFORM_OPTION -DMPV_SW_RENDER)
endif ()

# On systems that do not support framebuffer, let MPV to draw to full screen and
# then cover unnecessary areas with UI.
option(MPV_NO_FB "Using system provided framebuffer" OFF)
if (MPV_NO_FB)
    list(APPEND APP_PLATFORM_OPTION -DMPV_NO_FB)
endif()

# Bundle mpv.dll into wiliwili.exe (Windows only)
cmake_dependent_option(MPV_BUNDLE_DLL "Bundle mpv.dll" OFF "USE_LIBROMFS;WIN32" OFF)
if (MPV_BUNDLE_DLL)
    list(APPEND APP_PLATFORM_OPTION -DMPV_BUNDLE_DLL)
    list(APPEND APP_PLATFORM_LIB MemoryModule)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-D_DEBUG)
    add_definitions(-D_GLIBCXX_ASSERTIONS)
    if (DEBUG_SANITIZER)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -O0 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address")
    endif ()
endif ()

# toolchain
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

git_info(GIT_TAG_VERSION GIT_TAG_SHORT)
list(APPEND APP_PLATFORM_OPTION -DBUILD_TAG_VERSION=${GIT_TAG_VERSION} -DBUILD_TAG_SHORT=${GIT_TAG_SHORT})

add_definitions(-DFONS_HASH_LUT_SIZE=4096)

set(CMAKE_CXX_STANDARD 20)

# project info
project(nxplayer)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_REVISION "0")
if (NOT VERSION_BUILD)
    set(VERSION_BUILD "0")
endif ()
set(PROJECT_AUTHOR "anonymous5l")
set(PACKAGE_NAME cn.anonymous5l.nxplayer)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(APP_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/icon.jpg)

option(BRLS_UNITY_BUILD "Unity build" OFF)

find_package(Threads REQUIRED)
list(APPEND APP_PLATFORM_LIB ${CMAKE_THREAD_LIBS_INIT})

if (PLATFORM_DESKTOP)
    find_package(MPV REQUIRED)

    message(STATUS "Found libmpv: ${MPV_VERSION} ${MPV_INCLUDE_DIR} ${MPV_LIBRARY}")
    list(APPEND APP_PLATFORM_INCLUDE ${MPV_INCLUDE_DIR})
    if (NOT MPV_BUNDLE_DLL)
        list(APPEND APP_PLATFORM_LIB ${MPV_LIBRARY})
    endif ()
else ()
    find_package(PkgConfig REQUIRED)

    pkg_search_module(MPV REQUIRED mpv)
    message(STATUS "Found libmpv: ${MPV_VERSION} ${MPV_INCLUDE_DIRS} ${MPV_STATIC_LIBRARIES}")
    list(APPEND APP_PLATFORM_INCLUDE ${MPV_INCLUDE_DIRS})
    list(APPEND APP_PLATFORM_LIB ${MPV_STATIC_LIBRARIES})
    link_directories(${MPV_LIBRARY_DIRS})
    if (PLATFORM_IOS)
        list(APPEND APP_PLATFORM_LIB "-framework CoreMedia -framework CoreText -framework VideoToolbox")
    endif ()
endif ()

list(APPEND APP_PLATFORM_OPTION
    -DBUILD_PACKAGE_NAME=${PACKAGE_NAME}
    -DBUILD_VERSION_MAJOR=${VERSION_MAJOR}
    -DBUILD_VERSION_MINOR=${VERSION_MINOR}
    -DBUILD_VERSION_REVISION=${VERSION_REVISION}
)

if (BUILTIN_NSP)
    list(APPEND APP_PLATFORM_LIB nsp)
    list(APPEND APP_PLATFORM_OPTION -DBUILTIN_NSP)
    add_subdirectory(scripts/switch-forwarder)
endif ()

if (USE_LIBROMFS)
    add_libromfs(${PROJECT_NAME} ${PROJECT_RESOURCES})
endif ()

# set resources dir
if (CUSTOM_RESOURCES_DIR)
    set(BRLS_RESOURCES_DIR ${CUSTOM_RESOURCES_DIR})
elseif (INSTALL)
    set(BRLS_RESOURCES_DIR ${CMAKE_INSTALL_PREFIX}/share/wiliwili)
else ()
    set(BRLS_RESOURCES_DIR ".")
endif ()

file(GLOB_RECURSE MAIN_SRC src/source/*.cpp)
if (PLATFORM_SWITCH)
    list(APPEND MAIN_SRC ${BOREALIS_LIBRARY}/lib/platforms/switch/switch_wrapper.c)
endif ()

add_subdirectory(thirdparty)

program_target(${PROJECT_NAME} "${MAIN_SRC}")

# building release file
if (PLATFORM_DESKTOP)
    # Copy resources to build dir
    add_custom_target(${PROJECT_NAME}.data
            COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
            COMMAND "echo" "copy resources"
            )
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)
elseif (PLATFORM_SWITCH)
    set(BUILD_FONT_DIR ${CMAKE_BINARY_DIR}/resources/font)
    if (GIT_TAG_VERSION)
        string(SUBSTRING ${GIT_TAG_VERSION} 1 -1 APP_VERSION)
    endif ()
    if (BUILTIN_NSP)
        add_dependencies(${PROJECT_NAME} nsp_forwarder.nsp)
    endif ()
    if (USE_DEKO3D)
        gen_dksh("${PROJECT_RESOURCES}/shaders")
    endif ()
    add_custom_target(${PROJECT_NAME}.nro
        DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create ${PROJECT_NAME} ${PROJECT_AUTHOR} ${APP_VERSION} ${PROJECT_NAME}.nacp
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E remove -f ${BUILD_FONT_DIR}/*.txt ${BUILD_FONT_DIR}/switch_font.ttf
            ${BUILD_FONT_DIR}/keymap*.ttf
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON}
            --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
        ALL
    )
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE src/include ${APP_PLATFORM_INCLUDE})
target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections -Wunused-variable ${APP_PLATFORM_OPTION})
target_link_libraries(${PROJECT_NAME} PRIVATE borealis ${APP_PLATFORM_LIB})
target_link_options(${PROJECT_NAME} PRIVATE ${APP_PLATFORM_LINK_OPTION})
