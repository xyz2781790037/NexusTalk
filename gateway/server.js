// gateway_node/server.js
const WebSocket = require('ws');
const net = require('net');
const express = require('express');
const multer = require('multer');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

// ================= 配置区 =================
const WS_PORT = 8081;         // 前端 WebSocket 连接端口
const HTTP_PORT = 8082;       // 前端 HTTP 文件上传/下载端口
const TCP_HOST = '127.0.0.1'; // 你的 C++ Muduo 服务器 IP
const TCP_PORT = 8080;        // 你的 C++ Muduo 服务器主聊天端口
const UPLOAD_DIR = path.join(__dirname, 'uploads');

// 确保上传目录存在
if (!fs.existsSync(UPLOAD_DIR)) {
    fs.mkdirSync(UPLOAD_DIR);
}

// ================= 1. HTTP 文件服务 (Express + Multer) =================
const app = express();
app.use(cors()); // 允许前端跨域请求

// 配置文件存储引擎
const storage = multer.diskStorage({
    destination: function (req, file, cb) {
        cb(null, UPLOAD_DIR);
    },
    filename: function (req, file, cb) {
        // 防止文件名冲突，追加时间戳
        const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1E9);
        cb(null, uniqueSuffix + '-' + file.originalname);
    }
});
const upload = multer({ storage: storage });

// 文件上传接口
app.post('/api/upload', upload.single('file'), (req, res) => {
    if (!req.file) {
        return res.status(400).json({ code: 400, msg: 'No file uploaded' });
    }
    console.log(`[HTTP] 文件上传成功: ${req.file.filename}`);
    // 返回文件访问路径供前端发送消息使用
    res.json({
        code: 200,
        data: {
            fileName: req.file.originalname,
            savedName: req.file.filename,
            size: req.file.size,
            url: `http://localhost:${HTTP_PORT}/api/download/${req.file.filename}`
        }
    });
});

// 文件下载/静态资源访问接口
app.use('/api/download', express.static(UPLOAD_DIR));

app.listen(HTTP_PORT, () => {
    console.log(`[HTTP] 文件服务已启动，监听端口: ${HTTP_PORT}`);
});

// ================= 2. WebSocket -> TCP 代理服务 =================
const wss = new WebSocket.Server({ port: WS_PORT });

console.log(`[WS] WebSocket 网关已启动，监听端口: ${WS_PORT}`);

wss.on('connection', (ws) => {
    console.log('[WS] 前端建立新连接');

    // 为当前 WS 客户端建立专属的 TCP 连接到 Muduo
    const tcpClient = new net.Socket();
    
    tcpClient.connect(TCP_PORT, TCP_HOST, () => {
        console.log(`[TCP] 已连接到 Muduo 聊天服务器 ${TCP_HOST}:${TCP_PORT}`);
    });

    // 接收前端 WS 消息 -> 加上 4 字节大端序长度头 -> 转发给 Muduo
    ws.on('message', (message) => {
        const payload = Buffer.isBuffer(message) ? message : Buffer.from(message);
        const header = Buffer.alloc(4);
        header.writeInt32BE(payload.length, 0); // 对应 C++ 的 htonl(len)
        
        tcpClient.write(header);
        tcpClient.write(payload);
    });

    // 接收 Muduo TCP 消息 -> 拆包 -> 转发给前端 WS
    let recvBuffer = Buffer.alloc(0);
    
    tcpClient.on('data', (data) => {
        recvBuffer = Buffer.concat([recvBuffer, data]);

        // 处理粘包与半包 (对应 C++ MessageSplitter 逻辑)
        while (recvBuffer.length >= 4) {
            const msgLen = recvBuffer.readInt32BE(0); // 对应 C++ 的 ntohl(len)
            
            if (recvBuffer.length >= 4 + msgLen) {
                // 提取完整消息体
                const msgPayload = recvBuffer.slice(4, 4 + msgLen);
                
                // 转发给前端 Vue
                if (ws.readyState === WebSocket.OPEN) {
                    ws.send(msgPayload.toString('utf8'));
                }
                
                // 截断已处理的 Buffer
                recvBuffer = recvBuffer.slice(4 + msgLen);
            } else {
                // 数据不够一个完整的包，等待下一次 data 事件
                break;
            }
        }
    });

    // 异常与断开清理
    ws.on('close', () => {
        console.log('[WS] 前端断开连接');
        tcpClient.destroy();
    });
    
    tcpClient.on('close', () => {
        console.log('[TCP] 对应 Muduo 连接已断开');
        if (ws.readyState === WebSocket.OPEN) ws.close();
    });

    ws.on('error', (err) => console.error('[WS] 错误:', err.message));
    tcpClient.on('error', (err) => console.error('[TCP] 错误:', err.message));
});