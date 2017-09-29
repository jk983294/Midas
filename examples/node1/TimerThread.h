#ifndef MIDAS_EXAMPLE_TIMER_THREAD_H
#define MIDAS_EXAMPLE_TIMER_THREAD_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

using namespace std;

class TimerThread {
public:
    typedef std::function<void()> TCallback;
    uint32_t interval{0};
    bool isPause{true};
    bool isShutdown{false};
    TCallback callback;
    std::thread thread_;
    std::mutex mtx;
    std::condition_variable cv;

public:
    TimerThread() {
        thread_ = std::thread([this] { worker(); });
    }
    ~TimerThread() {
        if (!isShutdown) shutdown();
    }

    void set_callback(const TCallback& cb) {
        if (isShutdown) return;

        std::lock_guard<decltype(mtx)> lock(mtx);
        callback = cb;
        cv.notify_one();
    }
    void set_interval(uint32_t timeout) {
        if (isShutdown) return;

        std::lock_guard<decltype(mtx)> lock(mtx);
        interval = timeout;
        cv.notify_one();
    }

    void pause() {
        if (isShutdown) return;

        std::lock_guard<decltype(mtx)> lock(mtx);
        isPause = true;
        cv.notify_one();
    }
    void resume() {
        if (isShutdown) return;

        std::lock_guard<decltype(mtx)> lock(mtx);
        isPause = false;
        cv.notify_one();
    }
    void shutdown() {
        if (isShutdown) return;

        std::unique_lock<decltype(mtx)> lock(mtx);
        isShutdown = true;
        cv.notify_one();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void worker() {
        time_t now, last;
        while (!isShutdown) {
            std::unique_lock<decltype(mtx)> lock(mtx);
            time(&last);

            if (callback && !isPause && interval) {
                cv.wait_for(lock, std::chrono::seconds(interval));
            } else {
                cv.wait(lock);
            }

            if (isShutdown) break;

            if (isPause || !interval || !callback) {
                continue;
            }

            time(&now);

            if (last + interval < now) {
                continue;
            }

            callback();
        }
    }
};

#endif
