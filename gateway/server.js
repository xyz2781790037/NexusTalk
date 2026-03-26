// gateway/server.js
const WebSocket = require('ws');
const net = require('net');
const express = require('express');
const multer = require('multer');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

const WS_PORT = 8081;         
const HTTP_PORT = 8082;       
const TCP_HOST = '127.0.0.1'; 
const TCP_PORT = 8080;        
const UPLOAD_DIR = path.join(__dirname, 'uploads');

if (!fs.existsSync(UPLOAD_DIR)) {
    fs.mkdirSync(UPLOAD_DIR);
}

const app = express();
app.use(cors()); 

const storage = multer.diskStorage({
    destination: function (req, file, cb) {
        cb(null, UPLOAD_DIR);
    },
    filename: function (req, file, cb) {
        const ext = path.extname(file.originalname) || '';
        const safeName = Date.now() + '-' + Math.random().toString(36).substring(2, 8) + ext;
        cb(null, safeName);
    }
});
const upload = multer({ storage: storage });

app.get('/ping', (req, res) => {
    res.send('8082 端口已打通，网关文件微服务运行正常！');
});

app.post('/api/upload', upload.single('file'), (req, res) => {
    if (!req.file) {
        return res.status(400).json({ code: 400, msg: 'No file uploaded' });
    }
    
    const host = req.hostname; 
    let realName = '未知文件';
    try {
        realName = decodeURIComponent(req.file.originalname);
    } catch (e) {
        realName = req.file.originalname;
    }

    // 【核心修复 1】：把真实的中文名挂在 URL 参数 ?name= 后面，供下载时提取
    const encodedRealName = encodeURIComponent(realName);
    const fileUrl = `http://${host}:${HTTP_PORT}/api/download/${req.file.filename}?name=${encodedRealName}`;

    res.json({
        code: 200,
        data: {
            fileName: realName,
            savedName: req.file.filename,
            size: req.file.size,
            url: fileUrl
        }
    });
});

app.use('/api/download', express.static(UPLOAD_DIR));

app.get('/api/download/:filename', (req, res) => {
    const rawFilename = req.params.filename; 
    // 【核心修复 2】：提取 URL 参数中的真实中文名
    const originalName = req.query.name;

    const path1 = path.join(UPLOAD_DIR, rawFilename); 
    const path2 = path.join(UPLOAD_DIR, decodeURIComponent(rawFilename)); 
    const path3 = path.join(UPLOAD_DIR, encodeURIComponent(rawFilename)); 
    
    let targetPath = null;
    if (fs.existsSync(path1)) targetPath = path1;
    else if (fs.existsSync(path2)) targetPath = path2;
    else if (fs.existsSync(path3)) targetPath = path3;

    if (targetPath) {
        if (originalName) {
            // 触发浏览器的强制下载，并命名为真实的中文名
            res.download(targetPath, originalName);
        } else {
            res.sendFile(targetPath);
        }
    } else {
        res.status(404).send('文件在服务器硬盘上已丢失');
    }
});

app.listen(HTTP_PORT, '0.0.0.0', () => {
    console.log(`[HTTP] 文件服务已启动，监听端口: ${HTTP_PORT}`);
});

const wss = new WebSocket.Server({ port: WS_PORT, host: '0.0.0.0' });

console.log(`[WS] WebSocket 网关已启动，监听端口: ${WS_PORT}`);

wss.on('connection', (ws) => {
    const tcpClient = new net.Socket();
    
    tcpClient.connect(TCP_PORT, TCP_HOST, () => {});

    ws.on('message', (message) => {
        const payload = Buffer.isBuffer(message) ? message : Buffer.from(message);
        const header = Buffer.alloc(4);
        header.writeInt32BE(payload.length, 0); 
        tcpClient.write(header);
        tcpClient.write(payload);
    });

    let recvBuffer = Buffer.alloc(0);
    
    tcpClient.on('data', (data) => {
        recvBuffer = Buffer.concat([recvBuffer, data]);
        while (recvBuffer.length >= 4) {
            const msgLen = recvBuffer.readInt32BE(0); 
            if (recvBuffer.length >= 4 + msgLen) {
                const msgPayload = recvBuffer.slice(4, 4 + msgLen);
                if (ws.readyState === WebSocket.OPEN) {
                    ws.send(msgPayload.toString('utf8'));
                }
                recvBuffer = recvBuffer.slice(4 + msgLen);
            } else {
                break;
            }
        }
    });

    ws.on('close', () => tcpClient.destroy());
    tcpClient.on('close', () => { if (ws.readyState === WebSocket.OPEN) ws.close(); });
    ws.on('error', () => {});
    tcpClient.on('error', () => {});
});