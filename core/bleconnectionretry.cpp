// SPDX-License-Identifier: GPL-2.0
#include "bleconnectionretry.h"

BleConnectRetryResult runBleConnectAttempts(
	const std::function<BleConnectAttemptResult(int)> &attempt,
	const std::function<bool()> &cancelled,
	const std::function<void()> &retryDelay)
{
	constexpr int maximumAttempts = 3;
	for (int attemptNumber = 1; attemptNumber <= maximumAttempts; ++attemptNumber) {
		if (cancelled())
			return { BleConnectAttemptResult::Cancelled, attemptNumber - 1 };
		BleConnectAttemptResult result = attempt(attemptNumber);
		if (result != BleConnectAttemptResult::TransientFailure || attemptNumber == maximumAttempts)
			return { result, attemptNumber };
		retryDelay();
		if (cancelled())
			return { BleConnectAttemptResult::Cancelled, attemptNumber };
	}
	return { BleConnectAttemptResult::PermanentFailure, maximumAttempts };
}
