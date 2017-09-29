#ifndef COMMON_MIDAS_SINGLETON_H
#define COMMON_MIDAS_SINGLETON_H

namespace midas {

template <class T>
class Singleton {
public:
    // lazy initialization, C++11 does guarantee that this is thread-safe
    static T &instance() {
        static T *instance = new T;
        // return reference to removes temptation to try and delete the returned instance.
        return *instance;
    }

private:
    Singleton() {}

    // this prevents accidental copying of the only instance of the class.
    explicit Singleton(const Singleton &old) = delete;
    explicit Singleton(Singleton &old) = delete;
    Singleton &operator=(const Singleton &old) = delete;
    Singleton &operator=(Singleton &old) = delete;

    // This prevents others from deleting our one single instance, which was otherwise created on the heap
    ~Singleton() {}
};
}

#endif
