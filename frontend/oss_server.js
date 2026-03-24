const express = require('express');
const multer = require('multer');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

const app = express();
app.use(cors());

const uploadDir = path.join(__dirname, 'uploads');
if (!fs.existsSync(uploadDir)) {
    fs.mkdirSync(uploadDir);
}

const storage = multer.diskStorage({
    destination: function (req, file, cb) {
        cb(null, uploadDir);
    },
    filename: function (req, file, cb) {
        // 【核心修复】：后端解码前端传来的 URL 编码，还原纯净中文名
        const decodedName = decodeURIComponent(file.originalname);
        cb(null, Date.now() + '-' + decodedName);
    }
});

const upload = multer({ storage: storage });

app.post('/api/upload', upload.single('file'), (req, res) => {
    if (!req.file) {
        return res.status(400).json({ code: 400, msg: "文件流为空" });
    }
    // 注意：这里后续上云要改成你的云服务器IP
    const fileUrl = `http://127.0.0.1:8082/uploads/${req.file.filename}`;
    res.json({
        code: 200,
        data: {
            url: fileUrl,
            fileName: decodeURIComponent(req.file.originalname)
        }
    });
});

app.use('/uploads', express.static(uploadDir));

app.listen(8082, () => {
    console.log('分布式文件存储微服务已启动，监听端口: 8082');
});