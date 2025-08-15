#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <iomanip>

#ifdef DEBUG
#include <vector>
#endif

template <size_t Capacity>
class Arena {
#ifdef DEBUG
private:
    struct AllocationInfo {
        const void* address;
        size_t size;
        const char* type_name;
    };
#endif
public:
    // Constructors
#ifdef DEBUG
    Arena() noexcept : m_used(0) { m_allocations.reserve(128); };
#else
    Arena() noexcept : m_used(0) {};
#endif
    ~Arena() = default;

    // Prevent copying and moving arena
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    // Class methods
    template <typename T, typename... Args>
    inline T* allocate(Args&&... args) {
        const size_t alignment   = alignof(T);
        const uintptr_t cur_addr = reinterpret_cast<uintptr_t>(m_buffer) + m_used;
        const size_t padding     = (alignment - (cur_addr % alignment)) % alignment;

        if (m_used + padding + sizeof(T) > Capacity)
            return nullptr;

        m_used += padding;
        void* memory = &m_buffer[m_used];
        m_used += sizeof(T);

#ifdef DEBUG
        m_allocations.push_back({memory, sizeof(T), typeid(T).name()});
#endif

        // Use placement new to construct the object in-place.
        return new (memory) T(std::forward<Args>(args)...);
    }

    inline void reset() noexcept {
        m_used = 0ULL;
    }

    // Visual utils
    void display_map() const noexcept {
        uintptr_t last_offset = 0;
        std::cout << std::left;
        std::cout << "--- Arena Memory Map (compile with -DDEBUG) ---\n";
        std::cout << "Capacity: " << Capacity << " bytes | Used: " << m_used << " bytes\n\n";
        std::cout << std::setw(18) << "Address (Offset)"
                  << std::setw(25) << "Type"
                  << std::setw(10) << "Size" << "\n";
        std::cout << "--------------------------------------------------\n";

#ifdef DEBUG
        for (const AllocationInfo& alloc : m_allocations) {
            uintptr_t current_addr      = reinterpret_cast<uintptr_t>(alloc.address);
            uintptr_t buffer_start_addr = reinterpret_cast<uintptr_t>(m_buffer);
            uintptr_t current_offset    = current_addr - buffer_start_addr;

            // Display any padding that occurred before this allocation
            if (current_offset > last_offset) {
                size_t padding = current_offset - last_offset;
                std::cout << std::setw(18) << ("+ " + std::to_string(last_offset))
                          << std::setw(25) << "(Padding)"
                          << std::setw(10) << padding << "\n";
            }

            // Display the allocation itself
            std::cout << std::setw(18) << ("+ " + std::to_string(current_offset))
                      << std::setw(25) << alloc.type_name
                      << std::setw(10) << alloc.size << "\n";
            
            last_offset = current_offset + alloc.size;
        }
#endif

        // Show remaining free space
        if (m_used < Capacity) {
             std::cout << std::setw(18) << ("+ " + std::to_string(m_used))
                       << std::setw(25) << "(Free Space)"
                       << std::setw(10) << (Capacity - m_used) << "\n";
        }
        std::cout << "--------------------------------------------------\n\n";
    }

    inline void memstat() const noexcept {
        double used         = static_cast<double>(m_used);
        double capacity     = static_cast<double>(Capacity);
        double percent_used = (used / capacity) * 100.0;

        std::cout << "----------- Memory Stats -----------\n Used: " << used << "\n Capacity: " << capacity
                  << "\n Usage:     " << std::fixed << std::setprecision(2) << percent_used << "%\n";
        
        // Display a simple visual usage bar (20 characters wide)
        unsigned int bar_width    = 20;
        unsigned int filled_width = static_cast<int>((used / capacity) * bar_width);
        unsigned int empty_width  = bar_width - filled_width;

        std::cout << " Visual:    [";
        for (unsigned int i = 0; i < filled_width; ++i) std::cout << '-';
        for (unsigned int i = 0; i < empty_width; ++i) std::cout << ' ';
        std::cout << "]\n";
        std::cout << "------------------------------------\n\n";
    }

private:
    alignas(max_align_t) std::byte m_buffer[Capacity];
    size_t m_used;

#ifdef DEBUG
    std::vector<AllocationInfo> m_allocations;
#endif 
};

#endif // ARENA_ALLOCATOR_H