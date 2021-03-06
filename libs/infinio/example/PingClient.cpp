/*
 * (C) Copyright 2015 ETH Zurich Systems Group (http://www.systems.ethz.ch/) and others.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors:
 *     Markus Pilman <mpilman@inf.ethz.ch>
 *     Simon Loesing <sloesing@inf.ethz.ch>
 *     Thomas Etter <etterth@gmail.com>
 *     Kevin Bocksrocker <kevin.bocksrocker@gmail.com>
 *     Lucas Braun <braunl@inf.ethz.ch>
 */
#include <crossbow/infinio/Endpoint.hpp>
#include <crossbow/infinio/InfinibandService.hpp>
#include <crossbow/infinio/InfinibandSocket.hpp>
#include <crossbow/program_options.hpp>
#include <crossbow/string.hpp>

#include <chrono>
#include <iostream>
#include <system_error>

using namespace crossbow::infinio;

class PingConnection: private InfinibandSocketHandler {
public:
    PingConnection(InfinibandService& service, const crossbow::string& server, uint16_t port, uint64_t maxSend)
            : mService(service),
              mProcessor(mService.createProcessor()),
              mSocket(mService.createSocket(*mProcessor)),
              mMaxSend(maxSend),
              mSend(0) {
        Endpoint ep(Endpoint::ipv4(), server, port);
        mSocket->open();
        mSocket->setHandler(this);
        mSocket->connect(ep, "PingClient");
        std::cout << "Connecting to server" << std::endl;
    }

private:
    virtual void onConnected(const crossbow::string& data, const std::error_code& ec) override;

    virtual void onReceive(const void* buffer, size_t length, const std::error_code& ec) override;

    virtual void onDisconnect() override;

    virtual void onDisconnected() override;

    void handleError(std::string message, const std::error_code& ec);

    void sendMessage();

    InfinibandService& mService;
    std::unique_ptr<InfinibandProcessor> mProcessor;

    InfinibandSocket mSocket;

    uint64_t mMaxSend;
    uint64_t mSend;
};

void PingConnection::onConnected(const crossbow::string& data, const std::error_code& ec) {
    if (ec) {
        std::cout << "Connect failed " << ec << " - " << ec.message() << std::endl;
        return;
    }
    std::cout << "Connected [data = \"" << data << "\"]" << std::endl;

    sendMessage();
}

void PingConnection::onReceive(const void* buffer, size_t length, const std::error_code& /* ec */) {
    // Calculate RTT
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto start = std::chrono::nanoseconds(*reinterpret_cast<const uint64_t*>(buffer));
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now) - start;
    std::cout << "RTT is " << duration.count() << "ns" << std::endl;

    if (mSend < mMaxSend) {
        sendMessage();
    } else {
        std::cout << "Disconnecting after " << mSend << " pings" << std::endl;
        try {
            mSocket->disconnect();
        } catch (std::system_error& e) {
            std::cout << "Disconnect failed " << e.code() << " - " << e.what() << std::endl;
        }
    }
}

void PingConnection::onDisconnect() {
    std::cout << "Disconnect" << std::endl;
    try {
        mSocket->disconnect();
    } catch (std::system_error& e) {
        std::cout << "Disconnect failed " << e.code() << " - " << e.what() << std::endl;
    }
}

void PingConnection::onDisconnected() {
    std::cout << "Disconnected" << std::endl;
    try {
        mSocket->close();
    } catch (std::system_error& e) {
        std::cout << "Close failed " << e.code() << " - " << e.what() << std::endl;
    }

    std::cout << "Stopping ping client" << std::endl;
    mService.shutdown();
}

void PingConnection::handleError(std::string message, const std::error_code& ec) {
    std::cout << message << " [" << ec << " - " << ec.message() << "]" << std::endl;
    std::cout << "Disconnecting after error" << std::endl;
    try {
        mSocket->disconnect();
    } catch (std::system_error& e) {
        std::cout << "Disconnect failed " << e.code() << " - " << e.what() << std::endl;
    }
}

void PingConnection::sendMessage() {
    // Acquire buffer
    auto sbuffer = mSocket->acquireSendBuffer(8);
    if (sbuffer.id() == InfinibandBuffer::INVALID_ID) {
        handleError("Error acquiring buffer", error::invalid_buffer);
        return;
    }

    // Increment send counter
    ++mSend;

    // Write time since epoch in nano seconds to buffer
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    *reinterpret_cast<uint64_t*>(sbuffer.data()) = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

    // Send message to server
    std::error_code ec;
    mSocket->send(sbuffer, 0x0u, ec);
    if (ec) {
        handleError("Send failed", ec);
    }
}

int main(int argc, const char** argv) {
    bool help = false;
    int count = 5;
    crossbow::string server;
    uint16_t port = 4488;
    auto opts = crossbow::program_options::create_options(argv[0],
            crossbow::program_options::value<'h'>("help", &help),
            crossbow::program_options::value<'c'>("count", &count),
            crossbow::program_options::value<'s'>("server", &server),
            crossbow::program_options::value<'p'>("port", &port));

    try {
        crossbow::program_options::parse(opts, argc, argv);
    } catch (crossbow::program_options::argument_not_found e) {
        std::cerr << e.what() << std::endl << std::endl;
        crossbow::program_options::print_help(std::cout, opts);
        return 1;
    }

    if (help) {
        crossbow::program_options::print_help(std::cout, opts);
        return 0;
    }

    std::cout << "Starting ping client" << std::endl;
    InfinibandService service;
    PingConnection con(service, server, port, count);

    service.run();

    return 0;
}
