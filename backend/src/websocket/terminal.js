const WebSocket = require('ws');
const fs = require('fs/promises');
const path = require('path');
const os = require('os');
const { spawn } = require('child_process');

function setupWebSocket(server) {
	const wss = new WebSocket.Server({ server });

	wss.on('connection', (ws) => {
		console.log('WS client connected');

		let child = '';
		let tempDir = '';

		ws.on('message', async (message) => {
			try {
				const payload = JSON.parse(message.toString());

				if (payload.type === 'run') {
					if (child) {
						child.kill('SIGKILL');
						child = null;
					}

					if (tempDir) {
						try {
							await fs.rm(tempDir, { recursive: true, force: true });
						} catch {}
						tempDir = '';
					}

					const code = payload.code;

					if (typeof code !== 'string') {
						if (ws.readyState === WebSocket.OPEN) {
							ws.send(JSON.stringify({
								type: 'stderr',
								data: 'Invalid code format'
							}));
						}
						return;
					}

					const compilerPath = path.resolve(__dirname, '../../compiler/tuna-lang/tuna');

					tempDir = await fs.mkdtemp(path.join(os.tmpdir(), 'tuna-'));
					const sourcePath = path.join(tempDir, 'main.tuna');
					await fs.writeFile(sourcePath, code, 'utf-8');

					child = spawn(compilerPath, [sourcePath], {
						stdio: ['pipe', 'pipe', 'pipe']
					});

					child.stdout.on('data', (data) => {
						if (ws.readyState === WebSocket.OPEN) {
							ws.send(JSON.stringify({
								type: 'stdout',
								data: data.toString()
							}));
						}
					});

					child.stderr.on('data', (data) => {
						if (ws.readyState === WebSocket.OPEN) {
							ws.send(JSON.stringify({
								type: 'stderr',
								data: data.toString()
							}));
						}
					});

					child.on('close', async (code) => {
						if (ws.readyState === WebSocket.OPEN) {
							ws.send(JSON.stringify({
								type: 'exit',
								code: code ?? 1
							}));
						}
						
						if (tempDir) {
							try {
								await fs.rm(tempDir, { recursive: true, force: true });
							} catch {}
							tempDir = '';
						}
						child = null;
					});

					child.on('error', async (error) => {
						if (ws.readyState === WebSocket.OPEN) {
							ws.send(JSON.stringify({
								type: 'stderr',
								data: error.message || 'Execution failed'
							}));
						}
						
						if (tempDir) {
							try {
								await fs.rm(tempDir, { recursive: true, force: true });
							} catch {}
							tempDir = '';
						}
						child = null;
					});
				}
				if (payload.type === 'stdin') {
					if (child) {
						child.stdin.write(payload.data);
					}
				}
				if (payload.type === 'stop') {
					if (child) {
						child.kill('SIGKILL');
					}
				}
			} catch (error) {
				console.error('WS message error:', error);
				if (ws.readyState === WebSocket.OPEN) {
					ws.send(JSON.stringify({
						type: 'stderr',
						data: 'Invalid WebSocket message'
					}));
				}
			}
		});

		ws.on('close', async () => {
			console.log('WS client disconnected');

			if (child) {
				child.kill('SIGKILL');
			}

			if (tempDir) {
				try {
					await fs.rm(tempDir, { recursive: true, force: true });
				} catch {}
			}
		});
	});
}

module.exports = { setupWebSocket };