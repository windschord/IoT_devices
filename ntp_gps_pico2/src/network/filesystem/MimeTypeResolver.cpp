#include "MimeTypeResolver.h"

// MIMEタイプマッピングテーブル（拡張子順でソート）
const MimeTypeResolver::MimeTypeMapping MimeTypeResolver::mimeTypes_[] = {
    {".css",   "text/css",                   "utf-8", true,  true},
    {".gif",   "image/gif",                  "",      false, false},
    {".htm",   "text/html",                  "utf-8", true,  true},
    {".html",  "text/html",                  "utf-8", true,  true},
    {".ico",   "image/x-icon",               "",      false, false},
    {".jpeg",  "image/jpeg",                 "",      false, false},
    {".jpg",   "image/jpeg",                 "",      false, false},
    {".js",    "text/javascript",            "utf-8", true,  true},
    {".json",  "application/json",           "utf-8", true,  true},
    {".png",   "image/png",                  "",      false, false},
    {".svg",   "image/svg+xml",              "utf-8", true,  true},
    {".txt",   "text/plain",                 "utf-8", true,  true},
    {".xml",   "application/xml",            "utf-8", true,  true}
};

const int MimeTypeResolver::mimeTypesCount_ = sizeof(mimeTypes_) / sizeof(mimeTypes_[0]);

String MimeTypeResolver::getMimeType(const String& filepath) {
    String extension = extractExtension(filepath);
    const MimeTypeMapping* mapping = findMimeTypeMapping(extension);
    
    if (mapping) {
        return String(mapping->mimeType);
    }
    
    return getDefaultMimeType();
}

MimeTypeResolver::MimeInfo MimeTypeResolver::getMimeInfo(const String& filepath) {
    MimeInfo info;
    String extension = extractExtension(filepath);
    const MimeTypeMapping* mapping = findMimeTypeMapping(extension);
    
    if (mapping) {
        info.mimeType = String(mapping->mimeType);
        info.charset = String(mapping->charset);
        info.isText = mapping->isText;
        info.isCompressible = mapping->isCompressible;
    } else {
        info.mimeType = getDefaultMimeType();
        info.charset = "";
        info.isText = true;  // デフォルトはテキスト
        info.isCompressible = true;
    }
    
    return info;
}

String MimeTypeResolver::getMimeTypeByExtension(const String& extension) {
    String normalizedExt = extension;
    normalizedExt.toLowerCase();
    
    // ピリオドがない場合は追加
    if (!normalizedExt.startsWith(".")) {
        normalizedExt = "." + normalizedExt;
    }
    
    const MimeTypeMapping* mapping = findMimeTypeMapping(normalizedExt);
    if (mapping) {
        return String(mapping->mimeType);
    }
    
    return getDefaultMimeType();
}

String MimeTypeResolver::getMimeTypeByContent(const uint8_t* content, size_t size) {
    if (!content || size < 4) {
        return getDefaultMimeType();
    }
    
    // PNG署名をチェック
    const uint8_t pngSig[] = {0x89, 0x50, 0x4E, 0x47};
    if (checkSignature(content, size, pngSig, 4)) {
        return "image/png";
    }
    
    // JPEG署名をチェック
    if (size >= 2 && content[0] == 0xFF && content[1] == 0xD8) {
        return "image/jpeg";
    }
    
    // GIF署名をチェック
    const uint8_t gifSig87a[] = {'G', 'I', 'F', '8', '7', 'a'};
    const uint8_t gifSig89a[] = {'G', 'I', 'F', '8', '9', 'a'};
    if (size >= 6 && (checkSignature(content, size, gifSig87a, 6) || 
                      checkSignature(content, size, gifSig89a, 6))) {
        return "image/gif";
    }
    
    // HTML開始タグをチェック
    if (size >= 5) {
        String start = String((char*)content).substring(0, 5);
        start.toLowerCase();
        if (start.indexOf("<!doc") >= 0 || start.indexOf("<html") >= 0) {
            return "text/html";
        }
    }
    
    // JSONの開始をチェック
    if (content[0] == '{' || content[0] == '[') {
        return "application/json";
    }
    
    // XMLの開始をチェック
    if (size >= 5 && String((char*)content).substring(0, 5) == "<?xml") {
        return "application/xml";
    }
    
    return getDefaultMimeType();
}

String MimeTypeResolver::generateContentTypeHeader(const String& filepath) {
    MimeInfo info = getMimeInfo(filepath);
    String header = info.mimeType;
    
    if (info.isText && info.charset.length() > 0) {
        header += "; charset=" + info.charset;
    }
    
    return header;
}

bool MimeTypeResolver::isTextFile(const String& mimeType) {
    return mimeType.startsWith("text/") || 
           mimeType == "application/json" ||
           mimeType == "application/xml" ||
           mimeType == "application/javascript";
}

bool MimeTypeResolver::isImageFile(const String& mimeType) {
    return mimeType.startsWith("image/");
}

bool MimeTypeResolver::isJavaScriptFile(const String& mimeType) {
    return mimeType == "text/javascript" || 
           mimeType == "application/javascript";
}

bool MimeTypeResolver::isCssFile(const String& mimeType) {
    return mimeType == "text/css";
}

bool MimeTypeResolver::isCompressible(const String& mimeType) {
    // テキストファイルは一般的に圧縮可能
    if (isTextFile(mimeType)) {
        return true;
    }
    
    // SVGは圧縮可能
    if (mimeType == "image/svg+xml") {
        return true;
    }
    
    // その他のファイルタイプは圧縮しない
    return false;
}

String MimeTypeResolver::getDefaultMimeType() {
    return "text/plain";
}

// プライベートメソッド実装
String MimeTypeResolver::extractExtension(const String& filepath) {
    int lastDot = filepath.lastIndexOf('.');
    if (lastDot >= 0 && lastDot < filepath.length() - 1) {
        String extension = filepath.substring(lastDot);
        extension.toLowerCase();
        return extension;
    }
    return "";
}

const MimeTypeResolver::MimeTypeMapping* MimeTypeResolver::findMimeTypeMapping(const String& extension) {
    // 線形検索（テーブルが小さいため）
    for (int i = 0; i < mimeTypesCount_; i++) {
        if (extension.equals(mimeTypes_[i].extension)) {
            return &mimeTypes_[i];
        }
    }
    return nullptr;
}

bool MimeTypeResolver::checkSignature(const uint8_t* content, size_t size, 
                                     const uint8_t* signature, size_t sigSize) {
    if (!content || !signature || size < sigSize) {
        return false;
    }
    
    for (size_t i = 0; i < sigSize; i++) {
        if (content[i] != signature[i]) {
            return false;
        }
    }
    
    return true;
}