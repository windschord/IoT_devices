#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <Arduino.h>

/**
 * @brief Generic data structures optimized for embedded systems
 * 
 * Provides memory-efficient, template-based data structures with
 * compile-time size optimization and minimal dynamic allocation.
 */

/**
 * @brief Fixed-size circular buffer (ring buffer)
 * @tparam T Element type
 * @tparam N Buffer capacity
 */
template<typename T, size_t N>
class CircularBuffer {
private:
    T buffer_[N];
    size_t head_;
    size_t tail_;
    bool full_;

public:
    CircularBuffer() : head_(0), tail_(0), full_(false) {}
    
    /**
     * @brief Add element to buffer (overwrites oldest if full)
     */
    void push(const T& item) {
        buffer_[head_] = item;
        
        if (full_) {
            tail_ = (tail_ + 1) % N;
        }
        
        head_ = (head_ + 1) % N;
        full_ = (head_ == tail_);
    }
    
    /**
     * @brief Remove and return oldest element
     */
    bool pop(T& item) {
        if (empty()) {
            return false;
        }
        
        item = buffer_[tail_];
        full_ = false;
        tail_ = (tail_ + 1) % N;
        
        return true;
    }
    
    /**
     * @brief Peek at oldest element without removing
     */
    bool peek(T& item) const {
        if (empty()) {
            return false;
        }
        
        item = buffer_[tail_];
        return true;
    }
    
    /**
     * @brief Check if buffer is empty
     */
    bool empty() const {
        return (!full_) && (head_ == tail_);
    }
    
    /**
     * @brief Check if buffer is full
     */
    bool isFull() const {
        return full_;
    }
    
    /**
     * @brief Get current number of elements
     */
    size_t size() const {
        if (full_) {
            return N;
        }
        
        if (head_ >= tail_) {
            return head_ - tail_;
        }
        
        return N + head_ - tail_;
    }
    
    /**
     * @brief Get buffer capacity
     */
    constexpr size_t capacity() const {
        return N;
    }
    
    /**
     * @brief Clear all elements
     */
    void clear() {
        head_ = 0;
        tail_ = 0;
        full_ = false;
    }
    
    /**
     * @brief Access element by index (0 = oldest)
     */
    const T& operator[](size_t index) const {
        if (index >= size()) {
            static T defaultValue{};
            return defaultValue;
        }
        
        size_t actualIndex = (tail_ + index) % N;
        return buffer_[actualIndex];
    }
};

/**
 * @brief Fixed-size priority queue (min-heap)
 * @tparam T Element type (must support comparison operators)
 * @tparam N Maximum capacity
 */
template<typename T, size_t N>
class PriorityQueue {
private:
    T data_[N];
    size_t size_;
    
    void heapifyUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (data_[index] >= data_[parent]) {
                break;
            }
            
            // Swap with parent
            T temp = data_[index];
            data_[index] = data_[parent];
            data_[parent] = temp;
            
            index = parent;
        }
    }
    
    void heapifyDown(size_t index) {
        while (true) {
            size_t smallest = index;
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            
            if (left < size_ && data_[left] < data_[smallest]) {
                smallest = left;
            }
            
            if (right < size_ && data_[right] < data_[smallest]) {
                smallest = right;
            }
            
            if (smallest == index) {
                break;
            }
            
            // Swap with smallest child
            T temp = data_[index];
            data_[index] = data_[smallest];
            data_[smallest] = temp;
            
            index = smallest;
        }
    }

public:
    PriorityQueue() : size_(0) {}
    
    /**
     * @brief Insert element into queue
     */
    bool push(const T& item) {
        if (size_ >= N) {
            return false; // Queue full
        }
        
        data_[size_] = item;
        heapifyUp(size_);
        size_++;
        
        return true;
    }
    
    /**
     * @brief Remove and return minimum element
     */
    bool pop(T& item) {
        if (size_ == 0) {
            return false; // Queue empty
        }
        
        item = data_[0];
        
        // Move last element to root and heapify
        data_[0] = data_[size_ - 1];
        size_--;
        
        if (size_ > 0) {
            heapifyDown(0);
        }
        
        return true;
    }
    
    /**
     * @brief Peek at minimum element
     */
    bool top(T& item) const {
        if (size_ == 0) {
            return false;
        }
        
        item = data_[0];
        return true;
    }
    
    /**
     * @brief Check if queue is empty
     */
    bool empty() const {
        return size_ == 0;
    }
    
    /**
     * @brief Check if queue is full
     */
    bool full() const {
        return size_ >= N;
    }
    
    /**
     * @brief Get current size
     */
    size_t size() const {
        return size_;
    }
    
    /**
     * @brief Get capacity
     */
    constexpr size_t capacity() const {
        return N;
    }
    
    /**
     * @brief Clear all elements
     */
    void clear() {
        size_ = 0;
    }
};

