#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
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
    
    // Show critical/fatal errors in message box (in debug mode)
    if (type == QtCriticalMsg || type == QtFatalMsg) {
        QMessageBox::critical(nullptr, "Application Error", formattedMessage);
    }
    
    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char* argv[])
{
    // Initialize log file BEFORE QApplication
    QString logPath = "compare_observer_debug.log";
    g_logFile = new QFile(logPath);
    
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qInstallMessageHandler(messageHandler);
        qInfo() << "=== Compare Observer Starting ===" 
                << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    } else {
        std::cerr << "WARNING: Could not open log file: " 
                  << logPath.toStdString() << std::endl;
    }
    
    try {
        qInfo() << "Creating QApplication...";
        QApplication app(argc, argv);
        
        qInfo() << "Setting application metadata...";
        QApplication::setApplicationName("Compare Observer");
        QApplication::setApplicationVersion("1.0.0");
        QApplication::setApplicationDisplayName("Compare Observer - File Watcher");
        
        qInfo() << "Qt version:" << qVersion();
        qInfo() << "Application path:" << QCoreApplication::applicationDirPath();
        
        qInfo() << "Creating main window...";
        FileWatcherApp* window = nullptr;
        
        try {
            window = new FileWatcherApp();
            qInfo() << "Main window created successfully";
        } catch (const std::exception& e) {
            qCritical() << "Exception creating main window:" << e.what();
            QMessageBox::critical(nullptr, "Startup Error", 
                QString("Failed to create main window:\n%1").arg(e.what()));
            return 1;
        } catch (...) {
            qCritical() << "Unknown exception creating main window";
            QMessageBox::critical(nullptr, "Startup Error", 
                "Failed to create main window: Unknown error");
            return 1;
        }
        
        if (!window) {
            qCritical() << "Failed to create main window (null pointer)";
            QMessageBox::critical(nullptr, "Startup Error", 
                "Failed to create main window");
            return 1;
        }
        
        qInfo() << "Showing main window...";
        window->show();
        qInfo() << "Main window shown, starting event loop...";
        
        int result = app.exec();
        qInfo() << "Application exiting with code:" << result;
        
        delete window;
        return result;
        
    } catch (const std::exception& e) {
        qCritical() << "Unhandled exception in main:" << e.what();
        QMessageBox::critical(nullptr, "Fatal Error", 
            QString("Unhandled exception:\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        qCritical() << "Unknown unhandled exception in main";
        QMessageBox::critical(nullptr, "Fatal Error", 
            "Unknown unhandled exception occurred");
        return 1;
    }
}