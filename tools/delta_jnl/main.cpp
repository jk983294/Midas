#include <io/JournalManager.h>
#include <signal.h>
#include <utils/ConvertHelper.h>
#include <utils/convert/TimeHelper.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "TcpInput.h"
#include "TcpListener.h"
#include "TcpOutput.h"

using namespace std;
using namespace boost::filesystem;
namespace po = boost::program_options;

static bool goodNight{false};

static void shut_down(int sigNum) {
    goodNight = true;
    signal(sigNum, shut_down);
}

int main(int argc, const char** argv) {
    string dataPort, inputStream, journalDirectory;
    bool isZip{true};
    bool noJournaling{false};
    int mode{0}, outputBufferSize{20000000};
    po::options_description desc("Program options");
    desc.add_options()("help,h", "print_trading_hour help")
            ("directory,d", po::value<string>(&journalDirectory)->default_value("."), "directory to store")
            ("port,p", po::value<string>(&dataPort)->default_value(""), "data port")
            ("buffer,b", po::value<int>(&outputBufferSize)->default_value(20000000), "output buffer")
            ("mode,F", po::value<int>(&mode)->default_value(0), "F 0 pass through")
            ("zipped,z", po::value<bool>(&isZip)->default_value(true), "true zip false plain file")
            ("no_journaling,x", po::value<bool>(&noJournaling)->default_value(false), "true no journaling false journaling")
            ("input_stream,i", po::value<string>(&inputStream)->default_value(""), "input stream (host:port,host:port)");

    po::variables_map vm;
    auto parsed = po::parse_command_line(argc, argv, desc);
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    bool noDelay = true;
    TcpListener* listener = nullptr;
    if (!dataPort.empty()) {
        listener = new TcpListener(dataPort, noDelay);
        if (listener->isDeaf) {
            cerr << "listen error: " << dataPort << "\n";
            return 1;
        }
    } else {
        cerr << "no data port provided, make sure this is not pass through purpose.\n";
    }

    multimap<pair<string, string>, time_t> hatchery;  // pending upstream input
    set<pair<string, string>> dumpster;
    map<int, TcpInput*> inputs;
    map<int, TcpOutput*> outputs;

    string iface;

    // upstream address
    {
        timeval now;
        gettimeofday(&now, 0);
        vector<string> upstreamStrings = midas::string_split(inputStream);
        for (const auto& upstreamString : upstreamStrings) {
            vector<string> hostPort = midas::string_split(upstreamString, ":");
            string host = hostPort[0];
            string port = hostPort[1];
            if (host.empty() || host == "0") {
                host = "localhost";
            }
            hatchery.insert({{host, port}, now.tv_sec});
        }
    }

    if (signal(SIGINT, shut_down) == SIG_ERR) {
        cerr << "cannot catch SIGINT\n";
        return 1;
    }

    if (signal(SIGTERM, shut_down) == SIG_ERR) {
        cerr << "cannot catch SIGTERM\n";
        return 1;
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        cerr << "cannot ignore SIGPIPE\n";
        return 1;
    }

    if (signal(SIGUSR1, SIG_IGN) == SIG_ERR) {
        cerr << "cannot ignore SIGUSR1\n";
        return 1;
    }

    if (signal(SIGUSR2, SIG_IGN) == SIG_ERR) {
        cerr << "cannot catch SIGUSR2\n";
        return 1;
    }

    bool going2sleep = false;

    size_t maxBytesBehind{100000000}, maxJournalSize{100000000}, maxMsgLen{80000};
    long maxJournalSecs{86400};
    int freeBytes{0};
    const int MinBytesFree{100000000};
    size_t bufLen{maxBytesBehind + MinBytesFree};
    char* bufPtr = new char[bufLen + maxMsgLen];
    long long totalBytes{0};  // bytes received
    long long totalMsgs{0};   // msg received
    midas::Journal* journal{nullptr};

    if (!noJournaling) {
        if (isZip)
            journal = new midas::GzJournal(journalDirectory, ".gz", 0, cerr);
        else
            journal = new midas::FJournal(journalDirectory, "", 0, cerr);
    }

    timeval idleTime;
    gettimeofday(&idleTime, 0);

    cout << "initialization complete\n";

    timeval currentTime;

    for (gettimeofday(&currentTime, 0); true; gettimeofday(&currentTime, 0)) {
        if (going2sleep ? false : goodNight) {
            cerr << "shutdown in progress\n";

            // no more tcp input
            for (auto itr = inputs.begin(); itr != inputs.end(); ++itr) {
                const int fd{itr->first};
                TcpInput* tcpInput = itr->second;
                close(fd);
                delete tcpInput;
            }
            inputs.clear();
            hatchery.clear();
            dumpster.clear();

            // no new clients
            delete listener;
            listener = nullptr;
            going2sleep = true;
        }

        // make sure we have enough free bytes
        if (freeBytes < MinBytesFree) {
            freeBytes = static_cast<int>(bufLen);
            if (journal) {
                long freeJournalBytes = journal->free_bytes(static_cast<int>(bufLen), totalBytes);
                if (MinBytesFree > freeJournalBytes) {
                    long bytes = journal->pending_bytes(totalBytes);
                    cerr << "journal data lost\n" << journal->path << " dropped " << bytes << " bytes\n";
                    journal->straggle(totalBytes);
                } else {
                    if (freeBytes > freeJournalBytes) freeBytes = static_cast<int>(freeJournalBytes);
                }
            }

            set<int> erasures;
            for (auto itr = outputs.begin(); itr != outputs.end(); ++itr) {
                int fd = itr->first;
                TcpOutput* tcpOutput = itr->second;
                long freeOutputBytes = tcpOutput->freeBytes(static_cast<int>(bufLen), totalBytes);
                if (MinBytesFree > freeOutputBytes) {
                    const string address{tcpOutput->address};
                    close(fd);
                    delete tcpOutput;
                    erasures.insert(fd);
                    cerr << address << " has been straggled\n";
                } else {
                    if (freeBytes > freeOutputBytes) freeBytes = static_cast<int>(freeOutputBytes);
                }
            }

            for (auto itr = erasures.begin(); itr != erasures.end(); ++itr) {
                outputs.erase(*itr);
            }
        }

        // copy input data to output buffer
        {
            set<int> erasures;
            for (auto itr = inputs.begin(); itr != inputs.end(); ++itr) {
                int fd = itr->first;
                TcpInput* tcpInput = itr->second;
                while (true) {
                    if (TcpInput::maxBlockLen() > freeBytes) break;  // no enough space in buffer

                    long dataOff = totalBytes % static_cast<long long>(bufLen);
                    char* dataPtr = &bufPtr[dataOff];
                    long cc = tcpInput->getBlock(dataPtr, currentTime);
                    const bool dead = (cc < 0 || tcpInput->isBroken());
                    const bool sick = (tcpInput->isTrying() && currentTime.tv_sec > tcpInput->birth.tv_sec + 200);

                    if (dead || sick) {
                        const string host{tcpInput->host};
                        const string port{tcpInput->port};
                        if (dead) cerr << "disconnect from " << host << ":" << port << "\n";
                        if (sick) cerr << "connect timeout " << host << ":" << port << "\n";

                        close(fd);
                        delete tcpInput;
                        erasures.insert(fd);
                        timeval now;
                        gettimeofday(&now, 0);
                        hatchery.insert({{host, port}, now.tv_sec + 15});
                        break;
                    }

                    if (!cc) break;

                    totalBytes += cc;
                    totalMsgs++;
                    freeBytes -= cc;

                    char* dataEnd = &dataPtr[cc];
                    char* bufEnd = &bufPtr[bufLen];
                    if (dataEnd > bufEnd)  // recover overflow data to the begin of buffer
                        memcpy(bufPtr, bufEnd, dataEnd - bufEnd);
                }
            }

            for (auto itr = erasures.begin(); itr != erasures.end(); ++itr) {
                inputs.erase(*itr);
            }
        }

        // compute highest fd and raise select flags
        int fdMax = -1;
        fd_set fdRead;
        fd_set fdWrite;
        {
            FD_ZERO(&fdRead);
            FD_ZERO(&fdWrite);
            if (listener) {
                fdMax = listener->fd;
                FD_SET(listener->fd, &fdRead);
            }

            for (auto itr = outputs.begin(); itr != outputs.end(); ++itr) {
                int fd = itr->first;
                TcpOutput* tcpOutput = itr->second;
                FD_SET(fd, &fdRead);
                if (tcpOutput->pendingBytes(totalBytes) > 0) {
                    FD_SET(fd, &fdWrite);
                }

                if (fdMax < fd) fdMax = fd;
            }

            {
                for (auto itr = inputs.begin(); itr != inputs.end(); ++itr) {
                    int fd = itr->first;
                    TcpInput* tcpInput = itr->second;
                    if (tcpInput->isActive()) {
                        FD_SET(fd, &fdRead);
                        if (fdMax < fd) fdMax = fd;
                    } else if (tcpInput->isTrying()) {
                        FD_SET(fd, &fdWrite);
                        if (fdMax < fd) fdMax = fd;
                    }
                }
            }
        }

        // there is a good journal with pending bytes
        bool journalReady = journal ? (journal->bad() ? false : journal->pending_bytes(totalBytes) > 0) : false;

        // time to check low  priority stuff
        bool lowPriority = currentTime.tv_sec > idleTime.tv_sec;

        // no delay if journal is ready or it is time to check low priority
        static timeval delay;
        delay.tv_sec = (journalReady || lowPriority) ? 0 : 1;
        delay.tv_usec = 0;

        int fdCount = select(fdMax + 1, &fdRead, &fdWrite, 0, &delay);
        if (fdCount <= 0) {
            if (fdCount < 0) {
                cerr << "select fails: " << strerror(errno) << "\n";
            }

            // no fd ready
            if (journalReady) {
                journal->scribble(bufPtr, static_cast<int>(bufLen), totalBytes, cerr);

                // still room for jnl, then no need to create new jnl file
                if (journal->bytes() < static_cast<int>(maxJournalSize)) continue;
            }

            if (!lowPriority) continue;

            // low priority stuff, we do every second
            gettimeofday(&idleTime, 0);

            // journal rollover due to size, time, cut, bad for 3 secs
            if (journal) {
                time_t secs = idleTime.tv_sec - journal->birth.tv_sec;
                if ((journal->bad() && secs > 3) || journal->bytes() > static_cast<int>(maxJournalSize) ||
                    secs > maxJournalSecs || journal->cut) {
                    long long bytes = journal->totalBytes;
                    delete journal;

                    if (isZip)
                        journal = new midas::GzJournal(journalDirectory, ".gz", bytes, cerr);
                    else
                        journal = new midas::FJournal(journalDirectory, "", bytes, cerr);
                }
            }

            // background input
            for (auto itr = inputs.begin(); itr != inputs.end(); ++itr) {
                TcpInput* tcpInput = itr->second;
                tcpInput->occasional(currentTime);
            }

            // check pending input
            multiset<pair<string, string>> erasures;
            for (auto itr = hatchery.begin(); itr != hatchery.end(); ++itr) {
                const pair<string, string> addr = itr->first;
                const string host = addr.first;
                const string port = addr.second;
                time_t epoch = itr->second;
                if (time_t(idleTime.tv_sec) < epoch) continue;
                timeval now;
                gettimeofday(&now, 0);
                int fd = TcpInput::socket_fd();
                cout << "connect to " << host << ":" << port << "\n";
                if (mode == 0) {
                    inputs[fd] = new DeltaPass(fd, host, port, iface, noDelay, now);
                }
                erasures.insert(addr);
            }

            for (auto itr = erasures.begin(); itr != erasures.end(); ++itr) {
                const pair<string, string> addr = *itr;
                auto f = hatchery.find(addr);
                if (f != hatchery.end()) hatchery.erase(addr);
            }

            // check condemned inputs
            for (auto itr = dumpster.begin(); itr != dumpster.end(); ++itr) {
                hatchery.erase(*itr);  // kill pending server

                const pair<string, string> addr = *itr;
                const string host = addr.first;
                const string port = addr.second;

                set<int> erasureFds;

                for (auto s = inputs.begin(); s != inputs.end(); ++s) {
                    TcpInput* tcpInput = s->second;
                    if (tcpInput->host == host && tcpInput->port == port) {
                        // kill connected server
                        int fd = s->first;
                        erasureFds.insert(fd);
                        close(fd);
                        delete tcpInput;
                    }
                }

                for (auto e = erasureFds.begin(); e != erasureFds.end(); ++e) {
                    inputs.erase(*e);
                }
            }
            dumpster.clear();

            if (going2sleep) {
                bool isPending{false};
                for (auto itr = outputs.begin(); itr != outputs.end(); ++itr) {
                    TcpOutput* tcpOutput = itr->second;
                    if (tcpOutput->pendingBytes(totalBytes)) {
                        cerr << tcpOutput->address << " is pending\n";
                        isPending = true;
                    }
                }

                if (isPending) continue;

                for (auto itr = outputs.begin(); itr != outputs.end(); ++itr) {
                    int fd = itr->first;
                    TcpOutput* tcpOutput = itr->second;
                    cerr << tcpOutput->address << " disconnecting\n";
                    TcpOutput::gentleClose(fd);
                    delete tcpOutput;
                }

                delete journal;
                delete[] bufPtr;

                cerr << "shutdown complete\n";
                return 0;
            }
            continue;
        }

        // select driven I/O (highest priority)
        {
            // output
            set<int> erasures;
            for (auto itr = outputs.begin(); itr != outputs.end(); ++itr) {
                int fd = itr->first;
                TcpOutput* tcpOutput = itr->second;
                const string address{tcpOutput->address};
                if (FD_ISSET(fd, &fdRead)) {  // ready for read
                    if (!tcpOutput->input(fd)) {
                        close(fd);
                        delete tcpOutput;
                        erasures.insert(fd);
                        continue;
                    }
                }
                if (FD_ISSET(fd, &fdWrite)) {  // ready for write
                    if (!tcpOutput->output(fd, bufPtr, static_cast<int>(bufLen), totalBytes)) {
                        close(fd);
                        delete tcpOutput;
                        erasures.insert(fd);
                    }
                }
            }

            for (auto e = erasures.begin(); e != erasures.end(); ++e) {
                outputs.erase(*e);
            }
        }

        {
            // input
            for (auto itr = inputs.begin(); itr != inputs.end(); ++itr) {
                int fd = itr->first;
                TcpInput* tcpInput = itr->second;
                if (FD_ISSET(fd, &fdRead)) {  // ready for read
                    tcpInput->serviceRead(fd);
                } else if (FD_ISSET(fd, &fdWrite)) {  // ready for write
                    tcpInput->connectDone(fd);
                }
            }
        }

        // is there new clients
        if (listener) {
            if (FD_ISSET(listener->fd, &fdRead)) {
                int fd;
                string address;
                if (listener->admit(fd, address)) {
                    timeval now;
                    gettimeofday(&now, 0);
                    outputs[fd] = new TcpOutput(fd, address, totalBytes, now);
                    cout << "welcome: " << address << "\n";
                }
            }
        }
    }
    return 0;
}
