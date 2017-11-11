#ifndef MIDAS_JOURNAL_MANAGER_H
#define MIDAS_JOURNAL_MANAGER_H

#include "Journal.h"

namespace midas {

/**
 * used in single thread, multi-thread will behave unpredictable
 */
class JournalManager {
public:
    bool isZip{true};
    string journalDirectory;
    size_t maxBytesBehind{100000000}, maxJournalSize{100000000}, maxMsgLen{80000};
    long maxJournalSecs{86400};
    int freeBytes{0};
    const int MinBytesFree{100000000};
    size_t bufLen{maxBytesBehind + MinBytesFree};
    char *bufPtr;
    long long totalBytes{0};  // bytes received
    long long totalMsgs{0};   // msg received
    Journal *journal{nullptr};
    timeval idleTime;

public:
    JournalManager(string dir, bool isZip = true) : isZip(isZip), journalDirectory(dir) {
        bufPtr = new char[bufLen + maxMsgLen];
        if (isZip)
            journal = new GzJournal(journalDirectory, ".gz", 0, cerr);
        else
            journal = new FJournal(journalDirectory, "", 0, cerr);
    }

    ~JournalManager() {
        if (journal) {
            delete journal;
        }
        delete[] bufPtr;
    }

    /**
     * write to buffer first, if no space, then rollover buffer to disk
     * @param msg
     */
    void record(const string &msg) {
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

        record2buffer(msg);

        check_space_handle_if_not_enough();
    }

    /**
     * force to flush, just delete the journal file and create new one
     * TODO this is not thread safe, if want to thread safe, then need mutex for flush and record function
     */
    void flush() {
        long long bytes = journal->totalBytes;
        delete journal;
        journal = nullptr;

        if (isZip)
            journal = new GzJournal(journalDirectory, ".gz", bytes, cerr);
        else
            journal = new FJournal(journalDirectory, "", bytes, cerr);
    }

private:
    void record2buffer(const string &msg) {
        long dataOff = totalBytes % bufLen;
        char *dataPtr = &bufPtr[dataOff];

        size_t cc = msg.size();
        memcpy(dataPtr, msg.c_str(), cc);

        totalBytes += cc;
        totalMsgs++;
        freeBytes -= cc;

        char *dataEnd = &dataPtr[cc];
        char *bufEnd = &bufPtr[bufLen];
        if (dataEnd > bufEnd)  // recover overflow data to the begin of buffer
            memcpy(bufPtr, bufEnd, dataEnd - bufEnd);
    }

    void check_space_handle_if_not_enough() {
        // there is a good journal with pending bytes
        bool journalReady = journal ? (journal->bad() ? false : journal->pending_bytes(totalBytes) > 0) : false;

        if (journalReady) {
            journal->scribble(bufPtr, bufLen, totalBytes, cerr);

            // still room for jnl, then no need to create new jnl file
            if (journal->bytes() < static_cast<int>(maxJournalSize)) return;
        }

        gettimeofday(&idleTime, 0);

        // journal rollover due to size, time, cut, bad for 3 secs
        if (journal) {
            time_t secs = idleTime.tv_sec - journal->birth.tv_sec;
            if ((journal->bad() && secs > 3) || journal->bytes() > static_cast<int>(maxJournalSize) ||
                secs > maxJournalSecs || journal->cut) {
                flush();
            }
        }
    }
};
}

#endif
