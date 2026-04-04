'use client';

import { Editor, useMonaco } from "@monaco-editor/react";
import { useEffect, useState, useRef, useCallback } from "react";
import { registerTunaLanguage } from "../lib/tunaLanguage";
import XTerminal from "@/components/Terminal";

export default function Home() {
	const [code, setCode] = useState(`print "Hello, World!"`);
	const [error, setError] = useState('');
	const [running, setRunning] = useState(false);
	const [wsConnected, setWsConnected] = useState(false);

	const monaco = useMonaco();
	const socketRef = useRef<WebSocket | null>(null);
	const terminalRef = useRef<any>(null);
	const handleTerminalReady = useCallback((term: any) => {
		terminalRef.current = term;
	}, []);

	const clearTerminal = () => {
		const socket = socketRef.current;
		const term = terminalRef.current;


		if (socket && socket.readyState === WebSocket.OPEN && running) {
			socket.send(JSON.stringify({ type: 'stop' }));
		}

		if (term) {
			term.clear();
			term.focus();
		}

		setRunning(false);
	};

	useEffect(() => {
		if (!monaco) return;
		registerTunaLanguage(monaco);
	}, [monaco]);

	useEffect(() => {
		const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
		const ws = new WebSocket('ws://127.0.0.1:5050');
		socketRef.current = ws;

		ws.onopen = () => {
			setWsConnected(true);
			setError('');
		};

		ws.onmessage = (event) => {
			try {
				const msg = JSON.parse(event.data);

				if (!terminalRef.current) {
					return;
				}

				if (msg.type === 'stdout') {
					terminalRef.current.write(msg.data);
				}

				if (msg.type === 'stderr') {
					terminalRef.current.write(msg.data);
				}

				if (msg.type === 'exit') {
					// terminalRef.current.writeln(`\r\n[Process exited with code ${msg.code}]`);
					setRunning(false);
				}
			} catch (e) {
				console.error('Invalid WebSocket message', e);
			}
		};

		ws.onclose = () => {
			setWsConnected(false);
			setRunning(false);
		};

		ws.onerror = () => {
			setWsConnected(false);
			setRunning(false);
			setError('WebSocket connection failed');
		};

		return () => {
			ws.close();
		};
	}, []);

	const runCode = async () => {
		const socket = socketRef.current;
		const term = terminalRef.current;

		if (!socket || socket.readyState !== WebSocket.OPEN || !term) {
			setError('WebSocket not connected');
			return;
		}

		setError('');
		setRunning(true);

		term.clear();
		term.writeln('Running Tuna...');
		term.focus();

		socket.send(JSON.stringify({
			type: 'run',
			code
		}));
	};

	return (
		<main className='p-6 w-full max-w-4xl mx-auto'>
			<h1 className='text-3xl font-bold mb-4'>
				Tuna Online Compiler
			</h1>

			{/* Code Editor */}
			<Editor 
				height='400px'
				defaultLanguage='tuna'
				theme='vs-dark'
				value={code}
				onChange={(value) => setCode(value || '')}
				options={{
					minimap: { enabled: false },
					fontSize: 16,
					automaticLayout: true,
				}}
			/>

			{/* Run Button */}
			<button 
				onClick={runCode}
				className='mt-4 px-4 py-2 bg-black text-white rounded-lg hover:bg-gray-600 disabled:bg-gray-400'
				>
				Run
			</button>

			{/* Clear Button */}
			<button 
				onClick={clearTerminal}
				className='mt-4 px-4 py-2 bg-gray-500 text-white rounded-lg hover:bg-gray-600'
				>
				Clear
			</button>

			{/* Terminal */}
			<div className="mt-6">
				<h2 className="text-2xl font-semibold mb-2 text-black">Terminal</h2>
					<XTerminal
					socketRef={socketRef}
					onReady={handleTerminalReady}
				/>
			</div>

			{/* Error */}
			{error && (
				<div className='mt-6'>
					<h2 className='text-2xl font-semibold mb-2 text-red-600'>Error:</h2>
					<pre className='bg-red-100 p-4 rounded-lg min-h-24 whitespace-pre-wrap'>{error}</pre>
				</div>
			)}
		</main>
	)
}
