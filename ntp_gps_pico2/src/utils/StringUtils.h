#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <Arduino.h>

/**
 * @brief String processing utilities for embedded systems
 * 
 * Provides common string operations with memory-efficient implementations
 * specifically designed for Arduino/Raspberry Pi Pico environments.
 */
class StringUtils {
public:
    /**
     * @brief Safe string indexing with bounds checking
     * @param str Source string
     * @param searchStr String to search for
     * @param startIndex Starting position (default: 0)
     * @return Index of first occurrence, or -1 if not found
     */
    static int safeIndexOf(const String& str, const String& searchStr, int startIndex = 0) {
        if (str.length() == 0 || searchStr.length() == 0 || startIndex < 0) {
            return -1;
        }
        return str.indexOf(searchStr, startIndex);
    }

    /**
     * @brief Safe string indexing for single character
     * @param str Source string
     * @param ch Character to search for
     * @param startIndex Starting position (default: 0)
     * @return Index of first occurrence, or -1 if not found
     */
    static int safeIndexOf(const String& str, char ch, int startIndex = 0) {
        if (str.length() == 0 || startIndex < 0) {
            return -1;
        }
        return str.indexOf(ch, startIndex);
    }

    /**
     * @brief Safe substring extraction with bounds checking
     * @param str Source string
     * @param startIndex Starting position
     * @param endIndex Ending position (optional)
     * @return Substring or empty string if invalid indices
     */
    static String safeSubstring(const String& str, int startIndex, int endIndex = -1) {
        if (str.length() == 0 || startIndex < 0 || startIndex >= (int)str.length()) {
            return String("");
        }
        
        if (endIndex == -1) {
            endIndex = str.length();
        }
        
        if (endIndex <= startIndex || endIndex > (int)str.length()) {
            endIndex = str.length();
        }
        
        return str.substring(startIndex, endIndex);
    }

    /**
     * @brief Convert string to lowercase (memory efficient)
     * @param str String to convert (modified in place)
     */
    static void toLowerCaseInPlace(String& str) {
        str.toLowerCase();
    }

    /**
     * @brief Convert string to uppercase (memory efficient)
     * @param str String to convert (modified in place)
     */
    static void toUpperCaseInPlace(String& str) {
        str.toUpperCase();
    }

    /**
     * @brief Create lowercase copy of string
     * @param str Source string
     * @return Lowercase copy
     */
    static String toLowerCaseCopy(const String& str) {
        String result = str;
        result.toLowerCase();
        return result;
    }

    /**
     * @brief Create uppercase copy of string
     * @param str Source string
     * @return Uppercase copy
     */
    static String toUpperCaseCopy(const String& str) {
        String result = str;
        result.toUpperCase();
        return result;
    }

    /**
     * @brief Trim whitespace from both ends
     * @param str String to trim (modified in place)
     */
    static void trimInPlace(String& str) {
        str.trim();
    }

    /**
     * @brief Create trimmed copy of string
     * @param str Source string
     * @return Trimmed copy
     */
    static String trimCopy(const String& str) {
        String result = str;
        result.trim();
        return result;
    }

    /**
     * @brief Check if string starts with prefix (case sensitive)
     * @param str String to check
     * @param prefix Prefix to look for
     * @return true if string starts with prefix
     */
    static bool startsWith(const String& str, const String& prefix) {
        if (prefix.length() > str.length()) {
            return false;
        }
        return str.startsWith(prefix);
    }

    /**
     * @brief Check if string starts with prefix (case insensitive)
     * @param str String to check
     * @param prefix Prefix to look for
     * @return true if string starts with prefix (ignoring case)
     */
    static bool startsWithIgnoreCase(const String& str, const String& prefix) {
        if (prefix.length() > str.length()) {
            return false;
        }
        String strLower = toLowerCaseCopy(str.substring(0, prefix.length()));
        String prefixLower = toLowerCaseCopy(prefix);
        return strLower.equals(prefixLower);
    }

    /**
     * @brief Check if string ends with suffix (case sensitive)
     * @param str String to check
     * @param suffix Suffix to look for
     * @return true if string ends with suffix
     */
    static bool endsWith(const String& str, const String& suffix) {
        if (suffix.length() > str.length()) {
            return false;
        }
        return str.endsWith(suffix);
    }

    /**
     * @brief Check if string ends with suffix (case insensitive)
     * @param str String to check
     * @param suffix Suffix to look for
     * @return true if string ends with suffix (ignoring case)
     */
    static bool endsWithIgnoreCase(const String& str, const String& suffix) {
        if (suffix.length() > str.length()) {
            return false;
        }
        String strSuffix = str.substring(str.length() - suffix.length());
        return toLowerCaseCopy(strSuffix).equals(toLowerCaseCopy(suffix));
    }

    /**
     * @brief Extract HTTP header value from header line
     * @param headerLine Complete header line (e.g., "Content-Type: text/html")
     * @param headerName Header name to extract
     * @return Header value or empty string if not found
     */
    static String extractHeaderValue(const String& headerLine, const String& headerName) {
        String normalizedLine = toLowerCaseCopy(headerLine);
        String normalizedName = toLowerCaseCopy(headerName);
        
        int colonIndex = safeIndexOf(normalizedLine, ":");
        if (colonIndex == -1) {
            return String("");
        }
        
        String lineHeaderName = safeSubstring(normalizedLine, 0, colonIndex);
        lineHeaderName = trimCopy(lineHeaderName);
        
        if (lineHeaderName.equals(normalizedName)) {
            String value = safeSubstring(headerLine, colonIndex + 1);
            return trimCopy(value);
        }
        
        return String("");
    }

