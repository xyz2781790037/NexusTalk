#include "handleData.h"
ConnectionManager connectionmanger_;
std::unordered_map<std::string, mulib::net::TcpConnectionPtr> findAllConn_;
std::unordered_map<std::string, std::atomic<bool>> chatStatus_;
std::unordered_map<mulib::net::TcpConnectionPtr, time_t> lastActiveTime;
std::unordered_map<std::string, std::vector<std::string>> messageQueue;
std::mutex timeMutex;
std::map<std::string, std::mutex> key_mutexes;
ThreadPool pool(16);

void handleData::Megcycle(const TcpConnectionPtr conn,
                          std::string& meg,
                          redisCmd& redis,
                          mulib::base::Timestamp recviveTime) {
    LOG_DEBUG << "Megcycle running in thread: " << std::this_thread::get_id()
              << std::endl;
    if (meg[0] == '{' && meg[meg.size() - 1] == '}') {
        LOG_DEBUG << "\033[1;35m" << meg << "\033[0m";
        auto jsonData = nlohmann::json::parse(meg);
        if (jsonData.contains("type")) {
            LOG_DEBUG << "JSON 正常";
            Type::types type = Type::getDataType(jsonData["type"]);
            LOG_DEBUG << "type is :" << type;
            if (type == Type::MESSAGE) {
                std::string a = jsonData["from"];
                std::string b = jsonData["to"];
                if (redis.getData("blak:" + b.substr(5),
                                  "user:" + a.substr(5)) != "null") {
                    conn->send(MessageSplitter::encodeMessage(
                        sendMeg("\33\r[2K系统：发出但被拒收\n", Type::URETURN,
                                "1")
                            .dump()));
                    return;
                }

                // 【核心修复】：无论双方状态，无条件存入历史记录 read: 表
                pool.enqueue([jsonData, &redis, a, b, this] {
                    std::string key = tool::swapsort(a, b, "read:");
                    redis.storeReadMeg({jsonData.dump()}, key);

                    // 检查对方是否正在看着当前聊天界面
                    std::string key2 = tool::spellName(b, a, "chat:");
                    if (!chatStatus_[key2]) {
                        // 如果对方没在看你，额外存一份离线表触发未读红点
                        redis.storeMessages(b, a, jsonData.dump());
                    }
                });

                if (connectionmanger_.isOnline(jsonData["to"])) {
                    LOG_DEBUG << jsonData["to"] << "在线";
                    connectionmanger_.getConn(jsonData["to"])
                        ->send(MessageSplitter::encodeMessage(jsonData.dump()));
                } else {
                    LOG_DEBUG << jsonData["to"] << "不在线";
                }
            } else if (type == Type::REGISTER) {
                LOG_INFO << "进入register";
                handleRegister(conn, jsonData, redis);
            } else if (type == Type::LOGIN) {
                LOG_INFO << "进入login";
                handleLogin(conn, jsonData, redis);
            } else if (type == Type::GETPWD) {
                LOG_INFO << "进入getpwd";
                returnPwd(conn, jsonData, redis);
                LOG_INFO << "离开getpwd";
            } else if (type == Type::REVISE) {
                LOG_INFO << "进入revise";
                revise(conn, jsonData, redis);
            } else if (type == Type::DELETE) {
                LOG_INFO << "进入delete";
                deleteUser(conn, jsonData, redis);
            } else if (type == Type::ADD) {
                LOG_INFO << "进入add";
                addAll(conn, jsonData, redis);
                LOG_INFO << "退出add";
            } else if (type == Type::SHIP) {
                LOG_INFO << "进入ship";
                updataShip(conn, jsonData, redis);
            } else if (type == Type::MESSDATA) {
                LOG_INFO << "进入messdata";
                findmess(conn, jsonData, redis);
            } else if (type == Type::VERIFY) {
                LOG_INFO << "进入verify";
                verify(conn, jsonData, redis);
            } else if (type == Type::SEE) {
                LOG_INFO << "进入see";
                see(conn, jsonData, redis);
            } else if (type == Type::CHAT) {
                sendOfflineMeg(conn, jsonData, redis);
            } else if (type == Type::OFFMEG) {
                offMeg(conn, jsonData, redis);
            } else if (type == Type::ENDCHAT) {
                endChat(conn, jsonData, redis);
            } else if (type == Type::DELFRIEND) {
                deleteEnd(conn, jsonData, redis);
            } else if (type == Type::BLACK) {
                balckName(conn, jsonData, redis);
            } else if (type == Type::CREATEGROUP) {
                createGroup(conn, jsonData, redis);
            } else if (type == Type::DELGROUP) {
                deleteGroup(conn, jsonData, redis);
            } else if (type == Type::SEEGROUP) {
                seeGroup(conn, jsonData, redis);
            } else if (type == Type::GCHAT) {
                gChat(conn, jsonData, redis);
            } else if (type == Type::GMESSAGE) {
                handlegMessage(conn, jsonData, redis);
            } else if (type == Type::CMD) {
                handleCmd(conn, jsonData, redis);
            }
            // [修改] 移除了原有的 Type::FILE, Type::LOOK, Type::LIST
            // 路由判断。前端发文件现在直接当做普通文本（包含URL）的
            // Type::MESSAGE 路由。
            else if (type == Type::HISTORY) {
                seeHistory(conn, jsonData, redis);
            } else if (type == Type::TCP) {
                heartTcp(conn, jsonData, redis, recviveTime);
            }
        } else {
            LOG_ERROR << "JSON 数据缺少必要字段，忽略";
        }
    }
}
void handleData::handleRegister(const TcpConnectionPtr& conn,
                                nlohmann::json& jsonData,
                                redisCmd& redis) {
    // 【新增核心防御】：后端强制校验验证码
    if (!jsonData.contains("code") || codes_[conn] != jsonData["code"]) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("验证码错误或已失效", Type::EXECUTE).dump()));
        return;
    }

    if (!redis.isAccount(jsonData["account"])) {
        redis.setNewUser(jsonData);
        LOG_INFO << "准备发送注册成功消息给客户端";
        if (conn && conn->connected()) {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("注册成功！", Type::EXECUTE).dump()));
            std::string account = jsonData["account"];
            redis.hset("frie:" + account.substr(5), account, "0");
        }
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("账号已存在", Type::EXECUTE).dump()));
    }
}
void handleData::handleLogin(const TcpConnectionPtr& conn,
                             nlohmann::json& jsonData,
                             redisCmd& redis) {
    int result = redis.handleLogin(jsonData);
    nlohmann::json jdata;
    LOG_INFO << "result=" << result;
    if (result == 1) {
        jdata["account"] = jsonData["account"];
        redis.returnUser(jdata);
        LOG_INFO << "准备发送 :" << jdata.dump();
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("成功登陆", Type::UserStatus::UEXECUTE).dump()));
        conn->send(MessageSplitter::encodeMessage(jdata.dump()));
    } else if (result == 2) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("账号已登录", Type::EXECUTE).dump()));
    } else if (result == 0) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("密码错误", Type::EXECUTE).dump()));
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("账号不存在", Type::EXECUTE).dump()));
    }
    LOG_INFO << "handleData::handleLogin: end";
}
nlohmann::json handleData::sendMeg(std::string message, Type::Status state) {
    nlohmann::json j;
    j["type"] = "print";
    j["meg"] = message;
    j["state"] = state;
    j["n"] = "n";
    return j;
}
nlohmann::json handleData::sendMeg(std::string message,
                                   Type::UserStatus state,
                                   std::string N) {
    nlohmann::json j;
    j["type"] = "print";
    j["meg"] = message;
    j["userstate"] = state;
    j["n"] = N;
    return j;
}
void handleData::returnPwd(const TcpConnectionPtr& conn,
                           nlohmann::json& jsonData,
                           redisCmd& redis) {
    verCode vercode_;
    if (jsonData["return"] == "register") {
        codes_[conn] = vercode_.verify(jsonData["account"]);
        // 【新增】：告诉前端验证码发送成功
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("验证码已发送到邮箱", Type::EXECUTE).dump()));
        return;
    } else if (jsonData["return"] == "code") {
        if (codes_[conn] == jsonData["vcode"]) {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("绑定成功", Type::RETURNS).dump()));
        } else {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("验证码错误", Type::WAIT).dump()));
        }
        return;
    }
    int result = redis.Vuser(jsonData);
    LOG_INFO << "result is:" << result;
    if (result == 1) {
        conn->send(MessageSplitter::encodeMessage(jsonData.dump()));
    } else if (result == 0) {
        codes_[conn] =
            vercode_.verify(redis.getData(jsonData["account"], "email"));
        LOG_INFO << "验证码已发送为：" << codes_[conn];
        conn->send(MessageSplitter::encodeMessage(jsonData.dump()));
    } else if (result == -1) {
        LOG_INFO << "result == -1";
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("账号不存在", Type::RETURNS).dump()));
    } else if (result == 2) {
        if (codes_[conn] == jsonData["vcode"]) {
            LOG_INFO << "客户验证码正确";
            jsonData["return"] = "true";
            jsonData["state"] = Type::RETURNS;
            jsonData["password"] =
                redis.getData(jsonData["account"], "password");
            conn->send(MessageSplitter::encodeMessage(jsonData.dump()));
        } else {
            LOG_INFO << "客户验证码错误";
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("验证码错误", Type::EXECUTE).dump()));
        }
    }
}
void handleData::deleteUser(const TcpConnectionPtr& conn,
                            nlohmann::json& jsonData,
                            redisCmd& redis) {
    redis.deleteUser(jsonData["account"]);
    conn->send(MessageSplitter::encodeMessage(
        sendMeg("注销成功", Type::EXECUTE).dump()));
}
void handleData::addAll(const TcpConnectionPtr& conn,
                        nlohmann::json& jsonData,
                        redisCmd& redis) {
    std::string key = jsonData["account"];
    std::string name = jsonData["name"];
    std::string lastname;
    if (key.substr(0, 5) == "frie:") {
        lastname = "user:" + name;
        if (redis.isAccount(lastname)) {
            if (redis.isfriend(key, lastname) &&
                redis.isfriend(lastname, key)) {
                conn->send(MessageSplitter::encodeMessage(
                    sendMeg("他已经是好友了", Type::UEXECUTE).dump()));
            } else {
                nlohmann::json js;
                std::string messkey = "mess:user:" + key.substr(5);
                js["type"] = "addfriend";
                js["result"] = "no";
                js["account"] = "frie:" + name;
                if (redis.lookmess(messkey, js)) {
                    js["account"] = jsonData["account"];

                    redis.waitHandleMeg(lastname, js);
                    conn->send(MessageSplitter::encodeMessage(
                        sendMeg("好友申请已发送！", Type::UEXECUTE).dump()));
                } else {
                    LOG_INFO << "2222";
                    conn->send(MessageSplitter::encodeMessage(
                        sendMeg("你俩成为好友！", Type::UEXECUTE).dump()));
                }
            }
        } else {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("用户不存在！", Type::UEXECUTE).dump()));
        }
    } else if (key.substr(0, 5) == "mygp:") {
        lastname = "grop:" + name;
        if (redis.isgroup(name)) {
            if (redis.ismygroup(lastname, key)) {
                conn->send(MessageSplitter::encodeMessage(
                    sendMeg("你已经在群聊里了", Type::UEXECUTE).dump()));
            } else {
                nlohmann::json js;
                js["account"] = jsonData["account"];
                js["type"] = "addgroup";
                js["result"] = "no";
                redis.waitHandleMeg(lastname, js);
                conn->send(MessageSplitter::encodeMessage(
                    sendMeg("加群申请已发送！", Type::UEXECUTE).dump()));
            }
        } else {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("群聊不存在！", Type::UEXECUTE).dump()));
        }
    } else if (key.substr(0, 5) == "grop:") {
        std::string person = jsonData["person"];
        lastname = jsonData["name"];
        if (redis.isgroup(name) &&
            redis.getData("mygp:" + key.substr(5), name) != "null") {
            if (redis.isfriend(key, "user:" + person) &&
                redis.isfriend("user:" + person, key)) {
                if (redis.ismygroup("user:" + name, "mygp:" + person)) {
                    conn->send(MessageSplitter::encodeMessage(
                        sendMeg("好友已经在群聊里了", Type::UEXECUTE).dump()));
                } else {
                    nlohmann::json js;
                    js["account"] = "user:" + key.substr(5);
                    js["type"] = "invitation";
                    js["name"] = "grop:" + lastname;
                    js["result"] = "no";
                    redis.waitHandleMeg("user:" + person, js);
                    conn->send(MessageSplitter::encodeMessage(
                        sendMeg("邀请申请已发送！", Type::UEXECUTE).dump()));
                }
            } else {
                conn->send(MessageSplitter::encodeMessage(
                    sendMeg("好友不存在！", Type::UEXECUTE).dump()));
            }
        } else {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("群聊不存在！", Type::UEXECUTE).dump()));
        }
    }
}
void handleData::updataShip(const TcpConnectionPtr& conn,
                            nlohmann::json& jsonData,
                            redisCmd& redis) {
    redis.updataship(jsonData);
    auto Copyconn = conn;
    if (jsonData.contains("return")) {
        Copyconn = findAllConn_[jsonData["account"]];
    }
    if (jsonData["mystate"] == "online") {
        connectionmanger_.addUserConn(jsonData["account"], Copyconn);
        findAllConn_[jsonData["account"]] = Copyconn;
    } else {
        LOG_INFO << jsonData["account"];
        connectionmanger_.removeUserConn(jsonData["account"], conn);
    }
}
void handleData::findmess(const TcpConnectionPtr& conn,
                          nlohmann::json& jsonData,
                          redisCmd& redis) {
    std::string name = jsonData["account"];
    LOG_INFO << "findmess";
    std::string sum;
    if (name.substr(0, 10) == "mess:user:") {
        cpp_redis::reply result = redis.findmess(name);
        for (const auto& item : result.as_array()) {
            if (item.is_string()) {
                auto j = nlohmann::json::parse(item.as_string());
                j["use"] = j["type"];
                j["type"] = "messdata";
                LOG_INFO << j.dump();
                sum += MessageSplitter::encodeMessage(j.dump());
            }
        }
        conn->send(sum);
    } else {
        cpp_redis::reply result = redis.seeGroup("mygp:" + name.substr(10));
        const auto& arr = result.as_array();
        LOG_INFO << "arr.size()=" << arr.size();
        for (int i = 0; i + 1 < arr.size(); i += 2) {
            std::string item = arr[i].as_string();
            LOG_INFO << "groupName=" << item;
            LOG_INFO << redis.getLevel(item, name.substr(5));
            if (redis.getLevel(item, name.substr(5)) != "member" &&
                redis.getLevel(item, name.substr(5)) != "null") {
                cpp_redis::reply result_ = redis.findmess("mess:grop:" + item);
                for (const auto& item_ : result_.as_array()) {
                    if (item_.is_string()) {
                        auto j = nlohmann::json::parse(item_.as_string());
                        j["use"] = j["type"];
                        j["type"] = "messdata";
                        j["name"] = item;
                        LOG_INFO << j.dump();
                        sum += MessageSplitter::encodeMessage(j.dump());
                    }
                }
            }
        }
        conn->send(sum);
    }
    conn->send(MessageSplitter::encodeMessage(
        sendMeg("--------------------------", Type::URETURN).dump()));
}
void handleData::verify(const TcpConnectionPtr& conn,
                        nlohmann::json& jsonData,
                        redisCmd& redis) {
    int end = redis.verifyUser(jsonData);
    LOG_INFO << "end= " << end;
    if (end == 1) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("你们已经成为好友", Type::UWAIT).dump()));
    } else if (end == -1) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("你已拒绝成为好友", Type::UWAIT).dump()));
    } else if (end == 2) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("你已同意请求", Type::UWAIT).dump()));
    } else if (end == -2) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("你拒绝请求", Type::UWAIT).dump()));
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("信息有误！", Type::UWAIT).dump()));
    }
}
void handleData::sendOfflineMeg(const TcpConnectionPtr& conn,
                                nlohmann::json& jsonData,
                                redisCmd& redis) {
    if (redis.isfriend(jsonData["account"], jsonData["name"])) {
        redis.sendHisMeg1(jsonData, conn);
        redis.sendOfflineMeg(jsonData, conn);
        redis.sendHisOffineMeg(jsonData, conn);
        redis.sendBlackMeg(jsonData, conn);
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("开始聊天", Type::UCHAT).dump()));
        std::string key =
            tool::spellName(jsonData["account"], jsonData["name"], "chat:");
        LOG_INFO << "start key=" << key;
        chatStatus_[key] = true;
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("用户不存在或好友不存在", Type::URETURN).dump()));
    }
}
void handleData::handleCmd(const TcpConnectionPtr& conn,
                           nlohmann::json& jsonData,
                           redisCmd& redis) {
    std::string alevel = redis.getData(jsonData["name"], jsonData["account"]);
    std::string blevel = redis.getData(jsonData["name"], jsonData["target"]);
    LOG_INFO << "alevel=" << alevel << " blevel=" << blevel;
    if (blevel != "null") {
        if (tool::compareLevel(alevel, blevel)) {
            if (jsonData["use"] == "del") {
                redis.delmember(jsonData["name"], jsonData["target"]);
            } else if (jsonData["use"] == "upl" && alevel == "owner") {
                redis.reviseData(jsonData["name"], jsonData["target"], "admin");
            } else if (jsonData["use"] == "dnl" && alevel == "owner") {
                redis.reviseData(jsonData["name"], jsonData["target"],
                                 "member");
            }
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("\033[1;33m设置成功!\033[0m", Type::USAT).dump()));
        } else {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("\033[1;33m权限不够!\033[0m", Type::USAT).dump()));
        }
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("\033[1;33m用户不存在!\033[0m", Type::USAT).dump()));
    }
}
void handleData::offMeg(const TcpConnectionPtr& conn,
                        nlohmann::json& jsonData,
                        redisCmd& redis) {
    int sum = 0;
    int result1 = redis.see(jsonData["account"]);
    int result2 = redis.see(jsonData["group"]);
    std::string account = jsonData["account"];
    std::string key = "mess:user:" + account.substr(5);
    auto result3 = redis.getVerifyLen(key);
    sum = result1 + result2;
    nlohmann::json j;
    std::string meg = "你有" + std::to_string(sum) + "条未读消息";
    std::string meg2 = "你有" + std::to_string(result3) + "条验证消息";
    LOG_INFO << meg;
    if (sum != 0) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg(meg, Type::UEXECUTE).dump()));
    }
    if (result3 != 0) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg(meg2, Type::UEXECUTE).dump()));
    } else {
        conn->send(
            MessageSplitter::encodeMessage(sendMeg("", Type::UEXECUTE).dump()));
    }
}
void handleData::endChat(const TcpConnectionPtr& conn,
                         nlohmann::json& jsonData,
                         redisCmd& redis) {
    std::string key;
    if (jsonData["return"] == "1") {
        key = tool::spellName(jsonData["account"], jsonData["name"], "chat:");
    } else if (jsonData["return"] == "0") {
        key = tool::spellName(jsonData["group"], jsonData["account"], "grop:");
    }
    LOG_INFO << "end key=" << key;
    chatStatus_[key] = false;
}
void handleData::deleteEnd(const TcpConnectionPtr& conn,
                           nlohmann::json& jsonData,
                           redisCmd& redis) {
    if (redis.isfriend(jsonData["account"], jsonData["name"])) {
        redis.delFriend(jsonData["account"], jsonData["name"]);
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("删除成功！", Type::URETURN).dump()));
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("好友或用户不存在！", Type::URETURN).dump()));
    }
}
void handleData::balckName(const TcpConnectionPtr& conn,
                           nlohmann::json& jsonData,
                           redisCmd& redis) {
    std::string userAcc = jsonData["account"].get<std::string>();
    std::string targetAcc = jsonData["name"].get<std::string>();
    std::string blakKey = "blak:" + userAcc.substr(5);

    // 包含 return 字段代表解除拉黑
    if (jsonData.contains("return")) {
        if (redis.getData(blakKey, targetAcc) != "null") {
            redis.hdel(blakKey, targetAcc);
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("已解除拉黑", Type::URETURN).dump()));
        } else {
            conn->send(MessageSplitter::encodeMessage(
                sendMeg("名单上不存在", Type::URETURN).dump()));
        }
        return;
    }

    // 执行拉黑操作
    if (redis.isfriend(userAcc, targetAcc)) {
        redis.hset(blakKey, targetAcc, "0");
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("已拉黑", Type::URETURN).dump()));
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("好友或用户不存在！", Type::URETURN).dump()));
    }
}
void handleData::createGroup(const TcpConnectionPtr& conn,
                             nlohmann::json& jsonData,
                             redisCmd& redis) {
    if (redis.isgroup(jsonData["name"]) == false) {
        redis.createGroup(jsonData);
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("创建成功！", Type::UEXECUTE).dump()));
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("群聊已存在", Type::UEXECUTE).dump()));
    }
}
void handleData::deleteGroup(const TcpConnectionPtr& conn,
                             nlohmann::json& jsonData,
                             redisCmd& redis) {
    std::string key = jsonData["name"];
    if (redis.isgroup(key) == false) {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("群聊不存在", Type::UWAIT).dump()));
        return;
    }

    std::string userAcc = jsonData["account"];
    std::string rank = redis.getData("grop:" + key, userAcc);

    if (rank == "null") {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("你没有加入该群", Type::UWAIT).dump()));
        return;
    }

    // 立即执行权限分配与删库
    if (rank == "owner") {
        redis.delPerson(key, userAcc, "owner");
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("解散成功", Type::URETURN).dump()));
    } else {
        redis.delPerson(key, userAcc, "member");
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("退出成功", Type::URETURN).dump()));
    }
}

