/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "winpcapport.h"

#include "interfaceinfo.h"

#include <QCoreApplication> 
#include <QProcess> 

#ifdef Q_OS_WIN32

#include <ntddndis.h>
#include <ws2ipdef.h>

PIP_ADAPTER_ADDRESSES WinPcapPort::adapterList_ = NULL;

WinPcapPort::WinPcapPort(int id, const char *device, const char *description)
    : PcapPort(id, device)
{
    populateInterfaceInfo();

    monitorRx_->stop();
    monitorTx_->stop();
    monitorRx_->wait();
    monitorTx_->wait();

    delete monitorRx_;
    delete monitorTx_;

	// GREGORY
	capturer_->stop();
	capturer_->wait();
	delete capturer_;

    monitorRx_ = new PortMonitor(device, kDirectionRx, &stats_);
    monitorTx_ = new PortMonitor(device, kDirectionTx, &stats_);


	capturer_ = new PortCapturer(device, this);

	monitorRxFullPackage_ = new PortFullPackageMonitor(device, kDirectionRx, &stats_, this);

    data_.set_description(description);

    adapter_ = PacketOpenAdapter((CHAR*)device);
    if (!adapter_)
        qFatal("Unable to open adapter %s", device);
    linkStateOid_ = (PPACKET_OID_DATA) malloc(sizeof(PACKET_OID_DATA) + 
            sizeof(NDIS_LINK_STATE));
    if (!linkStateOid_)
        qFatal("failed to alloc oidData");

    data_.set_is_exclusive_control(hasExclusiveControl());
    minPacketSetSize_ = 256;

	// GREGORY
	pcapFilterLocalAdapter(monitorRx_->handle());
	pcapFilterLocalAdapter(monitorTx_->handle());
	pcapFilterLocalAdapter(monitorRxFullPackage_->handle());
}

WinPcapPort::~WinPcapPort()
{
	if (monitorRxFullPackage_)
	{
        monitorRxFullPackage_->stop();
        monitorRx_->wait();
		delete monitorRxFullPackage_;
	}
}

// GREGORY
void WinPcapPort::init()
{
	PcapPort::init();

    monitorRxFullPackage_->start();

}

// GREGORY
QString WinPcapPort::getMacAddress()
{
	PPACKET_OID_DATA macAddrOid = (PPACKET_OID_DATA) malloc(sizeof(PACKET_OID_DATA) + sizeof(uint));
    if (! macAddrOid)
	{
        qFatal("failed to alloc oidData");
		return "";
	}

    memset(macAddrOid, 0, sizeof(PACKET_OID_DATA) + sizeof(uint));

	macAddrOid->Oid = OID_802_3_CURRENT_ADDRESS;

	macAddrOid->Length = 6;
	 
	char* macAddrBuf = (char*) malloc(100 + sizeof(char));
	QString macAddr = "";

    if (PacketRequest(adapter_, 0, macAddrOid))
    {
		sprintf(macAddrBuf, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
			(macAddrOid->Data)[0],
			(macAddrOid->Data)[1],
			(macAddrOid->Data)[2],
			(macAddrOid->Data)[3],
			(macAddrOid->Data)[4],
			(macAddrOid->Data)[5]);

		macAddr = macAddrBuf;
    }
	else
	{
		qWarning("Failed to get MAC address of the adapter %s\n", adapter_->Name);
	}

	free(macAddrBuf);
	free(macAddrOid);

	qWarning("The MAC address of the adapter %s is %s\n", adapter_->Name, qPrintable(macAddr));

    return macAddr;
}

// GREGORY
void WinPcapPort::pcapFilterLocalAdapter(pcap_t *handle)
{
	const char* device = adapter_->Name;

	const int optimize = 1;
	struct bpf_program bpf;
	QString macAddr = getMacAddress();
	if ( macAddr.isEmpty() )
	{
		return;
	}

	QString capture_filter( QString("not ether src ") + macAddr );


    //Oleg FIX obsolet Ascii
    QString qdevice = QString::fromLatin1(device); //QString::fromAscii(device);

    //if (pcap_compile(handle, &bpf, capture_filter.toAscii().data(), optimize, 0xffffffff) < 0)
    if (pcap_compile(handle, &bpf, capture_filter.toLatin1().data(), optimize, 0xffffffff) < 0)
    {
        qWarning("%s: error compiling filter: %s", qPrintable(qdevice),
                pcap_geterr(handle));
    }
	else
	{
		if (pcap_setfilter(handle, &bpf) < 0)
		{
			qWarning("%s: error setting filter: %s", qPrintable(qdevice),
					pcap_geterr(handle));
		}
	}
}

