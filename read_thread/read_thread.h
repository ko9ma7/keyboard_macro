// read_thread.h

#ifndef READ_THREAD_H
#define READ_THREAD_H

#include <libusb-1.0/libusb.h>
#include <chrono>
#include <vector>
#include <queue>
#include <fcntl.h>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <iostream>
#include <atomic>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>  // fork, exec, pid_t
#include <signal.h>  // kill
#include "../utils/type.h"
#include "../utils/keyboard_util.h"

namespace read_thread_ns {
    struct ReplayRequest {
        std::string filename;
        int delayAfter;
    };
}

class ReadThread {
public:

    virtual void init() {

    }

    pid_t runMacroPid = -1;

    void readThreadFunc(libusb_context* ctx);
    static void cb_transfer(struct libusb_transfer* transfer);

    std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>>& getLogQueue();
    std::condition_variable& getLogCondition();
    std::mutex& getLogMutex();

    bool isRecording = false;
    bool isStartingToRecord = false;

    /* grpc service */
    void startMacroReplay(const std::string& filename, std::function<void(const std::string&)> eventCallback = nullptr) {
        if (!macroReplayThread.joinable()) {
            macroReplayThread = std::thread(&ReadThread::replayMacro, this, filename, eventCallback);
            stopRequested = false;
        }
    }

    void startMacroReplay_otherfile(const std::string& filename) {
        if (runMacroPid == -1) {
            pid_t pid = fork();
            if (pid == 0) { // 자식 프로세스
                execl("/home/ccxz84/run.sh", "run.sh", filename.c_str(), (char *) NULL);
            } else if (pid > 0) {
                runMacroPid = pid; // 부모 프로세스에서 PID 저장
            }
        }
    }

    void stopMacroReplay_otherfile() {
        if (runMacroPid != -1) {
            kill(runMacroPid, SIGKILL); // SIGTERM 신호를 보내어 프로세스 종료
            runMacroPid = -1;
        }
    }

    bool waitForCompletion_otherfile() {
        if (runMacroPid != -1) {
            int status;
            bool ret = true;
            std::cout << "Waiting for PID " << runMacroPid << " to complete...\n";
            pid_t result = waitpid(runMacroPid, &status, 0);  // runMacroPid 프로세스가 종료될 때까지 기다립니다
            if (result == -1) {
                std::cerr << "Error waiting for PID " << runMacroPid << std::endl;
                ret = true;
            } else {
                if (WIFEXITED(status)) {  // 자식 프로세스가 정상적으로 종료된 경우
                    std::cout << "Process " << runMacroPid << " exited with status " << WEXITSTATUS(status) << std::endl;
                    ret = false;
                } else if (WIFSIGNALED(status)) {  // 자식 프로세스가 시그널에 의해 종료된 경우
                    std::cout << "Process " << runMacroPid << " was killed by signal " << WTERMSIG(status) << std::endl;
                    ret = true;
                }
            }
            runMacroPid = -1;  // PID를 초기화하여 다음 사용을 위해 준비

            return ret;
        } else {
            std::cout << "No process is currently running.\n";
            return false;
        }
    }

    void waitForCompletion() {
        if (macroReplayThread.joinable()) {
            macroReplayThread.join();
        }
    }

    void stopMacroReplay() {
        stopRequested = true;
        if (macroReplayThread.joinable()) {
            macroReplayThread.join(); // 매크로 재생 중인 스레드 중지
        }
    }
    /* grpc service */

    void replayMacro(const std::string& logFilename, std::function<void(const std::string&)> eventCallback); // 기본 버전

    std::vector<KeyMacro::KeyEvent> readMacroFile(const std::string& filename);

    virtual void startComplexRequests(const std::vector<read_thread_ns::ReplayRequest>& requests, int repeatCount) {
        std::cout<<"반복 횟수 "<<repeatCount<<'\n';
        for (int i = 0; i < repeatCount; ++i) {
            for (const auto& request : requests) {
                startMacroReplay_otherfile(request.filename);
                bool stopSign = waitForCompletion_otherfile();
                if (stopSign) {
                    int hidg_fd = outputOpen(HIDG_MACRO_PATH, O_RDWR);

                    if (hidg_fd >= 0) {
                        unsigned char stopReport[] = {0, 0, 0, 0, 0, 0, 0, 0};
                        write(hidg_fd, stopReport, sizeof(stopReport));
                        outputClose(hidg_fd);
                        std::cout<<"reset fd: "<<hidg_fd<<"\n";
                    }
                    break; // 종료 플래그가 설정되면 루프를 종료
                }
                std::this_thread::sleep_for(std::chrono::seconds(request.delayAfter));  // 지정된 시간만큼 지연
            }
        }
    }

    virtual int outputWrite(int fd, const void *buf, size_t count) {
        return write(fd, buf, count);
    }
    
    virtual int outputOpen(const char* path, int flags) {
        return open(path, flags);
    }

    virtual int outputClose(int fd) {
        return close(fd);
    }

    friend class BluetoothReadThread;

private:
    std::queue<std::pair<std::chrono::nanoseconds, std::vector<unsigned char>>> logQueue;
    std::mutex logMutex;
    std::condition_variable logCondition;

    using TimePoint = std::chrono::high_resolution_clock::time_point;
    timespec startTime;
    timespec prevTime;

    int hidg_fd;
    std::thread macroReplayThread;

    std::atomic<bool> stopRequested{false}; // 종료 플래그
};

#endif // READ_THREAD_H

