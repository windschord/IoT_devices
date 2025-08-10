# GPS NTP Server - PlatformIO Build & Upload Makefile
# Raspberry Pi Pico 2 プロジェクト用統合ビルドスクリプト

# 変数定義
PIO = pio
ENV = pico
PORT ?= auto
BAUD ?= 9600

# デフォルトターゲット
.PHONY: all
all: build

# ヘルプ表示
.PHONY: help
help:
	@echo "GPS NTP Server - Build & Upload Commands"
	@echo ""
	@echo "Basic Commands:"
	@echo "  make build       - プロジェクトをビルド"
	@echo "  make upload      - ファームウェアをアップロード"
	@echo "  make uploadfs    - ファイルシステム（HTML/JS）をアップロード"
	@echo "  make full        - ビルド + ファームウェア + ファイルシステム アップロード"
	@echo "  make clean       - ビルド成果物をクリーンアップ"
	@echo ""
	@echo "Development Commands:"
	@echo "  make monitor     - シリアルモニター開始 ($(BAUD) baud)"
	@echo "  make test        - テスト実行"
	@echo "  make check       - コードチェック（コンパイルのみ）"
	@echo ""
	@echo "Advanced Commands:"
	@echo "  make rebuild     - クリーン + ビルド"
	@echo "  make flash       - ファームウェアのみアップロード"
	@echo "  make reset       - デバイスリセット"
	@echo ""
	@echo "File System Commands:"
	@echo "  make fs-build    - ファイルシステムイメージ作成"
	@echo "  make fs-check    - data/ ディレクトリの確認"
	@echo ""
	@echo "Options:"
	@echo "  PORT=<device>    - シリアルポート指定 (デフォルト: auto)"
	@echo "  BAUD=<rate>      - ボーレート指定 (デフォルト: 9600)"
	@echo ""
	@echo "Examples:"
	@echo "  make full                    - 完全ビルド&アップロード"
	@echo "  make monitor BAUD=115200     - 115200ボーレートでモニター"
	@echo "  make upload PORT=/dev/ttyACM0 - 特定ポートにアップロード"

# ビルドコマンド
.PHONY: build
build:
	@echo "🔨 Building GPS NTP Server..."
	$(PIO) run -e $(ENV)
	@echo "✅ Build completed successfully"

# ファームウェアアップロード
.PHONY: upload flash
upload flash:
	@echo "📤 Uploading firmware to Raspberry Pi Pico 2..."
	$(PIO) run -e $(ENV) -t upload
	@echo "✅ Firmware upload completed"

# ファイルシステムアップロード（HTML/JS）
.PHONY: uploadfs fs-upload
uploadfs fs-upload:
	@echo "📁 Checking data directory..."
	@if [ ! -d "data" ]; then \
		echo "❌ Error: data/ directory not found"; \
		echo "💡 Please ensure data/gps.html and data/gps.js exist"; \
		exit 1; \
	fi
	@if [ ! -f "data/gps.html" ]; then \
		echo "❌ Error: data/gps.html not found"; \
		exit 1; \
	fi
	@if [ ! -f "data/gps.js" ]; then \
		echo "❌ Error: data/gps.js not found"; \
		exit 1; \
	fi
	@echo "📁 Uploading filesystem (HTML/JS files)..."
	$(PIO) run -e $(ENV) -t uploadfs
	@echo "✅ Filesystem upload completed"

# 完全デプロイ（ビルド + ファームウェア + ファイルシステム）
.PHONY: full deploy
full deploy: build upload uploadfs
	@echo ""
	@echo "🎉 Complete deployment finished!"
	@echo "📋 Summary:"
	@echo "  ✅ Firmware built and uploaded"
	@echo "  ✅ Web files (HTML/JS) uploaded to LittleFS"
	@echo "  🌐 GPS Web interface available at: http://<device-ip>/gps"
	@echo ""
	@echo "🔍 Next steps:"
	@echo "  1. Connect to device via 'make monitor'"
	@echo "  2. Check IP address in serial output"
	@echo "  3. Open web browser to access GPS interface"

# クリーンビルド
.PHONY: clean
clean:
	@echo "🧹 Cleaning build artifacts..."
	$(PIO) run -e $(ENV) -t clean
	@echo "✅ Clean completed"

# リビルド（クリーン + ビルド）
.PHONY: rebuild
rebuild: clean build

# シリアルモニター
.PHONY: monitor
monitor:
	@echo "📺 Starting serial monitor ($(BAUD) baud)..."
	@echo "💡 Press Ctrl+C to exit"
	$(PIO) device monitor -b $(BAUD)

# テスト実行
.PHONY: test
test:
	@echo "🧪 Running tests..."
	$(PIO) test -e native

# コードチェック（コンパイルのみ）
.PHONY: check
check:
	@echo "🔍 Checking code (compile only)..."
	$(PIO) check -e $(ENV)

# ファイルシステム関連コマンド
.PHONY: fs-build
fs-build:
	@echo "🗂️ Building filesystem image..."
	$(PIO) run -e $(ENV) -t buildfs
	@echo "✅ Filesystem image built"

