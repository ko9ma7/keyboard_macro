// read_thread.h

#ifndef READ_THREAD_H
#define READ_THREAD_H

#include <libusb-1.0/libusb.h>
#include <chrono>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include "../utils/type.h"

class ReadThread {
public:
    void readThreadFunc(libusb_context* ctx);
    static void cb_transfer(struct libusb_transfer* transfer);

    void setLastTimestamp(const std::chrono::high_resolution_clock::time_point& timestamp);

    std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>>& getLogQueue();
    std::condition_variable& getLogCondition();
    std::mutex& getLogMutex();
    bool isRecording = false;
    bool isStartingToRecord = false;

    std::thread startMacroReplay(const std::string& filename, std::function<void(const std::string&)> eventCallback = nullptr) {
        if (macroReplayThread.joinable()) {
            macroReplayThread.join();  // 이미 실행 중인 스레드가 있다면 기다림
        }
        macroReplayThread = std::thread(&ReadThread::replayMacro, this, filename, eventCallback);
        return std::move(macroReplayThread);
    }

    void stopMacroReplay() {
        if (macroReplayThread.joinable()) {
            macroReplayThread.join(); // 매크로 재생 중인 쓰레드 중지
        }
    }

    void replayMacro(const std::string& logFilename, std::function<void(const std::string&)> eventCallback); // 기본 버전

    std::vector<KeyMacro::KeyEvent> readMacroFile(const std::string& filename);

private:
    std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>> logQueue;
    std::mutex logMutex;
    std::condition_variable logCondition;
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    TimePoint lastTimestamp;
    int hidg_fd;
    std::thread macroReplayThread;
};

#endif // READ_THREAD_H

