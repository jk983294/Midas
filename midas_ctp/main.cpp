#include "core/CtpProcess.h"
#include "process/MidasMain.h"
#include "utils/log/Log.h"

int main(int argc, char *argv[]) { return midas::MidasMain<CtpProcess>(argc, argv); }
