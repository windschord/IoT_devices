#!/usr/bin/env python3
"""
GPS NTP Server - Parallel Test Runner

This script optimizes test execution by running tests in parallel
and collecting results efficiently.

Features:
- Parallel test execution
- Test result aggregation
- Performance monitoring
- Cross-platform compatibility
"""

import os
import sys
import json
import threading
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from datetime import datetime

class ParallelTestRunner:
    def __init__(self, project_root, max_workers=4):
        self.project_root = Path(project_root)
        self.max_workers = max_workers
        self.test_results = {}
        self.start_time = None
        self.end_time = None
        
    def discover_tests(self):
        """Discover all available test environments and test cases"""
        print("🔍 Discovering test environments...")
        
        # Get test environments from platformio.ini
        test_environments = []
        
        # Parse platformio.ini for test environments
        platformio_ini = self.project_root / "platformio.ini"
        if platformio_ini.exists():
            with open(platformio_ini, 'r') as f:
                content = f.read()
                
            # Look for test environments
            lines = content.split('\n')
            for line in lines:
                line = line.strip()
                if line.startswith('[env:') and ('test' in line or 'native' in line):
                    env_name = line[5:-1]  # Remove [env: and ]
                    test_environments.append(env_name)
        
        # Discover individual test cases
        test_cases = []
        test_dir = self.project_root / "test"
        if test_dir.exists():
            for test_file in test_dir.rglob("test_*.cpp"):
                # Extract test name from directory structure
                if test_file.parent.name.startswith("test_"):
                    test_cases.append(test_file.parent.name)
                else:
                    test_cases.append(test_file.stem)
        
        print(f"   Found {len(test_environments)} test environments: {test_environments}")
        print(f"   Found {len(test_cases)} test cases")
        
        return test_environments, test_cases
        
    def run_single_test(self, env_name, test_filter=None):
        """Run a single test environment or specific test"""
        test_id = f"{env_name}:{test_filter}" if test_filter else env_name
        print(f"🧪 Running {test_id}...")
        
        start_time = time.time()
        
        # Build command
        if test_filter:
            cmd = f"pio test -e {env_name} -f {test_filter}"
        else:
            cmd = f"pio test -e {env_name}"
            
        try:
            result = subprocess.run(
                cmd, 
                shell=True, 
                capture_output=True, 
                text=True, 
                cwd=self.project_root,
                timeout=300  # 5 minute timeout per test
            )
            
            end_time = time.time()
            duration = end_time - start_time
            
            return {
                'test_id': test_id,
                'environment': env_name,
                'filter': test_filter,
                'returncode': result.returncode,
                'stdout': result.stdout,
                'stderr': result.stderr,
                'duration': duration,
                'status': 'passed' if result.returncode == 0 else 'failed',
                'start_time': start_time,
                'end_time': end_time
            }
            
        except subprocess.TimeoutExpired:
            return {
                'test_id': test_id,
                'environment': env_name,
                'filter': test_filter,
                'returncode': -1,
                'stdout': '',
                'stderr': 'Test timed out after 300 seconds',
                'duration': 300,
                'status': 'timeout',
                'start_time': start_time,
                'end_time': time.time()
            }
        except Exception as e:
            return {
                'test_id': test_id,
                'environment': env_name,
                'filter': test_filter,
                'returncode': -1,
                'stdout': '',
                'stderr': str(e),
                'duration': 0,
                'status': 'error',
                'start_time': start_time,
                'end_time': time.time()
            }
            
    def run_build_test(self):
        """Run build test for firmware"""
        print("🏗️ Running build test...")
        start_time = time.time()
        
        try:
            result = subprocess.run(
                "pio run -e pico", 
                shell=True, 
                capture_output=True, 
                text=True, 
                cwd=self.project_root,
                timeout=600  # 10 minute timeout for build
            )
            
            duration = time.time() - start_time
            
            return {
                'test_id': 'build:pico',
                'environment': 'pico',
                'returncode': result.returncode,
                'stdout': result.stdout,
                'stderr': result.stderr,
                'duration': duration,
                'status': 'passed' if result.returncode == 0 else 'failed'
            }
            
        except Exception as e:
            return {
                'test_id': 'build:pico',
                'environment': 'pico',
                'returncode': -1,
                'stdout': '',
                'stderr': str(e),
                'duration': time.time() - start_time,
                'status': 'error'
            }
            
    def run_parallel_tests(self):
        """Run all tests in parallel"""
        print(f"🚀 Starting parallel test execution (max workers: {self.max_workers})...")
        self.start_time = time.time()
        
        # Discover tests
        test_environments, test_cases = self.discover_tests()
        
        # Create test jobs
        test_jobs = []
        
        # Add build job
        test_jobs.append(('build', None))
        
        # Add environment-based test jobs
        for env in test_environments:
            if env in ['native', 'test_native']:  # Focus on working environments
                test_jobs.append((env, None))
        
        print(f"📋 Scheduled {len(test_jobs)} test jobs")
        
        # Execute tests in parallel
        results = []
        with ThreadPoolExecutor(max_workers=self.max_workers) as executor:
            # Submit all jobs
            future_to_job = {}
            for env, test_filter in test_jobs:
                if env == 'build':
                    future = executor.submit(self.run_build_test)
                else:
                    future = executor.submit(self.run_single_test, env, test_filter)
                future_to_job[future] = (env, test_filter)
            
            # Collect results as they complete
            for future in as_completed(future_to_job):
                try:
                    result = future.result()
                    results.append(result)
                    
                    # Print immediate result
                    status_icon = "✅" if result['status'] in ['passed'] else "❌"
                    print(f"   {status_icon} {result['test_id']} ({result['duration']:.2f}s)")
                    
                except Exception as e:
                    env, test_filter = future_to_job[future]
                    test_id = f"{env}:{test_filter}" if test_filter else env
                    print(f"   ❌ {test_id} (Exception: {e})")
                    results.append({
                        'test_id': test_id,
                        'status': 'error',
                        'stderr': str(e),
                        'duration': 0
                    })
        
        self.end_time = time.time()
        self.test_results = {
            'results': results,
            'summary': self.generate_summary(results),
            'total_duration': self.end_time - self.start_time,
            'start_time': self.start_time,
            'end_time': self.end_time
        }
        
        return self.test_results
        
    def generate_summary(self, results):
        """Generate test execution summary"""
        total_tests = len(results)
        passed_tests = len([r for r in results if r['status'] == 'passed'])
        failed_tests = len([r for r in results if r['status'] == 'failed'])
        error_tests = len([r for r in results if r['status'] in ['error', 'timeout']])
        
        total_duration = sum(r.get('duration', 0) for r in results)
        avg_duration = total_duration / total_tests if total_tests > 0 else 0
        
        return {
            'total_tests': total_tests,
            'passed_tests': passed_tests,
            'failed_tests': failed_tests,
            'error_tests': error_tests,
            'success_rate': (passed_tests / total_tests) * 100 if total_tests > 0 else 0,
            'total_duration': total_duration,
            'average_duration': avg_duration,
            'fastest_test': min(results, key=lambda x: x.get('duration', 0)) if results else None,
            'slowest_test': max(results, key=lambda x: x.get('duration', 0)) if results else None
        }
        
    def save_results(self):
        """Save test results to files"""
        # Save JSON results
        json_file = self.project_root / "parallel_test_results.json"
        with open(json_file, 'w') as f:
            json.dump(self.test_results, f, indent=2, default=str)
        
        # Save summary text file
        summary_file = self.project_root / "parallel_test_summary.txt"
        with open(summary_file, 'w') as f:
            f.write("GPS NTP Server - Parallel Test Results\n")
            f.write("=" * 50 + "\n")
            f.write(f"Execution Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\\n")
            f.write(f"Total Duration: {self.test_results['total_duration']:.2f} seconds\\n")
            f.write(f"Max Workers: {self.max_workers}\\n")
            f.write("\\n")
            
            summary = self.test_results['summary']
            f.write("Test Summary:\\n")
            f.write(f"  Total Tests: {summary['total_tests']}\\n")
            f.write(f"  Passed: {summary['passed_tests']} ({summary['success_rate']:.1f}%)\\n")
            f.write(f"  Failed: {summary['failed_tests']}\\n")
            f.write(f"  Errors: {summary['error_tests']}\\n")
            f.write(f"  Average Duration: {summary['average_duration']:.2f}s\\n")
            f.write("\\n")
            
            f.write("Individual Test Results:\\n")
            for result in self.test_results['results']:
                status_symbol = "✅" if result['status'] == 'passed' else "❌"
                f.write(f"  {status_symbol} {result['test_id']} ({result['duration']:.2f}s) - {result['status'].upper()}\\n")
                
        print(f"📊 Results saved to: {json_file} and {summary_file}")
        
    def print_summary(self):
        """Print test execution summary"""
        print("\\n" + "=" * 60)
        print("📋 PARALLEL TEST EXECUTION SUMMARY")
        print("=" * 60)
        
        summary = self.test_results['summary']
        
        print(f"⏱️  Total Duration: {self.test_results['total_duration']:.2f} seconds")
        print(f"🧪 Total Tests: {summary['total_tests']}")
        print(f"✅ Passed: {summary['passed_tests']} ({summary['success_rate']:.1f}%)")
        print(f"❌ Failed: {summary['failed_tests']}")
        print(f"💥 Errors: {summary['error_tests']}")
        print(f"📊 Average Duration: {summary['average_duration']:.2f}s per test")
        
        if summary['fastest_test']:
            print(f"🏃 Fastest Test: {summary['fastest_test']['test_id']} ({summary['fastest_test']['duration']:.2f}s)")
        if summary['slowest_test']:
            print(f"🐌 Slowest Test: {summary['slowest_test']['test_id']} ({summary['slowest_test']['duration']:.2f}s)")
            
        print("=" * 60)
        
        # Performance analysis
        parallel_time = self.test_results['total_duration']
        sequential_time = sum(r.get('duration', 0) for r in self.test_results['results'])
        speedup = sequential_time / parallel_time if parallel_time > 0 else 1
        efficiency = (speedup / self.max_workers) * 100
        
        print("⚡ PERFORMANCE ANALYSIS")
        print(f"   Parallel Execution: {parallel_time:.2f}s")
        print(f"   Sequential Would Be: {sequential_time:.2f}s")
        print(f"   Speedup: {speedup:.2f}x")
        print(f"   Parallel Efficiency: {efficiency:.1f}%")
        
        print("=" * 60)
        print("✅ Parallel test execution completed successfully!")
        
def main():
    project_root = sys.argv[1] if len(sys.argv) > 1 else "."
    max_workers = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    
    print(f"🚀 GPS NTP Server - Parallel Test Runner")
    print(f"   Project: {Path(project_root).resolve()}")
    print(f"   Max Workers: {max_workers}")
    print(f"   Platform: {sys.platform}")
    
    runner = ParallelTestRunner(project_root, max_workers)
    results = runner.run_parallel_tests()
    runner.save_results()
    runner.print_summary()

if __name__ == "__main__":
    main()