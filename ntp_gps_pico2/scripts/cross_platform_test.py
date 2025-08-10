#!/usr/bin/env python3
"""
GPS NTP Server - Cross-Platform Test Runner

This script provides cross-platform test execution capabilities
with Docker integration for consistent testing environments.

Features:
- Docker-based test isolation
- Cross-platform compatibility
- Test environment management
- Result aggregation
"""

import os
import sys
import json
import subprocess
import platform
import shutil
from pathlib import Path
from datetime import datetime

class CrossPlatformTestRunner:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.platform_info = {
            'system': platform.system(),
            'machine': platform.machine(),
            'python_version': platform.python_version(),
            'platform': platform.platform()
        }
        self.docker_available = self.check_docker()
        
    def check_docker(self):
        """Check if Docker is available"""
        try:
            result = subprocess.run(['docker', '--version'], capture_output=True, text=True)
            return result.returncode == 0
        except FileNotFoundError:
            return False
            
    def check_platformio(self):
        """Check if PlatformIO is available"""
        try:
            result = subprocess.run(['pio', '--version'], capture_output=True, text=True)
            return result.returncode == 0, result.stdout.strip()
        except FileNotFoundError:
            return False, "PlatformIO not found"
            
    def run_native_tests(self):
        """Run tests in native environment"""
        print(f"🖥️  Running native tests on {self.platform_info['system']} {self.platform_info['machine']}")
        
        pio_available, pio_version = self.check_platformio()
        if not pio_available:
            return {
                'status': 'skipped',
                'reason': 'PlatformIO not available in native environment',
                'platform': self.platform_info
            }
            
        print(f"   PlatformIO Version: {pio_version}")
        
        results = {
            'platform': self.platform_info,
            'pio_version': pio_version,
            'tests': {}
        }
        
        # Run build test
        print("   🏗️ Building firmware...")
        build_start = datetime.now()
        try:
            result = subprocess.run(
                ['pio', 'run', '-e', 'pico'], 
                capture_output=True, 
                text=True, 
                cwd=self.project_root,
                timeout=600
            )
            build_duration = (datetime.now() - build_start).total_seconds()
            
            results['tests']['build'] = {
                'status': 'passed' if result.returncode == 0 else 'failed',
                'duration': build_duration,
                'returncode': result.returncode,
                'stdout': result.stdout[:1000],  # Truncate for storage
                'stderr': result.stderr[:1000]
            }
            
        except subprocess.TimeoutExpired:
            results['tests']['build'] = {
                'status': 'timeout',
                'duration': 600,
                'reason': 'Build timed out after 10 minutes'
            }
        except Exception as e:
            results['tests']['build'] = {
                'status': 'error',
                'duration': 0,
                'reason': str(e)
            }
            
        # Run native tests
        test_environments = ['native', 'test_native']
        
        for env in test_environments:
            print(f"   🧪 Testing environment: {env}")
            test_start = datetime.now()
            
            try:
                result = subprocess.run(
                    ['pio', 'test', '-e', env], 
                    capture_output=True, 
                    text=True, 
                    cwd=self.project_root,
                    timeout=300
                )
                test_duration = (datetime.now() - test_start).total_seconds()
                
                results['tests'][env] = {
                    'status': 'passed' if result.returncode == 0 else 'failed',
                    'duration': test_duration,
                    'returncode': result.returncode,
                    'stdout': result.stdout[:1000],
                    'stderr': result.stderr[:1000]
                }
                
            except subprocess.TimeoutExpired:
                results['tests'][env] = {
                    'status': 'timeout',
                    'duration': 300,
                    'reason': 'Test timed out after 5 minutes'
                }
            except Exception as e:
                results['tests'][env] = {
                    'status': 'error',
                    'duration': 0,
                    'reason': str(e)
                }
                
        return results
        
    def build_docker_image(self):
        """Build Docker test image"""
        print("🐳 Building Docker test image...")
        
        dockerfile_path = self.project_root / "Dockerfile.test"
        if not dockerfile_path.exists():
            return False, "Dockerfile.test not found"
            
        try:
            result = subprocess.run([
                'docker', 'build', 
                '-f', str(dockerfile_path),
                '-t', 'gps-ntp-test',
                str(self.project_root)
            ], capture_output=True, text=True, timeout=600)
            
            return result.returncode == 0, result.stderr if result.returncode != 0 else "Image built successfully"
            
        except subprocess.TimeoutExpired:
            return False, "Docker build timed out"
        except Exception as e:
            return False, str(e)
            
    def run_docker_tests(self):
        """Run tests in Docker container"""
        print("🐳 Running tests in Docker container...")
        
        if not self.docker_available:
            return {
                'status': 'skipped',
                'reason': 'Docker not available',
                'platform': 'docker'
            }
            
        # Build Docker image first
        build_success, build_message = self.build_docker_image()
        if not build_success:
            return {
                'status': 'failed',
                'reason': f'Docker image build failed: {build_message}',
                'platform': 'docker'
            }
            
        print(f"   ✅ {build_message}")
        
        # Run tests in container
        docker_cmd = [
            'docker', 'run', '--rm',
            '-v', f'{self.project_root}:/home/testuser/project',
            '-w', '/home/testuser/project',
            'gps-ntp-test',
            'bash', '-c', '''
                echo "🐳 Running tests in Docker container"
                echo "Platform: $(uname -a)"
                echo "PlatformIO: $(pio --version)"
                echo ""
                
                echo "🏗️ Building firmware..."
                pio run -e pico
                build_result=$?
                
                echo "🧪 Running native tests..."
                pio test -e native
                native_result=$?
                
                echo "🧪 Running test_native tests..."
                pio test -e test_native  
                test_native_result=$?
                
                echo ""
                echo "=== Docker Test Results ==="
                echo "Build: $build_result"
                echo "Native: $native_result"
                echo "Test Native: $test_native_result"
                
                # Return overall success/failure
                if [ $build_result -eq 0 ] && [ $native_result -eq 0 ] && [ $test_native_result -eq 0 ]; then
                    exit 0
                else
                    exit 1
                fi
            '''
        ]
        
        try:
            result = subprocess.run(
                docker_cmd,
                capture_output=True,
                text=True,
                timeout=1200,  # 20 minute timeout
                cwd=self.project_root
            )
            
            return {
                'status': 'passed' if result.returncode == 0 else 'failed',
                'returncode': result.returncode,
                'stdout': result.stdout,
                'stderr': result.stderr,
                'platform': 'docker/ubuntu:22.04'
            }
            
        except subprocess.TimeoutExpired:
            return {
                'status': 'timeout',
                'reason': 'Docker tests timed out after 20 minutes',
                'platform': 'docker'
            }
        except Exception as e:
            return {
                'status': 'error',
                'reason': str(e),
                'platform': 'docker'
            }
            
    def generate_compatibility_matrix(self):
        """Generate cross-platform compatibility matrix"""
        print("📊 Generating compatibility matrix...")
        
        matrix = {
            'test_timestamp': datetime.now().isoformat(),
            'host_platform': self.platform_info,
            'docker_available': self.docker_available,
            'results': {}
        }
        
        # Test native environment
        print("Testing native environment...")
        matrix['results']['native'] = self.run_native_tests()
        
        # Test Docker environment
        if self.docker_available:
            print("Testing Docker environment...")
            matrix['results']['docker'] = self.run_docker_tests()
        else:
            print("Skipping Docker tests (Docker not available)")
            matrix['results']['docker'] = {
                'status': 'skipped',
                'reason': 'Docker not available'
            }
            
        return matrix
        
    def save_results(self, results):
        """Save cross-platform test results"""
        results_file = self.project_root / "cross_platform_results.json"
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2, default=str)
            
        # Generate summary report
        summary_file = self.project_root / "cross_platform_summary.txt"
        with open(summary_file, 'w') as f:
            f.write("GPS NTP Server - Cross-Platform Test Summary\\n")
            f.write("=" * 60 + "\\n")
            f.write(f"Test Date: {results['test_timestamp']}\\n")
            f.write(f"Host Platform: {results['host_platform']['system']} {results['host_platform']['machine']}\\n")
            f.write(f"Docker Available: {'Yes' if results['docker_available'] else 'No'}\\n")
            f.write("\\n")
            
            for platform, result in results['results'].items():
                f.write(f"{platform.upper()} ENVIRONMENT:\\n")
                f.write(f"  Status: {result['status'].upper()}\\n")
                
                if 'tests' in result:
                    for test_name, test_result in result['tests'].items():
                        status_icon = "✅" if test_result['status'] == 'passed' else "❌"
                        f.write(f"    {status_icon} {test_name}: {test_result['status']} ({test_result.get('duration', 0):.2f}s)\\n")
                elif 'reason' in result:
                    f.write(f"  Reason: {result['reason']}\\n")
                    
                f.write("\\n")
                
        print(f"✅ Results saved to: {results_file} and {summary_file}")
        
    def print_summary(self, results):
        """Print cross-platform test summary"""
        print("\\n" + "=" * 70)
        print("🌐 CROSS-PLATFORM TEST SUMMARY")
        print("=" * 70)
        
        host_info = results['host_platform']
        print(f"🖥️  Host Platform: {host_info['system']} {host_info['machine']}")
        print(f"🐍 Python Version: {host_info['python_version']}")
        print(f"🐳 Docker Available: {'Yes' if results['docker_available'] else 'No'}")
        print("")
        
        for platform, result in results['results'].items():
            status_icon = "✅" if result['status'] == 'passed' else "❌" if result['status'] == 'failed' else "⏭️"
            print(f"{status_icon} {platform.upper()}: {result['status'].upper()}")
            
            if 'tests' in result:
                passed = sum(1 for t in result['tests'].values() if t['status'] == 'passed')
                total = len(result['tests'])
                print(f"    Tests: {passed}/{total} passed")
                
        print("=" * 70)
        print("✅ Cross-platform testing completed!")
        
def main():
    project_root = sys.argv[1] if len(sys.argv) > 1 else "."
    
    print("🌐 GPS NTP Server - Cross-Platform Test Runner")
    print(f"   Project: {Path(project_root).resolve()}")
    
    runner = CrossPlatformTestRunner(project_root)
    results = runner.generate_compatibility_matrix()
    runner.save_results(results)
    runner.print_summary(results)

if __name__ == "__main__":
    main()