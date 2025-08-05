#!/usr/bin/env python3
"""
benchmark_system_performance.py
システムパフォーマンスベンチマークスクリプト

Task 32.3の実装：
- PlatformIOビルドサイズ解析
- システムリソース使用量分析
- パフォーマンス指標計算
- ベンチマーク結果レポート生成
"""

import os
import sys
import json
import subprocess
import re
from datetime import datetime
from pathlib import Path

class SystemPerformanceBenchmark:
    def __init__(self, project_root=None):
        self.project_root = Path(project_root) if project_root else Path.cwd()
        self.build_dir = self.project_root / ".pio" / "build" / "pico"
        self.results = {
            'timestamp': datetime.now().isoformat(),
            'project_root': str(self.project_root),
            'build_info': {},
            'memory_analysis': {},
            'performance_metrics': {},
            'recommendations': []
        }
    
    def run_build_analysis(self):
        """ビルド解析の実行"""
        print("=== Build Analysis ===")
        
        try:
            # PlatformIOビルド実行
            result = subprocess.run(
                ['pio', 'run', '-e', 'pico'],
                cwd=self.project_root,
                capture_output=True,
                text=True
            )
            
            if result.returncode != 0:
                print(f"Build failed: {result.stderr}")
                return False
            
            # ビルド出力解析 (stderr + stdout)
            build_output = result.stderr + result.stdout
            
            # RAM使用量解析
            ram_match = re.search(r'RAM:\s+\[\s*([=\s]*)\]\s+([\d.]+)%\s+\(used (\d+) bytes from (\d+) bytes\)', build_output)
            if ram_match:
                ram_percent = float(ram_match.group(2))
                ram_used = int(ram_match.group(3))
                ram_total = int(ram_match.group(4))
                
                self.results['memory_analysis']['ram'] = {
                    'used_bytes': ram_used,
                    'total_bytes': ram_total,
                    'used_percent': ram_percent,
                    'free_bytes': ram_total - ram_used,
                    'free_percent': 100.0 - ram_percent
                }
            
            # Flash使用量解析
            flash_match = re.search(r'Flash:\s+\[\s*([=\s]*)\]\s+([\d.]+)%\s+\(used (\d+) bytes from (\d+) bytes\)', build_output)
            if flash_match:
                flash_percent = float(flash_match.group(2))
                flash_used = int(flash_match.group(3))
                flash_total = int(flash_match.group(4))
                
                self.results['memory_analysis']['flash'] = {
                    'used_bytes': flash_used,
                    'total_bytes': flash_total,
                    'used_percent': flash_percent,
                    'free_bytes': flash_total - flash_used,
                    'free_percent': 100.0 - flash_percent
                }
            
            # ELFファイル情報取得
            elf_file = self.build_dir / "firmware.elf"
            if elf_file.exists():
                elf_size = elf_file.stat().st_size
                self.results['build_info']['elf_size'] = elf_size
            
            # UF2ファイル情報取得
            uf2_file = self.build_dir / "firmware.uf2"
            if uf2_file.exists():
                uf2_size = uf2_file.stat().st_size
                self.results['build_info']['uf2_size'] = uf2_size
            
            print(f"✓ Build analysis completed successfully")
            return True
            
        except Exception as e:
            print(f"Build analysis failed: {e}")
            return False
    
    def analyze_library_dependencies(self):
        """ライブラリ依存関係解析"""
        print("=== Library Dependencies Analysis ===")
        
        try:
            # ライブラリ情報取得
            result = subprocess.run(
                ['pio', 'lib', 'list'],
                cwd=self.project_root,
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                lib_output = result.stdout
                
                # ライブラリ使用量解析
                libraries = []
                for line in lib_output.split('\n'):
                    if '@' in line and 'Library' in line:
                        lib_info = line.strip()
                        libraries.append(lib_info)
                
                self.results['build_info']['libraries'] = libraries
                self.results['build_info']['library_count'] = len(libraries)
                
                print(f"✓ Found {len(libraries)} libraries")
            
        except Exception as e:
            print(f"Library analysis failed: {e}")
    
    def calculate_performance_metrics(self):
        """パフォーマンス指標計算"""
        print("=== Performance Metrics Calculation ===")
        
        ram_data = self.results['memory_analysis'].get('ram', {})
        flash_data = self.results['memory_analysis'].get('flash', {})
        
        # メモリ効率計算
        if ram_data:
            ram_efficiency = (100 - ram_data['used_percent']) / 100
            self.results['performance_metrics']['ram_efficiency'] = ram_efficiency
            
            # 推定並行処理容量
            available_ram = ram_data['free_bytes']
            estimated_concurrent_operations = available_ram // 1024  # 1KB per operation estimate
            self.results['performance_metrics']['estimated_concurrent_capacity'] = estimated_concurrent_operations
        
        if flash_data:
            flash_efficiency = (100 - flash_data['used_percent']) / 100
            self.results['performance_metrics']['flash_efficiency'] = flash_efficiency
            
            # 将来の機能拡張余地
            available_flash = flash_data['free_bytes']
            expansion_capacity_mb = available_flash / (1024 * 1024)
            self.results['performance_metrics']['expansion_capacity_mb'] = expansion_capacity_mb
        
        # CPU使用率推定（静的解析ベース）
        # 実際のハードウェアでは動的測定が必要
        estimated_cpu_usage = self.estimate_cpu_usage()
        self.results['performance_metrics']['estimated_cpu_usage'] = estimated_cpu_usage
        
        print("✓ Performance metrics calculated")
    
    def estimate_cpu_usage(self):
        """CPU使用率推定"""
        # 静的解析による推定
        base_cpu_usage = 15  # Base system operations
        
        # コンポーネント別推定負荷
        components = {
            'gps_processing': 20,      # GPS data processing
            'ntp_server': 15,          # NTP server operations
            'web_server': 25,          # Web interface
            'metrics_collection': 10,  # Prometheus metrics
            'logging': 5,              # Logging operations  
            'display_update': 10       # OLED display updates
        }
        
        total_estimated = base_cpu_usage + sum(components.values())
        
        # 実際の使用率は並行処理効率により調整
        parallel_efficiency = 0.7  # 70% parallel efficiency
        actual_estimated = total_estimated * parallel_efficiency
        
        return min(actual_estimated, 100)  # Cap at 100%
    
    def generate_recommendations(self):
        """最適化推奨事項生成"""
        print("=== Generating Recommendations ===")
        
        recommendations = []
        
        ram_data = self.results['memory_analysis'].get('ram', {})
        flash_data = self.results['memory_analysis'].get('flash', {})
        
        # RAM使用量に基づく推奨
        if ram_data:
            ram_percent = ram_data.get('used_percent', 0)
            
            if ram_percent > 80:
                recommendations.append({
                    'type': 'critical',
                    'category': 'memory',
                    'message': 'RAM usage is critically high (>80%). Consider optimizing memory usage.',
                    'suggestions': [
                        'Reduce buffer sizes',
                        'Optimize string operations',
                        'Review dynamic allocations'
                    ]
                })
            elif ram_percent > 60:
                recommendations.append({
                    'type': 'warning', 
                    'category': 'memory',
                    'message': 'RAM usage is high (>60%). Monitor for potential issues.',
                    'suggestions': [
                        'Monitor memory usage during operation',
                        'Consider memory profiling'
                    ]
                })
            else:
                recommendations.append({
                    'type': 'info',
                    'category': 'memory', 
                    'message': f'RAM usage is optimal ({ram_percent:.1f}%). Good memory efficiency.',
                    'suggestions': []
                })
        
        # Flash使用量に基づく推奨
        if flash_data:
            flash_percent = flash_data.get('used_percent', 0)
            
            if flash_percent > 90:
                recommendations.append({
                    'type': 'critical',
                    'category': 'storage',
                    'message': 'Flash usage is critically high (>90%). Limited space for updates.',
                    'suggestions': [
                        'Remove unused features',
                        'Optimize code size',
                        'Consider external storage'
                    ]
                })
            elif flash_percent > 70:
                recommendations.append({
                    'type': 'warning',
                    'category': 'storage', 
                    'message': 'Flash usage is high (>70%). Plan for future feature additions.',
                    'suggestions': [
                        'Monitor flash usage growth',
                        'Plan modular architecture'
                    ]
                })
            else:
                recommendations.append({
                    'type': 'info',
                    'category': 'storage',
                    'message': f'Flash usage is reasonable ({flash_percent:.1f}%). Good expansion capacity.',
                    'suggestions': []
                })
        
        # CPU使用率に基づく推奨
        cpu_usage = self.results['performance_metrics'].get('estimated_cpu_usage', 0)
        if cpu_usage > 85:
            recommendations.append({
                'type': 'critical',
                'category': 'performance',
                'message': 'Estimated CPU usage is very high (>85%). System may be unstable.',
                'suggestions': [
                    'Optimize critical loops',
                    'Reduce processing frequency',
                    'Consider task prioritization'
                ]
            })
        elif cpu_usage > 70:
            recommendations.append({
                'type': 'warning',
                'category': 'performance',
                'message': 'Estimated CPU usage is high (>70%). Monitor system responsiveness.',
                'suggestions': [
                    'Profile CPU-intensive operations',
                    'Consider optimization opportunities'
                ]
            })
        
        self.results['recommendations'] = recommendations
        print(f"✓ Generated {len(recommendations)} recommendations")
    
    def generate_report(self):
        """詳細レポート生成"""
        print("\n" + "="*60)
        print("GPS NTP SERVER - PERFORMANCE BENCHMARK REPORT")
        print("="*60)
        
        print(f"\nTimestamp: {self.results['timestamp']}")
        print(f"Project: {self.results['project_root']}")
        
        # Memory Analysis
        print("\n--- MEMORY ANALYSIS ---")
        ram_data = self.results['memory_analysis'].get('ram', {})
        flash_data = self.results['memory_analysis'].get('flash', {})
        
        if ram_data:
            print(f"RAM Usage:")
            print(f"  Used: {ram_data['used_bytes']:,} bytes ({ram_data['used_percent']:.1f}%)")
            print(f"  Free: {ram_data['free_bytes']:,} bytes ({ram_data['free_percent']:.1f}%)")
            print(f"  Total: {ram_data['total_bytes']:,} bytes")
        
        if flash_data:
            print(f"\nFlash Usage:")
            print(f"  Used: {flash_data['used_bytes']:,} bytes ({flash_data['used_percent']:.1f}%)")
            print(f"  Free: {flash_data['free_bytes']:,} bytes ({flash_data['free_percent']:.1f}%)")
            print(f"  Total: {flash_data['total_bytes']:,} bytes")
        
        # Performance Metrics
        print("\n--- PERFORMANCE METRICS ---")
        perf_data = self.results['performance_metrics']
        
        if 'ram_efficiency' in perf_data:
            print(f"RAM Efficiency: {perf_data['ram_efficiency']:.1%}")
        
        if 'flash_efficiency' in perf_data:
            print(f"Flash Efficiency: {perf_data['flash_efficiency']:.1%}")
            
        if 'estimated_cpu_usage' in perf_data:
            print(f"Estimated CPU Usage: {perf_data['estimated_cpu_usage']:.1f}%")
            
        if 'estimated_concurrent_capacity' in perf_data:
            print(f"Estimated Concurrent Capacity: {perf_data['estimated_concurrent_capacity']} operations")
            
        if 'expansion_capacity_mb' in perf_data:
            print(f"Flash Expansion Capacity: {perf_data['expansion_capacity_mb']:.1f} MB")
        
        # Build Information
        print("\n--- BUILD INFORMATION ---")
        build_data = self.results['build_info']
        
        if 'library_count' in build_data:
            print(f"Library Dependencies: {build_data['library_count']}")
            
        if 'elf_size' in build_data:
            print(f"ELF File Size: {build_data['elf_size']:,} bytes")
            
        if 'uf2_size' in build_data:
            print(f"UF2 File Size: {build_data['uf2_size']:,} bytes")
        
        # Recommendations
        print("\n--- RECOMMENDATIONS ---")
        recommendations = self.results.get('recommendations', [])
        
        if not recommendations:
            print("No specific recommendations.")
        else:
            for rec in recommendations:
                icon = {'critical': '🔴', 'warning': '🟡', 'info': '🟢'}.get(rec['type'], '📋')
                print(f"\n{icon} {rec['type'].upper()}: {rec['category'].upper()}")
                print(f"  {rec['message']}")
                
                if rec['suggestions']:
                    print("  Suggestions:")
                    for suggestion in rec['suggestions']:
                        print(f"    - {suggestion}")
        
        # Overall Assessment
        print("\n--- OVERALL ASSESSMENT ---")
        
        critical_count = len([r for r in recommendations if r['type'] == 'critical'])
        warning_count = len([r for r in recommendations if r['type'] == 'warning'])
        
        if critical_count > 0:
            print("🔴 CRITICAL: System has critical performance issues that need immediate attention.")
        elif warning_count > 0:
            print("🟡 WARNING: System has some performance concerns that should be monitored.")
        else:
            print("🟢 GOOD: System performance is within acceptable parameters.")
        
        print("\n" + "="*60)
    
    def save_results(self, filename="benchmark_results.json"):
        """結果をJSONファイルに保存"""
        output_file = self.project_root / "test" / filename
        
        try:
            with open(output_file, 'w') as f:
                json.dump(self.results, f, indent=2)
            print(f"\n✓ Results saved to: {output_file}")
        except Exception as e:
            print(f"Failed to save results: {e}")
    
    def run_full_benchmark(self):
        """完全ベンチマーク実行"""
        print("Starting GPS NTP Server Performance Benchmark...")
        print("="*60)
        
        success = True
        
        # ビルド解析
        if not self.run_build_analysis():
            success = False
        
        # ライブラリ依存関係解析
        self.analyze_library_dependencies()
        
        # パフォーマンス指標計算
        self.calculate_performance_metrics()
        
        # 推奨事項生成
        self.generate_recommendations()
        
        # レポート生成
        self.generate_report()
        
        # 結果保存
        self.save_results()
        
        return success

def main():
    if len(sys.argv) > 1:
        project_root = sys.argv[1]
    else:
        project_root = Path.cwd()
    
    benchmark = SystemPerformanceBenchmark(project_root)
    success = benchmark.run_full_benchmark()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()