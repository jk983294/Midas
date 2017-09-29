#ifndef MIDAS_CHANNEL_PUBLISHER_BASE_H
#define MIDAS_CHANNEL_PUBLISHER_BASE_H

#include "net/common/Member.h"

using namespace std;

namespace midas {

class ChannelPublisherBase : public Member {
public:
    typedef boost::function<void(boost::function<void(void)>&)> TPostFunction;
    typedef boost::intrusive_ptr<ChannelPublisherBase> SharedPtr;

    TPostFunction postFn;  // set by call to register_post
public:
    ChannelPublisherBase() {}
    virtual ~ChannelPublisherBase() {}
    virtual void start() {}
    virtual void stop() {}

    template <typename Module>
    void register_post(Module module) {
        postFn = boost::bind(&Module::element::post_task, module.get(), _1);
    }

    // Module must define this function
    virtual void set_callback_func(boost::function<void(ConstBuffer, bool)>& f) {}

    virtual string get_status() {
        ostringstream os;
        Member::stats(os);
        return os.str();
    }
};

class AsyncPublisherBase {
public:
    std::atomic<bool> isCompleted{true};

public:
    bool task_complete() { return isCompleted; }
};
}

#endif
