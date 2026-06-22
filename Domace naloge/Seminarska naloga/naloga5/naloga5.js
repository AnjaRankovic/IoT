import express from 'express';
import http from 'http';
import path from 'path';
import { fileURLToPath } from 'url';
import { WebSocketServer, WebSocket } from 'ws';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const app = express();
const server = http.createServer(app);

const wssChrome = new WebSocketServer({ port: 8888 });
const wssESP = new WebSocketServer({ port: 8811 });

app.use(express.static(__dirname));

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'naloga5.html'));
});

function broadcastToChrome(data) {
    wssChrome.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(data);
        }
    });
}

function broadcastToESP(data) {
    wssESP.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(data);
        }
    });
}

wssChrome.on('connection', (ws) => {
    ws.on('message', (message) => {
        try {
            let msg = JSON.parse(message);
            if (msg.tipSporočila === "POSTAVI_ZELENO") {
                broadcastToESP(JSON.stringify(msg));
            }
        } catch (e) { console.error(e); }
    });
});

wssESP.on('connection', (ws) => {
    ws.on('message', (message) => {
        try {
            let msg = JSON.parse(message);
            if (msg.tipSporočila === "pi_podatki") {
                broadcastToChrome(JSON.stringify(msg));
            }
        } catch (e) { console.error(e); }
    });
});

server.listen(8080, '0.0.0.0', () => {
    console.log('Server za 5. zadatak (PI regulator) pokrenut na portu 8080');
});