#pragma once

#include <crossbow/non_copyable.hpp>

#include <boost/context/fcontext.hpp>
#include <boost/version.hpp>

#include <cstddef>
#include <functional>
#include <new>
#include <queue>
#include <utility>

namespace crossbow {
namespace infinio {

class InfinibandProcessor;

class Fiber final : crossbow::non_copyable, crossbow::non_movable {
public:
    static constexpr size_t STACK_SIZE = 0x800000;

    template <typename... Args>
    static Fiber* create(Args&&... args) {
        return new Fiber(std::forward<Args>(args)...);
    }

    void* operator new(size_t size);

    void* operator new[](size_t size) = delete;

    void operator delete(void* ptr);

    void operator delete[](void* ptr) = delete;

    bool empty() const {
        return !mFun;
    }

    /**
     * @brief Interrupts execution of the fiber and schedules it for later reexecution
     *
     * Must only be called from within the fiber.
     */
    void yield();

    /**
     * @brief Interrupts the execution of the fiber without rescheduling it
     *
     * Must only be called from within the fiber.
     */
    void wait();

    /**
     * @brief Immediately resumes execution of the interrupted fiber
     *
     * Must only be called from within the associated polling thread.
     */
    void resume();

    /**
     * @brief Schedules a fiber for execution in its associated polling thread
     *
     * Must only be called on a interrupted fiber from outside the polling thread.
     */
    void unblock();

    void execute(std::function<void(Fiber&)> fun);

private:
    static void entry(intptr_t ptr);

    Fiber(InfinibandProcessor& processor);

    void start();

    InfinibandProcessor& mProcessor;

    std::function<void(Fiber&)> mFun;

#if BOOST_VERSION >= 105600
    boost::context::fcontext_t mContext;
#else
    boost::context::fcontext_t* mContext;
#endif

    boost::context::fcontext_t mReturnContext;
};

class ConditionVariable {
public:
    ~ConditionVariable();

    void wait(Fiber& fiber);

    template <typename Predicate>
    void wait(Fiber& fiber, Predicate pred);

    void notify_one();

    void notify_all();

private:
    std::queue<Fiber*> mWaiting;
};

template <typename Predicate>
void ConditionVariable::wait(Fiber& fiber, Predicate pred) {
    while (!pred()) {
        wait(fiber);
    }
}

} // namespace infinio
} // namespace crossbow
