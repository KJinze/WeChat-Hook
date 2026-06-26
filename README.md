# WeChat-Hook

Windows 微信 Hook DLL。当前分支针对微信 `4.1.10.27`，通过代理微信私有 DLL `ilink_wrapper.dll` 在微信主进程内初始化 Hook 与本地 HTTP 服务。

## 版本说明

- 目标微信版本：`4.1.10.27`
- 架构：Windows x64
- 代理 DLL：`ilink_wrapper.dll`
- 真实 DLL 备份名：`ilink_wrapper_real.dll`
- 默认监听地址：`0.0.0.0:30001`
- 默认本机调用地址：`http://127.0.0.1:30001`

> 注意：旧文档里的 `version.dll` 加载方式不适用于当前调试通过的 `4.1.10.27` 环境。实测主进程会加载系统 `version.dll`，所以当前改用 `4.1.10.27\ilink_wrapper.dll` 作为代理入口。

## 编译

使用 Visual Studio / MSBuild 编译：

```powershell
MSBuild.exe x64_Version_dll.vcxproj /m /t:Build /p:Configuration=Release /p:Platform=x64
```

Release 输出：

```text
x64\Release\ilink_wrapper.dll
```

## 安装

先关闭微信相关进程，然后在微信版本目录中备份原始 DLL：

```powershell
cd "C:\Program Files\Tencent\Weixin\4.1.10.27"
copy /Y ilink_wrapper.dll ilink_wrapper_real.dll
```

复制本项目生成的代理 DLL：

```powershell
copy /Y "C:\100code\WeChat-Hook\x64\Release\ilink_wrapper.dll" "C:\Program Files\Tencent\Weixin\4.1.10.27\ilink_wrapper.dll"
```

启动微信并指定 HTTP 服务端口：

```powershell
cd "C:\Program Files\Tencent\Weixin"
.\Weixin.exe StartPort=30001
```

## 启动参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `StartPort=` | `30001` | HTTP 服务监听端口 |
| `RecvType=` | `1` | 保留参数，当前只解析保存 |
| `CallBackURL=` | 空 | 保留参数，当前用于保存回调地址和解析端口 |

示例：

```powershell
.\Weixin.exe StartPort=30001 CallBackURL="http://127.0.0.1:8080/callback"
```

## HTTP 调用约定

所有 `POST` 接口都要求使用 raw JSON 请求体：

```http
Content-Type: application/json
```

不要把参数放在 URL Params、form-data 或 x-www-form-urlencoded 里。否则服务端会收到空字段，例如：

```json
{
  "msg_len": 0,
  "ret": -1,
  "retmsg": "send failed",
  "sendRet": -1,
  "wxidorgid": ""
}
```

源码当前注册 8 条 HTTP 路径：

- `POST /SendTextMsg`
- `POST /Decode_Pic`
- `POST /GetSelfProfile`
- `POST /ForwardXMLMsg`
- `POST /SendImgMsg`
- `POST /QueryDB/execute`
- `POST /QueryDB/GetAllDBName`
- `GET /QueryDB/status`

其中 `POST /SendTextMsg` 和 `POST /Decode_Pic` 在同一个路由文件 `src/SendTextMsg.cpp` 中注册。

## 接口说明

### 发送文本消息

```http
POST /SendTextMsg
```

请求：

```json
{
  "wxidorgid": "filehelper",
  "msg": "hello"
}
```

字段：

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `wxidorgid` | 是 | 接收方 wxid。文件传输助手为 `filehelper` |
| `wxid` | 否 | `wxidorgid` 为空时会尝试读取该字段 |
| `msg` | 是 | 文本内容，按 UTF-8 JSON 发送 |

响应：

```json
{
  "ret": 0,
  "retmsg": "success",
  "sendRet": 1,
  "wxidorgid": "filehelper",
  "msg_len": 5
}
```

说明：

- `ret = 0` 表示已调用内部发送函数。
- `sendRet` 是微信内部 `send_message` 调用返回值，用于诊断。
- `msg_len = 0` 或 `wxidorgid = ""` 通常表示请求体没有按 raw JSON 发送。

PowerShell 示例：

```powershell
$body = @{
  wxidorgid = "filehelper"
  msg = "hello"
} | ConvertTo-Json -Compress

Invoke-WebRequest `
  -Uri "http://127.0.0.1:30001/SendTextMsg" `
  -Method Post `
  -ContentType "application/json; charset=utf-8" `
  -Body $body
```

curl 示例：

```bash
curl -X POST http://127.0.0.1:30001/SendTextMsg \
  -H "Content-Type: application/json" \
  -d "{\"wxidorgid\":\"filehelper\",\"msg\":\"hello\"}"
```

### 发送图片消息

```http
POST /SendImgMsg
```

请求：

```json
{
  "wxidorgid": "filehelper",
  "path": "C:\\path\\image.jpg"
}
```

字段：

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `wxidorgid` | 是 | 接收方 wxid |
| `path` | 是 | 本机图片绝对路径 |

响应：

```json
{
  "ret": 0,
  "retmsg": "success"
}
```

注意：该接口当前没有校验内部发送结果，返回 `success` 只表示接口已调用发送逻辑。

### 解码图片

