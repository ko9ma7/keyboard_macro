// global_vars.h

#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include <atomic>
#include <chrono>
#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>


using TimePoint = std::chrono::high_resolution_clock::time_point;  // 별칭 추가
extern std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>> logQueue;
extern std::mutex logMutex;
extern std::condition_variable logCondition;


extern bool isStartingToRecord;

extern std::condition_variable recordingCondition;
extern std::mutex recordingMutex;
extern bool isRecording;
extern TimePoint lastTimestamp;

#endif // GLOBAL_VARS_H
