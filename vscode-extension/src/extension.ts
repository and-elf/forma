import * as path from 'path';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind,
    Executable
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

export function activate(context: vscode.ExtensionContext) {
    console.log('Forma language extension is now active');

    // Get configuration
    const config = vscode.workspace.getConfiguration('forma');
    let serverPath = config.get<string>('languageServer.path') || 'forma_lsp_server_stdio';

    // Check if server path is relative to workspace
    if (vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders.length > 0) {
        const workspaceRoot = vscode.workspace.workspaceFolders[0].uri.fsPath;
        const possiblePaths = [
            path.join(workspaceRoot, 'build', 'forma_lsp_server_stdio'),
            path.join(workspaceRoot, 'plugins', 'lsp-server', 'build', 'forma_lsp_server_stdio'),
            serverPath
        ];

        // Try to find the server executable
        for (const tryPath of possiblePaths) {
            if (tryPath !== serverPath) {
                serverPath = tryPath;
                break; // Use first match or default to configured path
            }
        }
    }

    // Server executable configuration
    const serverExecutable: Executable = {
        command: serverPath,
        args: [],
        transport: TransportKind.stdio
    };

    // Server options
    const serverOptions: ServerOptions = serverExecutable;

    // Client options
    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'forma' },
            { scheme: 'untitled', language: 'forma' }
        ],
        synchronize: {
            // Notify the server about file changes to '.forma' and '.fml' files
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.{forma,fml}')
        },
        outputChannelName: 'Forma Language Server',
        revealOutputChannelOn: 2 // RevealOutputChannelOn.OnInfo
    };

    // Create and start the language client
    client = new LanguageClient(
        'formaLanguageServer',
        'Forma Language Server',
        serverOptions,
        clientOptions
    );

    // Start the client (which also starts the server)
    client.start().catch((error) => {
        vscode.window.showErrorMessage(
            `Failed to start Forma Language Server: ${error.message}\n\n` +
            `Please ensure 'forma_lsp_server_stdio' is built and available at: ${serverPath}\n\n` +
            `You can configure the path in settings: forma.languageServer.path`
        );
        console.error('Failed to start language server:', error);
    });

    // Register commands
    const restartCommand = vscode.commands.registerCommand('forma.restartLanguageServer', async () => {
        if (client) {
            await client.stop();
            await client.start();
            vscode.window.showInformationMessage('Forma Language Server restarted');
        }
    });

    context.subscriptions.push(restartCommand);

    // Show status bar item
    const statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    statusBarItem.text = '$(check) Forma';
    statusBarItem.tooltip = 'Forma Language Server is running';
    statusBarItem.command = 'forma.restartLanguageServer';
    statusBarItem.show();
    context.subscriptions.push(statusBarItem);
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
