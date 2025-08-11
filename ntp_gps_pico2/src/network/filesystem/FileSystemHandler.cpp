#include "FileSystemHandler.h"
#include "../../config/LoggingService.h"

FileSystemHandler::FileSystemHandler() 
    : loggingService_(nullptr)
    , isMounted_(false) {
}

bool FileSystemHandler::initialize(bool autoFormat) {
    if (isMounted_) {
        return true;
    }

    // LittleFS初期化を試行
    if (LittleFS.begin()) {
        isMounted_ = true;
        logInfo("INIT", "", "LittleFS mounted successfully");
        return true;
    }

    // 初期化失敗時の自動フォーマット
    if (autoFormat) {
        logInfo("INIT", "", "LittleFS mount failed, attempting format...");
        
        // Pico用LittleFSはbegin(bool)をサポートしていないため、手動でフォーマット
        if (LittleFS.format() && LittleFS.begin()) {
            isMounted_ = true;
            logInfo("INIT", "", "LittleFS formatted and mounted successfully");
            return true;
        }
    }

    logError("INIT", "", "Failed to initialize LittleFS");
    return false;
}

bool FileSystemHandler::isMounted() const {
    return isMounted_;
}

bool FileSystemHandler::fileExists(const String& filepath) {
    if (!isMounted_ || !validatePath(filepath)) {
        return false;
    }

    return LittleFS.exists(filepath);
}

FileSystemHandler::FileInfo FileSystemHandler::getFileInfo(const String& filepath) {
    FileInfo info;
    info.name = filepath;
    info.size = 0;
    info.isDirectory = false;
    info.exists = false;

    if (!isMounted_ || !validatePath(filepath)) {
        return info;
    }

    File file = LittleFS.open(filepath, "r");
    if (file) {
        info.exists = true;
        info.size = file.size();
        info.isDirectory = file.isDirectory();
        file.close();
    }

    return info;
}

FileSystemHandler::OperationResult FileSystemHandler::readFile(const String& filepath, String& content) {
    OperationResult result;
    result.success = false;
    result.bytesRead = 0;
    content = "";

    if (!isMounted_) {
        result.errorMessage = "Filesystem not mounted";
        return result;
    }

    if (!validatePath(filepath)) {
        result.errorMessage = "Invalid file path";
        logError("READ", filepath, result.errorMessage);
        return result;
    }

    File file = openFile(filepath, "r");
    if (!file) {
        result.errorMessage = "Failed to open file";
        logError("READ", filepath, result.errorMessage);
        return result;
    }

    size_t fileSize = file.size();
    if (!checkSizeLimit(fileSize)) {
        result.errorMessage = "File too large";
        logError("READ", filepath, result.errorMessage);
        closeFile(file);
        return result;
    }

    // ファイル内容を読み込み
    content.reserve(fileSize + 1);
    while (file.available()) {
        content += char(file.read());
        result.bytesRead++;
    }

    closeFile(file);
    result.success = true;
    logInfo("READ", filepath, "File read successfully (" + String(result.bytesRead) + " bytes)");
    
    return result;
}

FileSystemHandler::OperationResult FileSystemHandler::readFile(const String& filepath, uint8_t* buffer, size_t bufferSize) {
    OperationResult result;
    result.success = false;
    result.bytesRead = 0;

    if (!isMounted_ || !buffer || bufferSize == 0) {
        result.errorMessage = "Invalid parameters";
        return result;
    }

    if (!validatePath(filepath)) {
        result.errorMessage = "Invalid file path";
        return result;
    }

    File file = openFile(filepath, "r");
    if (!file) {
        result.errorMessage = "Failed to open file";
        return result;
    }

    result.bytesRead = file.read(buffer, bufferSize);
    closeFile(file);

    result.success = true;
    return result;
}

FileSystemHandler::OperationResult FileSystemHandler::writeFile(const String& filepath, const String& content, bool append) {
    OperationResult result;
    result.success = false;
    result.bytesWritten = 0;

    if (!isMounted_) {
        result.errorMessage = "Filesystem not mounted";
        return result;
    }

    if (!validatePath(filepath)) {
        result.errorMessage = "Invalid file path";
        return result;
    }

    if (!checkSizeLimit(content.length())) {
        result.errorMessage = "Content too large";
        return result;
    }

    String mode = append ? "a" : "w";
    File file = openFile(filepath, mode);
    if (!file) {
        result.errorMessage = "Failed to open file for writing";
        logError("WRITE", filepath, result.errorMessage);
        return result;
    }

    result.bytesWritten = file.print(content);
    closeFile(file);

    result.success = (result.bytesWritten > 0);
    if (result.success) {
        logInfo("WRITE", filepath, "File written successfully (" + String(result.bytesWritten) + " bytes)");
    } else {
        result.errorMessage = "Failed to write content";
        logError("WRITE", filepath, result.errorMessage);
    }

    return result;
}

