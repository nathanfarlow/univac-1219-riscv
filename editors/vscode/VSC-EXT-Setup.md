## VSC Windows Extension

```bash
# cd to the extension directory
cd .\univac-1219-riscv\univac-1219-vscode

# Install dependencies and compile
npm install
npx tsc -p .

# Install locally into VS Code
Copy-Item -Recurse -Force "." "$env:USERPROFILE\.vscode\extensions\univac-1219-asm"

# Restart VS Code
```