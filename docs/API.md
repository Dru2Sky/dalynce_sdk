# dalynce_sdk 接口说明

> 公共 API：命名空间 `dalynce::sdk`。头文件在 `include/dalynce/sdk/`。

## 1. 头文件


| 头文件                          | 内容                               |
| ---------------------------- | -------------------------------- |
| `dalynce/sdk/version.hpp`    | 版本宏 `DALYNCE_SDK_VERSION_*`      |
| `dalynce/sdk/export.hpp`     | DLL 导出宏（用户一般不直接用）                |
| `dalynce/sdk/types.hpp`      | `ErrorCode` / `Result` / 设备与反馈类型 |
| `dalynce/sdk/rt_session.hpp` | `RtSession`（实时流）                 |
| `dalynce/sdk/client.hpp`     | `Client`（连接 + unary + 开流）        |


典型：

```cpp
#include <dalynce/sdk/client.hpp>
#include <dalynce/sdk/version.hpp>
```

---



## 2. 结果与错误



### 2.1 `ErrorCode`


| 枚举                | 值    | 含义                             |
| ----------------- | ---- | ------------------------------ |
| `Ok`              | 0    | 成功                             |
| `InvalidArgument` | 1001 | 参数非法                           |
| `NotConnected`    | 1002 | 未连接盒端或设备                       |
| `Timeout`         | 1003 | 超时                             |
| `NotFound`        | 1004 | 设备不存在                          |
| `AlreadyExists`   | 1005 | 已存在 / RT 流占用（如开流中 `moveJoint`） |
| `Internal`        | 1006 | 内部错误                           |
| `CommError`       | 2001 | 通信错误                           |
| `VendorError`     | 3001 | 厂商侧错误                          |
| `RtCycleError`    | 4001 | 实时环错误                          |
| `NotSupported`    | 5001 | 不支持（如 AGV 的 `moveJoint`）       |
| `Grpc`            | 9001 | gRPC 传输层失败（SDK 侧）              |




### 2.2 `Result<T>` / `Result<void>`


| 成员        | 说明                      |
| --------- | ----------------------- |
| `code`    | `ErrorCode`             |
| `message` | 文本说明                    |
| `value`   | 仅 `Result<T>`；成功时有效     |
| `ok()`    | `code == ErrorCode::Ok` |


Unary：gRPC 失败 → `Grpc`；gRPC OK 则填盒端 `CommandResponse.code`。

```cpp
auto r = client.health();
if (!r.ok()) {
    // r.code, r.message
    return;
}
std::string ver = r.value.version;
```

---



## 3. 数据类型



### `DeviceHandle`


| 字段       | 类型         | 说明    |
| -------- | ---------- | ----- |
| `id`     | `string`   | 设备 id |
| `serial` | `uint64_t` | 盒端序列号 |




### `DeviceInfo`


| 字段       | 类型         | 说明                                           |
| -------- | ---------- | -------------------------------------------- |
| `id`     | `string`   | 设备 id                                        |
| `serial` | `uint64_t` | 序列号                                          |
| `type`   | `string`   | 通常 `"robot"`                                 |
| `driver` | `string`   | `"sim"` / `"aubo"` / `"agv"` / `"sim_agv"` 等 |
| `status` | `string`   | 盒端状态字符串                                      |




### `HealthInfo`


| 字段          | 类型        | 说明   |
| ----------- | --------- | ---- |
| `version`   | `string`  | 盒端版本 |
| `uptime_ms` | `int64_t` | 运行毫秒 |
| `status`    | `string`  | 健康状态 |




### `RtFeedback`


| 字段             | 类型               | 说明                           |
| -------------- | ---------------- | ---------------------------- |
| `device_id`    | `string`         | 设备 id                        |
| `sequence`     | `uint64_t`       | 反馈序号                         |
| `timestamp_ns` | `int64_t`        | 时间戳（纳秒）                      |
| `values`       | `vector<double>` | 臂：6 关节（rad）；车：常为 `[x,y,yaw]` |
| `status`       | `string`         | 状态                           |




### `FeedbackCallback`

```cpp
using FeedbackCallback = std::function<void(const RtFeedback&)>;
```

在**反馈线程**调用；勿在回调里 `disable` / `stop` / 重入开流。

