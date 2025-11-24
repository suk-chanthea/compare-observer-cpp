#include <QApplication>
#include "main_window.h"

/**
 * @brief Main entry point for the Compare Observer application
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("Compare Observer");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setApplicationDisplayName("Compare Observer - File Watcher");

    // Create and show main window
    FileWatcherApp window;
    window.show();

    return app.exec();
}
