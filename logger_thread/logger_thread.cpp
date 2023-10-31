#include "logger_thread.h"
#include <fstream>
#include <iostream>

void LoggerThread::setLogQueue(std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>>& queue) {
    logQueue = &queue;
}

void LoggerThread::setLogCondition(std::condition_variable& cond) {
    logCondition = &cond;
}

void LoggerThread::setLogMutex(std::mutex& mtx) {
    logMutex = &mtx;
}

void LoggerThread::setIsRecording(bool* isRecording) {
    this->isRecording = isRecording;
}

void LoggerThread::loggerThreadFunc() {
    std::ofstream file("keyboard_logs.txt");

    while (true) {

        std::unique_lock<std::mutex> lock(*logMutex);

        logCondition->wait(lock, [this] { 
            return *isRecording && !logQueue->empty(); 
        });

        if (logQueue->empty()) {
            continue;
        }

        auto logItem = logQueue->front();
        logQueue->pop();
        lock.unlock();

        file << "Time diff: " << logItem.first.count() << "ns, Data: ";
        for (const auto& byte : logItem.second) {
            file << std::hex << std::uppercase << static_cast<int>(byte) << " ";
        }
        file << std::endl;

        // Check for errors
        if (!file) {
            std::cerr << "Error writing to keyboard_logs.txt!" << std::endl;
            // Optionally, you can add more error handling here.
        }
    }

    file.close();
}