void handleData::gChat(const TcpConnectionPtr& conn,
                       nlohmann::json& jsonData,
                       redisCmd& redis) {
    // 前端传来的是 mygp: 前缀，满足 ismygroup 的查询要求
    if (redis.ismygroup(jsonData["name"], jsonData["account"])) {
        redis.getGroupMeg(jsonData["name"], jsonData["account"], conn, -100);

        // 【核心修复】强制剥离 mygp:，转换为 user:
        // 前缀存入状态机，确保能收到实时消息
        std::string userAccount =
            "user:" + jsonData["account"].get<std::string>().substr(5);
        std::string key =
            tool::spellName(jsonData["name"], userAccount, "grop:");

        chatStatus_[key] = true;
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("开始聊天", Type::UCHAT).dump()));
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("群聊不存在", Type::URETURN).dump()));
    }
}
void handleData::handlegMessage(const TcpConnectionPtr& conn,
                                nlohmann::json& jsonData,
                                redisCmd& redis) {
    std::string account = jsonData["account"];
    if (redis.getData(jsonData["from"], account) != "null") {
        redis.storeGroupMeg(jsonData, jsonData.dump());

        std::string groupname = jsonData["from"];
        auto result = redis.seeGroup(jsonData["from"]);
        if (result != cpp_redis::reply()) {
            const auto& arr = result.as_array();
            for (int i = 0; i + 1 < arr.size(); i += 2) {
                if (arr[i].as_string() != account) {
                    LOG_INFO << jsonData.dump();
                    std::string key = tool::spellName(
                        jsonData["from"], arr[i].as_string(), "grop:");
                    if (chatStatus_[key]) {
                        connectionmanger_.getConn(arr[i].as_string())
                            ->send(MessageSplitter::encodeMessage(
                                jsonData.dump()));
                    } else {
                        std::string account1 = arr[i].as_string();
                        std::string key = "mygp:" + account1.substr(5);
                        {
                            std::unique_lock lock(storeMtx2);
                            int count = std::stoi(
                                redis.getData(key, groupname.substr(5)));
                            LOG_INFO << "key=" << key << "count=" << count;
                            count++;
                            redis.hset(key, groupname.substr(5),
                                       std::to_string(count));
                        }
                    }
                }
            }
        }
    } else {
        conn->send(MessageSplitter::encodeMessage(
            sendMeg("\33[2K\r你已被移除群聊，无法发消息\n",
                    Type::UserStatus::USAT, "1")
                .dump()));
    }
}
void handleData::seeHistory(const TcpConnectionPtr& conn,
                            nlohmann::json& jsonData,
                            redisCmd& redis) {
    std::string key = jsonData["account"];
    if (key.substr(0, 5) == "read:") {
        redis.sendHistoryMeg(jsonData, conn);
    } else {
        redis.sendGroupHistoryMeg(jsonData, conn);
    }
    // 【核心修复】：无论私聊还是群聊，强制下发历史结束标志打断前端转圈
    conn->send(MessageSplitter::encodeMessage(
        sendMeg("以上为历史消息", Type::USAT).dump()));
}
void handleData::heartTcp(const TcpConnectionPtr& conn,
                          nlohmann::json& jsonData,
                          redisCmd& redis,
                          mulib::base::Timestamp recviveTime) {
    {
        std::lock_guard<std::mutex> lock(timeMutex);
        lastActiveTime[conn] = recviveTime.now().secondsSinceEpoch();
        LOG_INFO << "收到heartbeat来自->" << conn;
    }
}
void handleData::revise(const TcpConnectionPtr& conn,
                        nlohmann::json& jsonData,
                        redisCmd& redis) {
    if (jsonData.contains("password")) {
        redis.reviseData(jsonData["account"], "password", jsonData["password"]);
    } else if (jsonData.contains("myname")) {
        redis.reviseData(jsonData["account"], "myname", jsonData["myname"]);
    } else if (jsonData.contains("color")) {
        // [新增] 监听前端发来的 color 修改请求，更新个人颜色
        redis.reviseData(jsonData["account"], "color", jsonData["color"]);
    } else if (jsonData.contains("groupColor")) {
        // [新增] 监听前端发来的群聊颜色修改请求（前缀严格对齐 gcor:）
        redis.reviseData("gcor:" + jsonData["groupName"].get<std::string>(),
                         "color", jsonData["groupColor"]);
    }
    LOG_INFO << "In handleData::revise";
    std::string a = sendMeg("修改成功！", Type::UEXECUTE).dump();
    LOG_INFO << a;
    conn->send(MessageSplitter::encodeMessage(a));
    LOG_INFO << "conn use count = " << conn.use_count();
}

