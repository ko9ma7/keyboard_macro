// logger_thread.h
#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <chrono>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "../read_thread/read_thread.h"
#include <fstream>
#include <filesystem>

class LoggerThread {
public:
    void loggerThreadFunc();
    void setReadThread(ReadThread* readThread) {
        this->readThread = readThread;
    }
    ReadThread* readThread;
    
    void setRecording(bool isRecording) {
        {
            std::lock_guard<std::mutex> lock(readThread->getLogMutex());
            this->readThread->isRecording = isRecording;
        }
        readThread->getLogCondition().notify_one();
    }

    bool renameLogFile(const std::string& newFilename);

private:
    std::ofstream file;
    std::mutex fileMutex;
};

#endif // LOGGER_THREAD_H
