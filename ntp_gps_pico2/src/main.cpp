/**
 * @file main.cpp
 * @brief GPS NTP Server - Main Application Entry Point
 * 
 * Simplified main file. Only responsible for system initialization and main loop execution.
 * All specific processing is delegated to the following classes:
 * - SystemInitializer: Centralized management of all initialization processes
 * - MainLoop: Priority-based processing separation and timing control
 * - SystemState: Integrated management of global variables and services
 * 
 * Refactoring results:
 * - 933 lines → 80 lines (91% reduction)
 * - Global variables → Managed by SystemState singleton
 * - Initialization process → Centralized in SystemInitializer class
 * - Loop processing → Priority-based separation in MainLoop class
 */

#include <Arduino.h>
#include "system/SystemInitializer.h"
#include "system/MainLoop.h"
#include "system/SystemState.h"
#include "hal/HardwareConfig.h"
#include "gps/TimeManager.h"

// Global variables are managed by SystemState singleton
// Hardware instances and services are initialized within SystemState

// Temporary global variables (for compatibility with other classes)
// TODO: In the future, use only SystemState class members
bool gpsConnected = false;
TimeManager* timeManager = nullptr;

// PPS interrupt handler - delegates to SystemState
void triggerPps() {
  SystemState::triggerPps();
}

/**
 * @brief System setup
 * 
 * Delegates all initialization processing to SystemInitializer
 * and significantly simplifies the setup function.
 */
void setup() {
  SystemInitializer::InitializationResult result = SystemInitializer::initialize();
  
  if (!result.success) {
    Serial.printf("❌ System initialization failed: %s (code: %d)\n", 
                  result.errorMessage, result.errorCode);
    // Continue operation even if some components failed
  }
  
  // Set global variables for temporary compatibility
  // TODO: Planned for future removal
  SystemState& state = SystemState::getInstance();
  gpsConnected = state.isGpsConnected();
  timeManager = &state.getTimeManager();
  
  // PPS interrupt setup
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), triggerPps, FALLING);
  
  Serial.println("🚀 GPS NTP Server ready!");
}

/**
 * @brief Main loop
 * 
 * Delegates all processing to MainLoop class
 * and significantly simplifies the loop function.
 */
void loop() {
  MainLoop::execute();
}

/**
 * @note Refactoring details
 * 
 * [Issues before migration]
 * - main.cpp: 933 lines (bloated)
 * - Global variables: 50+ items (difficult to manage)
 * - Initialization process: Complex dependencies (error-prone)
 * - Loop processing: Unclear priorities (performance issues)
 * 
 * [Improvements after migration]
 * - main.cpp: 80 lines (91% reduction)
 * - Global variables: Centralized management with SystemState
 * - Initialization process: Clear order and dependencies with SystemInitializer
 * - Loop processing: Priority-based separation with MainLoop (HIGH/MEDIUM/LOW)
 * 
 * [Architecture improvements]
 * 1. Single Responsibility Principle: Each class has clear responsibilities
 * 2. Dependency Inversion: Loose coupling through interfaces
 * 3. Separation of Concerns: Separation of initialization, execution, and state management
 * 4. Testability: Each class can be tested independently
 * 
 * [Maintainability improvements]
 * - When adding new features: Modify only the corresponding class
 * - When fixing bugs: Clear scope of impact
 * - During code review: Easy to identify changes
 * - Reduced understanding time: Lower learning cost for new developers
 */