#include "slanTxThread.h"
#include "qdebug.h"
#include "timestamp.h"
#include "sagent.h"

//SlanTxThread::SlanTxThread():PcapTxThread(0)
//{

//}

//

SlanTxThread::SlanTxThread(int id, const char *device,void* bind): PcapTxThread(0) //OLEG TODO
{
    m_bind = bind;
}



SlanTxThread::~SlanTxThread()
{

}

void SlanTxThread::run()
{
    //! \todo (MED) Stream Mode - continuous: define before implement

    // NOTE1: We can't use pcap_sendqueue_transmit() directly even on Win32
    // 'coz of 2 reasons - there's no way of stopping it before all packets
    // in the sendQueue are sent out and secondly, stats are available only
    // when all packets have been sent - no periodic updates
    //
    // NOTE2: Transmit on the Rx Handle so that we can receive it back
    // on the Tx Handle to do stats
    //
    // NOTE3: Update pcapExtra counters - port TxStats will be updated in the
    // 'stats callback' function so that both Rx and Tx stats are updated
    // together

    const int kSyncTransmit = 1;
    int i;
    long overHead = 0; // overHead should be negative or zero
    TimeStamp startTime, endTime;

    qDebug("packetSequenceList_.size = %d", packetSequenceList_.size());
    if (packetSequenceList_.size() <= 0) {
        lastTxDuration_ = 0.0;
        goto _exit2;
    }

    for(i = 0; i < packetSequenceList_.size(); i++) {
        qDebug("sendQ[%d]: rptCnt = %d, rptSz = %d, usecDelay = %ld", i,
               packetSequenceList_.at(i)->repeatCount_,
               packetSequenceList_.at(i)->repeatSize_,
               packetSequenceList_.at(i)->usecDelay_);
        qDebug("sendQ[%d]: pkts = %ld, usecDuration = %ld, ttagL4CksumOfs = %hu", i,
               packetSequenceList_.at(i)->packets_,
               packetSequenceList_.at(i)->usecDuration_,
               packetSequenceList_.at(i)->ttagL4CksumOffset_);
    }

    qDebug() << "Loop:" << (returnToQIdx_ >= 0)
             << "LoopDelay:" << loopDelay_;
    qDebug() << "First Ttag: " << firstTtagPkt_
             << "Ttag Markers:" << ttagDeltaMarkers_;

    lastStats_ = *stats_; // used for stream stats

    // Init Ttag related vars. If no packets need ttag, firstTtagPkt_ is -1,
    // so nextTagPkt_ is set to practically unreachable value (due to
    // 64 bit counter wraparound time!)
    ttagMarkerIndex_ = 0;
    nextTtagPkt_ = stats_->pkts + firstTtagPkt_;

    getTimeStamp(&startTime);
    state_ = kRunning;
    i = 0;
    while (i < packetSequenceList_.size()) {
    _restart:
        int rptSz  = packetSequenceList_.at(i)->repeatSize_;
        int rptCnt = packetSequenceList_.at(i)->repeatCount_;

        for (int j = 0; j < rptCnt; j++) {
            for (int k = 0; k < rptSz; k++) {
                int ret;
                PacketSequence *seq = packetSequenceList_.at(i+k);
#ifdef Q_OS_WIN32
                TimeStamp ovrStart, ovrEnd;

                // Use Windows-only pcap_sendqueue_transmit() if duration < 1s
                // and no stream timing is configured
/*                if (seq->usecDuration_ <= long(1e6) && firstTtagPkt_ < 0) {
                    getTimeStamp(&ovrStart);
                    //ret = pcap_sendqueue_transmit(handle_,seq->sendQueue_, kSyncTransmit);
                    if (ret >= 0) {
                        stats_->pkts += seq->packets_;
                        stats_->bytes += seq->bytes_;

                        getTimeStamp(&ovrEnd);
                        overHead += seq->usecDuration_
                                    - udiffTimeStamp(&ovrStart, &ovrEnd);
                        Q_ASSERT(overHead <= 0);
                    }
                    if (stop_)
                        ret = -2;
                } else*/
                {
                    ret = sendQueueTransmit(handle_, seq,
                                            overHead, kSyncTransmit);
                }
#else
                ret = sendQueueTransmit(handle_, seq,
                                        overHead, kSyncTransmit);
#endif

                if (ret >= 0) {
                    long usecs = seq->usecDelay_ + overHead;
                    if (usecs > 0) {
                        (*udelayFn_)(usecs);
                        overHead = 0;
                    } else
                        overHead = usecs;
                } else {
                    qDebug("error %d in sendQueueTransmit()", ret);
                    qDebug("overHead = %ld", overHead);
                    stop_ = false;
                    goto _exit;
                }
            } // rptSz
        } // rptCnt

        // Move to the next Packet Set
        i += rptSz;
    }

    if (returnToQIdx_ >= 0) {
        long usecs = loopDelay_ + overHead;

        if (usecs > 0) {
            (*udelayFn_)(usecs);
            overHead = 0;
        } else
            overHead = usecs;

        i = returnToQIdx_;
        goto _restart;
    }

_exit:
    getTimeStamp(&endTime);
    lastTxDuration_ = udiffTimeStamp(&startTime, &endTime)/1e6;

_exit2:
    qDebug("Tx duration = %fs", lastTxDuration_);
    //Q_ASSERT(lastTxDuration_ >= 0);

    if (trackStreamStats_)
        updateTxStreamStats();

    state_ = kFinished;
}


