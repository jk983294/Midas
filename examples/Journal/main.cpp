#include <io/Journal.h>
#include <io/JournalManager.h>
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
    desc.add_options()("help,h", "print_trading_hour help")
            ("directory,d", po::value<string>()->default_value("."), "journaling directory")
            ("address,i", po::value<string>()->default_value("localhost"), "source address")
            ("port,n", po::value<string>()->default_value(""), "source port")
            ("no_journaling,x", po::value<bool>()->implicit_value(false), "no journaling")
            ("zip,z", po::value<string>()->default_value("on"), "zip on or off");

    po::variables_map vm;
    auto parsed = po::parse_command_line(argc, argv, desc);
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << '\n';
        return 1;
    }

    string journalDirectory = vm["directory"].as<string>();
    string address = vm["address"].as<string>();
    string port = vm["port"].as<string>();
    bool isJournal = vm["no_journaling"].empty();
    bool isZip = (vm["zip"].as<string>() == "on");
    cout << "journal directory: " << journalDirectory << '\n'
         << "address: " << address << '\n'
         << "port: " << port << '\n'
         << "is journal: " << isJournal << '\n'
         << "is zip: " << isZip << '\n';

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

    JournalManager manager(journalDirectory, isZip);

    string msg{"hello world, this is test journal msg.\n"};

    while (1) {
        if (goodbye) break;

        sleep(1);

        for (int i = 0; i < 1000; ++i) {
            manager.record(msg);
        }
    }
    return 0;
}
