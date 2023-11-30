/* Copyright (c) 2019-2021 Marvell International Ltd. All rights reserved */
#ifndef ITXTHREAD_H
#define ITXTHREAD_H

#include "abstractport.h"
#include "packetsequence.h"
#include "statstuple.h"

#include <QMutex>
#include <QThread>
#include <pcap.h>

class ITxThread : public QThread
{
public:
    virtual bool setRateAccuracy(AbstractPort::Accuracy accuracy) = 0;
    virtual bool setStreamStatsTracking(bool enable)= 0;

    virtual void clearPacketList()= 0;
    virtual void loopNextPacketSet(qint64 size, qint64 repeats,
                                   long repeatDelaySec, long repeatDelayNsec)= 0;
    virtual bool appendToPacketList(long sec, long usec, const uchar *packet,
                                    int length)= 0;
    virtual void setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay)= 0;
    virtual bool setPacketListTtagMarkers(QList<uint> markers, uint repeatInterval)= 0;

    virtual void setHandle(pcap_t *handle)= 0;

    virtual void setStats(StatsTuple *stats)= 0;

    virtual StreamStats streamStats()= 0; // reset on read

    virtual void run()= 0;

    virtual void start()= 0;
    virtual void stop()= 0;
    virtual bool isRunning()= 0;
    virtual double lastTxDuration()= 0;

};
#endif // ITXTHREAD_H
