#ifndef MIDAS_DISRUPTOR_H
#define MIDAS_DISRUPTOR_H

#include <midas/MidasConfig.h>
#include <midas/MidasException.h>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include "BatchConsumerStrategy.h"
#include "DisruptorImpl.h"
#include "SequentialConsumerStrategy.h"

using namespace std;

namespace midas {

template <class Producer, class Consumer, class Payload, ConsumerStages Stages,
          class ConsumerStrategy = SequentialConsumerStrategy<> >
class Disruptor {
public:
    typedef boost::shared_ptr<Disruptor> SharedPtr;
    typedef typename ConsumerStrategy::TPostCallback TPostCallback;
    typedef typename ConsumerStrategy::TPostQueue TPostQueue;

    typename DisruptorImplBase<ConsumerStrategy>::SharedPtr disruptorPtr;
    std::string waitStrategy;

public:
    Disruptor(const std::string& name_, const std::string& cfgPath,
              std::vector<typename Producer::SharedPtr>& producers, Consumer& dataConsumer) {
        midas::Config& cfg = midas::Config::instance();
        const uint32_t maxMsgSize = cfg.get<uint32_t>(cfgPath + ".max_msg_size", 4096);
        const uint32_t ringSizeExponent = cfg.get<uint32_t>(cfgPath + ".ring_size_exponent", 10);
        waitStrategy = cfg.get<std::string>(cfgPath + ".wait_strategy", "");

        if (producers.size() > 1) {
            if (waitStrategy == BusySpinTag)
                disruptorPtr = boost::make_shared<TMultiBusySpinDisruptor>(name_, maxMsgSize, ringSizeExponent,
                                                                           producers, dataConsumer);
            else if (waitStrategy == YieldTag)
                disruptorPtr = boost::make_shared<TMultiYieldDisruptor>(name_, maxMsgSize, ringSizeExponent, producers,
                                                                        dataConsumer);
            else if (waitStrategy == BlockTag)
                disruptorPtr = boost::make_shared<TMultiBlockDisruptor>(name_, maxMsgSize, ringSizeExponent, producers,
                                                                        dataConsumer);
            else
                THROW_MIDAS_EXCEPTION("Disruptor=" << name_ << " unknown wait_strategy=" << waitStrategy);
        } else {
            if (waitStrategy == BusySpinTag)
                disruptorPtr = boost::make_shared<TSingleBusySpinDisruptor>(name_, maxMsgSize, ringSizeExponent,
                                                                            producers, dataConsumer);
            else if (waitStrategy == YieldTag)
                disruptorPtr = boost::make_shared<TSingleYieldDisruptor>(name_, maxMsgSize, ringSizeExponent, producers,
                                                                         dataConsumer);
            else if (waitStrategy == BlockTag)
                disruptorPtr = boost::make_shared<TSingleBlockDisruptor>(name_, maxMsgSize, ringSizeExponent, producers,
                                                                         dataConsumer);
            else
                THROW_MIDAS_EXCEPTION("Disruptor=" << name_ << " unknown wait_strategy=" << waitStrategy);
        }
    }

    std::string name() const {
        if (disruptorPtr)
            return disruptorPtr->name();
        else
            return "UNKNOWN";
    }

    void post(const TPostCallback& cb) {
        if (disruptorPtr)
            disruptorPtr->post(cb);
        else
            MIDAS_LOG_ERROR("Unknown Disruptor to post to");
    }

    void stop() {
        if (disruptorPtr)
            disruptorPtr->stop();
        else
            MIDAS_LOG_ERROR("Unknown Disruptor to stop");
    }

    std::string wait_strategy() { return waitStrategy; }

    void stats(ostream& os) { disruptorPtr->stats(os); }

    static const std::string BusySpinTag;
    static const std::string YieldTag;
    static const std::string BlockTag;

private:
    typedef midas::DisruptorImpl<Producer, Consumer, Payload, Stages, MultiThreadedStrategy, BusySpinWaitStrategy,
                                 ConsumerStrategy>
        TMultiBusySpinDisruptor;

    typedef midas::DisruptorImpl<Producer, Consumer, Payload, Stages, MultiThreadedStrategy, YieldWaitStrategy,
                                 ConsumerStrategy>
        TMultiYieldDisruptor;

    typedef midas::DisruptorImpl<Producer, Consumer, Payload, Stages, MultiThreadedStrategy, BlockWaitStrategy,
                                 ConsumerStrategy>
        TMultiBlockDisruptor;

    typedef midas::DisruptorImpl<Producer, Consumer, Payload, Stages, SingleThreadedStrategy, BusySpinWaitStrategy,
                                 ConsumerStrategy>
        TSingleBusySpinDisruptor;

    typedef midas::DisruptorImpl<Producer, Consumer, Payload, Stages, SingleThreadedStrategy, YieldWaitStrategy,
                                 ConsumerStrategy>
        TSingleYieldDisruptor;

    typedef midas::DisruptorImpl<Producer, Consumer, Payload, Stages, SingleThreadedStrategy, BlockWaitStrategy,
                                 ConsumerStrategy>
        TSingleBlockDisruptor;
};

template <class Producer, class Consumer, class Payload, ConsumerStages Stages, class ConsumerStrategy>
const std::string Disruptor<Producer, Consumer, Payload, Stages, ConsumerStrategy>::BusySpinTag{"busy_spin"};

template <class Producer, class Consumer, class Payload, ConsumerStages Stages, class ConsumerStrategy>
const std::string Disruptor<Producer, Consumer, Payload, Stages, ConsumerStrategy>::YieldTag{"yield"};

template <class Producer, class Consumer, class Payload, ConsumerStages Stages, class ConsumerStrategy>
const std::string Disruptor<Producer, Consumer, Payload, Stages, ConsumerStrategy>::BlockTag{"block"};
}

#endif
