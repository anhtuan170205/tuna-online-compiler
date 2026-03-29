'use client';

import { Editor, useMonaco } from "@monaco-editor/react";
import { useEffect, useState } from "react";
import { registerTunaLanguage } from "../lib/tunaLanguage";

export default function Home() {
	const [code, setCode] = useState(`print "Hello, World!"`);
	const [output, setOutput] = useState('');
	const [error, setError] = useState('');
	const [loading, setLoading] = useState(false);

	const monaco = useMonaco();

	useEffect(() => {
		if (!monaco) return;
		registerTunaLanguage(monaco);
	}, [monaco]);

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
				body: JSON.stringify({ code })
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

			{/* Output */}
			<div className='mt-6'>
				<h2 className='text-2xl font-semibold mb-2'>Output:</h2>
				<pre className='bg-gray-100 p-4 rounded-lg min-h-24 whitespace-pre-wrap'>{output || 'No output'}</pre>
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
