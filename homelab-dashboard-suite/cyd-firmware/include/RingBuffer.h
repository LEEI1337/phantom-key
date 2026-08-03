#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <vector>
#include <cstdint>
#include <cstring>

// Ring Buffer für Zeitreihendaten (24-48h Historie)
// Speichert im PSRAM oder Flash des CYD

template<typename T>
class RingBuffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    size_t head;
    size_t count;
    
public:
    RingBuffer(size_t _capacity) : capacity(_capacity), head(0), count(0) {
        buffer.resize(capacity);
    }
    
    // Neuen Wert hinzufügen (überschreibt ältesten wenn voll)
    void push(const T& value) {
        buffer[head] = value;
        head = (head + 1) % capacity;
        if (count < capacity) {
            count++;
        }
    }
    
    // Wert an Index holen (0 = neuester, n = ältester)
    T get(size_t index) const {
        if (index >= count) {
            return T(); // Default value
        }
        size_t actualIndex = (head + capacity - index - 1) % capacity;
        return buffer[actualIndex];
    }
    
    // Alle Werte als Vector (neu nach alt)
    std::vector<T> getAll() const {
        std::vector<T> result;
        result.reserve(count);
        for (size_t i = 0; i < count; i++) {
            result.push_back(get(i));
        }
        return result;
    }
    
    // Statistik
    size_t size() const { return count; }
    size_t max_size() const { return capacity; }
    bool empty() const { return count == 0; }
    bool full() const { return count == capacity; }
    
    // Clear
    void clear() {
        head = 0;
        count = 0;
    }
    
    // Durchschnitt der letzten N Werte
    T average(size_t n = 0) const {
        if (count == 0) return T();
        
        size_t avgCount = (n == 0 || n > count) ? count : n;
        T sum = T();
        
        for (size_t i = 0; i < avgCount; i++) {
            sum += get(i);
        }
        
        return sum / avgCount;
    }
    
    // Minimum der letzten N Werte
    T min(size_t n = 0) const {
        if (count == 0) return T();
        
        size_t minCount = (n == 0 || n > count) ? count : n;
        T minVal = get(0);
        
        for (size_t i = 1; i < minCount; i++) {
            T val = get(i);
            if (val < minVal) {
                minVal = val;
            }
        }
        
        return minVal;
    }
    
    // Maximum der letzten N Werte
    T max(size_t n = 0) const {
        if (count == 0) return T();
        
        size_t maxCount = (n == 0 || n > count) ? count : n;
        T maxVal = get(0);
        
        for (size_t i = 1; i < maxCount; i++) {
            T val = get(i);
            if (val > maxVal) {
                maxVal = val;
            }
        }
        
        return maxVal;
    }
};

// Datenstruktur für Metriken
struct MetricData {
    uint32_t timestamp;
    float value;
    uint8_t serviceId;
    
    MetricData() : timestamp(0), value(0.0f), serviceId(0) {}
    MetricData(uint32_t ts, float v, uint8_t sid) 
        : timestamp(ts), value(v), serviceId(sid) {}
};

// Spezialisierte Ring Buffer für verschiedene Datentypen
using FloatBuffer = RingBuffer<float>;
using MetricBuffer = RingBuffer<MetricData>;

#endif // RING_BUFFER_H
