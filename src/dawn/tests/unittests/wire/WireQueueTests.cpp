// Copyright 2021 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <memory>

#include "dawn/tests/unittests/wire/WireFutureTest.h"
#include "dawn/tests/unittests/wire/WireTest.h"
#include "dawn/wire/WireClient.h"
#include "gmock/gmock.h"

namespace dawn::wire {
namespace {

using testing::_;
using testing::InvokeWithoutArgs;
using testing::Return;

using WireQueueTestBase = WireFutureTest<WGPUQueueWorkDoneCallback,
                                         WGPUQueueWorkDoneCallbackInfo,
                                         wgpuQueueOnSubmittedWorkDone,
                                         wgpuQueueOnSubmittedWorkDoneF>;
class WireQueueTests : public WireQueueTestBase {
  protected:
    // Overriden version of wgpuQueueOnSubmittedWorkDone that defers to the API call based on the
    // test callback mode.
    void QueueOnSubmittedWorkDone(WGPUQueue q, void* userdata = nullptr) { CallImpl(userdata, q); }
};

DAWN_INSTANTIATE_WIRE_FUTURE_TEST_P(WireQueueTests);

// Test that a successful OnSubmittedWorkDone call is forwarded to the client.
TEST_P(WireQueueTests, OnSubmittedWorkDoneSuccess) {
    QueueOnSubmittedWorkDone(queue);
    EXPECT_CALL(api, OnQueueOnSubmittedWorkDone(apiQueue, _)).WillOnce(InvokeWithoutArgs([&] {
        api.CallQueueOnSubmittedWorkDoneCallback(apiQueue, WGPUQueueWorkDoneStatus_Success);
    }));
    FlushClient();
    FlushFutures();

    ExpectWireCallbacksWhen([&](auto& mockCb) {
        EXPECT_CALL(mockCb, Call(WGPUQueueWorkDoneStatus_Success, nullptr)).Times(1);

        FlushCallbacks();
    });
}

// Test that an error OnSubmittedWorkDone call is forwarded as an error to the client.
TEST_P(WireQueueTests, OnSubmittedWorkDoneError) {
    QueueOnSubmittedWorkDone(queue);
    EXPECT_CALL(api, OnQueueOnSubmittedWorkDone(apiQueue, _)).WillOnce(InvokeWithoutArgs([&] {
        api.CallQueueOnSubmittedWorkDoneCallback(apiQueue, WGPUQueueWorkDoneStatus_Error);
    }));
    FlushClient();
    FlushFutures();

    ExpectWireCallbacksWhen([&](auto& mockCb) {
        EXPECT_CALL(mockCb, Call(WGPUQueueWorkDoneStatus_Error, nullptr)).Times(1);

        FlushCallbacks();
    });
}

// Test registering an OnSubmittedWorkDone then disconnecting the wire after the server responded to
// the client will call the callback with instance dropped.
TEST_P(WireQueueTests, OnSubmittedWorkDoneBeforeDisconnectAfterReply) {
    // On Async and Spontaneous mode, it is not possible to simulate this because on the server
    // reponse, the callback would also be fired.
    DAWN_SKIP_TEST_IF(IsSpontaneous());

    QueueOnSubmittedWorkDone(queue);
    EXPECT_CALL(api, OnQueueOnSubmittedWorkDone(apiQueue, _)).WillOnce(InvokeWithoutArgs([&] {
        api.CallQueueOnSubmittedWorkDoneCallback(apiQueue, WGPUQueueWorkDoneStatus_Error);
    }));
    FlushClient();
    FlushFutures();

    ExpectWireCallbacksWhen([&](auto& mockCb) {
        EXPECT_CALL(mockCb, Call(WGPUQueueWorkDoneStatus_InstanceDropped, nullptr)).Times(1);

        GetWireClient()->Disconnect();
    });
}

// Test registering an OnSubmittedWorkDone then disconnecting the wire before the server responded
// to the client (i.e. before the event was ever ready) will call the callback with instance
// dropped.
TEST_P(WireQueueTests, OnSubmittedWorkDoneBeforeDisconnectBeforeReply) {
    QueueOnSubmittedWorkDone(queue);
    EXPECT_CALL(api, OnQueueOnSubmittedWorkDone(apiQueue, _)).WillOnce(InvokeWithoutArgs([&] {
        api.CallQueueOnSubmittedWorkDoneCallback(apiQueue, WGPUQueueWorkDoneStatus_Error);
    }));
    FlushClient();

    ExpectWireCallbacksWhen([&](auto& mockCb) {
        EXPECT_CALL(mockCb, Call(WGPUQueueWorkDoneStatus_InstanceDropped, nullptr)).Times(1);

        GetWireClient()->Disconnect();
    });
}

// Test registering an OnSubmittedWorkDone after disconnecting the wire calls the callback with
// success.
TEST_P(WireQueueTests, OnSubmittedWorkDoneAfterDisconnect) {
    GetWireClient()->Disconnect();

    ExpectWireCallbacksWhen([&](auto& mockCb) {
        EXPECT_CALL(mockCb, Call(WGPUQueueWorkDoneStatus_InstanceDropped, nullptr)).Times(1);

        QueueOnSubmittedWorkDone(queue);
    });
}

// Test that requests inside user callbacks before disconnect are called
TEST_P(WireQueueTests, OnSubmittedWorkDoneInsideCallbackBeforeDisconnect) {
    static constexpr size_t kNumRequests = 10;
    QueueOnSubmittedWorkDone(queue);
    EXPECT_CALL(api, OnQueueOnSubmittedWorkDone(apiQueue, _)).WillOnce(InvokeWithoutArgs([&] {
        api.CallQueueOnSubmittedWorkDoneCallback(apiQueue, WGPUQueueWorkDoneStatus_Error);
    }));
    FlushClient();

    ExpectWireCallbacksWhen([&](auto& mockCb) {
        EXPECT_CALL(mockCb, Call(WGPUQueueWorkDoneStatus_InstanceDropped, nullptr))
            .Times(kNumRequests + 1)
            .WillOnce([&]() {
                for (size_t i = 0; i < kNumRequests; i++) {
                    QueueOnSubmittedWorkDone(queue);
                }
            })
            .WillRepeatedly(Return());

        GetWireClient()->Disconnect();
    });
}

// Test releasing the default queue, then its device. Both should be released when the device is
// released since the device holds a reference to the queue. Regresssion test for crbug.com/1332926.
TEST_F(WireQueueTests, DefaultQueueThenDeviceReleased) {
    // Note: The test fixture gets the default queue.

    // Release the queue which is the last external client reference.
    // The device still holds a reference.
    wgpuQueueRelease(queue);
    FlushClient();

    // Release the device which holds an internal reference to the queue.
    // Now, the queue and device should be released on the server.
    wgpuDeviceRelease(device);

    EXPECT_CALL(api, QueueRelease(apiQueue));
    EXPECT_CALL(api, DeviceRelease(apiDevice));
    // These set X callback methods are called before the device is released.
    EXPECT_CALL(api, OnDeviceSetLoggingCallback(apiDevice, nullptr, nullptr)).Times(1);
    FlushClient();

    // Indicate to the fixture that the device was already released.
    DefaultApiDeviceWasReleased();
}

// Test the device, then its default queue. The default queue should be released when its external
// reference is dropped since releasing the device drops the internal reference. Regresssion test
// for crbug.com/1332926.
TEST_F(WireQueueTests, DeviceThenDefaultQueueReleased) {
    // Note: The test fixture gets the default queue.

    // Release the device which holds an internal reference to the queue.
    // Now, the should be released on the server, but not the queue since
    // the default queue still has one external reference.
    wgpuDeviceRelease(device);

    EXPECT_CALL(api, DeviceRelease(apiDevice));
    // These set X callback methods are called before the device is released.
    EXPECT_CALL(api, OnDeviceSetLoggingCallback(apiDevice, nullptr, nullptr)).Times(1);
    FlushClient();

    // Release the external queue reference. The queue should be released.
    wgpuQueueRelease(queue);
    EXPECT_CALL(api, QueueRelease(apiQueue));
    FlushClient();

    // Indicate to the fixture that the device was already released.
    DefaultApiDeviceWasReleased();
}

// Only one default queue is supported now so we cannot test ~Queue triggering ClearAllCallbacks
// since it is always destructed after the test TearDown, and we cannot create a new queue obj
// with wgpuDeviceGetQueue

}  // anonymous namespace
}  // namespace dawn::wire
