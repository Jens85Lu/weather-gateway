#pragma once

#include <mosquittopp.h>
#include <database.hpp>
#include <functional>


class MqttClient : public mosqpp::mosquittopp
{
public:
    MqttClient(Database& database);

    void on_connect(int rc) override;

    void on_disconnect(int rc) override;

    void on_message(const struct mosquitto_message* message) override;

    void requestSyncInfo();

    SyncInfo getSyncInfo() const { return sdInfo_; }

    using SyncInfoCallback = std::function<void()>;
    
    void setSyncInfoCallback(SyncInfoCallback callback)
    {
        syncInfoCallback_ = callback;
    }

    using IntervalCompleteCallback = std::function<void()>;

    void setIntervalCompleteCallback(IntervalCompleteCallback callback)
    {
        intervalCompleteCallback_ = callback;
    }

    bool historyStorageSuccessful = true;
    
private:
    Database& database_;

    SyncInfo sdInfo_;

    SyncInfoCallback syncInfoCallback_;
    
    IntervalCompleteCallback intervalCompleteCallback_;

};