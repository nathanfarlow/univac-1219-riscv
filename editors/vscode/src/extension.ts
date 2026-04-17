import * as vscode from "vscode";
import {
    instructions,
    instructionNames,
    directives,
    specialOperands,
    InstructionInfo,
} from "./instructionData";

const LANGUAGE_ID = "univac1219";

// ─── Hover Provider ─────────────────────────────────────────────────────────

class Univac1219HoverProvider implements vscode.HoverProvider {
    provideHover(
        document: vscode.TextDocument,
        position: vscode.Position,
    ): vscode.Hover | undefined {
        const wordRange = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);
        if (!wordRange) {
            return undefined;
        }

        const word = document.getText(wordRange).toUpperCase();
        const info = instructions.get(word);
        if (info) {
            return this.buildInstructionHover(info);
        }

        if (directives.includes(word)) {
            const md = new vscode.MarkdownString();
            md.appendCodeblock(word, "univac1219");
            md.appendMarkdown(`**Assembler directive**`);
            return new vscode.Hover(md, wordRange);
        }

        if (specialOperands.includes(word)) {
            const md = new vscode.MarkdownString();
            md.appendCodeblock(word, "univac1219");
            md.appendMarkdown(`**Special operand**`);
            return new vscode.Hover(md, wordRange);
        }

        // Check if it's a label reference — find its definition
        const labelDef = this.findLabelDefinition(document, word);
        if (labelDef !== undefined) {
            const md = new vscode.MarkdownString();
            md.appendMarkdown(`**Label** \`>${word}\` — defined at line ${labelDef + 1}`);
            return new vscode.Hover(md, wordRange);
        }

        return undefined;
    }

    private buildInstructionHover(info: InstructionInfo): vscode.Hover {
        const md = new vscode.MarkdownString();
        // Header
        const header = info.args ? `${info.name} ${info.args}` : info.name;
        md.appendCodeblock(header, "univac1219");

        // Details
        if (info.timing) {
            md.appendMarkdown(`**Timing:** ${info.timing}  \n`);
        }
        md.appendMarkdown(`**Description:** ${info.description}`);

        return new vscode.Hover(md);
    }

    private findLabelDefinition(document: vscode.TextDocument, symbol: string): number | undefined {
        const pattern = new RegExp(`^>${symbol}\\b`, "i");
        for (let i = 0; i < document.lineCount; i++) {
            if (pattern.test(document.lineAt(i).text)) {
                return i;
            }
        }
        return undefined;
    }
}

// ─── Completion Provider ─────────────────────────────────────────────────────

class Univac1219CompletionProvider implements vscode.CompletionItemProvider {
    provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position,
    ): vscode.CompletionItem[] {
        const items: vscode.CompletionItem[] = [];

        // Instructions
        for (const name of instructionNames) {
            const info = instructions.get(name)!;
            const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Keyword);
            item.detail = info.args ? `${name} ${info.args}` : name;
            item.documentation = new vscode.MarkdownString(
                info.timing
                    ? `**[${info.timing}]** ${info.description}`
                    : info.description
            );
            // Insert a space after the instruction for convenience
            item.insertText = name + " ";
            item.sortText = `0_${name}`;
            items.push(item);
        }

        // Directives
        for (const dir of directives) {
            const item = new vscode.CompletionItem(dir, vscode.CompletionItemKind.Snippet);
            item.detail = "Assembler directive";
            item.sortText = `1_${dir}`;
            items.push(item);
        }

        // Special operands
        for (const op of specialOperands) {
            const item = new vscode.CompletionItem(op, vscode.CompletionItemKind.Constant);
            item.detail = "Special operand";
            item.sortText = `2_${op}`;
            items.push(item);
        }

        // Labels defined in the current document
        const labelPattern = /^>([A-Za-z_][A-Za-z0-9_]*)/;
        for (let i = 0; i < document.lineCount; i++) {
            const match = document.lineAt(i).text.match(labelPattern);
            if (match) {
                const label = match[1];
                const item = new vscode.CompletionItem(label, vscode.CompletionItemKind.Reference);
                item.detail = `Label (line ${i + 1})`;
                item.sortText = `3_${label}`;
                items.push(item);
            }
        }

        return items;
    }
}

// ─── Go-to-Definition Provider ──────────────────────────────────────────────

