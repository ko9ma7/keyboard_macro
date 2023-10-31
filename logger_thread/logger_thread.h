#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <chrono>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class LoggerThread {
public:
    void loggerThreadFunc();
    void setLogQueue(std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>>& queue);
    void setLogCondition(std::condition_variable& cond);
    void setLogMutex(std::mutex& mtx);
    void setIsRecording(bool* isRecording);

private:
    std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>>* logQueue;
    std::condition_variable* logCondition;
    std::mutex* logMutex;
    bool* isRecording;
};

#endif // LOGGER_THREAD_H
