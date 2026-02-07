#!/usr/bin/env python3
"""
Micro-Hypervisor Web Interface
Dashboard para monitoramento e controle
"""

from flask import Flask, render_template, jsonify, request
import subprocess
import json
import os
from pathlib import Path
import psutil
import time

app = Flask(__name__)

class ProjectManager:
    """Gerencia operações do projeto"""

    def __init__(self):
        self.root = Path(__file__).parent.parent
        self.build_dir = self.root / 'build'

    def build(self, clean=False):
        """Compila o projeto"""
        if clean:
            subprocess.run(['rm', '-rf', str(self.build_dir)], cwd=self.root)

        self.build_dir.mkdir(exist_ok=True)

        # CMake configure
        result = subprocess.run(
            ['cmake', '..'],
            cwd=self.build_dir,
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            return {'success': False, 'error': result.stderr}

        # Make
        result = subprocess.run(
            ['make', '-j4'],
            cwd=self.build_dir,
            capture_output=True,
            text=True
        )

        return {
            'success': result.returncode == 0,
            'output': result.stdout,
            'error': result.stderr
        }

    def test(self):
        """Roda testes"""
        test_script = self.root / 'scripts' / 'test.sh'
        result = subprocess.run(
            ['bash', str(test_script)],
            capture_output=True,
            text=True,
            cwd=self.root
        )

        return {
            'success': result.returncode == 0,
            'output': result.stdout + result.stderr
        }

    def get_status(self):
        """Retorna status do projeto"""
        status = {
            'binary_exists': (self.build_dir / 'MicroHypervisor').exists(),
            'build_dir_exists': self.build_dir.exists(),
        }

        if status['binary_exists']:
            size = (self.build_dir / 'MicroHypervisor').stat().st_size
            status['binary_size'] = f"{size:,} bytes"
            status['binary_size_mb'] = f"{size / (1024*1024):.2f} MB"

        # Verificar dependências
        deps = {}
        for cmd in ['gcc', 'g++', 'nasm', 'cmake', 'make', 'qemu-system-x86_64']:
            try:
                subprocess.run(['which', cmd], capture_output=True, check=True)
                deps[cmd] = True
            except:
                deps[cmd] = False

        status['dependencies'] = deps

        # Informações do sistema
        status['system'] = {
            'cpu_count': psutil.cpu_count(),
            'cpu_percent': psutil.cpu_percent(interval=1),
            'memory_total': f"{psutil.virtual_memory().total / (1024**3):.2f} GB",
            'memory_available': f"{psutil.virtual_memory().available / (1024**3):.2f} GB",
            'memory_percent': psutil.virtual_memory().percent
        }

        return status

manager = ProjectManager()

@app.route('/')
def index():
    """Página principal"""
    return render_template('index.html')

@app.route('/api/status')
def api_status():
    """API: Status do projeto"""
    return jsonify(manager.get_status())

@app.route('/api/build', methods=['POST'])
def api_build():
    """API: Compilar projeto"""
    data = request.get_json() or {}
    clean = data.get('clean', False)

    result = manager.build(clean=clean)
    return jsonify(result)

@app.route('/api/test', methods=['POST'])
def api_test():
    """API: Rodar testes"""
    result = manager.test()
    return jsonify(result)

@app.route('/api/system')
def api_system():
    """API: Informações do sistema"""
    return jsonify({
        'cpu_percent': psutil.cpu_percent(interval=1),
        'memory': dict(psutil.virtual_memory()._asdict()),
        'disk': dict(psutil.disk_usage('/')._asdict()),
        'load_avg': os.getloadavg() if hasattr(os, 'getloadavg') else [0, 0, 0]
    })

if __name__ == '__main__':
    # Criar diretório de templates
    template_dir = Path(__file__).parent / 'templates'
    template_dir.mkdir(exist_ok=True)

    # Criar HTML template
    html_content = '''<!DOCTYPE html>
<html>
<head>
    <title>Micro-Hypervisor Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .header {
            background: rgba(255,255,255,0.95);
            padding: 30px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        .header h1 {
            color: #667eea;
            margin-bottom: 10px;
        }
        .cards {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .card {
            background: rgba(255,255,255,0.95);
            padding: 25px;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        }
        .card h3 {
            color: #764ba2;
            margin-bottom: 15px;
            font-size: 1.2em;
        }
        .status-item {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid #eee;
        }
        .status-item:last-child {
            border-bottom: none;
        }
        .status-label {
            font-weight: 500;
            color: #555;
        }
        .status-value {
            color: #667eea;
            font-weight: 600;
        }
        .status-value.yes {
            color: #10b981;
        }
        .status-value.no {
            color: #ef4444;
        }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            font-weight: 600;
            transition: transform 0.2s, box-shadow 0.2s;
            margin-right: 10px;
            margin-bottom: 10px;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.3);
        }
        .btn:active {
            transform: translateY(0);
        }
        .btn-secondary {
            background: linear-gradient(135deg, #10b981 0%, #059669 100%);
        }
        .output {
            background: #1f2937;
            color: #f3f4f6;
            padding: 20px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            max-height: 400px;
            overflow-y: auto;
            white-space: pre-wrap;
            margin-top: 15px;
        }
        .progress-bar {
            width: 100%;
            height: 8px;
            background: #e5e7eb;
            border-radius: 4px;
            overflow: hidden;
            margin-top: 15px;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
            transition: width 0.3s;
        }
        .loading {
            display: inline-block;
            width: 20px;
            height: 20px;
            border: 2px solid #fff;
            border-radius: 50%;
            border-top-color: transparent;
            animation: spin 1s linear infinite;
        }
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🛡️ Micro-Hypervisor Dashboard</h1>
            <p>Trusted Execution Environment - Monitoramento e Controle</p>
        </div>

        <div class="cards">
            <div class="card">
                <h3>📊 Status do Projeto</h3>
                <div id="project-status">Carregando...</div>
            </div>

            <div class="card">
                <h3>💻 Sistema</h3>
                <div id="system-info">Carregando...</div>
            </div>

            <div class="card">
                <h3>🔧 Ações</h3>
                <button class="btn" onclick="build(false)">Build</button>
                <button class="btn" onclick="build(true)">Rebuild</button>
                <button class="btn btn-secondary" onclick="runTests()">Rodar Testes</button>
                <button class="btn" onclick="refresh()">Atualizar</button>
                <div id="action-output"></div>
            </div>
        </div>
    </div>

    <script>
        function refresh() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    let html = '';
                    for (const [key, value] of Object.entries(data)) {
                        if (typeof value === 'object') continue;
                        const status = value ? 'yes' : 'no';
                        const label = value ? '✓' : '✗';
                        html += `<div class="status-item">
                            <span class="status-label">${key}</span>
                            <span class="status-value ${status}">${label}</span>
                        </div>`;
                    }
                    if (data.binary_size) {
                        html += `<div class="status-item">
                            <span class="status-label">Size</span>
                            <span class="status-value">${data.binary_size}</span>
                        </div>`;
                    }
                    document.getElementById('project-status').innerHTML = html;
                });

            fetch('/api/system')
                .then(r => r.json())
                .then(data => {
                    let html = `<div class="status-item">
                        <span class="status-label">CPU</span>
                        <span class="status-value">${data.cpu_percent.toFixed(1)}%</span>
                    </div>`;
                    document.getElementById('system-info').innerHTML = html;
                });
        }

        function build(clean) {
            const output = document.getElementById('action-output');
            output.innerHTML = '<div class="loading"></div> Building...';

            fetch('/api/build', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({clean: clean})
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    output.innerHTML = '<div class="output">✓ Build completo!</div>';
                } else {
                    output.innerHTML = `<div class="output">✗ Build falhou:\n${data.error}</div>`;
                }
                refresh();
            });
        }

        function runTests() {
            const output = document.getElementById('action-output');
            output.innerHTML = '<div class="loading"></div> Rodando testes...';

            fetch('/api/test', {method: 'POST'})
            .then(r => r.json())
            .then(data => {
                output.innerHTML = `<div class="output">${data.output}</div>`;
            });
        }

        refresh();
        setInterval(refresh, 5000);
    </script>
</body>
</html>'''

    with open(template_dir / 'index.html', 'w') as f:
        f.write(html_content)

    print("""
╔════════════════════════════════════════╗
║  Micro-Hypervisor Web Interface        ║
╚════════════════════════════════════════╝

Acesse: http://localhost:5000

Pressione Ctrl+C para parar
    """)

    app.run(host='0.0.0.0', port=5000, debug=False)