```http
POST /Decode_Pic
```

请求：

```json
{
  "src_path": "C:\\path\\source.dat",
  "dst_path": "C:\\path\\output.jpg"
}
```

字段：

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `src_path` | 是 | 源图片/缓存文件路径 |
| `dst_path` | 是 | 解码后的输出路径 |

响应：

```json
{
  "ret": 0,
  "retmsg": "success"
}
```

### 转发 XML 消息

```http
POST /ForwardXMLMsg
```

请求：

```json
{
  "to_wxid": "filehelper",
  "content": "<msg>...</msg>"
}
```

字段：

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `to_wxid` | 是 | 接收方 wxid |
| `content` | 是 | XML 消息内容 |

成功响应：

```json
{
  "ret": 0,
  "retmsg": "success"
}
```

失败响应：

```json
{
  "ret": 1,
  "retmsg": "fail"
}
```

### 获取当前账号资料

```http
POST /GetSelfProfile
```

请求：

```json
{}
```

响应：

```json
{
  "wxid": "",
  "alias": "",
  "nickname": "",
  "email": "",
  "qq": 0,
  "phone": "",
  "proiv": "",
  "area": "",
  "signinfo": ""
}
```

说明：当前代码只返回全局 `SelfInfo` 的缓存值。如果没有其它逻辑填充该结构，字段可能为空。

### 查询数据库

```http
POST /QueryDB/execute
```

请求：

```json
{
  "optDbName": "MicroMsg.db",
  "SQL": "SELECT * FROM ChatRoom LIMIT 10"
}
```

字段：

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `optDbName` | 是 | 数据库文件名，例如 `MicroMsg.db` |
| `SQL` | 是 | 要执行的 SQL |

响应：

```json
{
  "status": 0,
  "desc": "",
  "data": []
}
```

注意：数据库枚举依赖 `g_IsLogin`。当前代码没有完整登录状态填充逻辑时，可能返回空列表或找不到数据库句柄。

### 获取数据库列表

```http
POST /QueryDB/GetAllDBName
```

请求：

```json
{}
```

响应：

```json
[
  {
    "dbName": "MicroMsg.db",
    "dbHandle": 123456789
  }
]
```

注意：如果返回 `[]`，通常表示当前进程内数据库句柄未被扫描到，或 `g_IsLogin` 仍为 `0`。

### 查询运行状态

```http
GET /QueryDB/status
```

响应：

```json
{
  "IsLogin": 0,
  "hWeixin": 140734531174400
}
```

字段：

| 字段 | 说明 |
| --- | --- |
| `IsLogin` | 全局登录标志。当前代码里可能长期为 `0` |
| `hWeixin` | 当前进程中 `Weixin.dll` 的模块基址 |

## 错误响应

JSON 解析失败时，多数接口返回：

```json
{
  "ret": -1,
  "msg": "invalid json"
}
```

`/SendTextMsg` 参数为空时返回：

```json
{
  "ret": -1002,
  "retmsg": "send failed",
  "sendRet": -1002,
  "wxidorgid": "filehelper",
  "msg_len": 0,
  "error": "missing_msg",
  "detail": "request JSON field 'msg' is empty or missing"
}
```

`/SendTextMsg` 常见错误码：

| ret/sendRet | error | 说明 |
| --- | --- | --- |
| `-1000` | `invalid_json` | 请求体不是合法 raw JSON |
| `-1001` | `missing_wxidorgid` | `wxidorgid` 和 `wxid` 都为空 |
| `-1002` | `missing_msg` | `msg` 为空或未传 |
| `-2001` | `weixin_dll_not_loaded` | HTTP 服务所在进程未加载 `Weixin.dll` |
| `-3001` | `alloc_msg_buffer_failed` | 文本消息结构分配失败 |
| `-3002` | `alloc_data_buffer_failed` | 发送数据结构分配失败 |
| `-3003` | `alloc_arg1_failed` | 发送参数 1 分配失败 |
| `-3004` | `alloc_arg2_failed` | 发送参数 2 分配失败 |

## 项目结构

- `dllmain.cpp`：DLL 入口，解析启动参数，只在微信主进程初始化 Hook。
- `src/version_proxy.cpp`：当前实现为 `ilink_wrapper_real.dll` 加载与初始化入口。
- `src/ilink_wrapper_proxy.asm`：`ilink_wrapper.dll` 8 个导出函数的跳板。
- `src/inline_weixin_dll_load.cpp`：等待并初始化 `Weixin.dll`，启动 HTTP 服务。
- `src/http_routes.cpp`：HTTP 路由注册入口。
- `src/SendTextMsg.cpp`：文本发送和图片解码接口。
- `src/SendImageMsg.cpp`：图片发送接口。
- `src/ForwardXMLMsg.cpp`：XML 消息转发接口。
- `src/GetSelfProfile.cpp`：当前账号资料接口。
- `src/QueryDB.cpp`、`xdb/`：微信进程内 SQLite 数据库查询接口。

## 备注

本项目依赖微信内部偏移、结构体布局和调用约定。微信升级后需要重新核对偏移和调用参数。当前说明对应微信 `4.1.10.27` 与 `ilink_wrapper.dll` 代理方案。
