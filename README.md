# Compare Observer - C++ Qt Application

A modern C++ Qt 6 file monitoring application with Telegram notifications.

## 🚀 Quick Start

### Two Commands to Run:

```bash
# Command 1: Setup Qt MSVC support (first time only, ~15 min)
build_app.cmd

# Command 2: Build and run the app (~5 min)
build_app.cmd
```

## ✨ Features

- **Real-time File Monitoring** - Watch directories for changes (create, modify, delete)
- **File Comparison** - View diffs with syntax highlighting
- **Telegram Notifications** - Send change alerts to Telegram
- **Settings Persistence** - Saves configuration across sessions
- **Activity Logging** - Complete event history
- **Multi-threaded** - Responsive UI with background monitoring

## 📋 Project Structure

```
src/
├── main.cpp                 # Application entry point
├── main_window.h/cpp        # Main window (FileWatcherApp)
├── config.h/cpp             # Configuration constants
├── core/                    # Core business logic
│   ├── models.h/cpp         # FileChangeEntry data model
│   └── events.h/cpp         # Custom Qt events
├── services/                # Services
│   ├── file_watcher.h/cpp   # WatcherThread (QFileSystemWatcher)
│   └── telegram_service.h/cpp # TelegramService (QNetworkAccessManager)
├── utils/                   # Utilities
│   └── helpers.h/cpp        # Helper functions (MD5, file I/O)
└── ui/                      # User interface
    ├── styles.h/cpp         # UI stylesheets
    ├── dialogs/             # 6 dialog windows
    ├── widgets/             # Custom widgets
    └── models/              # Qt table models
```

## 🛠️ System Requirements

- Windows 10 or later
- Visual Studio 2026 Community (C++ tools) ✅ Installed
- Qt 6.10.x with MSVC 2022 64-bit 📦 Will install on first run
- CMake 3.21+ ✅ Installed
- OpenSSL 3.x ✅ Installed

## 🏗️ Build Info

- **Language:** C++17
- **GUI Framework:** Qt 6 (Core, Gui, Widgets, Network, Concurrent)
- **Build System:** CMake 3.21+
- **Compiler:** MSVC 2022 (Visual Studio)
- **Source Files:** 34 C++ files
- **Project Size:** 0.07 MB (source only)

## 🔧 How It Works

1. **FileWatcherApp** - Main window, orchestrates all components
2. **WatcherThread** - Uses QFileSystemWatcher to monitor directories
3. **FileChangeEntry** - Models file changes with old/new content
4. **TelegramService** - Sends notifications via Telegram Bot API
5. **Dialogs** - Settings, logs, file diffs, review windows

## ⚙️ Configuration

Settings are saved automatically using QSettings:
- Windows: `HKEY_CURRENT_USER\Software\CompareObserver`
- Linux: `~/.config/CompareObserver`
- macOS: `~/Library/Preferences/com.CompareObserver`

## 📝 License

This project is provided as-is for file monitoring and notification purposes.

---

**Status:** ✅ Ready to build!  
**Last Updated:** Nov 24, 2025  
**Build Time:** ~5-10 minutes (after Qt setup)

- **Change Review**: Review and selectively apply file changes

## Installation

### Prerequisites

- Python 3.8 or higher
- pip (Python package installer)

### Setup

1. Clone or download the repository:
```bash
git clone <repository-url>
cd compare_observer
```

2. Create a virtual environment (recommended):
```bash
# Windows
python -m venv venv
venv\Scripts\activate

# Linux/Mac
python3 -m venv venv
source venv/bin/activate
```

3. Install dependencies:
```bash
pip install -r requirements.txt
```

## Running the Application

### Development Mode

Run directly with Python:

```bash
python main.py
```

Or use the legacy file:

```bash
python compare_observer.py
```

## Building EXE File

### Method 1: Using PyInstaller (Recommended)

1. **Install PyInstaller** (if not already installed):
```bash
pip install pyinstaller
```

2. **Build the executable**:

For a single-file executable:
```bash
pyinstaller --onefile --windowed --name compare_observer main.py
```

For a directory-based executable (faster startup):
```bash
pyinstaller --onedir --windowed --name compare_observer main.py
```

3. **Advanced build with custom icon** (optional):
```bash
pyinstaller --onefile --windowed --name compare_observer --icon=icon.ico main.py
```

4. **Using the existing spec file**:
```bash
pyinstaller compare_observer.spec
```

### Method 2: Manual PyInstaller Configuration

Create or edit `compare_observer.spec`:

```python
# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

a = Analysis(
    ['main.py'],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=[
        'PyQt6.QtCore',
        'PyQt6.QtGui', 
        'PyQt6.QtWidgets',
        'watchdog',
        'watchdog.observers',
        'watchdog.events',
        'requests',
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='compare_observer',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,  # Set to True to see console output for debugging
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
```

Then build:
```bash
pyinstaller compare_observer.spec
```

### Build Output

After building, the executable will be located in:
- **onefile**: `dist/compare_observer.exe`
- **onedir**: `dist/compare_observer/compare_observer.exe`

### Troubleshooting Build Issues

1. **Missing modules error**:
   - Add the module name to `hiddenimports` in the spec file
   - Example: `hiddenimports=['missing_module_name']`

2. **Application doesn't start**:
   - Build with console enabled to see error messages:
     ```bash
     pyinstaller --console main.py
     ```

3. **Large file size**:
   - Use `--onedir` instead of `--onefile`
   - Exclude unnecessary packages using `--exclude-module`

4. **PyQt6 issues**:
   - Ensure PyQt6 is properly installed: `pip install --upgrade PyQt6`
   - Clear PyInstaller cache: `pyinstaller --clean compare_observer.spec`

## Configuration

### First Run

1. Launch the application
2. Go to `Config` → `App Settings`
3. Configure:
   - Username
   - Telegram Bot Token (optional)
   - Telegram Chat ID (optional)
   - Source/Destination/Git paths for each system
   - File/folder exclusions

### Telegram Setup (Optional)

1. Create a Telegram bot via [@BotFather](https://t.me/botfather)
2. Get your bot token
3. Get your chat ID (use [@userinfobot](https://t.me/userinfobot))
4. Enter credentials in App Settings

## Usage

1. **Configure Paths**: Set up source, destination, and git paths
2. **Start Watching**: Click the "Start" button
3. **Monitor Changes**: Changes will appear in the tables
4. **Copy Files**: Use "Copy" or "Copy & Send" buttons
5. **Review Changes**: Click on files to see diff comparison
6. **Git Compare**: Use "Git Compare" to sync with git repository

## Development

### Code Style

The project follows clean architecture principles:
- **Core**: Business logic and domain models
- **Services**: External integrations and business services  
- **UI**: User interface components
- **Utils**: Helper functions and utilities

### Adding New Features

1. Add models to `core/models.py`
2. Add business logic to `services/`
3. Add UI components to `ui/dialogs/` or `ui/widgets/`
4. Update `main.py` to integrate new features

## License

[Your License Here]

## Support

For issues or questions, please open an issue on the repository.

## Version History

- **v1.0.0**: Initial release with clean architecture refactoring
  - Multi-system file monitoring
  - Git integration
  - Telegram notifications
  - Side-by-side file comparison

## Credits

Developed using:
- PyQt6 for GUI
- Watchdog for file monitoring
- Python standard library

