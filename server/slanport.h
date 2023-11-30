#ifndef _SERVER_SLAN_PORT_H
#define _SERVER_SLAN_PORT_H

#include <QTemporaryFile>
#include <QThread>
#define HAVE_REMOTE 1 // Gregory

#include "abstractport.h"
#include "slantransmitter.h"
#include <pcap.h>
#include "pcapextra.h"

class SlanPort : public AbstractPort
{
public:
    SlanPort(int id, const char *device);
    ~SlanPort();

    void init();

	void recievedPacket(const char* buff, unsigned long len);

	virtual OstProto::LinkState linkState();
    virtual bool hasExclusiveControl() { return false; }
    virtual bool setExclusiveControl(bool /*exclusive*/) { return false; }

    virtual bool setRateAccuracy(AbstractPort::Accuracy accuracy); 

    virtual void clearPacketList() { 
        transmitter_->clearPacketList();
        setPacketListLoopMode(false, 0, 0);
    }
    virtual void loopNextPacketSet(qint64 size, qint64 repeats,
            long repeatDelaySec, long repeatDelayNsec) {
        transmitter_->loopNextPacketSet(size, repeats, 
                repeatDelaySec, repeatDelayNsec);
    }
    virtual bool appendToPacketList(long sec, long nsec, const uchar *packet, 
            int length) {
        return transmitter_->appendToPacketList(sec, nsec, packet, length); 
    }
    virtual void setPacketListLoopMode(bool loop, quint64 secDelay, quint64 nsecDelay)
    {
        transmitter_->setPacketListLoopMode(loop, secDelay, nsecDelay);
    }
    virtual bool setPacketListTtagMarkers(QList<uint> markers,
                                          uint repeatInterval)
    {
        return transmitter_->setPacketListTtagMarkers(markers, repeatInterval);
    }
    virtual void startTransmit() { 
        Q_ASSERT(!isDirty());
        transmitter_->transmit(); // GREGORY: instead of transmitter_->start()
    }

	// GREGORY
	virtual void singlePacketTransmit() { 
        Q_ASSERT(!isDirty());
        transmitter_->singlePacketTransmit(); 
    }

    //oleg
    virtual double lastTransmitDuration() {
        return transmitter_->lastTxDuration();
    }

    virtual void stopTransmit()  { transmitter_->stop();  }
    virtual bool isTransmitOn() { return transmitter_->isRunning(); }

    virtual void startCapture() { capturer_->start(); }
    virtual void stopCapture()  { capturer_->stop(); }
    virtual bool isCaptureOn()  { return capturer_->isRunning(); }
    virtual QIODevice* captureData() { return capturer_->captureFile(); }

    virtual void startDeviceEmulation();
    virtual void stopDeviceEmulation();
    virtual int sendEmulationPacket(PacketBuffer *pktBuf);

protected:
    enum Direction
    {
        kDirectionRx,
        kDirectionTx
    };

    void updateNotes();

private:
	
	static char * receive(
        unsigned long  msg_code,
        unsigned long  res,
        unsigned long  sender_tid,
        unsigned long  len,
        void     * usr_info,
        char     * buff);

	void* m_bind;

private:
	class PortCapturer
    {
    public:
        PortCapturer(const char *device);
        ~PortCapturer();
        void start();
        void stop();
        bool isRunning();
        QFile* captureFile();

		void recievedPacket(struct pcap_pkthdr* hdr, const uchar* data);

    private:
        enum State 
        {
            kNotStarted,
            kRunning,
            kFinished
        };

        QString         device_;
        volatile bool   stop_;
        QTemporaryFile  capFile_;
		pcap_t          *handle_;
        pcap_dumper_t   *dumpHandle_;
        volatile State  state_;
    };

	SlanPortTransmitter *transmitter_;
	PortCapturer    *capturer_;
};

#endif
