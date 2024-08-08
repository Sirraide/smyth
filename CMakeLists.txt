cmake_minimum_required(VERSION 3.27)
project(Smyth VERSION 0.1 LANGUAGES CXX C)

## ============================================================================
##  Global CMake Variables.
## ============================================================================
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(
    -DSMYTH_ISSUES_URL="https://github.com/Sirraide/smyth/issues/new"
)

## ============================================================================
##  Global compiler options.
## ============================================================================
## Turn on diagnostics colours.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

## Use mold as the default linker, if it exists.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(MOLD_LINKER "mold")
    if (MOLD_LINKER)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-fuse-ld=mold)
        endif()
        add_link_options(-fuse-ld=mold)
    endif()
endif()

## ============================================================================
##  Compiler options.
## ============================================================================
add_library(options INTERFACE)

## Flags for Clang and GCC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(options INTERFACE
        ## Warnings.
        -Wall -Wextra     # Enable ‘all’ warnings.
        -Wundef           # Invalid #undef or undefined macro in #if.
        -Wcast-align      # Casting that changes alignment.
        -Wconversion      # Implicit conversions.
        -Wsign-conversion # Implicit sign conversions.
        -Wformat=2        # Stricter format checking.

        ## Disabled warnings.
        -Wno-unused-function
        -Wno-unused-local-typedefs

        ## NULL Errors.
        -Werror=nonnull # Passing NULL to nonnull parameter.

        ## Memory Errors.
        -Werror=address              # Suspicious use of addresses.
        -Werror=init-self            # Initialization of a variable with itself.
        -Werror=uninitialized

        ## Return type.
        -Werror=return-type
        -Wmissing-noreturn

        ## C/C++.
        -Werror=implicit-fallthrough
        -Werror=missing-include-dirs # User-specified include dir does not exist.
        -Werror=pointer-arith        # Disallow void* and function pointer arithmetic.
        -Werror=string-compare       # Nonsensical string comparisons.
        -Werror=switch               # Missing switch cases.
        # -Werror=switch-enum          # Switch on enum (even if there is a default case).
        -Werror=write-strings        # Strings in C should be const char*.

        ## C++.
        -Werror=missing-field-initializers
        -Werror=non-virtual-dtor
        -Werror=pessimizing-move
    )
endif()

## Additional flags for GCC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(options INTERFACE
        -Wlogical-op      # Duplicate or unintended logical operators.
        -Werror=invalid-memory-model # For atomics.
        -Werror=maybe-uninitialized
        -Werror=missing-requires
        -Werror=return-local-addr
    )
endif()

## Additional flags for Clang.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(options INTERFACE
        -fno-assumptions # Currently horribly broken in Clang.
        -Werror=dangling
        -Werror=return-stack-address
    )
endif()

## Flags for MSVC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(options INTERFACE
        /W4 # Enable ‘all’ warnings.

        # Allow unnamed structs/unions.
        /wd4201

        # Source code is UTF-8.
        /utf-8
    )
endif()

## On Windows, don’t suggest the _s nonsense functions.
if (WIN32)
    target_compile_definitions(options INTERFACE
        _CRT_SECURE_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS_GLOBALS
        _CRT_NONSTDC_NO_WARNINGS
    )
endif()

## Address Sanitiser.
if (ENABLE_ASAN)
    target_compile_options(options INTERFACE -fsanitize=address)
    target_link_options(options INTERFACE -fsanitize=address)
endif()

## Debug/Release flags.
if (NOT MSVC)
    target_compile_definitions(options INTERFACE
        $<$<CONFIG:RELEASE>:-DNDEBUG>
    )
    target_compile_options(options INTERFACE
        $<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3>
        $<$<CONFIG:RELEASE>:-O3 -march=native>
    )
    target_link_options(options INTERFACE
        $<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3 -rdynamic>
        $<$<CONFIG:RELEASE>:-O3 -march=native>
    )
