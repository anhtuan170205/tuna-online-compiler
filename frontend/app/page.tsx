'use client';

import { Editor, useMonaco } from "@monaco-editor/react";
import { useEffect, useState, useRef } from "react";
import { registerTunaLanguage } from "../lib/tunaLanguage";
import XTerminal from "@/components/Terminal";

export default function Home() {
	const [code, setCode] = useState(`print "Hello, World!"`);
	const [error, setError] = useState('');
	const [interactiveRunning, setInteractiveRunning] = useState(false);
	const [wsConnected, setWsConnected] = useState(false);

	const monaco = useMonaco();
	const socketRef = useRef<WebSocket | null>(null);
	const terminalRef = useRef<any>(null);

	useEffect(() => {
		if (!monaco) return;
		registerTunaLanguage(monaco);
	}, [monaco]);

	useEffect(() => {
		const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
		const wsHost = 'localhost:5050'; // Change if backend is hosted elsewhere
		const socket = new WebSocket(`${protocol}://${wsHost}/`);
		socketRef.current = socket;

		socket.onopen = () => {
			console.log('WebSocket connected');
			setWsConnected(true);
			setError('');
		};

		socket.onmessage = (event) => {
			try {
				const msg = JSON.parse(event.data);

				if (!terminalRef.current) return;

				if (msg.type === 'stdout') {
					terminalRef.current.write(msg.data);
				}

				if (msg.type === 'stderr') {
					terminalRef.current.write(msg.data);
				}

				if (msg.type === 'exit') {
					terminalRef.current.write(`\r\n[Process exited with code ${msg.code}]`);
					setInteractiveRunning(false);
				}
			} catch {
				console.error('Invalid WebSocket message');
			}
		};

		socket.onclose = () => {
			console.log('WebSocket disconnected');
			setWsConnected(false);
			setInteractiveRunning(false);
		};

		socket.onerror = () => {
			console.error('WebSocket error');
			setWsConnected(false);
			setInteractiveRunning(false);
			setError('WebSocket connection failed');
		};

		return () => {
			socket.close();
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
		setInteractiveRunning(true);

		term.clear();
		term.writeln('Running Tuna...');
		term.writeln('');

		socket.send(JSON.stringify({
			type: 'run',
			code
		}));
	};

	const stopCode = () => {
		const socket = socketRef.current;

		if (!socket || socket.readyState !== WebSocket.OPEN) return;

		socket.send(JSON.stringify({
			type: 'stop'
		}));
	}

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
				{interactiveRunning ? 'Running...' : 'Run'}
			</button>


			{/* Stop Button */}
			<button 
				onClick={stopCode}
				className='mt-4 px-4 py-2 bg-red-500 text-white rounded-lg hover:bg-red-600 disabled:bg-gray-400'
				disabled={!interactiveRunning}
				>
				Stop 
			</button>

			{/* Terminal */}
			<div className="mt-6">
				<h2 className="text-2xl font-semibold mb-2 text-black">Terminal</h2>
					<XTerminal
					socketRef={socketRef}
					onReady={(term) => {
						terminalRef.current = term;
					}}
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