    /**
     * @brief Parse URL to extract path and query string
     * @param url Complete URL
     * @param path Output parameter for path part
     * @param queryString Output parameter for query string part
     */
    static void parseUrl(const String& url, String& path, String& queryString) {
        int queryStart = safeIndexOf(url, '?');
        if (queryStart == -1) {
            path = url;
            queryString = String("");
        } else {
            path = safeSubstring(url, 0, queryStart);
            queryString = safeSubstring(url, queryStart + 1);
        }
    }

    /**
     * @brief Extract file extension from filepath
     * @param filepath Complete file path
     * @return File extension (including dot) or empty string
     */
    static String getFileExtension(const String& filepath) {
        int lastDot = filepath.lastIndexOf('.');
        if (lastDot == -1 || lastDot == (int)filepath.length() - 1) {
            return String("");
        }
        return safeSubstring(filepath, lastDot);
    }

    /**
     * @brief Check if string contains only numeric characters
     * @param str String to check
     * @return true if string is numeric
     */
    static bool isNumeric(const String& str) {
        if (str.length() == 0) {
            return false;
        }
        
        for (unsigned int i = 0; i < str.length(); i++) {
            if (!isDigit(str.charAt(i))) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Safe integer parsing with error handling
     * @param str String to parse
     * @param defaultValue Value to return if parsing fails
     * @return Parsed integer or default value
     */
    static long parseInt(const String& str, long defaultValue = 0) {
        if (str.length() == 0) {
            return defaultValue;
        }
        
        long result = str.toInt();
        // Arduino's String::toInt() returns 0 for invalid input
        // Check if the string actually represents zero or is invalid
        if (result == 0 && !str.equals("0")) {
            return defaultValue;
        }
        
        return result;
    }

    /**
     * @brief Safe float parsing with error handling
     * @param str String to parse
     * @param defaultValue Value to return if parsing fails
     * @return Parsed float or default value
     */
    static double parseFloat(const String& str, double defaultValue = 0.0) {
        if (str.length() == 0) {
            return defaultValue;
        }
        
        double result = str.toDouble();
        // Arduino's String::toDouble() returns 0.0 for invalid input
        // Check if the string actually represents zero or is invalid
        if (result == 0.0 && !str.equals("0") && !str.equals("0.0")) {
            return defaultValue;
        }
        
        return result;
    }

    /**
     * @brief Simple wildcard pattern matching (* and ?)
     * @param text Text to match against
     * @param pattern Pattern with wildcards
     * @return true if pattern matches text
     */
    static bool wildcardMatch(const String& text, const String& pattern) {
        if (pattern.equals("*")) {
            return true;
        }
        
        // Simple cases
        if (safeIndexOf(pattern, '*') == -1 && safeIndexOf(pattern, '?') == -1) {
            return text.equals(pattern);
        }
        
        // Handle patterns like "*text*"
        if (pattern.startsWith("*") && pattern.endsWith("*") && pattern.length() > 2) {
            String middle = safeSubstring(pattern, 1, pattern.length() - 1);
            return safeIndexOf(text, middle) >= 0;
        }
        
        // Handle patterns like "*suffix"
        if (pattern.startsWith("*") && !pattern.endsWith("*")) {
            String suffix = safeSubstring(pattern, 1);
            return endsWith(text, suffix);
        }
        
        // Handle patterns like "prefix*"
        if (!pattern.startsWith("*") && pattern.endsWith("*")) {
            String prefix = safeSubstring(pattern, 0, pattern.length() - 1);
            return startsWith(text, prefix);
        }
        
        // Complex patterns would require more sophisticated matching
        return false;
    }

    /**
     * @brief Sanitize string for safe usage (remove dangerous characters)
     * @param str String to sanitize
     * @param maxLength Maximum allowed length
     * @return Sanitized string
     */
    static String sanitize(const String& str, int maxLength = 255) {
        String result = str;
        
        // Remove dangerous characters for path traversal and injection
        result.replace("..", "");
        result.replace("<", "&lt;");
        result.replace(">", "&gt;");
        result.replace("\"", "&quot;");
        result.replace("'", "&#x27;");
        
        // Limit length
        if ((int)result.length() > maxLength) {
            result = safeSubstring(result, 0, maxLength);
        }
        
        return result;
    }

    /**
     * @brief Count occurrences of character in string
     * @param str String to search in
     * @param ch Character to count
     * @return Number of occurrences
     */
    static int countOccurrences(const String& str, char ch) {
        int count = 0;
        for (unsigned int i = 0; i < str.length(); i++) {
            if (str.charAt(i) == ch) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief Count occurrences of substring in string
     * @param str String to search in
     * @param searchStr Substring to count
     * @return Number of occurrences
     */
    static int countOccurrences(const String& str, const String& searchStr) {
        if (searchStr.length() == 0) {
            return 0;
        }
        
        int count = 0;
        int pos = 0;
        
        while ((pos = safeIndexOf(str, searchStr, pos)) != -1) {
            count++;
            pos += searchStr.length();
        }
        
        return count;
    }
};

#endif // STRING_UTILS_H