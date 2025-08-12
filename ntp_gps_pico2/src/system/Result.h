#ifndef RESULT_H
#define RESULT_H

#include <stddef.h>
#include <new>

// Forward declaration to avoid circular dependency
enum class ErrorType;

/**
 * @brief Type-safe error handling for embedded systems
 * 
 * Result<T, E> is a type that represents either a success with value T
 * or an error of type E. This provides exception-free error handling
 * suitable for embedded systems.
 * 
 * Usage:
 * ```cpp
 * Result<int, ErrorType> divide(int a, int b) {
 *     if (b == 0) {
 *         return Result<int, ErrorType>::error(ErrorType::SYSTEM_ERROR);
 *     }
 *     return Result<int, ErrorType>::ok(a / b);
 * }
 * 
 * auto result = divide(10, 2);
 * if (result.isOk()) {
 *     Serial.println(result.value());
 * } else {
 *     Serial.print("Error: ");
 *     Serial.println(static_cast<int>(result.error()));
 * }
 * ```
 */
template<typename T, typename E>
class Result {
private:
    bool success_;
    union {
        T value_;
        E error_;
    };
    
    // Private constructors to enforce factory methods
    explicit Result(T&& value) : success_(true) {
        new (&value_) T(static_cast<T&&>(value));
    }
    
    explicit Result(const T& value) : success_(true) {
        new (&value_) T(value);
    }
    
    explicit Result(E&& error) : success_(false) {
        new (&error_) E(static_cast<E&&>(error));
    }
    
    explicit Result(const E& error) : success_(false) {
        new (&error_) E(error);
    }

public:
    // Factory methods for creating Result instances
    static Result<T, E> ok(T&& value) {
        return Result<T, E>(static_cast<T&&>(value));
    }
    
    static Result<T, E> ok(const T& value) {
        return Result<T, E>(value);
    }
    
    static Result<T, E> error(E&& error) {
        return Result<T, E>(static_cast<E&&>(error));
    }
    
    static Result<T, E> error(const E& error) {
        return Result<T, E>(error);
    }
    
    // Copy constructor
    Result(const Result& other) : success_(other.success_) {
        if (success_) {
            new (&value_) T(other.value_);
        } else {
            new (&error_) E(other.error_);
        }
    }
    
    // Move constructor
    Result(Result&& other) : success_(other.success_) {
        if (success_) {
            new (&value_) T(static_cast<T&&>(other.value_));
        } else {
            new (&error_) E(static_cast<E&&>(other.error_));
        }
    }
    
    // Assignment operators
    Result& operator=(const Result& other) {
        if (this != &other) {
            this->~Result();
            success_ = other.success_;
            if (success_) {
                new (&value_) T(other.value_);
            } else {
                new (&error_) E(other.error_);
            }
        }
        return *this;
    }
    
    Result& operator=(Result&& other) {
        if (this != &other) {
            this->~Result();
            success_ = other.success_;
            if (success_) {
                new (&value_) T(static_cast<T&&>(other.value_));
            } else {
                new (&error_) E(static_cast<E&&>(other.error_));
            }
        }
        return *this;
    }
    
    // Destructor
    ~Result() {
        if (success_) {
            value_.~T();
        } else {
            error_.~E();
        }
    }
    
    // Status checking
    bool isOk() const { return success_; }
    bool isError() const { return !success_; }
    
    // Value access (for Ok case)
    const T& value() const {
        return value_;
    }
    
    T& value() {
        return value_;
    }
    
    // Error access (for Error case)
    const E& error() const {
        return error_;
    }
    
    E& error() {
        return error_;
    }
    
    // Safe value access with default
    T valueOr(const T& defaultValue) const {
        return success_ ? value_ : defaultValue;
    }
    
    // Monadic operations for chaining
    template<typename F>
    auto andThen(F func) const -> decltype(func(value_)) {
        using ReturnType = decltype(func(value_));
        if (success_) {
            return func(value_);
        } else {
            return ReturnType::error(error_);
        }
    }
    
    // Map operation for transforming success values
    template<typename F>
    auto map(F func) const -> Result<decltype(func(value_)), E> {
        using ReturnValueType = decltype(func(value_));
        if (success_) {
            return Result<ReturnValueType, E>::ok(func(value_));
        } else {
            return Result<ReturnValueType, E>::error(error_);
        }
    }
    
    // Map error operation for transforming errors
    template<typename F>
    auto mapError(F func) const -> Result<T, decltype(func(error_))> {
        using ReturnErrorType = decltype(func(error_));
        if (success_) {
            return Result<T, ReturnErrorType>::ok(value_);
        } else {
            return Result<T, ReturnErrorType>::error(func(error_));
        }
    }
    
    // Or else operation for providing fallback
    Result<T, E> orElse(const Result<T, E>& fallback) const {
        return success_ ? *this : fallback;
    }
    
    // Unwrap with panic (only for development/testing)
    T unwrap() const {
        if (!success_) {
            // In embedded systems, we prefer to reset rather than panic
            Serial.println("FATAL: Result::unwrap() called on error value");
            while(true) { /* Infinite loop to trigger watchdog */ }
        }
        return value_;
    }
    
    // Match-like operation
    template<typename OkFunc, typename ErrFunc>
    auto match(OkFunc okFunc, ErrFunc errFunc) const -> decltype(okFunc(value_)) {
        if (success_) {
            return okFunc(value_);
        } else {
            return errFunc(error_);
        }
    }
};

// Specialized version for void success type
template<typename E>
class Result<void, E> {
private:
    bool success_;
    E error_;
    
    explicit Result(bool success) : success_(success) {}
    explicit Result(E error) : success_(false), error_(error) {}

public:
    static Result<void, E> ok() {
        return Result<void, E>(true);
    }
    
    static Result<void, E> error(const E& error) {
        return Result<void, E>(error);
    }
    
    bool isOk() const { return success_; }
    bool isError() const { return !success_; }
    
    const E& error() const { return error_; }
    
    template<typename F>
    auto andThen(F func) const -> decltype(func()) {
        using ReturnType = decltype(func());
        if (success_) {
            return func();
        } else {
            return ReturnType::error(error_);
        }
    }
    
    template<typename F>
    Result<void, E> andThen(F func) const {
        if (success_) {
            func();
            return Result<void, E>::ok();
        } else {
            return Result<void, E>::error(error_);
        }
    }
};

// Common Result types for the system
using SystemResult = Result<void, ErrorType>;
using InitResult = Result<bool, ErrorType>;
using StringResult = Result<const char*, ErrorType>;

// Helper macros for common patterns
#define TRY(result) \
    do { \
        auto __temp_result = (result); \
        if (__temp_result.isError()) { \
            return typeof(__temp_result)::error(__temp_result.error()); \
        } \
    } while(0)

#define TRY_VALUE(result) \
    ({ \
        auto __temp_result = (result); \
        if (__temp_result.isError()) { \
            return typeof(__temp_result)::error(__temp_result.error()); \
        } \
        __temp_result.value(); \
    })

// Utility functions for common operations
template<typename T>
Result<T, ErrorType> okIf(bool condition, T value, ErrorType error) {
    return condition ? Result<T, ErrorType>::ok(value) : Result<T, ErrorType>::error(error);
}

inline SystemResult okIf(bool condition, ErrorType error) {
    return condition ? SystemResult::ok() : SystemResult::error(error);
}

#endif // RESULT_H