int SlanTxThread::sendQueueTransmit(pcap_t *p, PacketSequence *seq,
                                    long &overHead, int sync)
{
    TimeStamp ovrStart, ovrEnd;
    struct timeval ts;
    pcap_send_queue *queue = seq->sendQueue_;
    struct pcap_pkthdr *hdr = (struct pcap_pkthdr*) queue->buffer;
    char *end = queue->buffer + queue->len;

    ts = hdr->ts;
    getTimeStamp(&ovrStart);
    while((char*) hdr < end) {
        uchar *pkt = (uchar*)hdr + sizeof(*hdr);
        int pktLen = hdr->caplen;
        bool ttagPkt = false;
#if 0
        quint16 origCksum = 0;
#endif

        // Time for a T-Tag packet?
        if (stats_->pkts == nextTtagPkt_) {
            ttagPkt = true;
            // XXX: write 2xBytes instead of 1xHalf-word to avoid
            // potential alignment problem
            *(pkt+pktLen-5) = SignProtocol::kTypeLenTtag;
            *(pkt+pktLen-6) = ttagId_;

#if 0 \
    // Recalc L4 checksum; use incremental checksum as per RFC 1624 \
    // HC' = ~(~HC + ~m + m')
            if (seq->ttagL4CksumOffset_) {
                quint16 *cksum = reinterpret_cast<quint16*>(
                                        pkt + seq->ttagL4CksumOffset_);
                origCksum = qFromBigEndian<quint16>(*cksum);
                // XXX: SignProtocol trailer
                //      ... | <guid> | 0x61 |     0x00 | 0x22 | 0x1d10c0da
                //      ... | <guid> | 0x61 | <TtagId> | 0x23 | 0x1d10c0da
                // For odd pkt Length, Ttag spans across 2 half-words
                // XXX: Hardcoded values instead of sign protocol constants
                // used below for readability
                quint32 newCksum = pktLen & 1 ?
                    quint16(~origCksum) + quint16(~0x221d) + 0x231d
                                        + quint16(~0x6100) + (0x6100 | ttagId_) :
                    quint16(~origCksum) + quint16(~0x0022) + (ttagId_ << 8 | 0x23);
                while (newCksum > 0xffff)
                    newCksum = (newCksum & 0xffff) + (newCksum >> 16);
                // XXX: For IPv4/UDP, if ~newcksum is 0x0000 we are supposed to
                // set the checksum as 0xffff since 0x0000 indicates no cksum
                // is present - we choose not to do this to avoid extra cost
                *cksum = qToBigEndian(quint16(~newCksum));
            }
#endif
            ttagId_++;
            nextTtagPkt_ += ttagDeltaMarkers_.at(ttagMarkerIndex_);
            ttagMarkerIndex_++;
            if (ttagMarkerIndex_ >= ttagDeltaMarkers_.size())
                ttagMarkerIndex_ = 0;
        }

//Oleg TODO disabled sync
        //        if (sync) {
//            long usec = (hdr->ts.tv_sec - ts.tv_sec) * 1000000 +
//                        (hdr->ts.tv_usec - ts.tv_usec);

//            getTimeStamp(&ovrEnd);
//            overHead -= udiffTimeStamp(&ovrStart, &ovrEnd);
//            Q_ASSERT(overHead <= 0);
//            usec += overHead;
//            if (usec > 0) {
//                (*udelayFn_)(usec);
//                overHead = 0;
//            } else
//                overHead = usec;

//            ts = hdr->ts;
//            getTimeStamp(&ovrStart);
//        }

        Q_ASSERT(pktLen > 0);

        //pcap_sendpacket(p, pkt, pktLen);
        unsigned int ret = SAGNTG_transmit(m_bind,0,SAGNTG_normal_msg_rsn_CNS,pktLen,(char *) pkt);
        stats_->pkts++;
        stats_->bytes += pktLen;

        // Revert T-Tag packet changes
        if (ttagPkt) {
            *(pkt+pktLen-5) = SignProtocol::kTypeLenTtagPlaceholder;
            *(pkt+pktLen-6) = 0;
#if 0
            if (seq->ttagL4CksumOffset_) {
                quint16 *cksum = reinterpret_cast<quint16*>(
                                         pkt + seq->ttagL4CksumOffset_);
                *cksum = qToBigEndian(origCksum);
            }
#endif
        }

        // Step to the next packet in the buffer
        hdr = (struct pcap_pkthdr*) (pkt + pktLen);
        pkt = (uchar*) ((uchar*)hdr + sizeof(*hdr)); // FIXME: superfluous?

        if (stop_) {
            return -2;
        }
    }
    return 0;
}


//bool SlanTxThread::setRateAccuracy(AbstractPort::Accuracy accuracy)
//{

//}

//bool SlanTxThread::setStreamStatsTracking(bool enable)
//{

//}

//void SlanTxThread::clearPacketList()
//{

//}

//void SlanTxThread::loopNextPacketSet(qint64 size, qint64 repeats, long repeatDelaySec, long repeatDelayNsec)
//{

//}

//bool SlanTxThread::appendToPacketList(long sec, long usec, const uchar *packet, int length)
//{

//}

//void SlanTxThread::setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay)
//{

//}

//bool SlanTxThread::setPacketListTtagMarkers(QList<uint> markers, uint repeatInterval)
//{

//}

//void SlanTxThread::setHandle(pcap_t *handle)
//{

//}

//void SlanTxThread::setStats(StatsTuple *stats)
//{

//}

//StreamStats SlanTxThread::streamStats()
//{

//}

//void SlanTxThread::start()
//{

//}

//void SlanTxThread::stop()
//{

//}

//bool SlanTxThread::isRunning()
//{
//    //TODO Oleg
//    return false;
//}

//double SlanTxThread::lastTxDuration()
//{

//}