else()
    target_compile_options(options INTERFACE
        $<$<CONFIG:DEBUG>:/Od>
        $<$<CONFIG:RELEASE>:/O2>
    )
endif()

## ============================================================================
##  Submodules and include dirs.
## ============================================================================
include(FetchContent)

## Find ICU.
set(ICU_FIND_COMPONENTS uc i18n data)
include(FindICU)
if (NOT ICU_FOUND)
    message(FATAL_ERROR "ICU not found. Required components are: uc, i18n, data.")
endif ()

## Add libbase.
FetchContent_Declare(
    libbase
    GIT_REPOSITORY https://github.com/Sirraide/libbase
    GIT_TAG        0fb774c  ## Last version before modules refactor.
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/libbase"
)

## Add nlohmann/json.
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/json"
)

## Add libassert.
#[[FetchContent_Declare(
  libassert
  GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert.git
  GIT_TAG        v2.0.2
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/libassert"
)]]

## Download them.
message(STATUS "Downloading dependencies ...")
FetchContent_MakeAvailable(
    libbase
    json
    #[[libassert]]
)

## ‘src’ should be an include directory.
target_include_directories(options INTERFACE src include)

## As well as everything in ‘libs’.
if (EXISTS ${PROJECT_SOURCE_DIR}/libs)
    file(GLOB libs ${PROJECT_SOURCE_DIR}/libs/*)

    ## Add the include directory to the include path, or the root
    ## directory if there is no include directory.
    foreach(lib ${libs})
        if (IS_DIRECTORY ${lib}/include)
            target_include_directories(options INTERFACE ${lib}/include)
        elseif(IS_DIRECTORY ${lib})
            target_include_directories(options INTERFACE ${lib})
        endif()
    endforeach()

    ## Also add all of them as subdirectories if they have a CMakeLists.txt.
    foreach (lib ${libs})
        if (EXISTS ${lib}/CMakeLists.txt)
            add_subdirectory(${lib})
        endif()
    endforeach()
endif()

## Link against libraries.
target_link_libraries(options INTERFACE
    libbase
    nlohmann_json::nlohmann_json
)

## ============================================================================
##  Executables and libraries.
## ============================================================================
file(GLOB_RECURSE smyth-lib-sources lib/*.cc include/Smyth/*.hh)
add_library(smyth-lib STATIC ${smyth-lib-sources} )
set_target_properties(smyth-lib PROPERTIES OUTPUT_NAME smyth)
target_link_libraries(smyth-lib PRIVATE options ICU::i18n ICU::uc ICU::data)
add_compile_definitions(LEXURGY_ROOT="${PROJECT_SOURCE_DIR}/thirdparty/lexurgy")

## ============================================================================
##  Qt Stuff
## ============================================================================
set(CMAKE_AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR}/ui")

## Can’t set this earlier; otherwise, FindICU will find the *libraries* of whatever
## outdated version of ICU is shipped with Qt, but the *headers* of the system ICU,
## which simply does not work.
if (NOT DEFINED SMYTH_QT_PREFIX_PATH)
    message(FATAL_ERROR "Set SMYTH_QT_PREFIX_PATH to the install prefix of your Qt installation.")
endif()
set(CMAKE_PREFIX_PATH "${SMYTH_QT_PREFIX_PATH}")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Gui Qml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui Qml)

if (${QT_VERSION_MAJOR} LESS 6)
    message(FATAL_ERROR "Qt6 or later is required, but Qt${QT_VERSION_MAJOR} was found.")
endif()

file(GLOB_RECURSE smyth-sources src/*.cc include/UI/*.hh ui/*.ui)
qt_add_executable(smyth MANUAL_FINALIZATION ${smyth-sources})

target_link_libraries(smyth PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    smyth-lib
    options
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER org.nguh.smyth)
endif()
set_target_properties(smyth PROPERTIES
    ${BUNDLE_ID_OPTION}
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS smyth
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(smyth)
