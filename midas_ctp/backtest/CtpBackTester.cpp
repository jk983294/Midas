#include "CtpBackTester.h"

CtpBackTester::CtpBackTester(int argc, char** argv) : MidasProcessBase(argc, argv) { init_admin(); }

CtpBackTester::~CtpBackTester() {}

void CtpBackTester::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }
}

void CtpBackTester::app_stop() { MidasProcessBase::app_stop(); }
