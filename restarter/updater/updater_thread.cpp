#include "updater_thread.h"
#include <fcntl.h>
#include <unistd.h>

size_t UpdaterThread::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int UpdaterThread::ProgressCallback(void* ptr, curl_off_t totalDownload, curl_off_t nowDownload,
                                    curl_off_t totalUpload, curl_off_t nowUpload) {
    UpdaterThread* updater = static_cast<UpdaterThread*>(ptr);
    if (totalDownload > 0) {
        int rawProgress = static_cast<int>((nowDownload * 100) / totalDownload);
        int adjustedProgress = 10 + (rawProgress * 80) / 100; // Map 0-100% to 10-90%
        if (adjustedProgress % 10 == 0 && updater->prevPrgress != adjustedProgress){
            std::cout << "Download progress: " << adjustedProgress << "%\n";
            std::lock_guard<std::mutex> lock(updater->writerMutex); // Protect writer access
            updater->prevPrgress = adjustedProgress;
            updater->progressCallback(adjustedProgress, "Downloading update...");
        }
    }
    return 0;
}

std::string UpdaterThread::getLatestVersionFromGitHub() {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/ccxz84/keyboard_macro/releases/latest");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.68.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, UpdaterThread::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    Json::Value jsonData;
    Json::Reader jsonReader;
    if (jsonReader.parse(readBuffer, jsonData)) {
        return jsonData["tag_name"].asString();
    }

    return "";
}

std::string UpdaterThread::getCurrentVersion() {
    std::ifstream versionFile("version.txt");
    std::string version;
    std::getline(versionFile, version);
    return version;
}

size_t UpdaterThread::WriteFileCallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void UpdaterThread::downloadUpdateFile(const std::string &url, const std::string &outputPath) {
    CURL *curl;
    int fd;
    CURLcode res;
    long http_code = 0;

    // 파일 열기 (O_WRONLY 모드로, 필요시 파일 생성, 기존 파일 내용을 덮어쓰기)
    fd = open(outputPath.c_str(), O_WRONLY | O_CREAT, 0666);
    if (fd == -1) {
        std::cerr << "Failed to open file: " << outputPath << std::endl;
        return;
    }

    // 파일 포인터를 0으로 이동 (기존 내용 덮어쓰기)
    if (lseek(fd, 0, SEEK_SET) == -1) {
        std::cerr << "Failed to seek to the beginning of the file: " << outputPath << std::endl;
        close(fd);
        return;
    }

    // 파일 크기를 0으로 설정 (기존 내용 삭제)
    if (ftruncate(fd, 0) == -1) {
        std::cerr << "Failed to truncate the file: " << outputPath << std::endl;
        close(fd);
        return;
    }

    curl = curl_easy_init();
    if (curl) {
        FILE *fp = fdopen(fd, "wb");
        if (!fp) {
            std::cerr << "Failed to associate a stream with the file descriptor: " << outputPath << std::endl;
            close(fd);
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, UpdaterThread::WriteFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, UpdaterThread::ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); // Enable progress meter

        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);  // Fail on HTTP errors
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects

        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else if(http_code != 200) {
            std::cerr << "HTTP request failed with code: " << http_code << std::endl;
        } else {
            std::cout << "Download succeeded: " << url << std::endl;
        }

        curl_easy_cleanup(curl);
        fclose(fp); // This also closes the file descriptor fd
    } else {
        std::cerr << "Failed to initialize CURL" << std::endl;
        close(fd);
    }
}

bool UpdaterThread::verifyDownloadedFile(const std::string &filePath, const std::string &expectedHash) {
    // 해시 검증 로직을 여기에 추가하세요.
    return true; // 예시로 true 반환
}

void UpdaterThread::installUpdate(const std::string &updateFilePath) {
    const std::string targetDirectory = "/home/ccxz84";
    const std::string targetFilePath = targetDirectory + "/KeyboardDriver"; // 변경할 타겟 파일 이름

    try {
        // 타겟 디렉토리가 존재하는지 확인하고, 없으면 생성
        if (!fs::exists(targetDirectory)) {
            fs::create_directories(targetDirectory);
        }

        // 다운로드한 파일을 타겟 디렉토리로 복사
        fs::copy_file(updateFilePath, targetFilePath, fs::copy_options::overwrite_existing);

        // 실행 권한 부여
        fs::permissions(targetFilePath, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add);

        std::cout << "Update installed successfully to " << targetFilePath << std::endl;
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "General error: " << e.what() << std::endl;
    }
}

void UpdaterThread::performUpdate() {
    std::cout << "start\n";
    std::string currentVersion = getCurrentVersion();
    std::cout << currentVersion << '\n';
    std::string latestVersion = getLatestVersionFromGitHub();
    std::cout << latestVersion << '\n';

    if (latestVersion != currentVersion) {
        std::string updateFileName = "KeyboardDriver" + std::string(BIT_SUFFIX) + std::string(BLUETOOTH_SUFFIX);
        std::string updateUrl = "https://github.com/ccxz84/keyboard_macro/releases/download/" + latestVersion + "/" + updateFileName;
        std::cout << updateUrl << '\n';
        std::string updateFilePath = "/home/ccxz84/update";

        {
            std::lock_guard<std::mutex> lock(writerMutex); // Protect writer access
            progressCallback(10, "Update Download Start");
        }
        downloadUpdateFile(updateUrl, updateFilePath);
        {
            std::lock_guard<std::mutex> lock(writerMutex); // Protect writer access
            progressCallback(90, "Update Download end");
        }

        if (verifyDownloadedFile(updateFilePath, "expected_hash_value")) {
            installUpdate(updateFilePath);
            std::ofstream versionFile("version.txt");
            versionFile << latestVersion;
            {
                std::lock_guard<std::mutex> lock(writerMutex); // Protect writer access
                progressCallback(100, "Update Install Success");
            }
        } else {
            {
                std::lock_guard<std::mutex> lock(writerMutex); // Protect writer access
                progressCallback(0, "Download Error");
            }
            std::cerr << "Downloaded file verification failed!" << std::endl;
        }
    } else {
        {
            std::lock_guard<std::mutex> lock(writerMutex); // Protect writer access
            progressCallback(0, "No updates available.");
        }
        std::cout << "No updates available." << std::endl;
    }
}

void UpdaterThread::runUpdate() {
    std::string currentVersion = getCurrentVersion();
    std::cout << "App Version: " << currentVersion << std::endl;
    std::cout << "Platform: " << PLATFORM << std::endl;
    std::cout << "Bluetooth: " << BLUETOOTH << std::endl;

    performUpdate();
}
