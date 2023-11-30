#ifndef SLANTRANSMITTER_H
#define SLANTRANSMITTER_H

#include "pcaptransmitter.h"





class SlanPortTransmitter: public PcapTransmitter
{
public:
    SlanPortTransmitter(const char *device, void* bind);
    ~SlanPortTransmitter();

    virtual void run();

private:
    //SlanTxThread txThread;
    int sendQueueTransmit(
        pcap_send_queue *queue, long &overHead, int sync, int startPacketNum=-1, bool isSinglePacket=false);
    void* m_bind;
};


#endif // SLANTRANSMITTER_H
