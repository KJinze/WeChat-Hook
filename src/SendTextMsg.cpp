#include "httplib.h"
#include "json.hpp"
#include <windows.h>
#include "wx_send.h"
#include "SendTextMsg.h"

using json = nlohmann::json;

static const char* SendTextErrorName(int64_t code)
{
    switch (code)
    {
    case -1001: return "missing_wxidorgid";
    case -1002: return "missing_msg";
    case -2001: return "weixin_dll_not_loaded";
    case -3001: return "alloc_msg_buffer_failed";
    case -3002: return "alloc_data_buffer_failed";
    case -3003: return "alloc_arg1_failed";
    case -3004: return "alloc_arg2_failed";
    default: return "send_failed";
    }
}

static const char* SendTextErrorDetail(int64_t code)
{
    switch (code)
    {
    case -1001: return "request JSON field 'wxidorgid' is empty or missing";
    case -1002: return "request JSON field 'msg' is empty or missing";
    case -2001: return "Weixin.dll is not loaded in the current HTTP service process";
    case -3001: return "failed to allocate text message buffer";
    case -3002: return "failed to allocate send data buffer";
    case -3003: return "failed to allocate send arg1 buffer";
    case -3004: return "failed to allocate send arg2 buffer";
    default: return "internal send_message call failed or returned an unexpected failure code";
    }
}

void Route_SendTextMsg(httplib::Server& svr)
{
    svr.Post("/SendTextMsg", [](const httplib::Request& req, httplib::Response& res)
        {
            json reqJson;
            json resp;

            try
            {
                reqJson = json::parse(req.body);
            }
            catch (...)
            {
                resp["ret"] = -1000;
                resp["retmsg"] = "invalid json";
                resp["error"] = "invalid_json";
                resp["detail"] = "request body must be raw JSON";
                res.set_content(resp.dump(), "application/json");
                return;
            }

            std::string wxidorgid = reqJson.value("wxidorgid", "");
            if (wxidorgid.empty())
                wxidorgid = reqJson.value("wxid", "");
            std::string msg = reqJson.value("msg", "");

            int64_t sendRet = 0;
            if (wxidorgid.empty())
                sendRet = -1001;
            else if (msg.empty())
                sendRet = -1002;
            else
                sendRet = WeixinSend::SendText(wxidorgid, msg);


            resp["ret"] = sendRet >= 0 ? 0 : static_cast<int>(sendRet);
            resp["retmsg"] = sendRet >= 0 ? "success" : "send failed";
            resp["sendRet"] = sendRet;
            resp["wxidorgid"] = wxidorgid;
            resp["msg_len"] = msg.size();
            if (sendRet < 0)
            {
                resp["error"] = SendTextErrorName(sendRet);
                resp["detail"] = SendTextErrorDetail(sendRet);
            }

            res.set_content(resp.dump(), "application/json");
        });

    svr.Post("/Decode_Pic", [](const httplib::Request& req, httplib::Response& res)
        {
            json reqJson;
            json resp;

            try
            {
                reqJson = json::parse(req.body);
            }
            catch (...)
            {
                resp["ret"] = -1;
                resp["msg"] = "invalid json";
                res.set_content(resp.dump(), "application/json");
                return;
            }
			

            std::string src_path = reqJson.value("src_path", "");
            std::string dst_path = reqJson.value("dst_path", "");

            OutputDebugStringA(("Decode_Pic src_path: " + src_path + "\n").c_str());
            OutputDebugStringA(("Decode_Pic dst_path: " + dst_path + "\n").c_str());


            WeixinSend::DecodePic(src_path, dst_path);


            resp["ret"] = 0;
            resp["retmsg"] = "success";

            res.set_content(resp.dump(), "application/json");
        });

}
