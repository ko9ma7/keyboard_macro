// logger_thread.cpp
#include "logger_thread.h"
#include <fstream>
#include <iostream>

void LoggerThread::loggerThreadFunc() {
    file.open("keyboard_logs.txt"); // 파일 열기

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

    file.close();
}

bool LoggerThread::renameLogFile(const std::string& newFilename) {
    std::lock_guard<std::mutex> lock(fileMutex);
    file.close(); // 현재 로그 파일 닫기

    try {
        std::filesystem::rename("keyboard_logs.txt", newFilename);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "File rename error: " << e.what() << std::endl;
        file.open("keyboard_logs.txt", std::ios::app); // 다시 기존 파일 열기
        return false;
    }

    file.open("keyboard_logs.txt", std::ios::app); // 새 로그 파일 열기
    return true;
}