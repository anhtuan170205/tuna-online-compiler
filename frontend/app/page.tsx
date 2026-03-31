'use client';

import { Editor, useMonaco } from "@monaco-editor/react";
import { useEffect, useState, useRef } from "react";
import { registerTunaLanguage } from "../lib/tunaLanguage";
import XTerminal from "@/components/Terminal";

export default function Home() {
	const [code, setCode] = useState(`print "Hello, World!"`);
	const [input, setInput] = useState('');
	const [output, setOutput] = useState('');
	const [error, setError] = useState('');
	const [loading, setLoading] = useState(false);
	const [interactiveRunning, setInteractiveRunning] = useState(false);

	const monaco = useMonaco();

	const socketRef = useRef<WebSocket | null>(null);
	const terminalRef = useRef<any>(null);

	useEffect(() => {
		if (!monaco) return;
		registerTunaLanguage(monaco);
	}, [monaco]);

	useEffect(() => {
		const socket = new WebSocket('ws://localhost:5050/');
		socketRef.current = socket;

		socket.onopen = () => {
			console.log('WebSocket connected');
		};

		socket.onmessage = (event) => {
			try {
				const msg = JSON.parse(event.data);

				if (!terminalRef.current) return;

				if (msg.type === 'stdout') {
					terminalRef.current.write(msg.data);
				}

				if (msg.type === 'stderr') {
					terminalRef.current.write(`\r\n[stderr] \r\n${msg.data}`);
				}

				if (msg.type === 'exit') {
					terminalRef.current.write(`\r\n[Process exited with code ${msg.code}]`);
				}
			} catch {
				console.error('Invalid WebSocket message');
			}
		};

		socket.onclose = () => {
			console.log('WebSocket disconnected');
			setInteractiveRunning(false);
		};

		socket.onerror = () => {
			console.error('WebSocket error');
			setInteractiveRunning(false);
		};

		return () => {
			socket.close();
		};
	}, []);

	const runCode = async () => {
		setLoading(true);
		setOutput("");
		setError("");

		try {
			const res = await fetch('http://localhost:5050/api/run', {
				method: 'POST',
				headers: {
					'Content-Type': 'application/json'
				},
				body: JSON.stringify({ code, input })
			});

			const data = await res.json();

			setOutput(data.stdout || '');
			setError(data.stderr || '');
		} catch (err: any) {
			setError(err.message || 'Request failed');
		} finally {
			setLoading(false);
		}
	};

	const runInteractive = () => {
		const socket = socketRef.current;
		const term = terminalRef.current;

		if (!socket || socket.readyState !== WebSocket.OPEN || !term) {
			setError('WebSocket not connected');
			return;
		}

		setError('');
		setInteractiveRunning(true);

		term.clear();
		term.writeln('Running Tuna');
		term.writeln('');

		socket.send(JSON.stringify({
			type: 'run',
			data: code
		}));
	};

	const stopInteractive = () => {
		const socket = socketRef.current;

		if (!socket || socket.readyState !== WebSocket.OPEN) return;
		
		socket.send(JSON.stringify({
			type: 'stop'
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
				{loading ? 'Running...' : 'Run Code'}
			</button>

			{/* Run Interactive Button */}
			<button 
				onClick={runInteractive}
				className='mt-4 px-4 py-2 bg-blue-500 text-white rounded-lg hover:bg-blue-600 disabled:bg-gray-400'
				disabled={interactiveRunning}
				>
				{interactiveRunning ? 'Interactive Running...' : 'Run Interactive'}
			</button>

			{/* Stop Interactive Button */}
			<button 
				onClick={stopInteractive}
				className='mt-4 px-4 py-2 bg-red-500 text-white rounded-lg hover:bg-red-600 disabled:bg-gray-400'
				disabled={!interactiveRunning}
				>
				Stop Interactive
			</button>

			<div className='mt-6'>
				<h2 className='text-2xl font-semibold mb-2 text-black'>Interactive Terminal</h2>
				<XTerminal
					socketRef={socketRef}
					onReady={(term) => {
						terminalRef.current = term;
					}}
				/>
			</div>

			{/* Output */}
			<div className='mt-6'>
				<h2 className='text-2xl font-semibold mb-2'>Output:</h2>
				<pre className='bg-gray-100 p-4 rounded-lg min-h-24 whitespace-pre-wrap'>{output || 'No output'}</pre>
			</div>

			{/* Input Area */}
			<h2 className='text-xl font-semibold mb-2'>Input:</h2>
			<textarea
				value={input}
				onChange={(e) => setInput(e.target.value)}
				placeholder='Program stdin here...'
				className='w-full h-40 rounded-lg border border-gray-300 p-4 resize-y'
			/>

			{/* Error */}
			{error && (
				<div className='mt-6'>
					<h2 className='text-2xl font-semibold mb-2 text-red-600'>Error:</h2>
					<pre className='bg-red-100 p-4 rounded-lg min-h-24 whitespace-pre-wrap'>{error || 'No error'}</pre>
				</div>
			)}
		</main>
	)
}
