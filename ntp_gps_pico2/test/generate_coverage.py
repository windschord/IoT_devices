#!/usr/bin/env python3
"""
GPS NTP Server - Test Coverage Report Generator

This script generates comprehensive test coverage reports using gcov and lcov.
Based on PlatformIO best practices for unit test coverage measurement.

Usage:
    python generate_coverage.py [--html] [--clean] [--verbose]

Requirements:
    - gcov (version 9+)
    - lcov (for HTML reports)
    - PlatformIO Core
"""

import os
import sys
import subprocess
import argparse
import json
from pathlib import Path
from datetime import datetime

class CoverageGenerator:
    def __init__(self, project_root=None, verbose=False):
        self.project_root = Path(project_root) if project_root else Path(__file__).parent.parent
        self.test_dir = self.project_root / "test"
        self.src_dir = self.project_root / "src"
        self.coverage_dir = self.test_dir / "coverage"
        self.verbose = verbose
        
        # Coverage file patterns
        self.gcov_files = []
        self.gcda_files = []
        
    def log(self, message, level="INFO"):
        """Log message with timestamp"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        if self.verbose or level in ["ERROR", "WARNING"]:
            print(f"[{timestamp}] {level}: {message}")
    
    def run_command(self, cmd, cwd=None):
        """Execute shell command and return result"""
        if isinstance(cmd, str):
            cmd = cmd.split()
        
        try:
            result = subprocess.run(
                cmd, 
                cwd=cwd or self.project_root,
                capture_output=True, 
                text=True, 
                check=True
            )
            if self.verbose:
                self.log(f"Command: {' '.join(cmd)}")
                if result.stdout:
                    self.log(f"Output: {result.stdout.strip()}")
            return result
        except subprocess.CalledProcessError as e:
            self.log(f"Command failed: {' '.join(cmd)}", "ERROR")
            self.log(f"Error: {e.stderr}", "ERROR")
            return None
    
    def clean_coverage_data(self):
        """Clean previous coverage data"""
        self.log("Cleaning previous coverage data...")
        
        # Remove coverage directory
        if self.coverage_dir.exists():
            import shutil
            shutil.rmtree(self.coverage_dir)
        
        # Clean build directory coverage files
        build_dirs = list(self.project_root.glob(".pio/build/*/"))
        for build_dir in build_dirs:
            for pattern in ["*.gcda", "*.gcno", "*.gcov"]:
                for file in build_dir.glob(f"**/{pattern}"):
                    file.unlink(missing_ok=True)
        
        self.log("Coverage data cleaned")
    
    def run_tests_with_coverage(self):
        """Run tests with coverage environment"""
        self.log("Running tests with coverage measurement...")
        
        # Change to test directory for PlatformIO test execution
        result = self.run_command([
            "pio", "test", 
            "--environment", "coverage",
            "--verbose"
        ], cwd=self.test_dir)
        
        if not result:
            self.log("Test execution failed", "ERROR")
            return False
        
        self.log("Tests completed successfully")
        return True
    
    def find_coverage_files(self):
        """Find generated coverage files"""
        self.log("Searching for coverage files...")
        
        # Search in build directory
        build_pattern = self.project_root / ".pio" / "build" / "coverage"
        
        # Find .gcda and .gcno files
        if build_pattern.exists():
            self.gcda_files = list(build_pattern.glob("**/*.gcda"))
            gcno_files = list(build_pattern.glob("**/*.gcno"))
            
            self.log(f"Found {len(self.gcda_files)} .gcda files")
            self.log(f"Found {len(gcno_files)} .gcno files")
            
            if self.verbose:
                for file in self.gcda_files[:5]:  # Show first 5
                    self.log(f"Coverage data: {file}")
        
        return len(self.gcda_files) > 0
    
    def generate_gcov_reports(self):
        """Generate gcov reports"""
        self.log("Generating gcov reports...")
        
        # Create coverage directory
        self.coverage_dir.mkdir(parents=True, exist_ok=True)
        
        # Coverage data collection
        coverage_stats = {
            "files": {},
            "total_lines": 0,
            "covered_lines": 0,
            "coverage_percentage": 0.0,
            "timestamp": datetime.now().isoformat()
        }
        
        # Process each source file
        src_files = []
        for ext in ["*.cpp", "*.h"]:
            src_files.extend(self.src_dir.glob(f"**/{ext}"))
        
        if not src_files:
            self.log("No source files found", "WARNING")
            return coverage_stats
        
        build_dir = self.project_root / ".pio" / "build" / "coverage"
        
        for src_file in src_files:
            # Skip header files for now (gcov doesn't handle them well)
            if src_file.suffix == ".h":
                continue
                
            # Generate gcov for this file
            gcov_result = self.run_command([
                "gcov", 
                "-b",  # Branch coverage
                "-c",  # Unconditional branch coverage
                str(src_file)
            ], cwd=build_dir)
            
            if gcov_result:
                # Parse gcov output for coverage info
                self.parse_gcov_output(src_file, gcov_result.stdout, coverage_stats)
        
        # Calculate total coverage
        if coverage_stats["total_lines"] > 0:
            coverage_stats["coverage_percentage"] = (
                coverage_stats["covered_lines"] / coverage_stats["total_lines"] * 100
            )
        
        # Save coverage stats
        stats_file = self.coverage_dir / "coverage_stats.json"
        with open(stats_file, 'w') as f:
            json.dump(coverage_stats, f, indent=2)
        
        self.log(f"Coverage: {coverage_stats['coverage_percentage']:.1f}% "
                f"({coverage_stats['covered_lines']}/{coverage_stats['total_lines']} lines)")
        
        return coverage_stats
    
    def parse_gcov_output(self, src_file, gcov_output, stats):
        """Parse gcov output and extract coverage information"""
        lines = gcov_output.split('\n')
        
        file_stats = {
            "total_lines": 0,
            "covered_lines": 0,
            "coverage_percentage": 0.0
        }
        
        # Simple parsing - look for "Lines executed" summary
        for line in lines:
            if "Lines executed:" in line:
                # Format: "Lines executed:75.00% of 20"
                parts = line.split()
                if len(parts) >= 4:
                    try:
                        percentage_str = parts[1].rstrip('%')
                        total_lines = int(parts[3])
                        covered_lines = int(float(percentage_str) * total_lines / 100)
                        
                        file_stats["total_lines"] = total_lines
                        file_stats["covered_lines"] = covered_lines
                        file_stats["coverage_percentage"] = float(percentage_str)
                        
                        stats["total_lines"] += total_lines
                        stats["covered_lines"] += covered_lines
                        stats["files"][str(src_file.relative_to(self.project_root))] = file_stats
                        
                        if self.verbose:
                            self.log(f"{src_file.name}: {percentage_str}% ({covered_lines}/{total_lines})")
                        
                    except (ValueError, IndexError):
                        pass
                break
    
    def generate_html_report(self):
        """Generate HTML coverage report using lcov"""
        self.log("Generating HTML coverage report...")
        
        # Check if lcov is available
        lcov_check = self.run_command(["which", "lcov"])
        if not lcov_check:
            self.log("lcov not found. Install with: sudo apt-get install lcov", "WARNING")
            return False
        
        # Create lcov info file
        info_file = self.coverage_dir / "coverage.info"
        html_dir = self.coverage_dir / "html"
        
        # Generate lcov info
        lcov_result = self.run_command([
            "lcov",
            "--capture",
            "--directory", str(self.project_root / ".pio" / "build" / "coverage"),
            "--output-file", str(info_file),
            "--rc", "lcov_branch_coverage=1"
        ])
        
        if not lcov_result:
            self.log("Failed to generate lcov info file", "ERROR")
            return False
        
        # Generate HTML report
        genhtml_result = self.run_command([
            "genhtml",
            str(info_file),
            "--output-directory", str(html_dir),
            "--title", "GPS NTP Server Coverage Report",
            "--branch-coverage",
            "--function-coverage"
        ])
        
        if genhtml_result:
            self.log(f"HTML report generated: {html_dir / 'index.html'}")
            return True
        else:
            self.log("Failed to generate HTML report", "ERROR")
            return False
    
    def analyze_current_coverage(self):
        """Analyze current test coverage status"""
        self.log("Analyzing current test coverage...")
        
        # Get all source files
        cpp_files = list(self.src_dir.glob("**/*.cpp"))
        h_files = list(self.src_dir.glob("**/*.h"))
        
        # Get all test files
        test_files = list(self.test_dir.glob("test_*_coverage.cpp"))
        test_files.extend(list(self.test_dir.glob("test_*.cpp")))
        
        analysis = {
            "source_files": {
                "cpp_files": len(cpp_files),
                "header_files": len(h_files),
                "total": len(cpp_files) + len(h_files)
            },
            "test_files": {
                "coverage_tests": len([f for f in test_files if "_coverage" in f.name]),
                "other_tests": len([f for f in test_files if "_coverage" not in f.name]),
                "total": len(test_files)
            },
            "coverage_estimate": 0.0
        }
        
        # Estimate coverage based on test files
        if analysis["source_files"]["total"] > 0:
            analysis["coverage_estimate"] = (
                analysis["test_files"]["coverage_tests"] / analysis["source_files"]["cpp_files"] * 100
            )
        
        # Detailed file analysis
        tested_files = set()
        for test_file in test_files:
            # Extract tested component name from test file
            if "_coverage" in test_file.name:
                component = test_file.name.replace("test_", "").replace("_coverage.cpp", "")
                tested_files.add(component)
        
        untested_files = []
        for cpp_file in cpp_files:
            component = cpp_file.stem.lower()
            if component not in tested_files:
                untested_files.append(str(cpp_file.relative_to(self.project_root)))
        
        analysis["untested_files"] = untested_files
        analysis["tested_components"] = list(tested_files)
        
        return analysis
    
    def print_coverage_summary(self, stats, analysis):
        """Print comprehensive coverage summary"""
        print("\n" + "="*60)
        print("          GPS NTP SERVER - COVERAGE REPORT")
        print("="*60)
        
        # Current coverage stats
        if stats.get("coverage_percentage", 0) > 0:
            print(f"Actual Coverage:     {stats['coverage_percentage']:.1f}%")
            print(f"Lines Covered:       {stats['covered_lines']}/{stats['total_lines']}")
        else:
            print(f"Estimated Coverage:  {analysis['coverage_estimate']:.1f}%")
        
        print(f"Source Files:        {analysis['source_files']['total']}")
        print(f"Test Files:          {analysis['test_files']['total']}")
        print(f"Coverage Tests:      {analysis['test_files']['coverage_tests']}")
        
        # Tested components
        print(f"\nTested Components ({len(analysis['tested_components'])}):")
        for component in sorted(analysis['tested_components']):
            print(f"  ✅ {component}")
        
        # Untested files
        if analysis['untested_files']:
            print(f"\nUntested Files ({len(analysis['untested_files'])}):")
            for file in sorted(analysis['untested_files']):
                print(f"  ❌ {file}")
        
        # Coverage by category (if actual stats available)
        if stats.get("files"):
            print(f"\nPer-File Coverage:")
            for file, file_stats in sorted(stats["files"].items()):
                status = "✅" if file_stats["coverage_percentage"] > 80 else "⚠️" if file_stats["coverage_percentage"] > 50 else "❌"
                print(f"  {status} {file}: {file_stats['coverage_percentage']:.1f}%")
        
        print("="*60)
    
    def run(self, generate_html=False, clean=False):
        """Main coverage generation workflow"""
        self.log("Starting coverage analysis...")
        
        # Clean if requested
        if clean:
            self.clean_coverage_data()
        
        # Analyze current status
        analysis = self.analyze_current_coverage()
        
        # Try to run tests and generate actual coverage
        coverage_stats = {"coverage_percentage": 0.0}
        
        if self.run_tests_with_coverage():
            if self.find_coverage_files():
                coverage_stats = self.generate_gcov_reports()
                
                if generate_html:
                    self.generate_html_report()
            else:
                self.log("No coverage files found - showing analysis only", "WARNING")
        else:
            self.log("Test execution failed - showing analysis only", "WARNING")
        
        # Print summary
        self.print_coverage_summary(coverage_stats, analysis)
        
        return coverage_stats["coverage_percentage"]

def main():
    parser = argparse.ArgumentParser(description="Generate test coverage report for GPS NTP Server")
    parser.add_argument("--html", action="store_true", help="Generate HTML report")
    parser.add_argument("--clean", action="store_true", help="Clean previous coverage data")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--project-root", help="Project root directory")
    
    args = parser.parse_args()
    
    generator = CoverageGenerator(
        project_root=args.project_root,
        verbose=args.verbose
    )
    
    try:
        coverage_percentage = generator.run(
            generate_html=args.html,
            clean=args.clean
        )
        
        # Exit with appropriate code
        if coverage_percentage >= 90:
            sys.exit(0)  # Excellent coverage
        elif coverage_percentage >= 70:
            sys.exit(0)  # Good coverage
        else:
            sys.exit(1)  # Needs improvement
            
    except KeyboardInterrupt:
        print("\nCoverage generation interrupted")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()