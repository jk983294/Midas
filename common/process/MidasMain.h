#ifndef COMMON_PROCESS_MIDASMAIN_H
#define COMMON_PROCESS_MIDASMAIN_H

#include <syscall.h>
#include <csignal>
#include <memory>
#include "utils/log/Log.h"

namespace midas {

template <class Proc>
int MidasMain(int argc, char* argv[]) {
    using ProcPtr = std::unique_ptr<Proc>;
    ProcPtr proc;
    int returnCode = 0;

    {
        char* tz = std::getenv("TZ");
        if (!tz) {
            static char tz1[32] = "TZ=/etc/localtime";
            putenv(tz1);  // update /proc/*/environ
            tz = std::getenv("TZ");
            std::cout << "tz set to " << tz << '\n';
        }
    }

    try {
        proc = std::unique_ptr<Proc>(new Proc(argc, argv));
        proc->init_config(argc, argv);
        proc->start();
        sleep(1);
        proc->run_control_script();
        proc->run();
    } catch (const std::exception& e) {
        MIDAS_LOG_ERROR("caught exception: " << e.what());
        if (proc) proc->stop();
        try {
            midas::Log::instance().get_writer()->flush_log();
        } catch (...) {
        }
        return 1;
    } catch (...) {
        MIDAS_LOG_ERROR("caught unexpected exception\n");
        try {
            midas::Log::instance().get_writer()->flush_log();
        } catch (...) {
        }
        abort();  // generate coredump
    }

    proc.reset();  // release Proc
    MIDAS_LOG_INFO("process normal exit with code " << returnCode);
    try {
        midas::Log::instance().get_writer()->flush_log();
    } catch (...) {
    }
    return returnCode;
}
}

#endif