class Univac1219DefinitionProvider implements vscode.DefinitionProvider {
    provideDefinition(
        document: vscode.TextDocument,
        position: vscode.Position,
    ): vscode.Location | undefined {
        const wordRange = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);
        if (!wordRange) {
            return undefined;
        }

        const word = document.getText(wordRange).toUpperCase();

        // Don't try to find definitions for instructions/directives/operands
        if (instructions.has(word) || directives.includes(word) || specialOperands.includes(word)) {
            return undefined;
        }

        // Search for label definition: >LABELNAME
        const pattern = new RegExp(`^>${word}\\b`, "i");

        // Search current document first
        for (let i = 0; i < document.lineCount; i++) {
            if (pattern.test(document.lineAt(i).text)) {
                return new vscode.Location(
                    document.uri,
                    new vscode.Position(i, 1) // position after the >
                );
            }
        }

        return undefined;
    }
}

// ─── Document Symbol Provider ───────────────────────────────────────────────

class Univac1219DocumentSymbolProvider implements vscode.DocumentSymbolProvider {
    provideDocumentSymbols(
        document: vscode.TextDocument,
    ): vscode.DocumentSymbol[] {
        const symbols: vscode.DocumentSymbol[] = [];
        const labelPattern = /^>([A-Za-z_][A-Za-z0-9_]*)/;

        for (let i = 0; i < document.lineCount; i++) {
            const line = document.lineAt(i);
            const match = line.text.match(labelPattern);
            if (match) {
                const name = match[1];
                const range = line.range;
                const selectionRange = new vscode.Range(
                    new vscode.Position(i, 1),
                    new vscode.Position(i, 1 + name.length)
                );
                const symbol = new vscode.DocumentSymbol(
                    name,
                    "label",
                    vscode.SymbolKind.Function,
                    range,
                    selectionRange
                );
                symbols.push(symbol);
            }
        }

        return symbols;
    }
}

// ─── Workspace Symbol Provider ──────────────────────────────────────────────
// Allows finding labels across all .univac files via Ctrl+T

class Univac1219WorkspaceSymbolProvider implements vscode.WorkspaceSymbolProvider {
    async provideWorkspaceSymbols(query: string): Promise<vscode.SymbolInformation[]> {
        const symbols: vscode.SymbolInformation[] = [];
        const files = await vscode.workspace.findFiles("**/*.univac");
        const labelPattern = /^>([A-Za-z_][A-Za-z0-9_]*)/;
        const queryUpper = query.toUpperCase();

        for (const fileUri of files) {
            const doc = await vscode.workspace.openTextDocument(fileUri);
            for (let i = 0; i < doc.lineCount; i++) {
                const match = doc.lineAt(i).text.match(labelPattern);
                if (match) {
                    const name = match[1];
                    if (!query || name.toUpperCase().includes(queryUpper)) {
                        symbols.push(new vscode.SymbolInformation(
                            name,
                            vscode.SymbolKind.Function,
                            "",
                            new vscode.Location(fileUri, new vscode.Position(i, 1))
                        ));
                    }
                }
            }
        }

        return symbols;
    }
}

// ─── Extension Activation ───────────────────────────────────────────────────

export function activate(context: vscode.ExtensionContext) {
    const selector: vscode.DocumentSelector = { language: LANGUAGE_ID };

    // Register Hover Provider (instruction docs on hover)
    context.subscriptions.push(
        vscode.languages.registerHoverProvider(selector, new Univac1219HoverProvider())
    );

    // Register Completion Provider (auto-complete instructions, directives, labels)
    context.subscriptions.push(
        vscode.languages.registerCompletionItemProvider(
            selector,
            new Univac1219CompletionProvider(),
        )
    );

    // Register Definition Provider (Ctrl+Click / F12 go-to-definition for labels)
    context.subscriptions.push(
        vscode.languages.registerDefinitionProvider(selector, new Univac1219DefinitionProvider())
    );

    // Register Document Symbol Provider (Ctrl+Shift+O outline of labels)
    context.subscriptions.push(
        vscode.languages.registerDocumentSymbolProvider(selector, new Univac1219DocumentSymbolProvider())
    );

    // Register Workspace Symbol Provider (Ctrl+T search labels across files)
    context.subscriptions.push(
        vscode.languages.registerWorkspaceSymbolProvider(new Univac1219WorkspaceSymbolProvider())
    );

    console.log("UNIVAC-1219 Assembly extension activated");
}

export function deactivate() {}
