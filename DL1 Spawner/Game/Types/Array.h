#pragma once

template <typename T>
struct Array {
    T* Data{};
    uint32_t Count{};

    T& operator[](uint32_t Index) {
        assert(Index < Count);
        return Data[Index];
    }

    const T& operator[](uint32_t Index) const {
        assert(Index < Count);
        return Data[Index];
    }

    T* begin() const { return Data; }
    T* end() const { return Data + Count; }

    explicit operator bool() const { return Data && Count; }
};