void handleData::see(const TcpConnectionPtr& conn,
                     nlohmann::json& jsonData,
                     redisCmd& redis) {
    std::string sum;
    cpp_redis::reply result = redis.seeGroup(jsonData["account"]);
    const auto& arr = result.as_array();
    for (size_t i = 0; i + 1 < arr.size(); i += 2) {
        nlohmann::json j;
        std::string field = arr[i].as_string();
        std::string value = arr[i + 1].as_string();
        LOG_INFO << "field=" << field << " value=" << value;
        if (jsonData.contains("see") && jsonData["see"] == "black") {
            LOG_INFO << "see::black";
            j["type"] = "see";
            j["name"] = field;
            j["see"] = "black";
        } else {
            j["type"] = "see";
            j["name"] = field;
            j["myname"] = redis.getData(field, "myname");
            // [新增] 获取好友列表时，提取并下发该好友的专属颜色
            j["color"] = redis.getData(field, "color");
            j["see"] = "friend";
            j["mystate"] = redis.getData(field, "mystate");
            j["degree"] = value;
        }
        sum += MessageSplitter::encodeMessage(j.dump());
    }
    conn->send(sum);
    conn->send(MessageSplitter::encodeMessage(
        sendMeg("--------------------------", Type::URETURN).dump()));
}

