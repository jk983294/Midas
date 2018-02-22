#ifndef MIDAS_MD_CLIENT_H
#define MIDAS_MD_CLIENT_H

#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/log/Log.h>
#include <cstdint>
#include <string>

class MdClient {
public:
    uint64_t startTime = 0;
    uint64_t session = 0;
    pid_t pid;
    uint32_t subscriptionFlags = 0;
    uint8_t clientId = 0;
    std::string user;
    std::string password;
    std::string name;

public:
    MdClient(std::string const &fileConfig, std::string const &key, midas::MDConsumerCallbacks callbacks)
        : pid(getpid()) {
        startTime = midas::ntime();
        session = startTime;

        if (callbacks.cbBookRefreshed) subscriptionFlags |= midas::FLAG_SEND_BOOK_REFRESHED;
        if (callbacks.cbBookChanged) subscriptionFlags |= midas::FLAG_SEND_BOOK_CHANGED;
        if (callbacks.cbTradingAction) subscriptionFlags |= midas::FLAG_SEND_TRADING_ACTION;
        if (callbacks.cbDataHeartbeat) subscriptionFlags |= midas::FLAG_SEND_DATA_HEARTBEAT;

        static const char root[] = "client";

        user = midas::get_cfg_value<string>(root, "user", "");
        password = midas::get_cfg_value<string>(root, "password", "");
        name = midas::get_cfg_value<string>(root, "name", "");
        MIDAS_LOG_INFO("client " << name << " user: " << user);
    }

    MdClient(MdClient const &) = delete;

    MdClient &operator=(MdClient const &) = delete;

    virtual ~MdClient() = default;

    virtual void on_control_channel_connected() = 0;

    virtual void on_control_channel_disconnected() = 0;

    virtual void on_control_channel_established() = 0;

    virtual void on_trading_action(midas::Header *, midas::DataTradingAction *) = 0;

    virtual void on_book_refreshed(midas::Header *, midas::DataBookRefreshed *) = 0;

    virtual void on_book_changed(midas::Header *, midas::DataBookChanged *) = 0;

    virtual void on_data_heartbeat(midas::Header *, midas::DataHeartbeat *) = 0;

    virtual void on_connect_response(midas::Header *, midas::CtrlConnectResponse *, bool *isConnected) = 0;

    virtual void on_disconnect_response(midas::Header *, midas::CtrlDisconnectResponse *) = 0;

    virtual void on_subscribe_response(midas::Header *, midas::CtrlSubscribeResponse *) = 0;

    virtual void on_unsubscribe_response(midas::Header *, midas::CtrlUnsubscribeResponse *) = 0;

    virtual void callback_periodically() {}

    virtual uint32_t callback_interval() const { return 1000; }
};

#endif