OstProto::LinkState WinPcapPort::linkState()
{
    memset(linkStateOid_, 0, sizeof(PACKET_OID_DATA) + sizeof(NDIS_LINK_STATE));

    linkStateOid_->Oid = OID_GEN_LINK_STATE;
    linkStateOid_->Length = sizeof(NDIS_LINK_STATE);

    // TODO: migrate to the npcap-only pcap_oid_get_request() when Ostinato
    // stops supporting WinPcap
    if (PacketRequest(adapter_, 0, linkStateOid_))
    {
        uint state;

        if (linkStateOid_->Length == sizeof(NDIS_LINK_STATE))
        {
            memcpy((void*)&state,
                   (void*)(linkStateOid_->Data+sizeof(NDIS_OBJECT_HEADER)),
                   sizeof(state));
            //qDebug("%s: state = %d", data_.description().c_str(), state);
            if (state == 0)
                linkState_ = OstProto::LinkStateUnknown;
            else if (state == 1)
                linkState_ = OstProto::LinkStateUp;
            else if (state == 2)
                linkState_ = OstProto::LinkStateDown;
        }
        else {
            //qDebug("%s: link state fail", data_.description().c_str());
        }
    }
    else {
        //qDebug("%s: link state request fail", data_.description().c_str());
    }

    return linkState_;
}

bool WinPcapPort::hasExclusiveControl() 
{
    QString portName(adapter_->Name + strlen("\\Device\\NPF_"));
    QString bindConfigFilePath(QCoreApplication::applicationDirPath()
                + "/bindconfig.exe");
    int exitCode;

    qDebug("%s: %s", __FUNCTION__, qPrintable(portName));

    if (!QFile::exists(bindConfigFilePath))
        return false;

    exitCode = QProcess::execute(bindConfigFilePath, 
            QStringList() << "comp" << portName);

    qDebug("%s: exit code %d", __FUNCTION__, exitCode);

    if (exitCode == 0)
        return true;
    else
        return false;
}

bool WinPcapPort::setExclusiveControl(bool exclusive) 
{
    QString portName(adapter_->Name + strlen("\\Device\\NPF_"));
    QString bindConfigFilePath(QCoreApplication::applicationDirPath()
                + "/bindconfig.exe");
    QString status;

    qDebug("%s: %s", __FUNCTION__, qPrintable(portName));

    if (!QFile::exists(bindConfigFilePath))
        return false;

    status = exclusive ? "disable" : "enable";

    QProcess::execute(bindConfigFilePath, 
            QStringList() << "comp" << portName << status);

    updateNotes(); 

    return (exclusive == hasExclusiveControl());
}

WinPcapPort::PortMonitor::PortMonitor(const char *device, Direction direction,
    AbstractPort::PortStats *stats)
    : PcapPort::PortMonitor(device, direction, stats)
{
    if (handle())
        pcap_setmode(handle(), MODE_STAT);
}

void WinPcapPort::PortMonitor::run()
{
    struct timeval lastTs;
    quint64 lastTxPkts = 0;
    quint64 lastTxBytes = 0;

    qDebug("in %s", __PRETTY_FUNCTION__);

    lastTs.tv_sec = 0;
    lastTs.tv_usec = 0;

    while (!stop_)
    {
        int ret;
        struct pcap_pkthdr *hdr;
        const uchar *data;

        ret = pcap_next_ex(handle(), &hdr, &data);
        switch (ret)
        {
            case 1:
            {
                quint64 pkts  = *((quint64*)(data + 0));
                quint64 bytes = *((quint64*)(data + 8));

                // TODO: is it 12 or 16?
                bytes -= pkts * 12;

                uint usec = (hdr->ts.tv_sec - lastTs.tv_sec) * 1000000 + 
                    (hdr->ts.tv_usec - lastTs.tv_usec);

                switch (direction())
                {
                case kDirectionRx:
                    stats_->rxPkts += pkts;
                    stats_->rxBytes += bytes;
                    stats_->rxPps = qRound64(pkts  * 1e6 / usec);
                    stats_->rxBps = qRound64(bytes * 1e6 / usec);
                    break;

                case kDirectionTx:
                    if (isDirectional())
                    {
                        stats_->txPkts += pkts;
                        stats_->txBytes += bytes;
                    }
                    else
                    {
                        // Assuming stats_->txXXX are updated externally
                        quint64 txPkts = stats_->txPkts;
                        quint64 txBytes = stats_->txBytes;

                        pkts = txPkts - lastTxPkts;
                        bytes = txBytes - lastTxBytes;

                        lastTxPkts = txPkts;
                        lastTxBytes = txBytes;
                    }
                    stats_->txPps = qRound64(pkts  * 1e6 / usec);
                    stats_->txBps = qRound64(bytes * 1e6 / usec);
                    break;

                default:
                    Q_ASSERT(false);
                }

                break;
            }
            case 0:
                //qDebug("%s: timeout. continuing ...", __PRETTY_FUNCTION__);
                continue;
            case -1:
                qWarning("%s: error reading packet (%d): %s", 
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle()));
				//oleg fix
				//qWarning("count %d: case (%d)",count, -1);
				QThread::msleep(5000);
                break;
            case -2:
                qWarning("%s: error reading packet (%d): %s", 
                        __PRETTY_FUNCTION__, ret, pcap_geterr(handle()));
                break;
            default:
                qFatal("%s: Unexpected return value %d", __PRETTY_FUNCTION__, ret);
        }
        lastTs.tv_sec  = hdr->ts.tv_sec;
        lastTs.tv_usec = hdr->ts.tv_usec;
        if (!stop_)
            QThread::msleep(1000);
    }
}

