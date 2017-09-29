#include <fcntl.h>
#include <io/Journal.h>
#include <signal.h>
#include <boost/program_options.hpp>

using namespace std;
using namespace midas;
namespace po = boost::program_options;

static bool goodbye{false};

static void shutdown(int signalNumber) {
    goodbye = true;
    signal(signalNumber, shutdown);
}

int main(int argc, char* argv[]) {
    po::options_description desc("Program options");
    desc.add_options()("help,h", "print help")("directory,d", po::value<string>()->default_value("."),
                                               "journaling directory")(
        "address,i", po::value<string>()->default_value("localhost"), "source address")(
        "port,n", po::value<string>()->default_value(""), "source port")(
        "no_journaling,x", po::value<bool>()->implicit_value(false), "no journaling")(
        "zip,z", po::value<string>()->default_value("on"), "zip on or off");

    po::variables_map vm;
    auto parsed = po::parse_command_line(argc, argv, desc);
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return 1;
    }

    string journalDirectory = vm["directory"].as<string>();
    string address = vm["address"].as<string>();
    string port = vm["port"].as<string>();
    bool isJournal = vm["no_journaling"].empty();
    bool isZip = (vm["zip"].as<string>() == "on");
    cout << "journal directory: " << journalDirectory << endl
         << "address: " << address << endl
         << "port: " << port << endl
         << "is journal: " << isJournal << endl
         << "is zip: " << isZip << endl;

    if (signal(SIGINT, shutdown) == SIG_ERR) {
        cerr << "cannot catch SIGINT\n";
        return 1;
    }
    if (signal(SIGTERM, shutdown) == SIG_ERR) {
        cerr << "cannot catch SIGINT\n";
        return 1;
    }
    if (signal(SIGPIPE, shutdown) == SIG_ERR) {
        cerr << "cannot catch SIGINT\n";
        return 1;
    }
    if (signal(SIGUSR1, shutdown) == SIG_ERR) {
        cerr << "cannot catch SIGINT\n";
        return 1;
    }
    if (signal(SIGUSR2, shutdown) == SIG_ERR) {
        cerr << "cannot catch SIGINT\n";
        return 1;
    }

    size_t maxBytesBehind{20000000}, maxJournalSize{20000000}, maxMsgLen{80000};
    long maxJournalSecs{86400};
    int freeBytes{0};
    const int MinBytesFree{20000000};
    size_t bufLen{maxBytesBehind + MinBytesFree};
    char* bufPtr = new char[bufLen + maxMsgLen];
    long long totalBytes{0};  // bytes received
    long long totalMsgs{0};   // msg received

    string msg{"hello world, this is test journal msg.\n"};

    Journal* journal{nullptr};

    if (isZip)
        journal = new GzJournal(journalDirectory, ".gz", 0, cerr);
    else
        journal = new FJournal(journalDirectory, "", 0, cerr);

    timeval currentTime, idleTime;
    while (1) {
        if (goodbye) break;

        sleep(1);

        gettimeofday(&currentTime, 0);

        // make sure we have enough free bytes
        if (freeBytes < MinBytesFree) {
            freeBytes = bufLen;
            if (journal) {
                long freeJournalBytes = journal->free_bytes(bufLen, totalBytes);
                if (MinBytesFree > freeJournalBytes) {
                    long bytes = journal->pending_bytes(totalBytes);
                    cerr << "journal data lost\n" << journal->path << " dropped " << bytes << " bytes\n";
                } else {
                    if (freeBytes > freeJournalBytes) freeBytes = static_cast<int>(freeJournalBytes);
                }
            }
        }

        for (int i = 0; i < 1000; ++i) {
            if (maxMsgLen > static_cast<size_t>(freeBytes)) break;

            // record msg
            long dataOff = totalBytes % bufLen;
            char* dataPtr = &bufPtr[dataOff];

            size_t cc = msg.size();
            memcpy(dataPtr, msg.c_str(), cc);

            totalBytes += cc;
            totalMsgs++;
            freeBytes -= cc;

            char* dataEnd = &dataPtr[cc];
            char* bufEnd = &bufPtr[bufLen];
            if (dataEnd > bufEnd)  // recover overflow data to the begin of buffer
                memcpy(bufPtr, bufEnd, dataEnd - bufEnd);
        }

        // there is a good journal with pending bytes
        bool journalReady = journal ? (journal->bad() ? false : journal->pending_bytes(totalBytes) > 0) : false;

        if (journalReady) {
            journal->scribble(bufPtr, bufLen, totalBytes, cerr);

            // still room for jnl, then no need to create new jnl file
            if (journal->bytes() < static_cast<int>(maxJournalSize)) continue;
        }

        gettimeofday(&idleTime, 0);

        // journal rollover due to size, time, cut, bad for 3 secs
        if (journal) {
            time_t secs = idleTime.tv_sec - journal->birth.tv_sec;
            if ((journal->bad() && secs > 3) || journal->bytes() > static_cast<int>(maxJournalSize) ||
                secs > maxJournalSecs || journal->cut) {
                long long bytes = journal->totalBytes;
                delete journal;
                journal = nullptr;

                if (isZip)
                    journal = new GzJournal(journalDirectory, ".gz", bytes, cerr);
                else
                    journal = new FJournal(journalDirectory, "", bytes, cerr);
            }
        }
    }

    if (journal) delete journal;
    delete[] bufPtr;
    return 0;
}