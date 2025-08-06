#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>
#include "../network/NtpTypes.h"

/**
 * @brief 時刻処理の共通機能を提供するユーティリティクラス
 * 
 * 複数のクラスで重複していた時刻変換、計算、フォーマット処理を
 * 統一化し、NTPタイムスタンプと Unix時間の相互変換を提供する。
 */
class TimeUtils {
public:
    // 時刻変換定数
    static const uint32_t UNIX_TO_NTP_OFFSET = 2208988800UL; // 1900-1970 の秒数
    static const uint64_t MICROS_PER_SECOND = 1000000UL;
    static const uint32_t MILLIS_PER_SECOND = 1000UL;
    static const uint32_t YEAR_2020_UNIX = 1577836800UL; // 2020年1月1日 00:00:00 UTC

    /**
     * @brief Unix時間をNTPタイムスタンプに変換
     * @param unix_time Unix時間（秒）
     * @param microseconds マイクロ秒部分
     * @return NTPタイムスタンプ（64bit）
     */
    static uint64_t unixToNtpTimestamp(uint32_t unix_time, uint32_t microseconds = 0) {
        uint32_t ntp_seconds = unix_time + UNIX_TO_NTP_OFFSET;
        uint32_t ntp_fraction = static_cast<uint32_t>((static_cast<uint64_t>(microseconds) * 0x100000000ULL) / MICROS_PER_SECOND);
        return (static_cast<uint64_t>(ntp_seconds) << 32) | ntp_fraction;
    }

    /**
     * @brief NTPタイムスタンプをUnix時間に変換
     * @param ntp_timestamp NTPタイムスタンプ（64bit）
     * @param microseconds_out マイクロ秒部分の出力先（オプション）
     * @return Unix時間（秒）
     */
    static uint32_t ntpToUnixTime(uint64_t ntp_timestamp, uint32_t* microseconds_out = nullptr) {
        uint32_t ntp_seconds = static_cast<uint32_t>(ntp_timestamp >> 32);
        uint32_t ntp_fraction = static_cast<uint32_t>(ntp_timestamp & 0xFFFFFFFF);
        
        if (microseconds_out) {
            *microseconds_out = static_cast<uint32_t>((static_cast<uint64_t>(ntp_fraction) * MICROS_PER_SECOND) >> 32);
        }
        
        return ntp_seconds - UNIX_TO_NTP_OFFSET;
    }

    /**
     * @brief 現在のマイクロ秒タイムスタンプを取得
     * @return マイクロ秒精度のタイムスタンプ
     */
    static uint64_t getCurrentMicros() {
        static uint32_t last_millis = 0;
        static uint32_t micros_offset = 0;
        
        uint32_t current_millis = millis();
        uint32_t current_micros = micros();
        
        // millis()のオーバーフロー検出と調整
        if (current_millis < last_millis) {
            micros_offset += (0xFFFFFFFF - last_millis) + current_millis;
        } else {
            micros_offset += (current_millis - last_millis);
        }
        
        last_millis = current_millis;
        
        return static_cast<uint64_t>(micros_offset) * 1000ULL + (current_micros % 1000);
    }

    /**
     * @brief 高精度NTPタイムスタンプ生成
     * @param unix_base_time ベースのUnix時間
     * @param use_micros_precision マイクロ秒精度使用フラグ
     * @return 高精度NTPタイムスタンプ
     */
    static uint64_t generatePreciseNtpTimestamp(uint32_t unix_base_time, bool use_micros_precision = true) {
        uint32_t microseconds = 0;
        
        if (use_micros_precision) {
            uint64_t precise_time = getCurrentMicros();
            microseconds = static_cast<uint32_t>(precise_time % MICROS_PER_SECOND);
        }
        
        return unixToNtpTimestamp(unix_base_time, microseconds);
    }

    /**
     * @brief 時刻の妥当性チェック
     * @param unix_time 検証対象のUnix時間
     * @return true: 妥当, false: 不正
     */
    static bool isValidUnixTime(uint32_t unix_time) {
        // 2020年以前または2100年以降は不正と判断
        const uint32_t YEAR_2100_UNIX = 4102444800UL; // 2100年1月1日
        return (unix_time >= YEAR_2020_UNIX && unix_time < YEAR_2100_UNIX);
    }

