#include "global_vars.h"

std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>> logQueue;
std::mutex logMutex;
std::condition_variable logCondition;

std::condition_variable recordingCondition;
std::mutex recordingMutex;
bool isRecording = false;
bool isStartingToRecord = false;

TimePoint lastTimestamp;