/**
 * @brief Fixed-size hash table with linear probing
 * @tparam K Key type
 * @tparam V Value type  
 * @tparam N Hash table size (should be prime)
 */
template<typename K, typename V, size_t N>
class HashTable {
private:
    struct Entry {
        K key;
        V value;
        bool occupied;
        bool deleted;
        
        Entry() : occupied(false), deleted(false) {}
    };
    
    Entry table_[N];
    size_t size_;
    
    /**
     * @brief Simple hash function for various types
     */
    size_t hash(const K& key) const {
        return hashImpl(key) % N;
    }
    
    // Hash implementation for different types
    template<typename T>
    size_t hashImpl(const T& key) const {
        // Generic hash using memory representation
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&key);
        size_t hash = 5381;
        
        for (size_t i = 0; i < sizeof(T); i++) {
            hash = ((hash << 5) + hash) + ptr[i];
        }
        
        return hash;
    }
    
    // Specialized hash for strings
    size_t hashImpl(const String& key) const {
        size_t hash = 5381;
        for (size_t i = 0; i < key.length(); i++) {
            hash = ((hash << 5) + hash) + key[i];
        }
        return hash;
    }
    
    // Specialized hash for C strings
    size_t hashImpl(const char* key) const {
        size_t hash = 5381;
        while (*key) {
            hash = ((hash << 5) + hash) + *key++;
        }
        return hash;
    }
    
    /**
     * @brief Find entry index for key
     */
    size_t findIndex(const K& key) const {
        size_t index = hash(key);
        size_t original = index;
        
        do {
            if (!table_[index].occupied || 
                (!table_[index].deleted && table_[index].key == key)) {
                return index;
            }
            index = (index + 1) % N;
        } while (index != original);
        
        return N; // Not found / table full
    }

