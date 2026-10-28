/**
 * @file client.hpp
 * @brief 控制盒 gRPC 客户端（一元 RPC + RobotRtStream）。
 * @author ehow
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "dalynce/sdk/export.hpp"
#include "dalynce/sdk/rt_session.hpp"
#include "dalynce/sdk/types.hpp"

namespace dalynce::sdk {

/**
 * @brief 一个实例对应一条 Insecure gRPC channel。
 * @author ehow
 */
class DALYNCE_SDK_API Client {
public:
    Client();
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    /**
     * @brief 连接控制盒。
     * @param target 形如 `192.168.1.10:50051`
     */
    Result<void> connect(const std::string& target);

    /** @brief 释放 channel / stub。 */
    void shutdown();

    /**
     * @brief 查询盒端版本与 uptime。
     * @return 成功时 value 为 HealthInfo
     */
    Result<HealthInfo> health();

    /**
     * @brief 列出已创建设备（含 YAML 预创建）。
     */
    Result<std::vector<DeviceInfo>> listDevices();

    /**
     * @brief 动态创建设备。YAML 已有同名 id 时盒端会失败，不要重复创建。
     */
    Result<DeviceHandle> createDevice(const std::string& id,
                                      const std::string& type,
                                      const std::string& driver,
                                      const std::string& params_json = {});

    /** @brief 销毁设备（会先 detach RT 流）。 */
    Result<void> destroyDevice(const std::string& id);

    /** @brief 连接厂商/TCP。gRPC OK 时仍要看 CommandResponse.code。 */
    Result<void> connectDevice(const std::string& id);

    /** @brief 断开设备（先 detach 流）。 */
    Result<void> disconnectDevice(const std::string& id);

    /** @brief 使能。Aubo 可能十几秒。 */
    Result<void> enable(const std::string& id);

    /** @brief 下使能（先 detach 流）。 */
    Result<void> disable(const std::string& id);

    /** @brief 紧急停止该设备 RT 环。 */
    Result<void> emergencyStop(const std::string& id);

    /**
     * @brief 无 RT 流时的相对点动。流占用返回 1005；AGV 返回 5001。
     */
    Result<void> moveJoint(const std::string& id, const std::vector<double>& delta);

    /**
     * @brief 打开 RobotRtStream。成功后由调用方立刻 write，第一帧完成 attach。
     * @note 回调在反馈线程；不要在回调里 disable / stop。
     */
    Result<std::unique_ptr<RtSession>> startRtStream(const std::string& device_id,
                                                     FeedbackCallback on_feedback);

private:
    class Impl;
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
