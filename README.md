# dalynce_sdk 0.1.0

Windows 上位机 **C++ SDK** 预编译包（共享库）。命名空间：`dalynce::sdk`。  
不含 Ruckig；不含 protobuf 公共头。接口细则见同目录 [`docs/API.md`](docs/API.md)。

| 项 | 值 |
|----|-----|
| 版本 | **0.1.0** |
| CMake 包名 | `dalynce_sdk` |
| 导入目标 | `dalynce::sdk` |
| C++ | 20（MSVC） |
| 依赖 | 无额外 find_package；gRPC 已链入 DLL |

## 目录

```text
dalynce_sdk-0.1.0/
  README.md
  docs/API.md
  include/dalynce/sdk/
  bin/dalynce_sdk.dll
  lib/dalynce_sdk.lib
  lib/cmake/dalynce_sdk/
```

---

## 1. CMake 接入

```cmake
cmake_minimum_required(VERSION 3.24)
project(my_host LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# 本地目录或 git submodule 检出根（包根含 include/bin/lib）
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/third_party/dalynce_sdk")
# 或: list(APPEND CMAKE_PREFIX_PATH "D:/path/to/dalynce_sdk-0.1.0")

find_package(dalynce_sdk 0.1 REQUIRED CONFIG)

add_executable(my_host main.cpp)
target_link_libraries(my_host PRIVATE dalynce::sdk)
```

```powershell
cmake -B build -DCMAKE_PREFIX_PATH="D:/path/to/dalynce_sdk-0.1.0"
cmake --build build
# 将 bin/dalynce_sdk.dll 拷到 exe 旁，或加入 PATH
```

预编译包可单独推到 GitHub（例如 `https://github.com/Dru2Sky/dalynce_sdk.git`），下游：

```powershell
git submodule add https://github.com/Dru2Sky/dalynce_sdk.git third_party/dalynce_sdk
```

再将 `third_party/dalynce_sdk` 加入 `CMAKE_PREFIX_PATH` 后 `find_package`。

---

## 2. 公共头

```cpp
#include <dalynce/sdk/client.hpp>    // Client + RtSession
#include <dalynce/sdk/types.hpp>     // 通常被 client 间接包含
#include <dalynce/sdk/version.hpp>   // DALYNCE_SDK_VERSION_STRING
```

---

## 3. 结果判断（所有接口通用）

```cpp
auto r = client.health();
if (!r.ok()) {
    std::cerr << "code=" << static_cast<int>(r.code)
              << " msg=" << r.message << "\n";
    return;
}
// 成功：r.value（Result<void> 无 value）
```

---

## 4. 各接口使用示例

以下默认已创建：

```cpp
dalynce::sdk::Client client;
```

### 4.1 `connect` / `shutdown`

```cpp
auto c = client.connect("192.168.1.13:50051");
if (!c.ok()) { /* ... */ }

// 程序结束前
client.shutdown();
```

### 4.2 `health`

```cpp
auto h = client.health();
if (h.ok()) {
    std::cout << "version=" << h.value.version
              << " uptime_ms=" << h.value.uptime_ms
              << " status=" << h.value.status << "\n";
}
```

### 4.3 `listDevices`

```cpp
auto list = client.listDevices();
if (list.ok()) {
    for (const auto& d : list.value) {
        std::cout << d.id << " driver=" << d.driver
                  << " status=" << d.status << "\n";
    }
}
```

### 4.4 `createDevice` / `destroyDevice`

仅 **动态设备**（如 sim）。YAML 已预创建的 aubo/agv **不要**再 Create。

```cpp
// sim 臂示例；params_json 按盒端约定填写，可为空 "{}"
auto created = client.createDevice("robot_host_test_sim", "robot", "sim", "{}");
if (!created.ok()) { /* ... */ }
std::cout << "created id=" << created.value.id
          << " serial=" << created.value.serial << "\n";

// 用完后
(void)client.destroyDevice("robot_host_test_sim");
```

### 4.5 `connectDevice` / `disconnectDevice`

```cpp
const std::string id = "robot_aubo_1";  // 或 listDevices 得到的 id

auto cd = client.connectDevice(id);
if (!cd.ok()) { /* ... */ }

// ... enable / RT ...

auto dd = client.disconnectDevice(id);
```