void handleData::seeGroup(const TcpConnectionPtr& conn,
                          nlohmann::json& jsonData,
                          redisCmd& redis) {
    std::string sum;
    auto result = redis.seeGroup(jsonData["account"]);
    LOG_INFO << "退出seegroup";

    // [修改]
    // 提取判断标志：区分前端是在拉取【群聊列表】还是在拉取【群内成员列表】
    bool isMembers =
        jsonData["account"].get<std::string>().substr(0, 5) == "grop:";

    if (result != cpp_redis::reply()) {
        const auto& arr = result.as_array();
        LOG_INFO << "rank=" << jsonData["rank"];
        if (jsonData["rank"] == "all") {
            for (int i = 0; i + 1 < arr.size(); i += 2) {
                nlohmann::json j;
                j["name"] = arr[i].as_string();
                j["amount"] = arr[i + 1].as_string();
                j["type"] = "seegroup";

                // [新增核心] 如果拉取的是群聊列表，获取表 gcor:
                // 中的群聊专属颜色
                if (!isMembers) {
                    j["color"] =
                        redis.getData("gcor:" + arr[i].as_string(), "color");
                } else {
                    // [新增核心] 如果拉取的是群成员，下发各成员的个人昵称和颜色
                    j["myname"] = redis.getData(arr[i].as_string(), "myname");
                    j["color"] = redis.getData(arr[i].as_string(), "color");
                }

                LOG_INFO << "发送:" << j.dump();
                sum += MessageSplitter::encodeMessage(j.dump());
            }
        } else if (jsonData["rank"] == "owner") {
            std::string account = jsonData["account"];
            std::string field = "user:" + account.substr(5);
            for (int i = 0; i + 1 < arr.size(); i += 2) {
                nlohmann::json j;
                j["name"] = arr[i].as_string();
                if (redis.getData("grop:" + arr[i].as_string(), field) ==
                    "owner") {
                    j["type"] = "seegroup";
                    if (!isMembers) {
                        j["color"] = redis.getData("gcor:" + arr[i].as_string(),
                                                   "color");
                    } else {
                        j["myname"] =
                            redis.getData(arr[i].as_string(), "myname");
                        j["color"] = redis.getData(arr[i].as_string(), "color");
                    }
                    LOG_INFO << "发送:" << j.dump();
                    sum += MessageSplitter::encodeMessage(j.dump());
                }
            }
        } else {
            std::string account = jsonData["account"];
            std::string field = "user:" + account.substr(5);
            for (int i = 0; i + 1 < arr.size(); i += 2) {
                nlohmann::json j;
                j["name"] = arr[i].as_string();
                if (redis.getData("grop:" + arr[i].as_string(), field) !=
                    "owner") {
                    j["type"] = "seegroup";
                    if (!isMembers) {
                        j["color"] = redis.getData("gcor:" + arr[i].as_string(),
                                                   "color");
                    } else {
                        j["myname"] =
                            redis.getData(arr[i].as_string(), "myname");
                        j["color"] = redis.getData(arr[i].as_string(), "color");
                    }
                    LOG_INFO << "发送:" << j.dump();
                    sum += MessageSplitter::encodeMessage(j.dump());
                }
            }
        }
        conn->send(sum);
    }
    conn->send(MessageSplitter::encodeMessage(
        sendMeg("----------------------", Type::URETURN).dump()));
}