.PHONY: fs-check
fs-check:
	@echo "📋 Checking data directory contents:"
	@if [ -d "data" ]; then \
		find data -type f -name "*.html" -o -name "*.js" -o -name "*.css" | while read file; do \
			size=$$(wc -c < "$$file" 2>/dev/null || echo "0"); \
			echo "  📄 $$file ($$size bytes)"; \
		done; \
		echo ""; \
		total_files=$$(find data -type f | wc -l); \
		total_size=$$(find data -type f -exec wc -c {} + 2>/dev/null | tail -1 | awk '{print $$1}' || echo "0"); \
		echo "📊 Total: $$total_files files, $$total_size bytes"; \
	else \
		echo "❌ data/ directory not found"; \
		echo "💡 Run 'mkdir data' and add your web files"; \
	fi

# デバイスリセット
.PHONY: reset
reset:
	@echo "🔄 Resetting device..."
	$(PIO) device monitor --echo --eol LF --raw -b $(BAUD) | head -1
	@echo "✅ Device reset completed"

# ライブラリ更新
.PHONY: lib-update
lib-update:
	@echo "📚 Updating libraries..."
	$(PIO) lib update
	@echo "✅ Libraries updated"

# 開発用快速コマンド
.PHONY: dev
dev: build upload monitor

# プロジェクト情報表示
.PHONY: info
info:
	@echo "📋 Project Information:"
	@echo "  Project: GPS NTP Server"
	@echo "  Target: Raspberry Pi Pico 2 (RP2350)"
	@echo "  Environment: $(ENV)"
	@echo "  Framework: Arduino"
	@echo ""
	@echo "📊 Project Stats:"
	@$(PIO) run -e $(ENV) --target size || echo "  Run 'make build' first to see size info"
	@echo ""
	@echo "📚 Dependencies:"
	@$(PIO) lib list | grep -E "(Ethernet|SparkFun|ArduinoJson|LittleFS)" || echo "  Run 'pio lib list' for full dependency list"

# デバッグビルド
.PHONY: debug
debug:
	@echo "🐛 Building in debug mode..."
	$(PIO) run -e $(ENV) --target debug

# インストール確認
.PHONY: check-install
check-install:
	@echo "🔍 Checking PlatformIO installation..."
	@which $(PIO) > /dev/null || (echo "❌ PlatformIO not found. Install with: pip install platformio" && exit 1)
	@$(PIO) --version
	@echo "✅ PlatformIO is ready"

# Makefileの使用方法確認
.PHONY: usage
usage: help

# エラーハンドリング付きの安全なアップロード
.PHONY: safe-upload
safe-upload:
	@echo "🛡️ Safe upload with validation..."
	@echo "1️⃣ Building project..."
	@$(MAKE) build
	@echo "2️⃣ Checking filesystem..."
	@$(MAKE) fs-check
	@echo "3️⃣ Uploading firmware..."
	@$(MAKE) upload
	@echo "4️⃣ Uploading filesystem..."
	@$(MAKE) uploadfs
	@echo "✅ Safe upload completed successfully"

# === Advanced Testing Commands ===

# 包括的テストレポート生成
.PHONY: test-report
test-report:
	@echo "📊 Generating comprehensive test report..."
	python3 scripts/generate_test_report.py
	@echo "✅ Test report generated: test_report.html"

# 並列テスト実行
.PHONY: test-parallel
test-parallel:
	@echo "⚡ Running parallel tests..."
	python3 scripts/parallel_test_runner.py . 4
	@echo "✅ Parallel tests completed"

# 高速並列テスト（2ワーカー）
.PHONY: test-fast
test-fast:
	@echo "🏃 Running fast parallel tests..."
	python3 scripts/parallel_test_runner.py . 2
	@echo "✅ Fast tests completed"

# クロスプラットフォームテスト
.PHONY: test-cross-platform
test-cross-platform:
	@echo "🌐 Running cross-platform tests..."
	python3 scripts/cross_platform_test.py
	@echo "✅ Cross-platform tests completed"

# 全テストスイート実行
.PHONY: test-all
test-all: test test-parallel test-report
	@echo ""
	@echo "🎉 All test suites completed!"
	@echo "📊 Reports available:"
	@echo "  - test_report.html (comprehensive test report)"
	@echo "  - parallel_test_results.json (parallel execution results)"
	@echo ""

# テスト環境確認
.PHONY: test-env
test-env:
	@echo "🔍 Checking test environment..."
	@echo "Python version: $$(python3 --version)"
	@echo "PlatformIO: $$($(PIO) --version)"
	@if command -v docker >/dev/null 2>&1; then \
		echo "Docker: $$(docker --version)"; \
	else \
		echo "Docker: Not available"; \
	fi
	@echo ""
	@echo "📁 Available test environments:"
	@$(PIO) test --list-tests || echo "  Run tests to see available environments"

# テスト結果クリーンアップ
.PHONY: test-clean
test-clean:
	@echo "🧹 Cleaning test results..."
	@rm -f test_report.html test_report.json
	@rm -f parallel_test_results.json parallel_test_summary.txt
	@rm -f cross_platform_results.json cross_platform_summary.txt
	@echo "✅ Test results cleaned"

# CI環境用テスト
.PHONY: test-ci
test-ci:
	@echo "🤖 Running CI tests..."
	@$(MAKE) build
	@$(MAKE) test-fast
	@$(MAKE) test-report
	@echo "✅ CI tests completed"