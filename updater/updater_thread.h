#ifndef UPDATER_THREAD_H
#define UPDATER_THREAD_H

#include <iostream>
#include <curl/curl.h>
#include <json/json.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <grpcpp/grpcpp.h>
#include "grpc/restart_service.grpc.pb.h"

namespace fs = std::filesystem;

// 플랫폼 및 Bluetooth 지원 매크로 정의
#if defined(__aarch64__)
#define PLATFORM "ARM 64bit"
#define BIT_SUFFIX "_64bit"
#elif defined(__arm__)
#define PLATFORM "ARM 32bit"
#define BIT_SUFFIX "_32bit"
#else
#error "Unsupported platform"
#endif

// Bluetooth 지원 여부 매크로 정의 (필요에 따라 설정)
#if defined(HAVE_BLUETOOTH)
#define BLUETOOTH "Bluetooth"
#define BLUETOOTH_SUFFIX "_bluetooth"
#else
#define BLUETOOTH "No Bluetooth"
#define BLUETOOTH_SUFFIX "_nonbluetooth"
#endif

class UpdaterThread {
public:
    UpdaterThread(grpc::ServerWriter<UpdateResponse>* writer) : writer(writer) {}
    void runUpdate();

private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    static int ProgressCallback(void* ptr, curl_off_t totalDownload, curl_off_t nowDownload,
                                curl_off_t totalUpload, curl_off_t nowUpload);

    std::string getLatestVersionFromGitHub();

    std::string getCurrentVersion();

    static size_t WriteFileCallback(void *ptr, size_t size, size_t nmemb, FILE *stream);

    void downloadUpdateFile(const std::string &url, const std::string &outputPath);

    bool verifyDownloadedFile(const std::string &filePath, const std::string &expectedHash);

    void installUpdate(const std::string &updateFilePath);

    void performUpdate();

    void sendUpdateProgress(int progress, const std::string& status_message);

    grpc::ServerWriter<UpdateResponse>* writer;
};

#endif // UPDATER_THREAD_H