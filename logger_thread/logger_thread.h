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
#include "../utils/type.h"

class LoggerThread {
public:
    void loggerThreadFunc();
    void setReadThread(ReadThread* readThread) {
        this->readThread = readThread;
    }
    ReadThread* readThread;
    
    void setRecording(bool isRecording) {
        {
            auto currentTimestamp = std::chrono::high_resolution_clock::now();
            this->readThread->setLastTimestamp(currentTimestamp);
            std::lock_guard<std::mutex> lock(readThread->getLogMutex());
            this->readThread->isRecording = isRecording;
            this->readThread->isStartingToRecord = isRecording;
        }
        readThread->getLogCondition().notify_one();
    }

    void startLogging(const std::string& filename);

    void stopLogging();

    void saveMacroToFile(const std::vector<KeyMacro::KeyEvent>& events, const std::string& filename);

private:
    std::ofstream file;
    std::mutex fileMutex;
};

#endif // LOGGER_THREAD_H
