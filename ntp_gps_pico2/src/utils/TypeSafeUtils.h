#ifndef TYPE_SAFE_UTILS_H
#define TYPE_SAFE_UTILS_H

#include <Arduino.h>

/**
 * @brief Type-safe utility templates for embedded systems
 * 
 * Provides compile-time type safety, bounds checking, and generic programming
 * constructs optimized for microcontroller environments.
 */

/**
 * @brief Strong typedef wrapper for type safety
 * @tparam T Underlying type
 * @tparam Tag Unique tag type for differentiation
 */
template<typename T, typename Tag>
class StrongType {
private:
    T value_;

public:
    explicit StrongType(const T& value) : value_(value) {}
    
    // Explicit conversion back to underlying type
    explicit operator T() const { return value_; }
    
    // Access underlying value
    const T& get() const { return value_; }
    T& get() { return value_; }
    
    // Comparison operators
    bool operator==(const StrongType& other) const { return value_ == other.value_; }
    bool operator!=(const StrongType& other) const { return value_ != other.value_; }
    bool operator<(const StrongType& other) const { return value_ < other.value_; }
    bool operator<=(const StrongType& other) const { return value_ <= other.value_; }
    bool operator>(const StrongType& other) const { return value_ > other.value_; }
    bool operator>=(const StrongType& other) const { return value_ >= other.value_; }
    
    // Assignment
    StrongType& operator=(const T& value) { value_ = value; return *this; }
    StrongType& operator=(const StrongType& other) { value_ = other.value_; return *this; }
};

// Strong type definitions for common use cases
struct PortTag {};
struct TimeoutTag {};
struct BufferSizeTag {};
struct IndexTag {};

using Port = StrongType<uint16_t, PortTag>;
using Timeout = StrongType<uint32_t, TimeoutTag>;
using BufferSize = StrongType<size_t, BufferSizeTag>;
using Index = StrongType<size_t, IndexTag>;

/**
 * @brief Bounded value template with compile-time and runtime checks
 * @tparam T Underlying type
 * @tparam MIN Minimum value
 * @tparam MAX Maximum value
 */
template<typename T, T MIN, T MAX>
class BoundedValue {
private:
    T value_;
    
    static constexpr bool isInRange(T val) {
        return val >= MIN && val <= MAX;
    }

public:
    static constexpr T min_value = MIN;
    static constexpr T max_value = MAX;
    
    explicit BoundedValue(T value) : value_(clamp(value)) {}
    
    // Safe assignment with clamping
    BoundedValue& operator=(T value) {
        value_ = clamp(value);
        return *this;
    }
    
    // Access value
    T get() const { return value_; }
    operator T() const { return value_; }
    
    // Validation
    static bool isValid(T value) { return isInRange(value); }
    
    // Clamping utility
    static constexpr T clamp(T value) {
        return (value < MIN) ? MIN : ((value > MAX) ? MAX : value);
    }
    
    // Comparison operators
    bool operator==(const BoundedValue& other) const { return value_ == other.value_; }
    bool operator!=(const BoundedValue& other) const { return value_ != other.value_; }
    bool operator<(const BoundedValue& other) const { return value_ < other.value_; }
    bool operator<=(const BoundedValue& other) const { return value_ <= other.value_; }
    bool operator>(const BoundedValue& other) const { return value_ > other.value_; }
    bool operator>=(const BoundedValue& other) const { return value_ >= other.value_; }
};

// Common bounded types for embedded systems
using Percentage = BoundedValue<uint8_t, 0, 100>;
using Priority = BoundedValue<uint8_t, 0, 255>;
using RetryCount = BoundedValue<uint8_t, 0, 10>;

/**
 * @brief Optional value template (similar to std::optional)
 * @tparam T Value type
 */
template<typename T>
class Optional {
private:
    bool has_value_;
    union {
        T value_;
        char dummy_; // For trivial construction
    };

public:
    Optional() : has_value_(false), dummy_('\0') {}
    
    explicit Optional(const T& value) : has_value_(true), value_(value) {}
    
    ~Optional() {
        if (has_value_) {
            value_.~T();
        }
    }
    
    // Copy constructor
    Optional(const Optional& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new(&value_) T(other.value_);
        } else {
            dummy_ = '\0';
        }
    }
    
    // Assignment operator
    Optional& operator=(const Optional& other) {
        if (this != &other) {
            if (has_value_) {
                value_.~T();
            }
            has_value_ = other.has_value_;
            if (has_value_) {
                new(&value_) T(other.value_);
            } else {
                dummy_ = '\0';
            }
        }
        return *this;
    }
    
    // Value assignment
    Optional& operator=(const T& value) {
        if (has_value_) {
            value_ = value;
        } else {
            new(&value_) T(value);
            has_value_ = true;
        }
        return *this;
    }
    
    // Check if has value
    bool hasValue() const { return has_value_; }
    explicit operator bool() const { return has_value_; }
    
    // Access value (unsafe - check hasValue() first)
    const T& value() const { return value_; }
    T& value() { return value_; }
    
    // Safe access with default
    const T& valueOr(const T& defaultValue) const {
        return has_value_ ? value_ : defaultValue;
    }
    
    // Reset to empty state
    void reset() {
        if (has_value_) {
            value_.~T();
            has_value_ = false;
            dummy_ = '\0';
        }
    }
};

/**
 * @brief Fixed-size array with bounds checking
 * @tparam T Element type
 * @tparam N Array size
 */
template<typename T, size_t N>
class SafeArray {
private:
    T data_[N];
    size_t size_;

public:
    SafeArray() : size_(0) {}
    
