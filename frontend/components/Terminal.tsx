'use client';

import React, { useEffect, useRef } from 'react'
import '@xterm/xterm/css/xterm.css';

type Props = {
	socketRef: React.RefObject<WebSocket | null>;
	onReady: (term: any) => void;
}

export default function XTerminal({ socketRef, onReady }: Props) {
	const containerRef = useRef<HTMLDivElement | null>(null);

	useEffect(() => {
		let term: any = null;
		let fitAddon: any = null;
		let resizeHandler: (() => void) | null = null;
		let dataDisposable: { dispose: () => void } | null = null;

		const initTerminal = async () => {
			if (!containerRef.current) return;

			const { Terminal } = await import('@xterm/xterm');
			const { FitAddon } = await import('@xterm/addon-fit');

			term = new Terminal({
				cursorBlink: true,
				convertEol: true,
				fontSize: 14,
				theme: {
					background: '#111111'
				}
			});

			fitAddon = new FitAddon();
			term.loadAddon(fitAddon);
			term.open(containerRef.current);
			fitAddon.fit();

			term.writeln('Tuna terminal ready.');
			term.writeln('');

			onReady(term);

			resizeHandler = () => {
				fitAddon.fit();
			};

			dataDisposable = term.onData((data: string) => {
				const socket = socketRef.current;
				if (!socket || socket.readyState !== WebSocket.OPEN) {
					console.error('WebSocket is not open. Cannot send data:', data);
					return;
				}

				if (data === '\r') {
					term.write('\r\n');
					socket.send(JSON.stringify({
						type: 'stdin',
						data: '\n'
					}));
					return;
				}

				if (data === '\u007f') {
					term.write('\b \b');
					socket.send(JSON.stringify({
						type: 'stdin',
						data
					}));
					return;
				}

				term.write(data);

				socket.send(JSON.stringify({
					type: 'stdin',
					data
				}));
			});
			window.addEventListener('resize', resizeHandler);
		};

		initTerminal();

		return () => {
			if (dataDisposable) dataDisposable.dispose();
			if (resizeHandler) window.removeEventListener('resize', resizeHandler);
			if (term) term.dispose();
		};
	}, []);

	return <div ref={containerRef} className='w-full h-96 rounded-lg overflow-hidden border border-gray-300' />;
}

