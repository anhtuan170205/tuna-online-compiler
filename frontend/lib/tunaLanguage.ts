export function registerTunaLanguage(monaco: any) {
	monaco.languages.register({ id: 'tuna' });

	monaco.languages.setMonarchTokensProvider('tuna', {
		keywords: [
			"set",
			"print",
			"if",
			"then",
			"else",
			"end",
			"while",
			"do",
			"true",
			"false",
			"and",
			"or",
			"not",
			"len",
			"readint",
			"readstr",
			"rand",
			"func",
			"return",
			"push",
			"remove",
			"chars",
		],
		tokenizer: {
			root: [
				[/#.*/, "comment"],
				[/\"([^\"\\\n]|\\[ntr"\\])*\"/, "string"],
				[/\d+/, "number"],

				[/[a-zA-Z_][a-zA-Z0-9_]*/, {
				cases: {
					"@keywords": "keyword",
					"@default": "identifier",
				},
				}],

				[/:=|==|!=|<=|>=|<|>|\+|-|\*|\/|%/, "operator"],
				[/\[|\]|\(|\)|,/, "delimiter"],

				[/\r?\n/, "white"],
				[/[ \t\r]+/, "white"],
			],
		}
	});

	monaco.editor.defineTheme('tunaTheme', {
		base: 'vs-dark',
		inherit: true,
		rules: [
			{ token: "keyword", foreground: "569CD6", fontStyle: "bold" },
			{ token: "identifier", foreground: "D4D4D4" },
			{ token: "number", foreground: "B5CEA8" },
			{ token: "string", foreground: "CE9178" },
			{ token: "comment", foreground: "6A9955", fontStyle: "italic" },
			{ token: "operator", foreground: "D4D4D4" },
			{ token: "delimiter", foreground: "D4D4D4" },
		],
		colors: {}
	})
}