    // Initialize with size
    explicit SafeArray(size_t initialSize) : size_((initialSize <= N) ? initialSize : N) {
        // Initialize elements to default value
        for (size_t i = 0; i < size_; i++) {
            data_[i] = T();
        }
    }
    
    // Bounds-checked access
    const T& at(size_t index) const {
        if (index >= size_) {
            // Return reference to static default value to avoid crash
            static T defaultValue = T();
            return defaultValue;
        }
        return data_[index];
    }
    
    T& at(size_t index) {
        if (index >= size_) {
            // Return reference to static default value to avoid crash
            static T defaultValue = T();
            return defaultValue;
        }
        return data_[index];
    }
    
    // Unchecked access (for performance-critical code)
    const T& operator[](size_t index) const { return data_[index]; }
    T& operator[](size_t index) { return data_[index]; }
    
    // Size management
    size_t size() const { return size_; }
    size_t capacity() const { return N; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == N; }
    
    // Add element (with bounds checking)
    bool push_back(const T& value) {
        if (size_ >= N) {
            return false; // Array full
        }
        data_[size_++] = value;
        return true;
    }
    
    // Remove last element
    bool pop_back() {
        if (size_ == 0) {
            return false; // Array empty
        }
        size_--;
        return true;
    }
    
    // Clear array
    void clear() { size_ = 0; }
    
    // Resize (with bounds checking)
    void resize(size_t newSize) {
        size_ = (newSize <= N) ? newSize : N;
        
        // Initialize new elements if growing
        if (newSize > size_) {
            for (size_t i = size_; i < newSize; i++) {
                data_[i] = T();
            }
        }
    }
    
    // Iterator support (basic)
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
    
    // Front and back access
    T& front() { return data_[0]; }
    T& back() { return data_[size_ - 1]; }
    const T& front() const { return data_[0]; }
    const T& back() const { return data_[size_ - 1]; }
};

/**
 * @brief Compile-time utilities
 */
struct CompileTimeUtils {
    /**
     * @brief Check if type is integral at compile time
     */
    template<typename T>
    static constexpr bool is_integral() {
        return false; // Default implementation
    }
    
    /**
     * @brief Get size of array at compile time
     */
    template<typename T, size_t N>
    static constexpr size_t array_size(T (&)[N]) {
        return N;
    }
    
    /**
     * @brief Compile-time minimum
     */
    template<typename T>
    static constexpr T min(T a, T b) {
        return (a < b) ? a : b;
    }
    
    /**
     * @brief Compile-time maximum
     */
    template<typename T>
    static constexpr T max(T a, T b) {
        return (a > b) ? a : b;
    }
    
    /**
     * @brief Compile-time power calculation
     */
    static constexpr uint32_t pow(uint32_t base, uint32_t exp) {
        return (exp == 0) ? 1 : base * pow(base, exp - 1);
    }
};

// Specializations for is_integral
template<>
constexpr bool CompileTimeUtils::is_integral<uint8_t>() { return true; }
template<>
constexpr bool CompileTimeUtils::is_integral<int8_t>() { return true; }
template<>
constexpr bool CompileTimeUtils::is_integral<uint16_t>() { return true; }
template<>
constexpr bool CompileTimeUtils::is_integral<int16_t>() { return true; }
template<>
constexpr bool CompileTimeUtils::is_integral<uint32_t>() { return true; }
template<>
constexpr bool CompileTimeUtils::is_integral<int32_t>() { return true; }

/**
 * @brief Type-safe enum class with additional utilities
 */
template<typename Enum>
class TypeSafeEnum {
private:
    Enum value_;

public:
    explicit TypeSafeEnum(Enum value) : value_(value) {}
    
    // Get underlying value
    Enum get() const { return value_; }
    
    // Comparison
    bool operator==(const TypeSafeEnum& other) const { return value_ == other.value_; }
    bool operator!=(const TypeSafeEnum& other) const { return value_ != other.value_; }
    
    // String conversion utility (must be specialized)
    const char* toString() const;
    
    // Validation utility (must be specialized)  
    static bool isValid(Enum value);
};

/**
 * @brief RAII wrapper for automatic resource management
 * @tparam Resource Resource type
 * @tparam Deleter Function pointer type for cleanup
 */
template<typename Resource, void(*Deleter)(Resource&)>
class RAII {
private:
    Resource resource_;
    bool owns_resource_;

public:
    explicit RAII(const Resource& resource) : resource_(resource), owns_resource_(true) {}
    
    // Move constructor
    RAII(RAII&& other) : resource_(other.resource_), owns_resource_(other.owns_resource_) {
        other.owns_resource_ = false;
    }
    
    // Destructor - automatic cleanup
    ~RAII() {
        if (owns_resource_ && Deleter) {
            Deleter(resource_);
        }
    }
    
    // No copy constructor or assignment (move-only)
    RAII(const RAII&) = delete;
    RAII& operator=(const RAII&) = delete;
    
    // Move assignment
    RAII& operator=(RAII&& other) {
        if (this != &other) {
            if (owns_resource_ && Deleter) {
                Deleter(resource_);
            }
            resource_ = other.resource_;
            owns_resource_ = other.owns_resource_;
            other.owns_resource_ = false;
        }
        return *this;
    }
    
    // Access resource
    Resource& get() { return resource_; }
    const Resource& get() const { return resource_; }
    
    // Release ownership
    Resource release() {
        owns_resource_ = false;
        return resource_;
    }
};

#endif // TYPE_SAFE_UTILS_H