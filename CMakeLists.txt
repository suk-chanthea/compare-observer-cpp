cmake_minimum_required(VERSION 3.21)

project(CompareObserver VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network
    Concurrent
)

# Source files
set(SOURCES
    src/main.cpp
    src/main_window.cpp
    src/config.cpp
    
    # Core
    src/core/events.cpp
    src/core/models.cpp
    
    # Services
    src/services/file_watcher.cpp
    src/services/telegram_service.cpp
    
    # Utils
    src/utils/helpers.cpp
    
    # UI - Styles
    src/ui/styles.cpp
    
    # UI - Dialogs
    src/ui/dialogs/settings_dialog.cpp
    src/ui/dialogs/log_dialog.cpp
    src/ui/dialogs/file_diff_dialog.cpp
    src/ui/dialogs/git_compare_dialog.cpp
    src/ui/dialogs/change_review_dialog.cpp
    src/ui/dialogs/chunk_review_dialog.cpp
    
    # UI - Widgets
    src/ui/widgets/file_watcher_table.cpp
    src/ui/widgets/custom_text_edit.cpp
    
    # UI - Models
    src/ui/models/log_table_model.cpp
)

# Header files
set(HEADERS
    src/main_window.h
    src/config.h
    
    # Core
    src/core/events.h
    src/core/models.h
    
    # Services
    src/services/file_watcher.h
    src/services/telegram_service.h
    
    # Utils
    src/utils/helpers.h
    
    # UI - Styles
    src/ui/styles.h
    
    # UI - Dialogs
    src/ui/dialogs/settings_dialog.h
    src/ui/dialogs/log_dialog.h
    src/ui/dialogs/file_diff_dialog.h
    src/ui/dialogs/git_compare_dialog.h
    src/ui/dialogs/change_review_dialog.h
    src/ui/dialogs/chunk_review_dialog.h
    
    # UI - Widgets
    src/ui/widgets/file_watcher_table.h
    src/ui/widgets/custom_text_edit.h
    
    # UI - Models
    src/ui/models/log_table_model.h
)

# Create executable
add_executable(${PROJECT_NAME} 
    WIN32  # This makes it a GUI app (no console)
    ${SOURCES}
    ${HEADERS}
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Windows specific settings
if(WIN32)
    # Set application icon (if you have one)
    # set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")
    # target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON_RESOURCE_WINDOWS})
    
    # Enable console for debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE OFF
        )
    endif()
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Post-build: Copy Qt DLLs (handled by windeployqt in build script)
# Installation rules (optional)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)