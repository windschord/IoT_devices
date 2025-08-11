#include "CacheManager.h"

CacheManager::CacheManager(int maxEntries, size_t maxTotalSize) 
    : maxEntries_(maxEntries)
    , maxTotalSize_(maxTotalSize)
    , entryCount_(0)
    , currentSize_(0)
    , hitCount_(0)
    , missCount_(0) {
    
    // キャッシュエントリ配列を動的に確保
    entries_ = new CacheEntry[maxEntries];
    
    // エントリを初期化
    for (int i = 0; i < maxEntries_; i++) {
        clearEntry(i);
    }
}

bool CacheManager::put(const String& key, const String& content, const String& mimeType, unsigned long ttlSeconds) {
    if (key.length() == 0 || content.length() == 0) {
        return false;
    }

    // 既存エントリをチェック
    int existingIndex = findEntry(key);
    if (existingIndex >= 0) {
        // 既存エントリを更新
        currentSize_ -= calculateEntrySize(entries_[existingIndex]);
        entries_[existingIndex].content = content;
        entries_[existingIndex].mimeType = mimeType;
        entries_[existingIndex].timestamp = millis();
        entries_[existingIndex].expires = ttlSeconds;
        currentSize_ += calculateEntrySize(entries_[existingIndex]);
        touchEntry(existingIndex);
        return true;
    }

    // 新しいエントリのサイズをチェック
    CacheEntry tempEntry;
    tempEntry.key = key;
    tempEntry.content = content;
    tempEntry.mimeType = mimeType;
    size_t entrySize = calculateEntrySize(tempEntry);

    if (entrySize > maxTotalSize_) {
        return false; // エントリが大きすぎる
    }

    // 空きエントリを検索
    int emptyIndex = findEmptyEntry();
    if (emptyIndex < 0) {
        // 空きがない場合はLRUエントリを削除
        emptyIndex = findLRUEntry();
        if (emptyIndex >= 0) {
            currentSize_ -= calculateEntrySize(entries_[emptyIndex]);
            clearEntry(emptyIndex);
        } else {
            return false; // 削除できるエントリがない
        }
    }

    // 容量チェック
    while (currentSize_ + entrySize > maxTotalSize_) {
        int lruIndex = findLRUEntry();
        if (lruIndex >= 0 && lruIndex != emptyIndex) {
            currentSize_ -= calculateEntrySize(entries_[lruIndex]);
            clearEntry(lruIndex);
        } else {
            return false; // 十分な容量を確保できない
        }
    }

    // 新しいエントリを追加
    entries_[emptyIndex].key = key;
    entries_[emptyIndex].content = content;
    entries_[emptyIndex].mimeType = mimeType;
    entries_[emptyIndex].timestamp = millis();
    entries_[emptyIndex].expires = ttlSeconds;
    entries_[emptyIndex].accessCount = 0;
    entries_[emptyIndex].isValid = true;
    
    currentSize_ += entrySize;
    if (emptyIndex >= entryCount_) {
        entryCount_ = emptyIndex + 1;
    }
    
    return true;
}

CacheManager::CacheEntry* CacheManager::get(const String& key) {
    int index = findEntry(key);
    if (index < 0) {
        missCount_++;
        return nullptr;
    }

    CacheEntry* entry = &entries_[index];
    
    // 期限切れチェック
    if (isExpired(*entry)) {
        clearEntry(index);
        missCount_++;
        return nullptr;
    }

    // アクセス情報を更新
    touchEntry(index);
    hitCount_++;
    
    return entry;
}

bool CacheManager::has(const String& key) {
    int index = findEntry(key);
    if (index < 0) {
        return false;
    }

    // 期限切れチェック
    if (isExpired(entries_[index])) {
        clearEntry(index);
        return false;
    }

    return true;
}

bool CacheManager::remove(const String& key) {
    int index = findEntry(key);
    if (index < 0) {
        return false;
    }

    currentSize_ -= calculateEntrySize(entries_[index]);
    clearEntry(index);
    return true;
}

void CacheManager::clear() {
    for (int i = 0; i < maxEntries_; i++) {
        clearEntry(i);
    }
    entryCount_ = 0;
    currentSize_ = 0;
}

int CacheManager::cleanupExpired() {
    int cleanedCount = 0;
    unsigned long currentTime = millis();
    
    for (int i = 0; i < entryCount_; i++) {
        if (entries_[i].isValid && isExpired(entries_[i])) {
            currentSize_ -= calculateEntrySize(entries_[i]);
            clearEntry(i);
            cleanedCount++;
        }
    }
    
    return cleanedCount;
}

CacheManager::CacheStats CacheManager::getStats() {
    CacheStats stats;
    stats.totalEntries = entryCount_;
    stats.validEntries = 0;
    stats.totalSize = currentSize_;
    stats.hitCount = hitCount_;
    stats.missCount = missCount_;
    stats.hitRatio = getHitRatio();
    
    // 有効エントリ数をカウント
    for (int i = 0; i < entryCount_; i++) {
        if (entries_[i].isValid && !isExpired(entries_[i])) {
            stats.validEntries++;
        }
    }
    
    return stats;
}

