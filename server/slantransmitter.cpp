
#include "slantransmitter.h"
#include "slanTxThread.h"
/*
 * ------------------------------------------------------------------- *
 * Port Transmitter
 * ------------------------------------------------------------------- *
 */

SlanPortTransmitter::SlanPortTransmitter(const char *device, void* bind) :
    PcapTransmitter(device, new SlanTxThread(0,0,bind)), m_bind(bind)
{
}

SlanPortTransmitter::~SlanPortTransmitter()
{
}


void SlanPortTransmitter::run()
{

}


int SlanPortTransmitter::sendQueueTransmit(
    pcap_send_queue *queue, long &overHead, int sync, int startPacketNum, bool isSinglePacket)
{
//    TimeStamp ovrStart, ovrEnd;
//    struct timeval ts;
//    struct pcap_pkthdr *hdr = (struct pcap_pkthdr*) queue->buffer;
//    char *end = queue->buffer + queue->len;

//    ts = hdr->ts;

//    int packetCounter = 1;

//    getTimeStamp(&ovrStart);
//    while((char*) hdr < end)
//    {
//        uchar *pkt = (uchar*)hdr + sizeof(*hdr);
//        int pktLen = hdr->caplen;

//        if (sync)
//        {
//            bool isSyncPacket = (startPacketNum <= 0 || (packetCounter > startPacketNum && ! isSinglePacket));

//            if (isSyncPacket)
//            {
//                long usec = (hdr->ts.tv_sec - ts.tv_sec) * 1000000 +
//                            (hdr->ts.tv_usec - ts.tv_usec);

//                getTimeStamp(&ovrEnd);

//                overHead -= udiffTimeStamp(&ovrStart, &ovrEnd);
//                Q_ASSERT(overHead <= 0);
//                usec += overHead;
//                if (usec > 0)
//                {
//                    (*udelayFn_)(usec);
//                    overHead = 0;
//                }
//                else
//                    overHead = usec;
//            }

//            ts = hdr->ts;
//            getTimeStamp(&ovrStart);
//        }

//        Q_ASSERT(pktLen > 0);

//        if (startPacketNum <= 0 || startPacketNum == packetCounter ||
//            (startPacketNum < packetCounter && ! isSinglePacket))
//        {
//            //pcap_sendpacket(p, pkt, pktLen);
//            unsigned int ret = SAGNTG_transmit(
//                m_bind,
//                0,
//                SAGNTG_normal_msg_rsn_CNS,
//                pktLen,
//                (char *) pkt
//                );

//            stats_->txPkts++;
//            stats_->txBytes += pktLen;

//            if (startPacketNum == packetCounter && isSinglePacket)
//            {
//                return 0;
//            }
//        }

//        ++packetCounter;

//        // Step to the next packet in the buffer
//        hdr = (struct pcap_pkthdr*) (pkt + pktLen);
//        pkt = (uchar*) ((uchar*)hdr + sizeof(*hdr));

//        if (stop_)
//        {
//            return -2;
//        }
//    }

//    return 0;
}

