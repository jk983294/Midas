#ifndef MIDAS_MY_PRODUCER_H
#define MIDAS_MY_PRODUCER_H

static vector<MyData> data{{0}, {1}, {2}, {3}, {4}};

class MyProducer {
public:
    typedef std::shared_ptr<MyProducer> SharedPtr;

    std::atomic<bool> isStart;

    /**
     * dataCallback called by producer to enqueue disruptor ring buffer
     */
    std::function<size_t(const MyData&, uint64_t, int64_t)> dataCallback;
    std::string name;
    std::shared_ptr<boost::thread> threadPtr;

public:
    MyProducer(const std::string& name, const uint32_t iterations)
        : isStart(false), name(name), threadPtr(new boost::thread(&MyProducer::run, boost::ref(*this), iterations)) {}

    void register_data_callback(const std::function<size_t(const MyData&, uint64_t, int64_t)>& cb) {
        dataCallback = cb;
    }

    void start() { isStart = true; }

    void run(const uint32_t iterations) {
        while (!isStart) {
        }

        size_t size = data.size();
        for (uint32_t i = 0; i < iterations; ++i) {
            dataCallback(data[i % size], 0, -1);
        }
    }
};

#endif