### 4.6 `enable` / `disable`

```cpp
auto en = client.enable(id);   // Aubo 可能阻塞十余秒
if (!en.ok()) { /* ... */ }

// 关流之后再下使能
auto dis = client.disable(id);
```

### 4.7 `emergencyStop`

随时可对设备急停 RT 环：

```cpp
(void)client.emergencyStop(id);
```

### 4.8 `moveJoint`（无 RT 流时）

开流期间调用常返回 1005；AGV 返回 5001。

```cpp
// 相对点动：6 轴增量（rad），按盒端约定
std::vector<double> delta(6, 0.0);
delta[0] = 0.01;
auto mj = client.moveJoint(id, delta);
if (!mj.ok()) {
    std::cerr << "moveJoint: " << mj.message << "\n";
}
```

### 4.9 `startRtStream` + `RtSession::write` / `stop`（臂 / sim）

周期 **≥ 40ms**。首帧完成 attach；臂首帧勿全 0。

```cpp
#include <chrono>
#include <thread>

std::vector<double> seed;  // 建议用反馈或已知安全姿态初始化

auto stream = client.startRtStream(id, [](const dalynce::sdk::RtFeedback& fb) {
    // 反馈线程：只做轻量处理
    if (fb.values.size() == 6) {
        // 可用 fb.values 作关节反馈
    }
});
if (!stream.ok()) { /* ... */ }
auto& session = *stream.value;

// 第一帧：用安全姿态 attach（示例：若已有 seed）
if (seed.size() != 6) {
    seed.assign(6, 0.1);  // 演示用；真机请换成实测关节角
}
auto w0 = session.write(seed);
if (!w0.ok()) { /* ... */ }

for (int i = 0; i < 100; ++i) {
    // 此处可改 seed 中某一轴做小幅运动
    auto w = session.write(seed);
    if (!w.ok()) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
}

(void)session.stop();  // 或让 unique_ptr 析构自动 stop
```

### 4.10 AGV 实时流（`[v, w]`）

```cpp
auto stream = client.startRtStream("robot_agv_1",
    [](const dalynce::sdk::RtFeedback& fb) {
        // fb.values.size()==3 时常为 [x, y, yaw]
    });
if (!stream.ok()) { /* ... */ }
auto& session = *stream.value;

(void)session.write({0.0, 0.0});                 // 首帧可零速
(void)session.write({0.1, 0.0});                 // v=0.1 m/s, w=0
std::this_thread::sleep_for(std::chrono::milliseconds(40));
// ... 循环 write，结束前多写几帧 [0,0] ...
(void)session.write({0.0, 0.0});
(void)session.stop();
```

### 4.11 `writeExplicit`（测序 / 丢包）

```cpp
(void)session.writeExplicit(/*sequence=*/42, seed, /*timestamp_ns=*/0);
```

### 4.12 `deviceId` / `running`

```cpp
std::cout << session.deviceId() << " running=" << session.running() << "\n";
```

### 4.13 版本宏

```cpp
std::cout << "SDK " << DALYNCE_SDK_VERSION_STRING << "\n";
// DALYNCE_SDK_VERSION_MAJOR / MINOR / PATCH
```

---

## 5. 推荐完整生命周期（伪代码）

```text
connect(target)
health() / listDevices()
[createDevice]          # 仅动态/sim
connectDevice(id)
enable(id)
startRtStream → write 循环（≥40ms）
stop()
disable(id)
disconnectDevice(id)
[destroyDevice]
shutdown()
```

---

## 6. 能力边界

| 有 | 无 |
|----|----|
| Health / List / 生命周期 / 急停 | Ruckig 点动往复（请自研或用 Studio） |
| RT 流 write / 反馈回调 | protobuf / gRPC 头暴露 |
| `moveJoint`（无流） | 盒端插值 / 厂商驱动 |

---

## 7. 版本兼容

`find_package(dalynce_sdk 0.1)`：**SameMajorVersion**。  
当前包：**0.1.0**。接口说明：[`docs/API.md`](docs/API.md)。