WinPcapPort::PortFullPackageMonitor::PortFullPackageMonitor(const char *device, Direction direction,
    AbstractPort::PortStats *stats, const WinPcapPort* port)
    : PcapPort::PortMonitor(device, direction, stats), port_(port)
{
}

//OLEG todo not implemented
void WinPcapPort::PortFullPackageMonitor::run()
{}



void WinPcapPort::populateInterfaceInfo()
{
    if (!adapterList_) {
        qWarning("Adapter List not available");
        return;
    }

    PIP_ADAPTER_ADDRESSES adapter = adapterList_;
    while (adapter && !QString(name()).endsWith(QString(adapter->AdapterName)))
        adapter = adapter->Next;

    if (!adapter) {
        qWarning("Adapter info not found for %s", name());
        return;
    }

    interfaceInfo_ = new InterfaceInfo;

    interfaceInfo_->speed = adapter->TransmitLinkSpeed != quint64(-1) ?
        adapter->TransmitLinkSpeed/1e6 : 0;
    interfaceInfo_->mtu = adapter->Mtu;

    if (adapter->PhysicalAddressLength == 6) {
        interfaceInfo_->mac = qFromBigEndian<quint64>(
                                    adapter->PhysicalAddress) >> 16;
    }
    else
        interfaceInfo_->mac = 0;

#define SOCKET_ADDRESS_FAMILY(x) \
    (x.lpSockaddr->sa_family)

#define SOCKET_ADDRESS_IP4(x) \
    (qFromBigEndian<quint32>(((sockaddr_in*)(x.lpSockaddr))->sin_addr.S_un.S_addr));

#define SOCKET_ADDRESS_IP6(x) \
    (UInt128(((PSOCKADDR_IN6)(x.lpSockaddr))->sin6_addr.u.Byte));

    // We may have multiple gateways - use the first for each family
    quint32 ip4Gateway = 0;
    PIP_ADAPTER_GATEWAY_ADDRESS gateway = adapter->FirstGatewayAddress;
    while (gateway) {
        if (SOCKET_ADDRESS_FAMILY(gateway->Address) == AF_INET) {
            ip4Gateway = SOCKET_ADDRESS_IP4(gateway->Address);
            break;
        }
        gateway = gateway->Next;
    }
    UInt128 ip6Gateway(0, 0);
    gateway = adapter->FirstGatewayAddress;
    while (gateway) {
        if (SOCKET_ADDRESS_FAMILY(gateway->Address) == AF_INET6) {
            ip6Gateway = SOCKET_ADDRESS_IP6(gateway->Address);
            break;
        }
        gateway = gateway->Next;
    }

    PIP_ADAPTER_UNICAST_ADDRESS ucast = adapter->FirstUnicastAddress;
    while (ucast) {
        if (SOCKET_ADDRESS_FAMILY(ucast->Address) == AF_INET) {
            Ip4Config ip;
            ip.address = SOCKET_ADDRESS_IP4(ucast->Address);
            ip.prefixLength = ucast->OnLinkPrefixLength;
            ip.gateway = ip4Gateway;
            interfaceInfo_->ip4.append(ip);
        }
        else if (SOCKET_ADDRESS_FAMILY(ucast->Address) == AF_INET6) {
            Ip6Config ip;
            ip.address = SOCKET_ADDRESS_IP6(ucast->Address);
            ip.prefixLength = ucast->OnLinkPrefixLength;
            ip.gateway = ip6Gateway;
            interfaceInfo_->ip6.append(ip);
        }
        ucast = ucast->Next;
    }
#undef SOCKET_ADDRESS_FAMILY
#undef SOCKET_ADDRESS_IP4
#undef SOCKET_ADDRESS_IP6
}

void WinPcapPort::fetchHostNetworkInfo()
{
    DWORD ret;
    ULONG bufLen = 15*1024; // MS recommended starting size

    while (1) {
        adapterList_ = (IP_ADAPTER_ADDRESSES *) malloc(bufLen);
        ret = GetAdaptersAddresses(AF_UNSPEC,
                                   GAA_FLAG_INCLUDE_ALL_INTERFACES
                                   | GAA_FLAG_INCLUDE_GATEWAYS,
                                   0, adapterList_, &bufLen);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            free(adapterList_);
            continue;
        }
        break;
    }

    if (ret != NO_ERROR) {
        free(adapterList_);
        adapterList_ = NULL;
        return;
    }
}

void WinPcapPort::freeHostNetworkInfo()
{
    free(adapterList_);
    adapterList_ = NULL;
}

WinPcapPort::PortCapturer::PortCapturer(const char *device, WinPcapPort* winPcapPort) :
    PcapPort::PortCapturer(device), m_winPcapPort(winPcapPort)
{
}

void WinPcapPort::PortCapturer::configCapture(pcap_t *handle_)
{
    m_winPcapPort->pcapFilterLocalAdapter(handle_);
}


#endif
