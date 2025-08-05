#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Unity Test Framework Configuration for Raspberry Pi Pico 2

// Memory management
#define UNITY_EXCLUDE_DETAILS
#define UNITY_EXCLUDE_SETJMP_H

// Include 64-bit integer support
#define UNITY_INCLUDE_64
#define UNITY_SUPPORT_64

// Floating point support
#define UNITY_INCLUDE_FLOAT
#define UNITY_FLOAT_PRECISION 0.00001f
#define UNITY_DOUBLE_PRECISION 0.0000001

// Test running configuration
#define UNITY_EXCLUDE_TIME_H

#endif // UNITY_CONFIG_H