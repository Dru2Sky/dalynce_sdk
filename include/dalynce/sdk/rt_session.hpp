/**
 * @file rt_session.hpp
 * @brief RobotRtStream 会话：40ms 写目标，后台读反馈。
 * @author ehow
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "dalynce/sdk/export.hpp"
#include "dalynce/sdk/types.hpp"

namespace dalynce::sdk {

class Client;

/**
 * @brief 每设备同时只能有一条流。析构会 stop。
 * @author ehow
 */
class DALYNCE_SDK_API RtSession {
public:
    RtSession(const RtSession&) = delete;
    RtSession& operator=(const RtSession&) = delete;
    RtSession(RtSession&&) noexcept;
    RtSession& operator=(RtSession&&) noexcept;
    ~RtSession();

    /**
     * @brief 写入一帧关节/速度目标，sequence 由会话自增（从 1 起）。
     */
    Result<void> write(const std::vector<double>& joint_positions,
                       std::int64_t timestamp_ns = 0);

    /**
     * @brief 指定 sequence 写入（测丢包）。
     */
    Result<void> writeExplicit(std::uint64_t sequence,
                               const std::vector<double>& joint_positions,
                               std::int64_t timestamp_ns = 0);

    /** @brief WritesDone + 读完反馈 + Finish。 */
    Result<void> stop();

    [[nodiscard]] const std::string& deviceId() const noexcept;
    [[nodiscard]] bool running() const noexcept;

private:
    friend class Client;
    class Impl;
    explicit RtSession(std::unique_ptr<Impl> impl);
    static Result<std::unique_ptr<RtSession>> open(void* stub,
                                                   std::string device_id,
                                                   FeedbackCallback on_feedback);
#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4251)  // pimpl unique_ptr across DLL boundary
#endif
    std::unique_ptr<Impl> impl_;
#if defined(_MSC_VER)
#    pragma warning(pop)
#endif
};

}  // namespace dalynce::sdk
