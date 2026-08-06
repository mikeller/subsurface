// SPDX-License-Identifier: GPL-2.0
#ifndef BTDISCOVERY_H
#define BTDISCOVERY_H

#include <QObject>
#include <QString>
#include <QLoggingCategory>
#include <QAbstractListModel>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothUuid>
#include "core/libdivecomputer.h"
#include "core/bluetoothaddress.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QJniEnvironment>
#endif

using BtDiscoveryGeneration = quint64;

BtDiscoveryGeneration beginBtDeviceInfoDiscovery();
void saveBtDeviceInfo(const QString &devaddr, QBluetoothDeviceInfo deviceInfo,
			 BtDiscoveryGeneration generation = 0);
bool hasBtDeviceInfo(const QString &devaddr, BtDiscoveryGeneration minimumGeneration = 0);
bool btDeviceInfoNeedsDiscovery(const QString &devaddr);
BtDiscoveryGeneration btDeviceInfoGeneration(const QString &devaddr);
void invalidateBtDeviceInfo(const QString &devaddr);
bool matchesKnownDiveComputerNames(QString btName);
QBluetoothDeviceInfo getBtDeviceInfo(const QString &devaddr,
				     BtDiscoveryGeneration minimumGeneration = 0);
QString btDeviceAddress(const QBluetoothDeviceInfo *device, bool isBle);
QString btDeviceAddressForAuto(const QBluetoothDeviceInfo *device);

class BTDiscovery : public QObject {
	Q_OBJECT

public:
	BTDiscovery(QObject *parent = NULL);
	~BTDiscovery();
	static BTDiscovery *instance();

	struct btPairedDevice {
		QString address;
		QString name;
	};

	struct btVendorProduct {
		btPairedDevice btpdi;
		dc_descriptor_t *dcDescriptor;
		int vendorIdx;
		int productIdx;
	};

	void btDeviceDiscoveryFinished();
	void btDeviceDiscovered(const QBluetoothDeviceInfo &device);
	void btDeviceDiscoveredMain(const btPairedDevice &device, bool fromPaired);
	bool btAvailable() const;
	void showNonDiveComputers(bool show);
	void stopAgent();

#if defined(Q_OS_ANDROID)
	void getBluetoothDevices();
#endif
	QList<btVendorProduct> getBtDcs();
	QBluetoothLocalDevice localBtDevice;
	void BTDiscoveryReDiscover();
	void discoverAddress(QString address);

private:
	static BTDiscovery *m_instance;
	bool m_btValid;
	bool m_showNonDiveComputers;

	QList<struct btVendorProduct> btDCs;		// recognized DCs
	QList<struct btVendorProduct> btAllDevices;	// all paired BT stuff

#if defined(Q_OS_ANDROID)
	bool checkException(const char* method, const QJniObject* obj);
#endif

	QList<struct btPairedDevice> btPairedDevices;
	QBluetoothDeviceDiscoveryAgent *discoveryAgent;
	BtDiscoveryGeneration discoveryGeneration = 0;

signals:
	void dcVendorChanged();
	void dcProductChanged();
	void dcBtChanged();
};
#endif // BTDISCOVERY_H
