// SPDX-License-Identifier: GPL-2.0
#ifndef BLECONNECTIONRETRY_H
#define BLECONNECTIONRETRY_H

#include <functional>

enum class BleConnectAttemptResult {
	Success,
	TransientFailure,
	PermanentFailure,
	Cancelled
};

struct BleConnectRetryResult {
	BleConnectAttemptResult result;
	int attempts;
};

BleConnectRetryResult runBleConnectAttempts(
	const std::function<BleConnectAttemptResult(int)> &attempt,
	const std::function<bool()> &cancelled,
	const std::function<void()> &retryDelay);

#endif