public:
    HashTable() : size_(0) {}
    
    /**
     * @brief Insert or update key-value pair
     */
    bool put(const K& key, const V& value) {
        if (size_ >= N * 3 / 4) { // Load factor limit
            return false;
        }
        
        size_t index = findIndex(key);
        if (index >= N) {
            return false; // Table full
        }
        
        if (!table_[index].occupied || table_[index].deleted) {
            // New entry
            if (!table_[index].occupied) {
                size_++;
            }
            table_[index].occupied = true;
            table_[index].deleted = false;
        }
        
        table_[index].key = key;
        table_[index].value = value;
        
        return true;
    }
    
    /**
     * @brief Get value for key
     */
    bool get(const K& key, V& value) const {
        size_t index = findIndex(key);
        
        if (index < N && table_[index].occupied && !table_[index].deleted) {
            value = table_[index].value;
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Check if key exists
     */
    bool contains(const K& key) const {
        V dummy;
        return get(key, dummy);
    }
    
    /**
     * @brief Remove key-value pair
     */
    bool remove(const K& key) {
        size_t index = findIndex(key);
        
        if (index < N && table_[index].occupied && !table_[index].deleted) {
            table_[index].deleted = true;
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Get current number of entries
     */
    size_t size() const {
        return size_;
    }
    
    /**
     * @brief Check if table is empty
     */
    bool empty() const {
        return size_ == 0;
    }
    
    /**
     * @brief Clear all entries
     */
    void clear() {
        for (size_t i = 0; i < N; i++) {
            table_[i].occupied = false;
            table_[i].deleted = false;
        }
        size_ = 0;
    }
    
    /**
     * @brief Get capacity
     */
    constexpr size_t capacity() const {
        return N;
    }
    
    /**
     * @brief Get load factor
     */
    float loadFactor() const {
        return static_cast<float>(size_) / static_cast<float>(N);
    }
};

/**
 * @brief Fixed-size stack
 * @tparam T Element type
 * @tparam N Stack capacity
 */
template<typename T, size_t N>
class Stack {
private:
    T data_[N];
    size_t top_;

public:
    Stack() : top_(0) {}
    
    /**
     * @brief Push element onto stack
     */
    bool push(const T& item) {
        if (top_ >= N) {
            return false; // Stack full
        }
        
        data_[top_++] = item;
        return true;
    }
    
    /**
     * @brief Pop element from stack
     */
    bool pop(T& item) {
        if (top_ == 0) {
            return false; // Stack empty
        }
        
        item = data_[--top_];
        return true;
    }
    
    /**
     * @brief Peek at top element
     */
    bool peek(T& item) const {
        if (top_ == 0) {
            return false; // Stack empty
        }
        
        item = data_[top_ - 1];
        return true;
    }
    
    /**
     * @brief Check if stack is empty
     */
    bool empty() const {
        return top_ == 0;
    }
    
    /**
     * @brief Check if stack is full
     */
    bool full() const {
        return top_ >= N;
    }
    
    /**
     * @brief Get current size
     */
    size_t size() const {
        return top_;
    }
    
    /**
     * @brief Get capacity
     */
    constexpr size_t capacity() const {
        return N;
    }
    
    /**
     * @brief Clear stack
     */
    void clear() {
        top_ = 0;
    }
};

/**
 * @brief Fixed-size queue (FIFO)
 * @tparam T Element type
 * @tparam N Queue capacity
 */
template<typename T, size_t N>
class Queue {
private:
    T data_[N];
    size_t head_;
    size_t tail_;
    size_t size_;

public:
    Queue() : head_(0), tail_(0), size_(0) {}
    
    /**
     * @brief Add element to back of queue
     */
    bool enqueue(const T& item) {
        if (size_ >= N) {
            return false; // Queue full
        }
        
        data_[tail_] = item;
        tail_ = (tail_ + 1) % N;
        size_++;
        
        return true;
    }
    
    /**
     * @brief Remove element from front of queue
     */
    bool dequeue(T& item) {
        if (size_ == 0) {
            return false; // Queue empty
        }
        
        item = data_[head_];
        head_ = (head_ + 1) % N;
        size_--;
        
        return true;
    }
    
    /**
     * @brief Peek at front element
     */
    bool front(T& item) const {
        if (size_ == 0) {
            return false; // Queue empty
        }
        
        item = data_[head_];
        return true;
    }
    
    /**
     * @brief Check if queue is empty
     */
    bool empty() const {
        return size_ == 0;
    }
    
    /**
     * @brief Check if queue is full
     */
    bool full() const {
        return size_ >= N;
    }
    
    /**
     * @brief Get current size
     */
    size_t size() const {
        return size_;
    }
    
    /**
     * @brief Get capacity
     */
    constexpr size_t capacity() const {
        return N;
    }
    
    /**
     * @brief Clear queue
     */
    void clear() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }
};

/**
 * @brief Statistics accumulator for numerical data
 * @tparam T Numerical type
 */
template<typename T>
class StatsAccumulator {
private:
    T sum_;
    T sumSquares_;
    T minimum_;
    T maximum_;
    size_t count_;

public:
    StatsAccumulator() : sum_(0), sumSquares_(0), count_(0) {
        reset();
    }
    
    /**
     * @brief Add new sample
     */
    void addSample(T value) {
        if (count_ == 0) {
            minimum_ = maximum_ = value;
        } else {
            if (value < minimum_) minimum_ = value;
            if (value > maximum_) maximum_ = value;
        }
        
        sum_ += value;
        sumSquares_ += value * value;
        count_++;
    }
    
    /**
     * @brief Get number of samples
     */
    size_t count() const { return count_; }
    
    /**
     * @brief Get sum of all samples
     */
    T sum() const { return sum_; }
    
    /**
     * @brief Get arithmetic mean
     */
    T mean() const {
        return (count_ > 0) ? (sum_ / static_cast<T>(count_)) : T(0);
    }
    
    /**
     * @brief Get minimum value
     */
    T min() const { return minimum_; }
    
    /**
     * @brief Get maximum value
     */
    T max() const { return maximum_; }
    
    /**
     * @brief Get range (max - min)
     */
    T range() const {
        return (count_ > 0) ? (maximum_ - minimum_) : T(0);
    }
    
    /**
     * @brief Get variance
     */
    T variance() const {
        if (count_ < 2) return T(0);
        
        T meanVal = mean();
        return (sumSquares_ / static_cast<T>(count_)) - (meanVal * meanVal);
    }
    
    /**
     * @brief Reset accumulator
     */
    void reset() {
        sum_ = T(0);
        sumSquares_ = T(0);
        count_ = 0;
        minimum_ = T(0);
        maximum_ = T(0);
    }
    
    /**
     * @brief Check if any samples have been added
     */
    bool empty() const {
        return count_ == 0;
    }
};

#endif // DATA_STRUCTURES_H