// read_thread.h

#ifndef READ_THREAD_H
#define READ_THREAD_H

#include <libusb-1.0/libusb.h>
#include <chrono>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class ReadThread {
public:
    void readThreadFunc(libusb_context* ctx);
    static void cb_transfer(struct libusb_transfer* transfer);

    void setLastTimestamp(const std::chrono::high_resolution_clock::time_point& timestamp);

    std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>>& getLogQueue();
    std::condition_variable& getLogCondition();
    std::mutex& getLogMutex();
    bool isRecording = false;

private:
    std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>> logQueue;
    std::mutex logMutex;
    std::condition_variable logCondition;
    bool isStartingToRecord = false;
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    TimePoint lastTimestamp;
    int hidg_fd;
};

#endif // READ_THREAD_H

