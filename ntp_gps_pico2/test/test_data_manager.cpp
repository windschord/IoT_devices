#include "test_data_manager.h"

ComprehensiveTestDataManager* g_testDataManager = nullptr;

void ComprehensiveTestDataManager::initializeScenarios() {
    // Initialize all 20 comprehensive test scenarios
    
    // ========== Initialization Scenarios ==========
    scenarios[0] = TestScenario("cold_boot_success", 
        "Cold boot initialization with all systems working",
        TestScenarioCategory::INITIALIZATION, true, ErrorType::SYSTEM_ERROR, 10000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[1] = TestScenario("cold_boot_gps_failure", 
        "Cold boot with GPS initialization failure",
        TestScenarioCategory::INITIALIZATION, false, ErrorType::GPS_ERROR, 5000)
        .withGpsData(GpsTestData::createNoFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData::createUnsynchronized())
        .withSystemData(SystemStateTestData::createPartialFailure());
    
    scenarios[2] = TestScenario("cold_boot_network_failure", 
        "Cold boot with network initialization failure",
        TestScenarioCategory::INITIALIZATION, false, ErrorType::NETWORK_ERROR, 5000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData::createDisconnected())
        .withNtpData(NtpTestData::createUnsynchronized())
        .withSystemData(SystemStateTestData::createPartialFailure());
    
    // ========== Normal Operation Scenarios ==========
    scenarios[3] = TestScenario("normal_operation_optimal", 
        "Normal operation with optimal conditions",
        TestScenarioCategory::NORMAL_OPERATION, true, ErrorType::SYSTEM_ERROR, 60000)
        .withGpsData(GpsTestData::createHighAccuracy())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[4] = TestScenario("normal_operation_2d_fix", 
        "Normal operation with 2D GPS fix",
        TestScenarioCategory::NORMAL_OPERATION, true, ErrorType::SYSTEM_ERROR, 30000)
        .withGpsData(GpsTestData::create2DFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[5] = TestScenario("normal_operation_dhcp_network", 
        "Normal operation with DHCP network configuration",
        TestScenarioCategory::NORMAL_OPERATION, true, ErrorType::SYSTEM_ERROR, 30000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})  // Default is DHCP
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[6] = TestScenario("normal_operation_static_ip", 
        "Normal operation with static IP configuration",
        TestScenarioCategory::NORMAL_OPERATION, true, ErrorType::SYSTEM_ERROR, 30000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData::createStaticIP())
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    // ========== Error Handling Scenarios ==========
    scenarios[7] = TestScenario("error_gps_signal_lost", 
        "GPS signal lost during operation",
        TestScenarioCategory::ERROR_HANDLING, false, ErrorType::GPS_ERROR, 15000)
        .withGpsData(GpsTestData::createNoFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData::createStratum2())  // Falls back to stratum 2
        .withSystemData(SystemStateTestData::createPartialFailure());
    
    scenarios[8] = TestScenario("error_network_disconnected", 
        "Network connection lost during operation",
        TestScenarioCategory::ERROR_HANDLING, false, ErrorType::NETWORK_ERROR, 15000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData::createDisconnected())
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createPartialFailure());
    
    scenarios[9] = TestScenario("error_memory_low", 
        "System running with low memory",
        TestScenarioCategory::ERROR_HANDLING, true, ErrorType::SYSTEM_ERROR, 20000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createLowMemory());
    
    scenarios[10] = TestScenario("error_overheating", 
        "System overheating condition",
        TestScenarioCategory::ERROR_HANDLING, false, ErrorType::HARDWARE_FAILURE, 10000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createOverheating());
    
    // ========== Recovery Scenarios ==========
    scenarios[11] = TestScenario("recovery_gps_restore", 
        "GPS signal recovery after loss",
        TestScenarioCategory::RECOVERY, true, ErrorType::SYSTEM_ERROR, 30000)
        .withGpsData(GpsTestData::create3DFix())  // Eventually recovers
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[12] = TestScenario("recovery_network_restore", 
        "Network connection recovery",
        TestScenarioCategory::RECOVERY, true, ErrorType::SYSTEM_ERROR, 25000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})  // Eventually reconnects
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[13] = TestScenario("recovery_after_reboot", 
        "System recovery after restart",
        TestScenarioCategory::RECOVERY, true, ErrorType::SYSTEM_ERROR, 15000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    // ========== Performance Scenarios ==========
    scenarios[14] = TestScenario("performance_high_ntp_load", 
        "High NTP request load handling",
        TestScenarioCategory::PERFORMANCE, true, ErrorType::SYSTEM_ERROR, 60000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData::createHighTraffic())
        .withNtpData(NtpTestData::createHighLoad())
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[15] = TestScenario("performance_packet_loss", 
        "Network operation with packet loss",
        TestScenarioCategory::PERFORMANCE, true, ErrorType::SYSTEM_ERROR, 45000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(TestDataUtilities::createNetworkDataWithPacketLoss(5.0f))
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    // ========== Stress Test Scenarios ==========
    scenarios[16] = TestScenario("stress_continuous_operation", 
        "24-hour continuous operation stress test",
        TestScenarioCategory::STRESS_TEST, true, ErrorType::SYSTEM_ERROR, 86400000)  // 24 hours
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[17] = TestScenario("stress_rapid_requests", 
        "Rapid NTP request burst handling",
        TestScenarioCategory::STRESS_TEST, true, ErrorType::SYSTEM_ERROR, 10000)
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData::createHighTraffic())
        .withNtpData(NtpTestData::createHighLoad())
        .withSystemData(SystemStateTestData::createHealthy());
    
    // ========== Integration Scenarios ==========
    scenarios[18] = TestScenario("integration_full_system", 
        "Full system integration test with all components",
        TestScenarioCategory::INTEGRATION, true, ErrorType::SYSTEM_ERROR, 120000)  // 2 minutes
        .withGpsData(GpsTestData::create3DFix())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
    
    scenarios[19] = TestScenario("integration_disaster_alert", 
        "Integration test with QZSS disaster alert",
        TestScenarioCategory::INTEGRATION, true, ErrorType::SYSTEM_ERROR, 30000)
        .withGpsData(GpsTestData::createDcxAlert())
        .withNetworkData(NetworkTestData{})
        .withNtpData(NtpTestData{})
        .withSystemData(SystemStateTestData::createHealthy());
}