int CacheManager::optimize(int targetSizePercent) {
    if (targetSizePercent < 10 || targetSizePercent > 100) {
        targetSizePercent = 80;
    }
    
    size_t targetSize = (maxTotalSize_ * targetSizePercent) / 100;
    int removedCount = 0;
    
    // まず期限切れエントリを削除
    removedCount += cleanupExpired();
    
    // まだサイズが大きい場合はLRUで削除
    while (currentSize_ > targetSize && entryCount_ > 0) {
        int lruIndex = findLRUEntry();
        if (lruIndex >= 0) {
            currentSize_ -= calculateEntrySize(entries_[lruIndex]);
            clearEntry(lruIndex);
            removedCount++;
        } else {
            break;
        }
    }
    
    return removedCount;
}

int CacheManager::removeByPattern(const String& pattern) {
    int removedCount = 0;
    
    for (int i = 0; i < entryCount_; i++) {
        if (entries_[i].isValid && matchPattern(pattern, entries_[i].key)) {
            currentSize_ -= calculateEntrySize(entries_[i]);
            clearEntry(i);
            removedCount++;
        }
    }
    
    return removedCount;
}

float CacheManager::getHitRatio() {
    size_t totalRequests = hitCount_ + missCount_;
    if (totalRequests == 0) {
        return 0.0f;
    }
    return (float)hitCount_ / (float)totalRequests;
}

size_t CacheManager::getCurrentSize() {
    return currentSize_;
}

size_t CacheManager::getAvailableSize() {
    return maxTotalSize_ > currentSize_ ? maxTotalSize_ - currentSize_ : 0;
}

void CacheManager::printDebugInfo() {
    CacheStats stats = getStats();
    Serial.println("=== Cache Debug Info ===");
    Serial.println("Total Entries: " + String(stats.totalEntries));
    Serial.println("Valid Entries: " + String(stats.validEntries));
    Serial.println("Total Size: " + String(stats.totalSize) + " bytes");
    Serial.println("Hit Count: " + String(stats.hitCount));
    Serial.println("Miss Count: " + String(stats.missCount));
    Serial.println("Hit Ratio: " + String(stats.hitRatio * 100, 1) + "%");
    Serial.println("Available Size: " + String(getAvailableSize()) + " bytes");
    Serial.println("========================");
}

// プライベートメソッド実装
int CacheManager::findEntry(const String& key) {
    for (int i = 0; i < entryCount_; i++) {
        if (entries_[i].isValid && entries_[i].key.equals(key)) {
            return i;
        }
    }
    return -1;
}

int CacheManager::findEmptyEntry() {
    for (int i = 0; i < maxEntries_; i++) {
        if (!entries_[i].isValid) {
            return i;
        }
    }
    return -1;
}

int CacheManager::findLRUEntry() {
    int lruIndex = -1;
    unsigned long oldestTime = ULONG_MAX;
    size_t lowestAccess = SIZE_MAX;
    
    for (int i = 0; i < entryCount_; i++) {
        if (!entries_[i].isValid) {
            continue;
        }
        
        // アクセス回数が少ないエントリを優先
        if (entries_[i].accessCount < lowestAccess || 
            (entries_[i].accessCount == lowestAccess && entries_[i].timestamp < oldestTime)) {
            lruIndex = i;
            oldestTime = entries_[i].timestamp;
            lowestAccess = entries_[i].accessCount;
        }
    }
    
    return lruIndex;
}

bool CacheManager::isExpired(const CacheEntry& entry) {
    if (entry.expires == 0) {
        return false; // 無期限
    }
    
    unsigned long currentTime = millis();
    return (currentTime - entry.timestamp) >= (entry.expires * 1000);
}

size_t CacheManager::calculateEntrySize(const CacheEntry& entry) {
    return entry.key.length() + entry.content.length() + entry.mimeType.length() + 
           sizeof(entry.timestamp) + sizeof(entry.expires) + sizeof(entry.accessCount);
}

bool CacheManager::matchPattern(const String& pattern, const String& text) {
    if (pattern == "*") {
        return true;
    }
    
    if (pattern.indexOf('*') < 0) {
        return pattern.equals(text);
    }
    
    // 簡単なワイルドカードマッチング
    if (pattern.startsWith("*") && pattern.endsWith("*")) {
        String middle = pattern.substring(1, pattern.length() - 1);
        return text.indexOf(middle) >= 0;
    }
    
    if (pattern.startsWith("*")) {
        String suffix = pattern.substring(1);
        return text.endsWith(suffix);
    }
    
    if (pattern.endsWith("*")) {
        String prefix = pattern.substring(0, pattern.length() - 1);
        return text.startsWith(prefix);
    }
    
    return pattern.equals(text);
}

void CacheManager::clearEntry(int index) {
    if (index >= 0 && index < maxEntries_) {
        entries_[index].key = "";
        entries_[index].content = "";
        entries_[index].mimeType = "";
        entries_[index].timestamp = 0;
        entries_[index].expires = 0;
        entries_[index].accessCount = 0;
        entries_[index].isValid = false;
    }
}

void CacheManager::touchEntry(int index) {
    if (index >= 0 && index < maxEntries_) {
        entries_[index].accessCount++;
        entries_[index].timestamp = millis();
    }
}