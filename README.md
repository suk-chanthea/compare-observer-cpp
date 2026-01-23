# Compare Observer - Architecture Documentation

## 📁 Project Structure

```
compare-observer/
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── main_window.{h,cpp}         # Main window (1847 lines)
│   ├── config.{h,cpp}              # Configuration management
│   │
│   ├── core/                       # Core business logic
│   │   ├── events.{h,cpp}          # Event definitions
│   │   └── models.{h,cpp}          # Data models
│   │
│   ├── services/                   # Business services
│   │   ├── file_watcher.{h,cpp}    # File monitoring service
│   │   └── telegram_service.{h,cpp}# Telegram notifications
│   │
│   ├── ui/                         # User interface
│   │   ├── styles.{h,cpp}          # Application styles
│   │   ├── ui_constants.h          # UI constants (NEW)
│   │   │
│   │   ├── dialogs/                # Dialog windows
│   │   │   ├── settings_dialog
│   │   │   ├── log_dialog
│   │   │   ├── file_diff_dialog
│   │   │   ├── change_review_dialog
│   │   │   └── chunk_review_dialog
│   │   │
│   │   ├── widgets/                # Custom widgets
│   │   │   ├── file_watcher_table
│   │   │   └── custom_text_edit
│   │   │
│   │   └── models/                 # UI data models
│   │       └── log_table_model
│   │
│   └── utils/                      # Utility functions
│       └── helpers.{h,cpp}         # Helper functions
│
├── resources/                      # Application resources
│   ├── app.rc                      # Windows resource file
│   └── application.ico             # Application icon
│
├── build/                          # Build output
├── dist/                           # Distribution files
├── CMakeLists.txt                  # Build configuration
└── build.sh                        # Build script
```

## 🏛️ Architecture Patterns

### 1. **MVC-like Pattern**
- **Model**: `SystemConfigData`, file data
- **View**: Qt widgets, dialogs
- **Controller**: `FileWatcherApp` (main window)

### 2. **Service Layer**
```
FileWatcherApp
    ↓ uses
FileWatcher Service ← monitors → File System
    ↓ signals
FileWatcherApp → updates → UI
```

### 3. **Qt Object Ownership**
- Parent-child relationships for automatic memory management
- `std::unique_ptr` for owned dialogs and services
- Proper signal/slot connections with Qt::QueuedConnection for threads

## 🔄 Data Flow

### Watching File Changes
```
1. User clicks "Start Watching"
2. FileWatcherApp::onToggleWatching()
3. FileWatcherApp::startWatching()
4. Creates WatcherThread for each system
5. WatcherThread monitors file system
6. Emits signals on changes
7. FileWatcherApp::handleFileChanged()
8. Updates UI and FileWatcherTable
9. Optionally sends Telegram notification
```

### Settings Management
```
1. User opens Settings dialog
2. SettingsDialog loads from QSettings
3. User edits configuration
4. FileWatcherApp::onSettingsClicked()
5. Saves settings via saveSettings()
6. Rebuilds system panels
7. Recreates file watchers
```

## 🧩 Key Components

### FileWatcherApp (Main Window)
**Responsibilities:**
- UI orchestration
- Settings management
- Watcher lifecycle
- Event handling

**Key Methods:**
- `setupUI()` - Creates user interface
- `startWatching()` - Initializes file monitoring
- `handleFileChanged()` - Processes file change events
- `updateStatusLabel()` - Updates status display

### WatcherThread (File Monitor)
**Responsibilities:**
- Monitor file system changes
- Emit signals for file events
- Respect exclusion rules

**Signals:**
- `fileChanged(QString path)`
- `fileCreated(QString path)`
- `fileDeleted(QString path)`

### SettingsDialog
**Responsibilities:**
- System configuration
- Telegram settings
- Exclusion rules

### FileWatcherTable
**Responsibilities:**
- Display file changes
- Show file status (Modified/Created/Deleted)
- Copy/diff functionality

## 🎨 UI Architecture

### Layout Hierarchy
```
QMainWindow (FileWatcherApp)
└── Central Widget
    └── QVBoxLayout (main)
        ├── Title Label
        ├── Control Buttons Row
        ├── Info Container (Select Systems + Status)
        └── Scroll Area
            └── System Panels (dynamic)
                ├── System 1 Panel
                ├── System 2 Panel
                └── ...
```

### Dynamic UI Elements
- **System Checkboxes** - Created in `updateSystemCheckboxes()`
- **Status Widgets** - Recreated in `updateStatusLabel()`
- **System Panels** - Built in `rebuildSystemPanels()`

## 🔧 Build System

### CMake Configuration
- Qt6 integration (Core, Gui, Widgets, Network, Concurrent)
- MSYS2/MinGW64 toolchain
- Resource compilation (app icon)

### Build Process
```bash
./build.sh
    ↓
CMake Configure
    ↓
Ninja Build
    ↓
Create dist/
    ↓
Copy .exe + dependencies (windeployqt6)
```

## 📦 Dependencies

### Required
- Qt6 (Core, Gui, Widgets, Network, Concurrent)
- MinGW-w64 compiler
- CMake 3.21+
- Ninja build system

### Optional
- Telegram Bot API (for notifications)

## 🔐 Thread Safety

### Cross-Thread Communication
- Uses `Qt::QueuedConnection` for all watcher signals
- Thread-safe signal/slot mechanism
- No shared mutable state between threads

### Thread Model
```
Main Thread (UI)
    ↓ creates
WatcherThread 1 → File System
WatcherThread 2 → File System
WatcherThread N → File System
    ↓ signals (queued)
Main Thread (updates UI)
```

## 🎯 Design Decisions

### Why Single Main Window?
- Simpler state management
- All features accessible from one place
- Better user experience

### Why Threads for Watching?
- Non-blocking UI
- Monitor multiple directories simultaneously
- Better performance

### Why Qt Framework?
- Cross-platform (future Linux/Mac support)
- Rich widget library
- Built-in threading support
- Excellent file system API

## 📈 Performance Considerations

1. **File Watching** - Uses QFileSystemWatcher (efficient OS-level notifications)
2. **UI Updates** - Batched through Qt event system
3. **Memory** - Parent-child ownership prevents leaks
4. **Threading** - Prevents UI blocking during I/O

## 🔮 Future Improvements

### Potential Features
- [ ] Multiple profiles/configurations
- [ ] File filter patterns (*.cpp, *.h)
- [ ] Real-time diff preview
- [ ] Export change logs
- [ ] Git integration
- [ ] Dark/Light theme toggle

### Technical Debt
- Extract large methods in `main_window.cpp`
- Create UI factory for widget creation
- Add unit tests for business logic
- Implement plugin system for extensibility

## 📚 Resources

- [Qt Documentation](https://doc.qt.io/qt-6/)
- [CMake Documentation](https://cmake.org/documentation/)
- [Project README](README.md)
- [Code Improvements](CODE_IMPROVEMENTS.md)

