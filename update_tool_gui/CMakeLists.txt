cmake_minimum_required(VERSION 3.10)
project(eso_update_tool_gui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Поиск Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Concurrent Core5Compat)

# Поиск OpenSSL
# На macOS с Homebrew может потребоваться указать OPENSSL_ROOT_DIR
if(APPLE AND NOT DEFINED OPENSSL_ROOT_DIR)
    # Пробуем найти OpenSSL через Homebrew
    execute_process(
        COMMAND brew --prefix openssl@3
        OUTPUT_VARIABLE HOMEBREW_OPENSSL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(EXISTS "${HOMEBREW_OPENSSL}")
        set(OPENSSL_ROOT_DIR "${HOMEBREW_OPENSSL}")
        message(STATUS "Found Homebrew OpenSSL@3: ${OPENSSL_ROOT_DIR}")
    else()
        execute_process(
            COMMAND brew --prefix openssl
            OUTPUT_VARIABLE HOMEBREW_OPENSSL
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(EXISTS "${HOMEBREW_OPENSSL}")
            set(OPENSSL_ROOT_DIR "${HOMEBREW_OPENSSL}")
            message(STATUS "Found Homebrew OpenSSL: ${OPENSSL_ROOT_DIR}")
        endif()
    endif()
endif()

find_package(OpenSSL REQUIRED)

# Поиск Zlib
find_package(ZLIB REQUIRED)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/KeyGeneratorWidget.cpp
    src/PatchCreatorWidget.cpp
    src/PckArchiverWidget.cpp
    src/LauncherPatcherWidget.cpp
    src/SettingsWidget.cpp
    src/LogWidget.cpp
    src/core/Crypto.cpp
    src/core/FileSystem.cpp
    src/core/Logger.cpp
    src/core/LauncherPatcher.cpp
    src/core/PckArchiver.cpp
    src/core/PatchGenerator.cpp
)

set(HEADERS
    src/MainWindow.h
    src/KeyGeneratorWidget.h
    src/PatchCreatorWidget.h
    src/PckArchiverWidget.h
    src/LauncherPatcherWidget.h
    src/SettingsWidget.h
    src/LogWidget.h
    src/core/Crypto.h
    src/core/FileSystem.h
    src/core/Logger.h
    src/core/LauncherPatcher.h
    src/core/PckArchiver.h
    src/core/PatchGenerator.h
)

# Исполняемый файл
add_executable(eso_update_tool_gui WIN32 MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
)

# Include директории
target_include_directories(eso_update_tool_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_INCLUDE_DIR}
)

# Подавляем предупреждения о deprecated функциях OpenSSL 3.0
if(APPLE)
    target_compile_options(eso_update_tool_gui PRIVATE
        -Wno-deprecated-declarations
    )
endif()

# Линковка
target_link_libraries(eso_update_tool_gui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Core5Compat
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
)

# Windows специфичные настройки
if(WIN32)
    target_link_libraries(eso_update_tool_gui PRIVATE
        ws2_32
        crypt32
    )
    set_target_properties(eso_update_tool_gui PROPERTIES
        WIN32_EXECUTABLE TRUE
        RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc
    )
endif()

# macOS специфичные настройки
if(APPLE)
    set_target_properties(eso_update_tool_gui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.eso.updatetool"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

# Установка
install(TARGETS eso_update_tool_gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