---



## 4. `Client`

一个实例对应一条 Insecure gRPC channel。多设备可共用一个 `Client`，每设备最多一条 `RtSession`。

### 4.1 连接


| 方法         | 签名                                                | 说明                                 |
| ---------- | ------------------------------------------------- | ---------------------------------- |
| `connect`  | `Result<void> connect(const std::string& target)` | `host:port`，如 `192.168.1.13:50051` |
| `shutdown` | `void shutdown()`                                 | 释放 channel / stub                  |




### 4.2 查询


| 方法            | 返回                           | 说明           |
| ------------- | ---------------------------- | ------------ |
| `health`      | `Result<HealthInfo>`         | 盒端版本与 uptime |
| `listDevices` | `Result<vector<DeviceInfo>>` | 含 YAML 预创建设备 |




### 4.3 设备生命周期


| 方法                                               | 说明                        |
| ------------------------------------------------ | ------------------------- |
| `createDevice(id, type, driver, params_json="")` | 动态创建；YAML 已有同名 id 时不要重复创建 |
| `destroyDevice(id)`                              | 销毁（会先 detach RT 流）        |
| `connectDevice(id)`                              | 连接厂商/TCP                  |
| `disconnectDevice(id)`                           | 断开（先 detach 流）            |
| `enable(id)`                                     | 使能；Aubo 可能十余秒             |
| `disable(id)`                                    | 下使能（先 detach 流）           |
| `emergencyStop(id)`                              | 急停该设备 RT 环                |


推荐顺序：

```text
Health → ListDevices
  → [CreateDevice 仅 sim / 动态]
  → ConnectDevice → Enable
  → startRtStream → write循环
  → stop → Disable → Disconnect
  → [DestroyDevice]
```



### 4.4 `moveJoint`

```cpp
Result<void> moveJoint(const std::string& id, const std::vector<double>& delta);
```

- 无 RT 流时的相对点动  
- 开流中调用 → 常返回 `AlreadyExists` (1005)  
- AGV → `NotSupported` (5001)



### 4.5 `startRtStream`

```cpp
Result<std::unique_ptr<RtSession>> startRtStream(
    const std::string& device_id,
    FeedbackCallback on_feedback);
```

成功后应**立刻** `write`；第一帧完成 attach。每设备同时仅一条流。

---



## 5. `RtSession`

由 `Client::startRtStream` 创建；析构会 `stop`。


| 方法                                           | 说明                         |
| -------------------------------------------- | -------------------------- |
| `write(values, timestamp_ns=0)`              | 写一帧；`sequence` 自增（从 1）     |
| `writeExplicit(seq, values, timestamp_ns=0)` | 指定 sequence（测丢包）           |
| `stop()`                                     | WritesDone + 读完反馈 + Finish |
| `deviceId()`                                 | 设备 id                      |
| `running()`                                  | 是否仍在跑                      |




### 单位与向量


| 设备      | 命令 `write`           | 反馈 `values`              |
| ------- | -------------------- | ------------------------ |
| 臂 / sim | 6 关节位置，**rad**       | 6 关节，rad                 |
| AGV     | `[v, w]`（m/s, rad/s） | `size==3` 时按 `[x,y,yaw]` |


- 写周期 **≥ 40ms**；长时间不写盒端可能 Estop  
- 臂首帧不要全 0；AGV 首帧 `[0,0]` 可以  
- 不要填 `tcp_pose`（协议层）

---



## 6. 版本宏

```cpp
#include <dalynce/sdk/version.hpp>
// DALYNCE_SDK_VERSION_MAJOR / MINOR / PATCH
// DALYNCE_SDK_VERSION_STRING  // "x.y.z"
```

---



## 7. 线程与生命周期注意

1. 一个 `Client` 可多设备；**每设备一条** RT 流。
2. 反馈回调线程安全：只做轻量拷贝/投递，勿在回调里关流或改生命周期。
3. 关流后再 `disable` / `disconnect`。
4. `emergencyStop` 可随时调用（仍建议随后规范化关流）。
5. Windows：保证 `dalynce_sdk.dll` 与 exe 同 CRT（`/MD`），DLL 在 PATH 或 exe 旁。

---

