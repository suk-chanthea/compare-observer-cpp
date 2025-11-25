#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <iostream>
#include "main_window.h"

// Global error log file
QFile* g_logFile = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString formattedMessage;
    QTextStream stream(&formattedMessage);
    
    stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");
    
    switch (type) {
        case QtDebugMsg:
            stream << "[DEBUG] ";
            break;
        case QtInfoMsg:
            stream << "[INFO] ";
            break;
        case QtWarningMsg:
            stream << "[WARNING] ";
            break;
        case QtCriticalMsg:
            stream << "[CRITICAL] ";
            break;
        case QtFatalMsg:
            stream << "[FATAL] ";
            break;
    }
    
    stream << msg;
    
    if (context.file) {
        stream << " (" << context.file << ":" << context.line << ")";
    }
    
    // Write to console
    std::cerr << formattedMessage.toStdString() << std::endl;
    
    // Write to log file
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream fileStream(g_logFile);
        fileStream << formattedMessage << "\n";
        fileStream.flush();
    }
    
    // Show critical errors in message box (but don't block)
    if (type == QtCriticalMsg) {
        // Don't show message box during init - just log it
        // QMessageBox::critical(nullptr, "Application Error", formattedMessage);
    }
    
    if (type == QtFatalMsg) {
        if (g_logFile) {
            g_logFile->flush();
            g_logFile->close();
        }
        abort();
    }
}

int main(int argc, char* argv[])
{
    // Initialize log file path
    QString logPath = QDir::currentPath() + "/compare_observer_debug.log";
    
    std::cout << "=== Compare Observer Starting ===" << std::endl;
    std::cout << "Current directory: " << QDir::currentPath().toStdString() << std::endl;
    std::cout << "Log file: " << logPath.toStdString() << std::endl;
    
    // Open log file BEFORE QApplication
    g_logFile = new QFile(logPath);
    
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qInstallMessageHandler(messageHandler);
        qInfo() << "=== Compare Observer Starting ===" 
                << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        qInfo() << "Log file opened successfully:" << logPath;
    } else {
        std::cerr << "WARNING: Could not open log file: " 
                  << logPath.toStdString() << std::endl;
    }
    
    try {
        qInfo() << "Creating QApplication with" << argc << "arguments...";
        for (int i = 0; i < argc; ++i) {
            qInfo() << "  arg[" << i << "]:" << argv[i];
        }
        
        QApplication app(argc, argv);
        qInfo() << "QApplication created successfully";
        
        qInfo() << "Setting application metadata...";
        QApplication::setApplicationName("Compare Observer");
        QApplication::setApplicationVersion("1.0.0");
        QApplication::setOrganizationName("CompareObserver");
        QApplication::setOrganizationDomain("compareobserver.local");
        
        qInfo() << "Qt version:" << qVersion();
        qInfo() << "Application path:" << QCoreApplication::applicationDirPath();
        qInfo() << "Application file path:" << QCoreApplication::applicationFilePath();
        
        // Check if required Qt libraries are available
        qInfo() << "Checking Qt modules...";
        qInfo() << "  Qt::Core:" << QT_VERSION_STR;
        
        qInfo() << "Creating main window...";
        FileWatcherApp* window = nullptr;
        
        try {
            window = new FileWatcherApp();
            qInfo() << "Main window created successfully";
        } catch (const std::bad_alloc& e) {
            qCritical() << "Memory allocation failed:" << e.what();
            QMessageBox::critical(nullptr, "Startup Error", 
                QString("Failed to allocate memory for main window:\n%1").arg(e.what()));
            return 1;
        } catch (const std::exception& e) {
            qCritical() << "Exception creating main window:" << e.what();
            QMessageBox::critical(nullptr, "Startup Error", 
                QString("Failed to create main window:\n%1\n\nCheck log file: %2")
                    .arg(e.what())
                    .arg(logPath));
            return 1;
        } catch (...) {
            qCritical() << "Unknown exception creating main window";
            QMessageBox::critical(nullptr, "Startup Error", 
                QString("Failed to create main window: Unknown error\n\nCheck log file: %1")
                    .arg(logPath));
            return 1;
        }
        
        if (!window) {
            qCritical() << "Failed to create main window (null pointer)";
            QMessageBox::critical(nullptr, "Startup Error", 
                QString("Failed to create main window\n\nCheck log file: %1")
                    .arg(logPath));
            return 1;
        }
        
        qInfo() << "Showing main window...";
        window->show();
        qInfo() << "Main window shown successfully";
        qInfo() << "Window visible:" << window->isVisible();
        qInfo() << "Window geometry:" << window->geometry();
        
        qInfo() << "Starting event loop...";
        int result = app.exec();
        qInfo() << "Application exiting with code:" << result;
        
        delete window;
        
        if (g_logFile) {
            g_logFile->close();
            delete g_logFile;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        qCritical() << "Unhandled exception in main:" << e.what();
        QMessageBox::critical(nullptr, "Fatal Error", 
            QString("Unhandled exception:\n%1\n\nCheck log file: %2")
                .arg(e.what())
                .arg(logPath));
        
        if (g_logFile) {
            g_logFile->close();
            delete g_logFile;
        }
        return 1;
    } catch (...) {
        qCritical() << "Unknown unhandled exception in main";
        QMessageBox::critical(nullptr, "Fatal Error", 
            QString("Unknown unhandled exception\n\nCheck log file: %1")
                .arg(logPath));
        
        if (g_logFile) {
            g_logFile->close();
            delete g_logFile;
        }
        return 1;
    }
}