    /**
     * @brief 時刻差分の計算（秒単位）
     * @param time1 時刻1（Unix時間）
     * @param time2 時刻2（Unix時間）
     * @return 時刻差分（秒）、オーバーフロー時は0
     */
    static uint32_t calculateTimeDifference(uint32_t time1, uint32_t time2) {
        if (time1 > time2) {
            return time1 - time2;
        } else {
            return time2 - time1;
        }
    }

    /**
     * @brief 時刻精度の計算（マイクロ秒）
     * @param reference_time 基準時刻（Unix時間）
     * @param measured_time 測定時刻（Unix時間）
     * @param reference_micros 基準マイクロ秒
     * @param measured_micros 測定マイクロ秒
     * @return 精度差分（マイクロ秒）
     */
    static int64_t calculatePrecisionDifference(uint32_t reference_time, uint32_t measured_time,
                                              uint32_t reference_micros, uint32_t measured_micros) {
        int64_t sec_diff = static_cast<int64_t>(measured_time) - static_cast<int64_t>(reference_time);
        int64_t micros_diff = static_cast<int64_t>(measured_micros) - static_cast<int64_t>(reference_micros);
        
        return (sec_diff * MICROS_PER_SECOND) + micros_diff;
    }

    /**
     * @brief 時刻文字列フォーマット（ISO 8601形式）
     * @param unix_time Unix時間
     * @param buffer 出力バッファ
     * @param buffer_size バッファサイズ
     * @param include_microseconds マイクロ秒を含むかどうか
     * @param microseconds マイクロ秒値
     */
    static void formatTimeString(uint32_t unix_time, char* buffer, size_t buffer_size,
                               bool include_microseconds = false, uint32_t microseconds = 0) {
        if (!buffer || buffer_size < 9) { // 最低限の文字数チェック（HH:MM:SS = 8文字 + NULL）
            return;
        }
        
        // Initialize buffer
        buffer[0] = '\0';
        
        // 簡易的な時刻フォーマット（実用的な実装では time.h を使用）
        uint32_t days = unix_time / 86400;
        uint32_t remaining = unix_time % 86400;
        uint32_t hours = remaining / 3600;
        remaining %= 3600;
        uint32_t minutes = remaining / 60;
        uint32_t seconds = remaining % 60;
        
        // Unix エポック（1970年1月1日）からの日数計算は複雑なため、
        // ここでは時:分:秒のみフォーマット
        if (include_microseconds && buffer_size >= 26) {
            snprintf(buffer, buffer_size, "%02u:%02u:%02u.%06u", 
                    hours, minutes, seconds, microseconds);
        } else {
            snprintf(buffer, buffer_size, "%02u:%02u:%02u", 
                    hours, minutes, seconds);
        }
    }

    /**
     * @brief うるう年判定
     * @param year 年（4桁）
     * @return true: うるう年, false: 平年
     */
    static bool isLeapYear(uint16_t year) {
        return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    }

    /**
     * @brief NTPのShort Format（32bit）をマイクロ秒に変換
     * @param ntp_short NTP Short Format値
     * @return マイクロ秒値
     */
    static uint32_t ntpShortToMicroseconds(uint32_t ntp_short) {
        return static_cast<uint32_t>((static_cast<uint64_t>(ntp_short) * MICROS_PER_SECOND) >> 16);
    }

    /**
     * @brief マイクロ秒をNTPのShort Format（32bit）に変換
     * @param microseconds マイクロ秒値
     * @return NTP Short Format値
     */
    static uint32_t microsecondsToNtpShort(uint32_t microseconds) {
        return static_cast<uint32_t>((static_cast<uint64_t>(microseconds) << 16) / MICROS_PER_SECOND);
    }

    /**
     * @brief システム時刻の単調性チェック
     * @param current_time 現在時刻
     * @param last_time 前回時刻
     * @return true: 単調増加, false: 時刻巻き戻り検出
     */
    static bool isMonotonicTime(uint32_t current_time, uint32_t last_time) {
        // 小さな時刻巻き戻り（1秒以内）は許容
        const uint32_t TOLERANCE_SECONDS = 1;
        
        if (current_time >= last_time) {
            return true; // 正常な時刻進行
        }
        
        // 巻き戻り量をチェック
        uint32_t rollback = last_time - current_time;
        return (rollback <= TOLERANCE_SECONDS);
    }
};

#endif // TIME_UTILS_H