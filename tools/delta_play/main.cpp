#include <utils/convert/TimeHelper.h>
#include <zlib.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <iostream>

using namespace std;
using namespace boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char** argv) {
    double start, end;
    po::options_description desc("Program options");
    desc.add_options()("help,h", "print_trading_hour help")
            ("directory,d", po::value<string>()->default_value("."), "directory to play")
            ("start,s", po::value<double>(&start)->default_value(-1), "start time to play")
            ("end,e", po::value<double>(&end)->default_value(-1), "end time to play");

    po::variables_map vm;
    auto parsed = po::parse_command_line(argc, argv, desc);
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    uint64_t snTime = 0, enTime = 0;

    string directory = vm["directory"].as<string>();

    path dir(directory);
    if (!is_directory(dir)) {
        cerr << "invalid directory: " << directory << '\n';
        return -1;
    }

    vector<string> files;
    vector<double> times;
    directory_iterator itrEnd;
    for (directory_iterator i(dir); i != itrEnd; ++i) {
        path p = i->path();

        if (is_regular_file(p) && p.extension().string() == ".gz") {
            files.push_back(p.parent_path().string() + '/' + p.filename().string());
        }
    }

    sort(files.begin(), files.end());

    int len = static_cast<int>(files.size());
    long si = 0, ei = len;

    for (int i = 0; i < len; ++i) {
        double currentTime = boost::lexical_cast<double>(files[i].substr(files[i].find_last_of('.') - 15, 15));
        times.push_back(currentTime);
    }

    if (start > 0) {
        snTime = midas::ntime_from_double(start);
        si = lower_bound(times.begin(), times.end(), start) - times.begin();
        if (si > 0 && start <= times[si]) --si;
    }
    if (end > 0) {
        enTime = midas::ntime_from_double(end);
        ei = upper_bound(times.begin(), times.end(), end) - times.begin();
        if (ei < len && end >= times[ei]) ++si;
    }

    const int READ_SIZE = 1024 * 1024;
    const int GZ_BUF_SIZE = READ_SIZE + 1;
    char buf[GZ_BUF_SIZE];
    for (long i = si; i < ei; ++i) {
        //        cout << files[i] << '\n';     // keep here for trouble shooting
        bool isFirstFile = (i == si);
        bool isLastFile = (i == (ei - 1));

        gzFile gzfp = gzopen(files[i].c_str(), "rb");
        if (!gzfp) continue;

        stringstream ss;
        int have;
        while ((have = gzread(gzfp, buf, READ_SIZE)) > 0) {
            buf[have] = '\0';
            if (isFirstFile || isLastFile)
                ss << buf;
            else
                cout << buf;
        }

        // only first and last zip file need filter by time
        if (isFirstFile || isLastFile) {
            string line;
            while (getline(ss, line)) {
                if (line.empty() || *(line.end() - 1) != ';') continue;
                auto itr1 = line.find_first_of("rcvt") + 5;
                auto itr2 = line.find_first_of(',', itr1) - 1;
                uint64_t rcvt = boost::lexical_cast<uint64_t>(line.substr(itr1, itr2 - itr1));
                if ((snTime == 0 || rcvt >= snTime) && (enTime == 0 || rcvt <= enTime)) cout << line << '\n';
            }
        } else {
            cout << '\n';
        }

        gzclose(gzfp);
    }
    return 0;
}
