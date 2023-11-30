#include "GlobalVC.h"

#include "slanport.h"
#include "devicemanager.h"
#include "packetbuffer.h"
#include "sagent.h"

#include <QtGlobal>

#ifdef Q_OS_WIN32

#include <windows.h>
#endif

#if defined(Q_OS_WIN32)
int win_gettimeofday(struct timeval *tv, struct timezone *tz);
#endif


SlanPort::SlanPort(int id, const char *device)
    : AbstractPort(id, device), m_bind(0)
{
	data_.set_name(device);
    data_.set_description("Slan port");

	data_.set_is_soft_crc(true);

	m_bind = SAGNTG_bind((char*) name(), 0, 12228, 0, (void*) this, receive);
	
	transmitter_ = new SlanPortTransmitter(device, m_bind);
	capturer_ = new PortCapturer(device);

	isUsable_ = (m_bind != 0);
}

void SlanPort::init()
{
    transmitter_->useExternalStats(&stats_);

    updateNotes();
}

SlanPort::~SlanPort()
{
    qDebug("In %s", __FUNCTION__);
	delete transmitter_;
	delete capturer_;

	if (m_bind)
	{
		SAGNTG_unbind(m_bind);
	}
}

void SlanPort::updateNotes()
{
    QString notes;
}

OstProto::LinkState SlanPort::linkState()
{
    if (m_bind)
    {
		linkState_ = OstProto::LinkStateUp;
	}
	else 
	{
		linkState_ = OstProto::LinkStateDown;
    }

    return linkState_; 
}

bool SlanPort::setRateAccuracy(AbstractPort::Accuracy accuracy)
{
    if (transmitter_->setRateAccuracy(accuracy)) 
	{
        AbstractPort::setRateAccuracy(accuracy);
        return true;
    }

    return false;
}

void SlanPort::startDeviceEmulation()
{
}

void SlanPort::stopDeviceEmulation()
{
}

int SlanPort::sendEmulationPacket(PacketBuffer *pktBuf)
{
    return 0;
}

void SlanPort::recievedPacket(const char* buff, unsigned long len)
{
	stats_.rxPkts += 1;
    stats_.rxBytes += len;
	if (len>1514)
		stats_.rxOversize+=1;

	struct pcap_pkthdr hdr;
	hdr.caplen = hdr.len = len;
#ifdef Q_OS_WIN32	
	win_gettimeofday(& hdr.ts, NULL);
#else
	gettimeofday(& hdr.ts, NULL);
#endif

	const uchar* data = (const uchar*) buff;

	const OstProto::Trigger* userTrigger = & userTrigger1();
	matchPacketTrigger(*userTrigger, len, data, stats_.triggeredRxPkts1);
	userTrigger = & userTrigger2();
	matchPacketTrigger(*userTrigger, len, data, stats_.triggeredRxPkts2);
	userTrigger = & userTrigger3();
	matchPacketTrigger(*userTrigger, len, data, stats_.triggeredRxPkts3);
	userTrigger = & userTrigger4();
	matchPacketTrigger(*userTrigger, len, data, stats_.triggeredRxPkts4);

	if ( capturer_->isRunning() )
	{
		capturer_->recievedPacket(& hdr, (const uchar*) buff);
	}
}

char * SlanPort::receive(
    unsigned long  msg_code,
    unsigned long  res,
    unsigned long  sender_tid,
    unsigned long  len,
    void     * usr_info,
    char     * buff)
{
    SlanPort* slanPort = static_cast<SlanPort*>(usr_info);
    
    switch (res)
    {
    case SAGNTG_get_buff_rsn_CNS:
        if(buff)
        {
			slanPort->recievedPacket(buff, len);
        }
        
        break;
	/*
    case SAGNTG_give_buff_succs_rsn_CNS:
        if(buff)
        {
			slanPort->recievedPacket(buff, len);
        }

        break;
	*/
   
    default:
        break;
    }

    return 0;
}




/*
 * ------------------------------------------------------------------- *
 * Port Capturer
 * ------------------------------------------------------------------- *
 */
SlanPort::PortCapturer::PortCapturer(const char *device)
{
    device_ = QString::fromLatin1(device);
    stop_ = false;
    state_ = kNotStarted;

    if (!capFile_.open())
        qWarning("Unable to open temp cap file");

    qDebug("cap file = %s", capFile_.fileName().toLatin1().constData());

	dumpHandle_ = NULL;
    handle_ = NULL;
}

SlanPort::PortCapturer::~PortCapturer()
{
    capFile_.close();
}

void SlanPort::PortCapturer::recievedPacket(struct pcap_pkthdr* hdr, const uchar* data)
{
	if ( isRunning() && dumpHandle_)
	{
		pcap_dump((uchar*) dumpHandle_, hdr, data);
	}
}

void SlanPort::PortCapturer::start()
{
    // FIXME: return error
    if (state_ == kRunning) {
        qWarning("Capture start requested but is already running!");
        return;
    }

	qDebug("In %s", __PRETTY_FUNCTION__);

    if (!capFile_.isOpen())
    {
        qWarning("temp cap file is not open");
        return;
    }

    handle_ = pcap_open_dead(DLT_EN10MB, 65535 /* snaplen */);

    if (! handle_)
    {
		return;
    }

    dumpHandle_ = pcap_dump_open(handle_, capFile_.fileName().toLatin1().constData());

	if (! dumpHandle_)
	{
		pcap_close(handle_);
		handle_ = NULL;

		return;
	}
    
	state_ = kRunning;
}

void SlanPort::PortCapturer::stop()
{
    if (state_ == kRunning) {
        stop_ = true;
        state_ = kFinished;

		pcap_dump_close(dumpHandle_);
		pcap_close(handle_);

		dumpHandle_ = NULL;
		handle_ = NULL;
    }
    else {
        // FIXME: return error
        qWarning("Capture stop requested but is not running!");
        return;
    }
}

bool SlanPort::PortCapturer::isRunning()
{
    return (state_ == kRunning);
}

QFile* SlanPort::PortCapturer::captureFile()
{
    return &capFile_;
}
