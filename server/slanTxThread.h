#ifndef SLANTXTHREAD_H
#define SLANTXTHREAD_H

#include "pcaptxthread.h"
//#include "ITxThread.h"
//PcapTxThread

class SlanTxThread :public PcapTxThread  {
public:
    SlanTxThread(int id, const char *device,void* bind);
    ~SlanTxThread();

    // QThread interface
protected:
    void run();
    int sendQueueTransmit(pcap_t *p, PacketSequence *seq,
                          long &overHead, int sync);
    // ITxThread interface
public:
    //bool setRateAccuracy(AbstractPort::Accuracy accuracy);
    //bool setStreamStatsTracking(bool enable);
    //void clearPacketList();
    //void loopNextPacketSet(qint64 size, qint64 repeats, long repeatDelaySec, long repeatDelayNsec);
    //bool appendToPacketList(long sec, long usec, const uchar *packet, int length);
    //void setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay);
    //bool setPacketListTtagMarkers(QList<uint> markers, uint repeatInterval);
    //void setHandle(pcap_t *handle);
    //void setStats(StatsTuple *stats);
    //StreamStats streamStats();
//    void start();
//    void stop();
//    bool isRunning();
//    double lastTxDuration();

private:
    void* m_bind;
} ;
#endif // SLANTXTHREAD_H
