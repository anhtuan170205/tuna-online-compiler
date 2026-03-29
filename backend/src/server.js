const express = require('express');
const cors = require('cors');
const fs = require('fs/promises');
const path = require('path');
const os = require('os');
const { execFile } = require('child_process');
const { promisify } = require('util');
const { stderr, exitCode, stdout } = require('process');

const app = express();
const PORT = 5050;

app.use(cors());
app.use(express.json( {limit: '100kb' }));

app.get('/api/health', (req, res) => {
  	res.json({ status: 'ok' });
});

app.post('/api/run', async (req, res) => {
	const { code } = req.body;

	if (typeof code !== 'string') {
		return res.status(400).json({
			success: false,
			stdout: '',
			stderr: 'Invalid code format. Expected a string.',
			exitCode: 1
		});
	}

	const compilerPath = path.resolve(__dirname, '../compiler/tuna-lang/tuna');
	let tempDir = '';
	try {
		tempDir = await fs.mkdtemp(path.join(os.tmpdir(), 'tuna-'));
		const sourcePath = path.join(tempDir, 'main.tuna');

		await fs.writeFile(sourcePath, code, 'utf-8');

		const { stdout, stderr } = await promisify(execFile)(compilerPath, [sourcePath], { 
			timeout: 3000,
			maxBuffer: 1024 * 1024
		});

		res.json({
			success: true,
			stdout,
			stderr,
			exitCode: 0
		});
	} catch (error) {
		res.json({
			success: false,
			stdout: error.stdout || '',
			stderr: error.stderr || error.message || 'Execution failed',
			exitCode: typeof error.code === 'number' ? error.code : 1
		});
	} finally {
		if (tempDir) {
			await fs.rm(tempDir, { recursive: true, force: true });
		}
	}
});

app.listen(PORT, () => {
	console.log(`Backend server is running on http://localhost:${PORT}`);
});