FileSystemHandler::OperationResult FileSystemHandler::writeFile(const String& filepath, const uint8_t* buffer, size_t size, bool append) {
    OperationResult result;
    result.success = false;
    result.bytesWritten = 0;

    if (!isMounted_ || !buffer || size == 0) {
        result.errorMessage = "Invalid parameters";
        return result;
    }

    if (!validatePath(filepath) || !checkSizeLimit(size)) {
        result.errorMessage = "Invalid parameters";
        return result;
    }

    String mode = append ? "a" : "w";
    File file = openFile(filepath, mode);
    if (!file) {
        result.errorMessage = "Failed to open file for writing";
        return result;
    }

    result.bytesWritten = file.write(buffer, size);
    closeFile(file);

    result.success = (result.bytesWritten == size);
    return result;
}

bool FileSystemHandler::deleteFile(const String& filepath) {
    if (!isMounted_ || !validatePath(filepath)) {
        return false;
    }

    bool success = LittleFS.remove(filepath);
    if (success) {
        logInfo("DELETE", filepath, "File deleted successfully");
    } else {
        logError("DELETE", filepath, "Failed to delete file");
    }

    return success;
}

FileSystemHandler::OperationResult FileSystemHandler::copyFile(const String& srcPath, const String& destPath) {
    OperationResult result;
    result.success = false;

    // ソースファイルを読み込み
    String content;
    OperationResult readResult = readFile(srcPath, content);
    if (!readResult.success) {
        result.errorMessage = "Failed to read source file: " + readResult.errorMessage;
        return result;
    }

    // 宛先ファイルに書き込み
    OperationResult writeResult = writeFile(destPath, content, false);
    if (!writeResult.success) {
        result.errorMessage = "Failed to write destination file: " + writeResult.errorMessage;
        return result;
    }

    result.success = true;
    result.bytesRead = readResult.bytesRead;
    result.bytesWritten = writeResult.bytesWritten;

    return result;
}

bool FileSystemHandler::createDirectory(const String& dirPath) {
    if (!isMounted_ || !validatePath(dirPath)) {
        return false;
    }

    // LittleFSはディレクトリ作成をサポートしていない
    // ファイル作成時に自動的にディレクトリが作成される
    return true;
}

int FileSystemHandler::listDirectory(const String& dirPath, FileInfo* fileList, int maxFiles) {
    if (!isMounted_ || !fileList || maxFiles <= 0) {
        return 0;
    }

    File dir = LittleFS.open(dirPath, "r");
    if (!dir || !dir.isDirectory()) {
        return 0;
    }

    int fileCount = 0;
    while (fileCount < maxFiles) {
        File entry = dir.openNextFile();
        if (!entry) {
            break;
        }

        fileList[fileCount].name = String(entry.name());
        fileList[fileCount].size = entry.size();
        fileList[fileCount].isDirectory = entry.isDirectory();
        fileList[fileCount].exists = true;

        entry.close();
        fileCount++;
    }

    dir.close();
    return fileCount;
}

bool FileSystemHandler::getFilesystemStats(size_t& totalBytes, size_t& usedBytes) {
    if (!isMounted_) {
        return false;
    }

    FSInfo fsInfo;
    if (!LittleFS.info(fsInfo)) {
        return false;
    }

    totalBytes = fsInfo.totalBytes;
    usedBytes = fsInfo.usedBytes;
    return true;
}

bool FileSystemHandler::formatFilesystem() {
    if (isMounted_) {
        LittleFS.end();
        isMounted_ = false;
    }

    logInfo("FORMAT", "", "Formatting filesystem...");
    
    if (LittleFS.format() && LittleFS.begin()) {
        isMounted_ = true;
        logInfo("FORMAT", "", "Filesystem formatted successfully");
        return true;
    }

    logError("FORMAT", "", "Failed to format filesystem");
    return false;
}

File FileSystemHandler::openFile(const String& filepath, const String& mode) {
    return LittleFS.open(filepath, mode.c_str());
}

void FileSystemHandler::closeFile(File& file) {
    if (file) {
        file.close();
    }
}

// プライベートメソッド実装
void FileSystemHandler::logError(const String& operation, const String& filepath, const String& error) {
    if (loggingService_) {
        loggingService_->error("FS", (operation + " " + filepath + ": " + error).c_str());
    }
}

void FileSystemHandler::logInfo(const String& operation, const String& filepath, const String& info) {
    if (loggingService_) {
        loggingService_->info("FS", (operation + " " + filepath + ": " + info).c_str());
    }
}

bool FileSystemHandler::validatePath(const String& filepath) {
    if (filepath.length() == 0 || filepath.length() > 255) {
        return false;
    }

    // パストラバーサル攻撃を防ぐ
    if (filepath.indexOf("..") >= 0) {
        return false;
    }

    // 有効な文字のみを許可
    for (unsigned int i = 0; i < filepath.length(); i++) {
        char c = filepath.charAt(i);
        if (!(isAlphaNumeric(c) || c == '/' || c == '.' || c == '-' || c == '_')) {
            return false;
        }
    }

    return true;
}

bool FileSystemHandler::checkSizeLimit(size_t size) {
    return size <= MAX_FILE_SIZE;
}