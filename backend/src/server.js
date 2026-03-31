const http = require('http');
const app = require('./app');
const { setupWebSocket } = require('./websocket/terminal');

const PORT = 5050;

const server = http.createServer(app);
setupWebSocket(server);

app.listen(PORT, () => {
	console.log(`Backend server is running on http://localhost:${PORT}`);
});