const express = require('express');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { spawn } = require('child_process');
const { stdout, exit } = require('process');

const router = express.Router();

router.post('/', async (req, res) => {
	const { code, input = '' } = req.body;

	if (typeof code !== 'string') {
		return res.status(400).json({
			success: false,
			stdout: '',
			stderr: 'Invalid code format. Expected a string.',
			exitCode: 1
		});
	}

	if (typeof input !== 'string') {
		return res.status(400).json({
			success: false,
			stdout: '',
			stderr: 'Invalid input format. Expected a string.',
			exitCode: 1
		});
	}

	const compilerPath = path.resolve(__dirname, '../../compiler/tuna-lang/tuna');
	let tempDir = '';

	try {
		tempDir = await fs.mkdtemp(path.join(os.tmpdir(), 'tuna-'));
		const sourcePath = path.join(tempDir, 'main.tuna');
		await fs.writeFileSync(sourcePath, code, 'utf-8');

		const child = spawn(compilerPath, [sourcePath], {
			stdio: ['pipe', 'pipe', 'pipe']
		});

		let stdout = '';
		let stderr = '';
		let responded = false;

		const timeout = setTimeout(() => {
			if (!responded) {
				child.kill('SIGKILL');
			}
		}, 5000);

		child.stdout.on('data', (data) => {
			stdout += data.toString();
		});

		child.stderr.on('data', (data) => {
			stderr += data.toString();
		});

		child.on('error', (error) => {
			if (responded) return;
			responded = true;
			clearTimeout(timeout);

			res.status(500).json({
				success: false,
				stdout,
				stderr: error.message || 'Execution failed',
				exitCode: 1
			});
		});

		child.on('close', async (code) => {
			if (responded) return;
			responded = true;
			clearTimeout(timeout);

			try {
				await fs.rm(tempDir, { recursive: true, force: true });
			} catch {}

			res.json({
				success: code === 0,
				stdout,
				stderr,
				exitCode: code ?? 1
			});
		});
		
		child.stdin.write(input);
		child.stdin.end();
	} catch (error) {
		if (tempDir) {
			try {
				await fs.rm(tempDir, { recursive: true, force: true });
			} catch {}
		}

		res.status(500).json({
			success: false,
			stdout: '',
			stderr: error.message || 'Server error',
			exitCode: 1
		});
	}
});

module.exports = router;