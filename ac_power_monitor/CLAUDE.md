# AC Power Monitor Project Guide

## Build Commands
- Build: `pio run`
- Upload: `pio run --target upload`
- Monitor: `pio run --target monitor`
- Clean: `pio run --target clean`
- Testing: `pio test`
- Single test: `pio test -f test_name`

## Code Style Guidelines

### Naming Conventions
- Class methods: camelCase (setup(), getVoltage())
- Variables: camelCase (alarmThreshold, powerFactor)
- Constants: UPPERCASE with underscores (BUTTON_PIN)
- Class members: snake_case for private variables (pzem_slave_addr)

### Formatting
- Indentation: 4 spaces
- Braces: opening brace on same line as function declaration
- Max line length: ~100 characters

### Error Handling
- Serial debug output for errors with descriptive messages
- Return code checking for operations (especially Modbus)
- Explicit error codes in logging statements

### Architecture
- Use FreeRTOS tasks for concurrent operations
- Follow modular design pattern with separate .h and .cpp files
- Implement Prometheus metrics for monitoring