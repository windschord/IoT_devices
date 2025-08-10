#!/usr/bin/env python3
"""
GPS NTP Server - Test Report Generator

This script generates comprehensive test coverage and quality reports
without relying on complex gcov/lcov integration.

Features:
- Test execution summary
- Code metrics analysis  
- Component coverage analysis
- Quality indicators
- Performance metrics
"""

import os
import sys
import json
import subprocess
import datetime
from pathlib import Path

class TestReportGenerator:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        self.test_dir = self.project_root / "test"
        self.report_data = {}
        
    def run_command(self, cmd, capture_output=True):
        """Run shell command and return output"""
        try:
            if capture_output:
                result = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=self.project_root)
                return result.returncode, result.stdout, result.stderr
            else:
                result = subprocess.run(cmd, shell=True, cwd=self.project_root)
                return result.returncode, "", ""
        except Exception as e:
            return 1, "", str(e)
            
    def analyze_code_structure(self):
        """Analyze project code structure"""
        print("📁 Analyzing code structure...")
        
        structure = {
            "source_files": [],
            "test_files": [],
            "header_files": [],
            "total_lines": 0,
            "test_lines": 0
        }
        
        # Count source files
        if self.src_dir.exists():
            for ext in ['*.cpp', '*.c']:
                for file in self.src_dir.rglob(ext):
                    structure["source_files"].append(str(file.relative_to(self.project_root)))
                    try:
                        with open(file, 'r', encoding='utf-8') as f:
                            structure["total_lines"] += len(f.readlines())
                    except:
                        pass
                        
            for ext in ['*.h', '*.hpp']:
                for file in self.src_dir.rglob(ext):
                    structure["header_files"].append(str(file.relative_to(self.project_root)))
        
        # Count test files
        if self.test_dir.exists():
            for file in self.test_dir.rglob('*.cpp'):
                if not file.name.startswith('.'):
                    structure["test_files"].append(str(file.relative_to(self.project_root)))
                    try:
                        with open(file, 'r', encoding='utf-8') as f:
                            structure["test_lines"] += len(f.readlines())
                    except:
                        pass
        
        self.report_data["code_structure"] = structure
        
    def run_tests(self):
        """Execute PlatformIO tests and collect results"""
        print("🧪 Running tests...")
        
        test_results = {
            "native_tests": {"status": "unknown", "output": ""},
            "test_native": {"status": "unknown", "output": ""},
            "build_status": {"status": "unknown", "output": ""}
        }
        
        # Try to build first
        print("   Building project...")
        returncode, stdout, stderr = self.run_command("pio run -e pico")
        if returncode == 0:
            test_results["build_status"]["status"] = "success"
            test_results["build_status"]["output"] = "Build successful"
        else:
            test_results["build_status"]["status"] = "failed"
            test_results["build_status"]["output"] = f"Build failed: {stderr}"
            
        # Run native tests
        print("   Running native tests...")
        returncode, stdout, stderr = self.run_command("pio test -e native")
        test_results["native_tests"]["output"] = stdout + stderr
        if returncode == 0:
            test_results["native_tests"]["status"] = "passed"
        else:
            test_results["native_tests"]["status"] = "failed"
            
        # Run test_native environment
        print("   Running test_native environment...")
        returncode, stdout, stderr = self.run_command("pio test -e test_native")
        test_results["test_native"]["output"] = stdout + stderr
        if returncode == 0:
            test_results["test_native"]["status"] = "passed"
        else:
            test_results["test_native"]["status"] = "failed"
            
        self.report_data["test_results"] = test_results
        
    def analyze_test_coverage(self):
        """Analyze test coverage based on file structure"""
        print("📊 Analyzing test coverage...")
        
        coverage = {
            "components": {},
            "total_components": 0,
            "tested_components": 0,
            "coverage_percentage": 0
        }
        
        # Define component mapping based on source structure
        component_map = {
            "src/config": ["ConfigManager", "LoggingService"],
            "src/display": ["DisplayManager", "PhysicalReset"],
            "src/gps": ["GpsClient", "TimeManager"],
            "src/hal": ["Button_HAL", "Storage_HAL"],
            "src/network": ["NetworkManager", "NtpServer", "webserver"],
            "src/system": ["ErrorHandler", "SystemController", "PrometheusMetrics", "SystemMonitor"],
            "src/utils": ["TimeUtils", "LogUtils", "I2CUtils"],
            "src/main.cpp": ["main"]
        }
        
        # Check which components have tests
        test_files = set()
        if self.test_dir.exists():
            for test_file in self.test_dir.rglob("test_*.cpp"):
                test_files.add(test_file.stem)
                
        for src_path, components in component_map.items():
            for component in components:
                coverage["total_components"] += 1
                
                # Check if there's a test file for this component
                test_exists = any(
                    component.lower() in test_name.lower() or
                    test_name.lower() in component.lower()
                    for test_name in test_files
                )
                
                coverage["components"][component] = {
                    "source_path": src_path,
                    "has_test": test_exists,
                    "test_files": [t for t in test_files if component.lower() in t.lower()]
                }
                
                if test_exists:
                    coverage["tested_components"] += 1
        
        if coverage["total_components"] > 0:
            coverage["coverage_percentage"] = (coverage["tested_components"] / coverage["total_components"]) * 100
            
        self.report_data["coverage"] = coverage
        
    def analyze_performance(self):
        """Analyze performance metrics"""
        print("⚡ Analyzing performance...")
        
        performance = {
            "build_size": {},
            "memory_usage": {},
            "flash_usage": {}
        }
        
        # Try to get build size information
        returncode, stdout, stderr = self.run_command("pio run -e pico -t size")
        if returncode == 0:
            performance["size_output"] = stdout
            # Parse memory usage if available
            lines = stdout.split('\n')
            for line in lines:
                if 'RAM:' in line:
                    performance["memory_usage"]["info"] = line.strip()
                elif 'Flash:' in line:
                    performance["flash_usage"]["info"] = line.strip()
        else:
            performance["size_output"] = "Size information not available"
            
        self.report_data["performance"] = performance
        
    def generate_html_report(self):
        """Generate HTML report"""
        print("📄 Generating HTML report...")
        
        html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GPS NTP Server - Test Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }}
        .header {{ text-align: center; margin-bottom: 30px; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border-radius: 8px; }}
        .section {{ margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }}
        .section h2 {{ color: #333; border-bottom: 2px solid #667eea; padding-bottom: 5px; }}
        .metrics {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; }}
        .metric {{ background: #f8f9fa; padding: 15px; border-radius: 5px; text-align: center; }}
        .metric h3 {{ margin: 0 0 10px 0; color: #495057; }}
        .metric .value {{ font-size: 2em; font-weight: bold; color: #667eea; }}
        .success {{ color: #28a745; }}
        .warning {{ color: #ffc107; }}
        .error {{ color: #dc3545; }}
        .component-grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; }}
        .component {{ padding: 10px; border-radius: 5px; text-align: center; font-size: 0.9em; }}
        .component.tested {{ background: #d4edda; border: 1px solid #c3e6cb; }}
        .component.untested {{ background: #f8d7da; border: 1px solid #f5c6cb; }}
        .timestamp {{ text-align: center; margin-top: 20px; color: #6c757d; font-style: italic; }}
        pre {{ background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; max-height: 300px; }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🛰️ GPS NTP Server Test Report</h1>
            <p>Comprehensive test coverage and quality analysis</p>
        </div>
        
        <div class="section">
            <h2>📊 Test Summary</h2>
            <div class="metrics">
                <div class="metric">
                    <h3>Build Status</h3>
                    <div class="value {self.get_status_class('build_status')}">{self.get_build_status()}</div>
                </div>
                <div class="metric">
                    <h3>Component Coverage</h3>
                    <div class="value">{self.report_data['coverage']['coverage_percentage']:.1f}%</div>
                </div>
                <div class="metric">
                    <h3>Total Components</h3>
                    <div class="value">{self.report_data['coverage']['total_components']}</div>
                </div>
                <div class="metric">
                    <h3>Tested Components</h3>
                    <div class="value">{self.report_data['coverage']['tested_components']}</div>
                </div>
            </div>
        </div>
        
        <div class="section">
            <h2>🧪 Test Results</h2>
            {self.generate_test_results_html()}
        </div>
        
        <div class="section">
            <h2>🎯 Component Coverage</h2>
            <div class="component-grid">
                {self.generate_component_coverage_html()}
            </div>
        </div>
        
        <div class="section">
            <h2>📁 Code Structure</h2>
            <div class="metrics">
                <div class="metric">
                    <h3>Source Files</h3>
                    <div class="value">{len(self.report_data['code_structure']['source_files'])}</div>
                </div>
                <div class="metric">
                    <h3>Test Files</h3>
                    <div class="value">{len(self.report_data['code_structure']['test_files'])}</div>
                </div>
                <div class="metric">
                    <h3>Header Files</h3>
                    <div class="value">{len(self.report_data['code_structure']['header_files'])}</div>
                </div>
                <div class="metric">
                    <h3>Lines of Code</h3>
                    <div class="value">{self.report_data['code_structure']['total_lines']:,}</div>
                </div>
            </div>
        </div>
        
        <div class="section">
            <h2>⚡ Performance</h2>
            <pre>{self.report_data['performance'].get('size_output', 'Performance data not available')}</pre>
        </div>
        
        <div class="timestamp">
            Generated on {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
        </div>
    </div>
</body>
</html>
        """
        
        report_file = self.project_root / "test_report.html"
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write(html_content)
            
        print(f"✅ HTML report generated: {report_file}")
        
    def get_status_class(self, test_type):
        """Get CSS class for test status"""
        if test_type in self.report_data.get('test_results', {}):
            status = self.report_data['test_results'][test_type]['status']
            if status == 'success' or status == 'passed':
                return 'success'
            elif status == 'failed':
                return 'error'
        return 'warning'
        
    def get_build_status(self):
        """Get build status text"""
        status = self.report_data.get('test_results', {}).get('build_status', {}).get('status', 'unknown')
        return status.upper()
        
    def generate_test_results_html(self):
        """Generate test results HTML section"""
        results = self.report_data.get('test_results', {})
        html = ""
        
        for test_type, result in results.items():
            status_class = 'success' if result['status'] in ['success', 'passed'] else 'error' if result['status'] == 'failed' else 'warning'
            html += f"""
            <div style="margin-bottom: 15px;">
                <h4>{test_type.replace('_', ' ').title()}: <span class="{status_class}">{result['status'].upper()}</span></h4>
            </div>
            """
            
        return html
        
    def generate_component_coverage_html(self):
        """Generate component coverage HTML"""
        components = self.report_data.get('coverage', {}).get('components', {})
        html = ""
        
        for component, info in components.items():
            css_class = 'tested' if info['has_test'] else 'untested'
            status_icon = '✅' if info['has_test'] else '❌'
            html += f"""
            <div class="component {css_class}">
                <div>{status_icon} {component}</div>
                <small>{info['source_path']}</small>
            </div>
            """
            
        return html
        
    def save_json_report(self):
        """Save detailed JSON report"""
        json_file = self.project_root / "test_report.json"
        with open(json_file, 'w', encoding='utf-8') as f:
            json.dump(self.report_data, f, indent=2, default=str)
        print(f"✅ JSON report saved: {json_file}")
        
    def generate_report(self):
        """Generate complete test report"""
        print("🚀 GPS NTP Server - Test Report Generator")
        print("=" * 50)
        
        self.analyze_code_structure()
        self.run_tests()
        self.analyze_test_coverage()
        self.analyze_performance()
        
        self.generate_html_report()
        self.save_json_report()
        
        # Print summary
        print("\n" + "=" * 50)
        print("📋 REPORT SUMMARY")
        print("=" * 50)
        print(f"🏗️  Build Status: {self.get_build_status()}")
        print(f"📊 Component Coverage: {self.report_data['coverage']['coverage_percentage']:.1f}%")
        print(f"📁 Source Files: {len(self.report_data['code_structure']['source_files'])}")
        print(f"🧪 Test Files: {len(self.report_data['code_structure']['test_files'])}")
        print(f"📄 Total Lines: {self.report_data['code_structure']['total_lines']:,}")
        print("=" * 50)
        print("✅ Report generation completed successfully!")

if __name__ == "__main__":
    project_root = sys.argv[1] if len(sys.argv) > 1 else "."
    generator = TestReportGenerator(project_root)
    generator.generate_report()