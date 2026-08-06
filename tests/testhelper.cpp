// SPDX-License-Identifier: GPL-2.0
#include "testhelper.h"
#include "core/bluetoothaddress.h"
#include "core/btdiscovery.h"
#include "core/bleconnectionretry.h"
#include <libdivecomputer/descriptor.h>

void TestHelper::initTestCase()
{
	TestBase::initTestCase();
}

void TestHelper::recognizeBtAddress()
{
	QCOMPARE(isBluetoothAddress("01:a2:b3:c4:d5:06"), true);
	QCOMPARE(isBluetoothAddress("LE:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothAddress("BT:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothAddress("BT:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothLowEnergyAddress("LE:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothClassicAddress("BT:01:A2:B3:C4:D5:06"), true);
	QCOMPARE(isBluetoothClassicAddress("BT:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(bluetoothAddressWithoutPrefix("BT:01:A2:B3:C4:D5:06"), QString("01:A2:B3:C4:D5:06"));
	QCOMPARE(bluetoothAddressWithoutPrefix("LE:01:A2:B3:C4:D5:06"), QString("01:A2:B3:C4:D5:06"));
	QCOMPARE(isBluetoothAddress("01A2B3C4D506"), true);
	QCOMPARE(isBluetoothAddress("01-A2-B3-C4-D5-06"), true);
	QCOMPARE(isBluetoothAddress("01:A2:b3:04:05"), false);
	QCOMPARE(isBluetoothAddress("LE:01:02:03:04:05"), false);
	QCOMPARE(isBluetoothAddress("01:02:03:04:051:67"), false);
	QCOMPARE(isBluetoothAddress("LE:01:g2:03:04:05"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), true);
	QCOMPARE(isBluetoothAddress("LE:{6e5ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdda33-4c43-a80a-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-1ed4c7d2d2a5}"), false);
	QCOMPARE(isBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-ag0a-1ed4c7d2d2a5}"), false);
}

void TestHelper::parseNameAddress()
{
	QString name, address;
	std::tie(address, name) = extractBluetoothNameAddress("01:a2:b3:c4:d5:06");
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("  01:a2:b3:c4:d5:06  ");
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("somename (01:a2:b3:c4:d5:06)");
	QCOMPARE(address, QString("01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString("somename"));
	std::tie(address, name) = extractBluetoothNameAddress("garbage");
	QCOMPARE(address, QString());
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("somename (garbage)");
	QCOMPARE(address, QString());
	QCOMPARE(name, QString());
	std::tie(address, name) = extractBluetoothNameAddress("somename (LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5})");
	QCOMPARE(address, QString("LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"));
	QCOMPARE(name, QString("somename"));
	std::tie(address, name) = extractBluetoothNameAddress("somename (BT:01:a2:b3:c4:d5:06)");
	QCOMPARE(address, QString("BT:01:a2:b3:c4:d5:06"));
	QCOMPARE(name, QString("somename"));

}

void TestHelper::automaticBluetoothAddress()
{
	QBluetoothDeviceInfo macDevice(QBluetoothAddress("01:A2:B3:C4:D5:06"), "test", 0);
	macDevice.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
	QCOMPARE(btDeviceAddressForAuto(&macDevice), QString("01:A2:B3:C4:D5:06"));

	QBluetoothUuid uuid(QStringLiteral("{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"));
	QBluetoothDeviceInfo uuidDevice(uuid, "test", 0);
	QCOMPARE(btDeviceAddressForAuto(&uuidDevice), QString("LE:") + uuid.toString());
}

void TestHelper::canonicalBluetoothCacheKey()
{
	QCOMPARE(canonicalBluetoothAddress("01:a2:b3:c4:d5:06"), QString("01:A2:B3:C4:D5:06"));
	QCOMPARE(canonicalBluetoothAddress("LE:01-a2-b3-c4-d5-06"), QString("01:A2:B3:C4:D5:06"));
	QCOMPARE(canonicalBluetoothAddress("Mares Quad Air (LE:01:a2:b3:c4:d5:06)"),
		 QString("01:A2:B3:C4:D5:06"));
	QCOMPARE(canonicalBluetoothAddress("LE:{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"),
		 QString("{6e50ff5d-cdd3-4c43-a80a-1ed4c7d2d2a5}"));

	QBluetoothDeviceInfo device(QBluetoothAddress("01:A2:B3:C4:D5:06"), "test", 0);
	invalidateBtDeviceInfo("01:A2:B3:C4:D5:06");
	saveBtDeviceInfo("01:A2:B3:C4:D5:06", device);
	QVERIFY(hasBtDeviceInfo("test device (LE:01:a2:b3:c4:d5:06)"));
	QVERIFY(!btDeviceInfoNeedsDiscovery("test device (LE:01:a2:b3:c4:d5:06)"));
	invalidateBtDeviceInfo("LE:01:A2:B3:C4:D5:06");
}

void TestHelper::bluetoothCacheFreshness()
{
	const QString address = "02:A2:B3:C4:D5:06";
	QBluetoothDeviceInfo oldDevice(QBluetoothAddress(address), "old", 0);
	QBluetoothDeviceInfo newDevice(QBluetoothAddress(address), "new", 0);
	invalidateBtDeviceInfo(address);
	BtDiscoveryGeneration oldGeneration = beginBtDeviceInfoDiscovery();
	saveBtDeviceInfo(address, oldDevice, oldGeneration);
	BtDiscoveryGeneration freshGeneration = beginBtDeviceInfoDiscovery();
	QVERIFY(!hasBtDeviceInfo("LE:" + address, freshGeneration));
	saveBtDeviceInfo("LE:" + address, newDevice, freshGeneration);
	QVERIFY(hasBtDeviceInfo(address, freshGeneration));
	QCOMPARE(getBtDeviceInfo(address, freshGeneration).name(), QString("new"));
	invalidateBtDeviceInfo(address);
}

void TestHelper::boundedBleRetry()
{
	int controllersCreated = 0;
	int freshDiscoveries = 0;
	BleConnectRetryResult failed = runBleConnectAttempts(
		[&](int) {
			++controllersCreated;
			++freshDiscoveries;
			return BleConnectAttemptResult::TransientFailure;
		}, [] { return false; }, [] {});
	QCOMPARE(failed.attempts, 3);
	QCOMPARE(controllersCreated, 3);
	QCOMPARE(freshDiscoveries, 3);

	controllersCreated = 0;
	freshDiscoveries = 0;
	BleConnectRetryResult succeeded = runBleConnectAttempts(
		[&](int attempt) {
			++controllersCreated;
			++freshDiscoveries;
			return attempt == 2 ? BleConnectAttemptResult::Success :
				BleConnectAttemptResult::TransientFailure;
		}, [] { return false; }, [] {});
	QCOMPARE(succeeded.attempts, 2);
	QCOMPARE(controllersCreated, 2);
	QCOMPARE(freshDiscoveries, 2);
}

void TestHelper::cancelledBleRetry()
{
	int attempts = 0;
	bool cancelled = false;
	BleConnectRetryResult result = runBleConnectAttempts(
		[&](int) {
			++attempts;
			return BleConnectAttemptResult::TransientFailure;
		}, [&] { return cancelled; }, [&] { cancelled = true; });
	QCOMPARE(result.result, BleConnectAttemptResult::Cancelled);
	QCOMPARE(attempts, 1);
}

void TestHelper::bluetoothTransportModes()
{
	QVERIFY(isBluetoothLowEnergyAddress("LE:01:A2:B3:C4:D5:06"));
	QVERIFY(!isBluetoothClassicAddress("LE:01:A2:B3:C4:D5:06"));
	QVERIFY(isBluetoothClassicAddress("BT:01:A2:B3:C4:D5:06"));
	QVERIFY(!isBluetoothLowEnergyAddress("BT:01:A2:B3:C4:D5:06"));
	QVERIFY(!isBluetoothLowEnergyAddress("01:A2:B3:C4:D5:06"));
	QVERIFY(!isBluetoothClassicAddress("01:A2:B3:C4:D5:06"));
}

void TestHelper::maresQuadAirTransports()
{
	dc_iterator_t *iterator = nullptr;
	QVERIFY(dc_descriptor_iterator(&iterator) == DC_STATUS_SUCCESS);
	dc_descriptor_t *descriptor = nullptr;
	unsigned int transports = 0;
	while (dc_iterator_next(iterator, &descriptor) == DC_STATUS_SUCCESS) {
		if (QString(dc_descriptor_get_vendor(descriptor)) == "Mares" &&
		    QString(dc_descriptor_get_product(descriptor)) == "Quad Air") {
			transports = dc_descriptor_get_transports(descriptor);
			dc_descriptor_free(descriptor);
			break;
		}
		dc_descriptor_free(descriptor);
	}
	dc_iterator_free(iterator);
	QVERIFY(transports & DC_TRANSPORT_SERIAL);
	QVERIFY(transports & DC_TRANSPORT_BLE);
	QVERIFY(!(transports & DC_TRANSPORT_BLUETOOTH));
	QCOMPARE(transports & (DC_TRANSPORT_BLUETOOTH | DC_TRANSPORT_BLE),
		 static_cast<unsigned int>(DC_TRANSPORT_BLE));
	QCOMPARE(transports & DC_TRANSPORT_BLUETOOTH, 0U);
	QCOMPARE(transports & DC_TRANSPORT_BLE, static_cast<unsigned int>(DC_TRANSPORT_BLE));
}

QTEST_GUILESS_MAIN(TestHelper)
