#pragma once

/**
 * @file CodingStandards.h
 * @brief GPS NTP Server Project Coding Standards and Naming Conventions
 * 
 * This file defines the project-wide coding standards and naming conventions
 * to ensure consistent, maintainable, and professional code quality across
 * the entire GPS NTP Server codebase.
 * 
 * @version 1.0.0
 * @date 2025-08-11
 * @author GPS NTP Server Development Team
 */

namespace CodingStandards {

/**
 * @brief Project-wide naming conventions and coding standards
 * 
 * These conventions ensure consistency across all source files and
 * facilitate international collaboration and maintenance.
 */

// ============================================================================
// NAMING CONVENTIONS
// ============================================================================

/**
 * @section Classes and Types
 * 
 * - Classes: PascalCase (e.g., GpsClient, TimeManager, SystemState)
 * - Structs: PascalCase (e.g., GpsSummaryData, SystemStatistics)
 * - Enums: PascalCase (e.g., ErrorCode, ConnectionState)
 * - Type aliases: PascalCase (e.g., using ConfigMap = std::map<String, String>)
 */

/**
 * @section Functions and Methods
 * 
 * - Public methods: camelCase (e.g., getGpsData(), updateConfiguration())
 * - Private methods: camelCase (e.g., initializeHardware(), processData())
 * - Static functions: camelCase (e.g., getInstance(), createDefault())
 * - Getter methods: get + PascalCase (e.g., getNetworkStatus())
 * - Setter methods: set + PascalCase (e.g., setGpsConnected())
 * - Boolean getters: is/has + PascalCase (e.g., isConnected(), hasValidFix())
 */

/**
 * @section Variables and Members
 * 
 * - Local variables: camelCase (e.g., gpsData, currentTime, networkStatus)
 * - Member variables: camelCase (e.g., gpsConnected, lastUpdateTime)
 * - Constants: UPPER_SNAKE_CASE (e.g., GPS_PPS_PIN, MAX_RETRY_COUNT)
 * - Volatile variables: camelCase with volatile keyword (e.g., volatile bool ppsReceived)
 */

/**
 * @section Files and Directories
 * 
 * - Header files: PascalCase + .h (e.g., GpsClient.h, TimeManager.h)
 * - Source files: PascalCase + .cpp (e.g., GpsClient.cpp, TimeManager.cpp)
 * - Directories: lowercase (e.g., src/, gps/, network/, system/)
 * - Test files: test_ + lower_snake_case + _test.cpp (e.g., test_gps_client_test.cpp)
 */

/**
 * @section Namespaces and Macros
 * 
 * - Namespaces: PascalCase (e.g., SystemConstants, NetworkUtils)
 * - Macros: UPPER_SNAKE_CASE (e.g., DEBUG_GPS, LOG_LEVEL_INFO)
 * - Include guards: FILENAME_H (e.g., GPS_CLIENT_H, TIME_MANAGER_H)
 */

// ============================================================================
// ABBREVIATION AND ACRONYM STANDARDS
// ============================================================================

/**
 * @brief Standardized abbreviations and acronyms
 * 
 * Consistent use of technical abbreviations to ensure code readability
 * and international understanding.
 */

/**
 * @section GPS/GNSS Related Terms
 * 
 * - GPS: Global Positioning System (keep uppercase in identifiers)
 * - GNSS: Global Navigation Satellite System (keep uppercase)
 * - PVT: Position, Velocity, Time (keep uppercase)
 * - SAT: Satellite (keep uppercase)
 * - PPS: Pulse Per Second (keep uppercase)
 * - UTC: Coordinated Universal Time (keep uppercase)
 * - RTC: Real Time Clock (keep uppercase)
 * - NMEA: National Marine Electronics Association (keep uppercase)
 * - UBX: u-blox protocol (keep uppercase)
 * 
 * Examples:
 * - GpsClient (not GPS_Client or gps_client)
 * - getPvtData() (not getPVTdata or get_pvt_data)
 * - ubxNavSatData (not UBX_NAV_SAT_data_t)
 */

/**
 * @section Network Related Terms
 * 
 * - NTP: Network Time Protocol (keep uppercase)
 * - HTTP: Hypertext Transfer Protocol (keep uppercase)
 * - UDP: User Datagram Protocol (keep uppercase)
 * - TCP: Transmission Control Protocol (keep uppercase)
 * - IP: Internet Protocol (keep uppercase)
 * - DHCP: Dynamic Host Configuration Protocol (keep uppercase)
 * - API: Application Programming Interface (keep uppercase)
 * - URL: Uniform Resource Locator (keep uppercase)
 * - JSON: JavaScript Object Notation (keep uppercase)
 * 
 * Examples:
 * - NtpServer (not NTP_Server or ntpServer)
 * - HttpRequestParser (not HTTP_Request_Parser)
 * - getApiEndpoint() (not getAPIendpoint)
 */

/**
 * @section Hardware Related Terms
 * 
 * - I2C: Inter-Integrated Circuit (keep uppercase)
 * - SPI: Serial Peripheral Interface (keep uppercase)
 * - UART: Universal Asynchronous Receiver-Transmitter (keep uppercase)
 * - GPIO: General Purpose Input/Output (keep uppercase)
 * - PWM: Pulse Width Modulation (keep uppercase)
 * - ADC: Analog-to-Digital Converter (keep uppercase)
 * - LED: Light Emitting Diode (keep uppercase)
 * - OLED: Organic Light Emitting Diode (keep uppercase)
 * 
 * Examples:
 * - I2cUtils (not i2c_utils or I2C_Utils)
 * - SpiConfiguration (not SPI_Configuration)
 * - configurePwmPin() (not configurePWMpin)
 */

// ============================================================================
// DOCUMENTATION STANDARDS
// ============================================================================

/**
 * @brief Doxygen documentation requirements
 * 
 * All public interfaces must be documented using Doxygen style comments
 * in English language for international compatibility.
 */

/**
 * @section Required Documentation Elements
 * 
 * For Classes:
 * - @brief: One-line description of class purpose
 * - @details: Detailed description of functionality and usage
 * - @note: Important usage notes or limitations
 * - @warning: Critical warnings about usage
 * - @version: Version information
 * - @author: Author information
 * 
 * For Functions:
 * - @brief: One-line description of function purpose
 * - @param: Description of each parameter (name and purpose)
 * - @return: Description of return value and possible values
 * - @throws: Description of exceptions (if applicable)
 * - @note: Important usage notes
 * - @warning: Critical warnings
 * - @see: Related functions or classes
 * 
 * For Variables:
 * - @brief: Description of variable purpose
 * - @note: Important notes about usage or limitations
 */

/**
 * @section Comment Style Requirements
 * 
 * - All comments must be in English
 * - Use Doxygen style comments (/** ... */) for API documentation
 * - Use regular comments (// or /* ... */) for implementation details
 * - Avoid obvious comments (e.g., // increment counter)
 * - Explain WHY, not WHAT (focus on business logic and decisions)
 * - Update comments when code changes
 */

// ============================================================================
// CODE ORGANIZATION STANDARDS
// ============================================================================

/**
 * @brief File organization and structure standards
 * 
 * Consistent file organization improves navigation and maintenance.
 */

/**
 * @section Header File Organization
 * 
 * 1. Copyright notice (if applicable)
 * 2. File description with @file Doxygen tag
 * 3. Include guard
 * 4. System includes (Arduino, standard library)
 * 5. Third-party includes (SparkFun, Ethernet, etc.)
 * 6. Project includes (relative paths)
 * 7. Forward declarations
 * 8. Type definitions and enums
 * 9. Class declaration
 * 10. Inline function implementations
 * 11. Include guard end
 */

/**
 * @section Source File Organization
 * 
 * 1. Copyright notice (if applicable)
 * 2. File description with @file Doxygen tag
 * 3. Corresponding header include
 * 4. System includes
 * 5. Third-party includes
 * 6. Project includes
 * 7. Using declarations (if necessary)
 * 8. Static/anonymous namespace definitions
 * 9. Class member implementations
 * 10. Free function implementations
 */

/**
 * @section Function Organization Within Classes
 * 
 * Public section order:
 * 1. Constructors
 * 2. Destructor
 * 3. Copy/move constructors and assignment operators
 * 4. Static factory methods
 * 5. Main public interface methods
 * 6. Getters and setters
 * 7. Operator overloads
 * 
 * Private section order:
 * 1. Private member functions
 * 2. Private member variables
 * 3. Static member variables
 */

// ============================================================================
// QUALITY ASSURANCE STANDARDS
// ============================================================================

/**
 * @brief Code quality and maintenance requirements
 */

/**
 * @section Testing Requirements
 * 
 * - All public methods must have corresponding unit tests
 * - Test names should be descriptive: test_<function>_<scenario>_<expected_result>
 * - Use GoogleTest framework for C++ unit tests
 * - Achieve >90% code coverage for critical components
 * - Mock hardware dependencies for reliable testing
 */

/**
 * @section Performance Requirements
 * 
 * - Optimize for embedded systems (limited RAM/Flash)
 * - Avoid dynamic memory allocation where possible
 * - Use constexpr for compile-time constants
 * - Minimize interrupt handler execution time
 * - Profile critical paths (NTP response time, GPS processing)
 */

/**
 * @section Compatibility Requirements
 * 
 * - Target: Raspberry Pi Pico 2 (RP2350)
 * - Compiler: C++17 standard minimum
 * - Arduino framework compatibility
 * - PlatformIO build system
 * - Cross-platform development support (macOS, Linux, Windows)
 */

} // namespace CodingStandards

/**
 * @example Naming Convention Examples
 * 
 * @code{.cpp}
 * // Class declaration (PascalCase)
 * class GpsTimeManager {
 * public:
 *   // Constructor
 *   GpsTimeManager();
 *   
 *   // Public methods (camelCase)
 *   bool initializeGps();
 *   void updateSystemTime();
 *   GpsData getCurrentGpsData() const;
 *   
 *   // Getters/Setters
 *   bool isGpsConnected() const;
 *   void setGpsConnected(bool connected);
 *   
 * private:
 *   // Private methods (camelCase)
 *   void processGpsData();
 *   bool validateTimeData(const GpsData& data);
 *   
 *   // Member variables (camelCase)
 *   bool gpsConnected;
 *   unsigned long lastUpdateTime;
 *   
 *   // Constants (UPPER_SNAKE_CASE)
 *   static constexpr unsigned long UPDATE_INTERVAL = 1000;
 * };
 * 
 * // Free functions (camelCase)
 * bool initializeHardware();
 * String formatTimestamp(unsigned long timestamp);
 * 
 * // Constants (UPPER_SNAKE_CASE)
 * constexpr uint8_t GPS_PPS_PIN = 8;
 * constexpr uint8_t MAX_RETRY_COUNT = 3;
 * @endcode
 */