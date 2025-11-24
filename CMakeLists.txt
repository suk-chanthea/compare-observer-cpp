cmake_minimum_required(VERSION 3.21)

project(CompareObserver 
    LANGUAGES CXX 
    VERSION 1.0.0
    DESCRIPTION "File Watcher Application with Telegram Notifications"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 COMPONENTS 
    Core
    Gui
    Widgets
    Concurrent
    Network
    REQUIRED
)

# Find additional libraries
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/main.cpp
    src/config.h
    src/config.cpp
    src/core/models.h
    src/core/models.cpp
    src/core/events.h
    src/core/events.cpp
    src/services/file_watcher.h
    src/services/file_watcher.cpp
    src/services/telegram_service.h
    src/services/telegram_service.cpp
    src/utils/helpers.h
    src/utils/helpers.cpp
    src/ui/styles.h
    src/ui/styles.cpp
    src/ui/dialogs/log_dialog.h
    src/ui/dialogs/log_dialog.cpp
    src/ui/dialogs/settings_dialog.h
    src/ui/dialogs/settings_dialog.cpp
    src/ui/dialogs/file_diff_dialog.h
    src/ui/dialogs/file_diff_dialog.cpp
    src/ui/dialogs/git_compare_dialog.h
    src/ui/dialogs/git_compare_dialog.cpp
    src/ui/dialogs/change_review_dialog.h
    src/ui/dialogs/change_review_dialog.cpp
    src/ui/dialogs/chunk_review_dialog.h
    src/ui/dialogs/chunk_review_dialog.cpp
    src/ui/widgets/custom_text_edit.h
    src/ui/widgets/custom_text_edit.cpp
    src/ui/widgets/file_watcher_table.h
    src/ui/widgets/file_watcher_table.cpp
    src/ui/models/log_table_model.h
    src/ui/models/log_table_model.cpp
    src/main_window.h
    src/main_window.cpp
)

# Create executable
add_executable(CompareObserver ${SOURCES})

# Link Qt libraries
target_link_libraries(CompareObserver
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Platform-specific settings
if(WIN32)
    set_target_properties(CompareObserver PROPERTIES
        WIN32_EXECUTABLE ON
    )
    target_link_libraries(CompareObserver ws2_32)
endif()

# Set output directory
set_target_properties(CompareObserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)


