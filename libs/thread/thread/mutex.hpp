#pragma once
#include <atomic>

namespace crossbow
{

class busy_mutex
{
    friend class mutex;
    std::atomic<bool> _m;
public:
    busy_mutex()
        : _m(true)
    {}

    busy_mutex(const busy_mutex&) = delete;

    busy_mutex& operator= (const busy_mutex&) = delete;
public:
    inline void lock()
    {
        bool val = _m.load();
        while (true) {
            if (val) {
                if (_m.compare_exchange_strong(val, false))
                    return;
            }
        }
    }

    inline bool try_lock()
    {
        bool val = _m.load();
        if (val) {
            if (_m.compare_exchange_strong(val, false))
                return true;
        }
        return false;
    }

    inline void unlock()
    {
        _m.store(true);
    }
};

class mutex
{
    std::atomic<bool> _m;
public:
    mutex() {}

    mutex(const mutex&) = delete;

    mutex& operator= (const mutex&) = delete;
public:
    void lock();

    bool try_lock();

    void unlock();
};

}