'use client';

import { FitAddon } from '@xterm/addon-fit';
import React, { useEffect, useRef } from 'react'
import { Terminal } from 'xterm';

type Props = {
	socketRef: React.RefObject<WebSocket | null>;
	onReady: (term: Terminal) => void;
}

export default function XTerminal({ socketRef, onReady }: Props) {
	const containerRef = useRef<HTMLDivElement | null>(null);

	useEffect(() => {
		if (!containerRef.current) return;

		const term = new Terminal({
			cursorBlink: true,
			convertEol: true,
			fontSize: 14,
			fontFamily: 'Martian Mono, monospace',
			theme: { background: '#111'}
		});

		const fitAddon = new FitAddon();
		term.loadAddon(fitAddon);

		term.open(containerRef.current);
		fitAddon.fit();

		term.writeln('Tuna terminal ready');
		term.writeln('');

		onReady(term);

		term.onData(data => {
			const socket = socketRef.current;
			if (socket && socket.readyState === WebSocket.OPEN) {
				socket.send(JSON.stringify({ 
					type: 'stdin', 
					data 
				}));
			}
		});

		const handleResize = () => fitAddon.fit();
		window.addEventListener('resize', handleResize);

		return () => {
			window.removeEventListener('resize', handleResize);
			term.dispose();
		};
	}, [socketRef, onReady]);

	return (
		<div ref={containerRef} className='w-full h-75 border rounded-lg overflow-hidden'/>
	)
}

