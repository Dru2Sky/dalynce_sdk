/**
 * @file types.hpp
 * @brief SDK 公共类型：错误码、结果、设备与反馈。
 * @author ehow
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "dalynce/sdk/export.hpp"

namespace dalynce::sdk {

/**
 * @brief 与盒端 CommandResponse.code 对齐；9001 仅 SDK 侧 gRPC 失败。
 * @author ehow
 */
enum class ErrorCode : int {
    Ok = 0,
    InvalidArgument = 1001,
    NotConnected = 1002,
    Timeout = 1003,
    NotFound = 1004,
    AlreadyExists = 1005,
    Internal = 1006,
    CommError = 2001,
    VendorError = 3001,
    RtCycleError = 4001,
    NotSupported = 5001,
    Grpc = 9001,
};

/**
 * @brief 带值的调用结果。
 * @author ehow
 */
template <typename T>
struct Result {
    ErrorCode code{ErrorCode::Ok};
    std::string message{"ok"};
    T value{};

    Result() = default;
    Result(ErrorCode code, std::string message)
        : code(code), message(std::move(message))
    {
    }

    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Ok; }
};

/**
 * @brief 无返回值的调用结果。
 * @author ehow
 */
template <>
struct Result<void> {
    ErrorCode code{ErrorCode::Ok};
    std::string message{"ok"};

    Result() = default;
    Result(ErrorCode code, std::string message)
        : code(code), message(std::move(message))
    {
    }

    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Ok; }
};

/**
 * @brief CreateDevice 返回的句柄。
 * @author ehow
 */
struct DeviceHandle {
    std::string id;
    std::uint64_t serial{};
};

/**
 * @brief ListDevices 中的一条设备。
 * @author ehow
 */
struct DeviceInfo {
    std::string id;
    std::uint64_t serial{};
    std::string type;
    std::string driver;
    std::string status;
};

/**
 * @brief Health RPC 结果。
 * @author ehow
 */
struct HealthInfo {
    std::string version;
    std::int64_t uptime_ms{};
    std::string status;
};

/**
 * @brief RobotRtStream 反馈（臂=关节；车=位姿，维数可能为 3）。
 * @author ehow
 */
struct RtFeedback {
    std::string device_id;
    std::uint64_t sequence{};
    std::int64_t timestamp_ns{};
    std::vector<double> values;
    std::string status;
};

using FeedbackCallback = std::function<void(const RtFeedback&)>;

}  // namespace dalynce::sdk
