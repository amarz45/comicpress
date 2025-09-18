// worker.h
#pragma once

#include <QObject>
#include <QRunnable>
#include <filesystem>
#include <string>
#include "task.h"

namespace fs = std::filesystem;

class Worker : public QObject, public QRunnable {
    Q_OBJECT

public:
    // We pass the file paths to the constructor.
    Worker(const PageTask& task);

    // This is the function that will run on the new thread.
    void run() override;

signals:
    // A signal to send log messages to the GUI.
    void logMessage(const QString& message);
    // A signal to report that this task is finished.
    void finished();

private:
    PageTask task; // Store the task
};
