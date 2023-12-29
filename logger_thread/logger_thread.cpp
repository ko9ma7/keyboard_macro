// logger_thread.cpp
#include "logger_thread.h"
#include <fstream>
#include <iostream>
#include "../utils/type.h"

void LoggerThread::loggerThreadFunc() {
    while (true) {

        std::unique_lock<std::mutex> lock(readThread->getLogMutex());

        readThread->getLogCondition().wait(lock, [this] { 
            return readThread->isRecording && !readThread->getLogQueue().empty(); 
        });

        if (readThread->getLogQueue().empty()) {
            continue;
        }

        auto logItem = readThread->getLogQueue().front();
        readThread->getLogQueue().pop();
        lock.unlock();

        {
            std::lock_guard<std::mutex> fileLock(fileMutex); // 파일 접근 동기화
            file << "Time diff: " << logItem.first.count() << "ns, Data: ";
            for (const auto& byte : logItem.second) {
                file << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            file << std::endl;

            // 오류 검사
            if (!file) {
                std::cerr << "Error writing to keyboard_logs.txt!" << std::endl;
            }
        }
    }
}

void LoggerThread::startLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::string fullPath = "save/" + filename + ".sav";
    file.open(fullPath, std::ios::out | std::ios::app); // 파일 열기
}

void LoggerThread::stopLogging() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (file.is_open()) {
        file.close(); // 로그 파일 닫기
    }
}

void LoggerThread::saveMacroToFile(const std::vector<KeyMacro::KeyEvent>& events, const std::string& filename) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::ofstream macroFile("save/" + filename + ".sav", std::ios::out | std::ios::trunc);

        if (!macroFile.is_open()) {
            std::cerr << "Error opening file: save/" << filename << ".sav" << std::endl;
            return;
        }

        for (const auto& event : events) {
            macroFile << "Time diff: " << event.delay << "ns, Data: ";
            for (const auto& byte : event.data) {
                macroFile << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            macroFile << std::endl;
        }

        macroFile.close();
    }
