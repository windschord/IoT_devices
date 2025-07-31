// GPS NTP Server Configuration JavaScript
// Handles form validation, API communication, and UI interactions

// Configuration loaded flag
let configLoaded = false;

// Tab management
function openTab(evt, tabName) {
    const tabContents = document.getElementsByClassName("tab-content");
    for (let i = 0; i < tabContents.length; i++) {
        tabContents[i].classList.remove("active");
    }
    
    const tabs = document.getElementsByClassName("tab");
    for (let i = 0; i < tabs.length; i++) {
        tabs[i].classList.remove("active");
    }
    
    document.getElementById(tabName).classList.add("active");
    evt.currentTarget.classList.add("active");
}

// Utility functions
function validateIP(ip) {
    const pattern = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    return pattern.test(ip);
}

function validatePort(port) {
    const portNum = parseInt(port);
    return portNum >= 1 && portNum <= 65535;
}

function validateHostname(hostname) {
    if (!hostname || hostname.length === 0 || hostname.length > 31) {
        return false;
    }
    const pattern = /^[a-zA-Z0-9-]+$/;
    return pattern.test(hostname);
}

// Field error display
function showFieldError(fieldId, message) {
    const field = document.getElementById(fieldId);
    field.classList.add('error-field');
    
    // Remove existing error message
    const existingError = field.nextElementSibling;
    if (existingError && existingError.classList.contains('field-error')) {
        existingError.remove();
    }
    
    // Add new error message
    const errorDiv = document.createElement('div');
    errorDiv.className = 'field-error';
    errorDiv.textContent = message;
    field.parentNode.insertBefore(errorDiv, field.nextSibling);
}

function clearFieldErrors() {
    const errorFields = document.querySelectorAll('.error-field');
    errorFields.forEach(field => field.classList.remove('error-field'));
    
    const errorMessages = document.querySelectorAll('.field-error');
    errorMessages.forEach(msg => msg.remove());
}

// Form validation
function validateFormData(formId, jsonData) {
    clearFieldErrors();
    let isValid = true;
    
    if (formId === 'networkForm') {
        if (!validateHostname(jsonData.hostname)) {
            showFieldError('hostname', 'Hostname must be 1-31 characters, alphanumeric and hyphens only');
            isValid = false;
        }
        
        if (!jsonData.use_dhcp) {
            if (jsonData.ip_address && !validateIP(jsonData.ip_address)) {
                showFieldError('ip_address', 'Invalid IP address format');
                isValid = false;
            }
            if (jsonData.netmask && !validateIP(jsonData.netmask)) {
                showFieldError('netmask', 'Invalid subnet mask format');
                isValid = false;
            }
            if (jsonData.gateway && !validateIP(jsonData.gateway)) {
                showFieldError('gateway', 'Invalid gateway IP format');
                isValid = false;
            }
            if (jsonData.dns_server && !validateIP(jsonData.dns_server)) {
                showFieldError('dns_server', 'Invalid DNS server IP format');
                isValid = false;
            }
        }
    }
    
    if (formId === 'gnssForm') {
        if (jsonData.gnss_update_rate && (jsonData.gnss_update_rate < 1 || jsonData.gnss_update_rate > 10)) {
            showFieldError('gnss_update_rate', 'Update rate must be between 1 and 10 Hz');
            isValid = false;
        }
    }
    
    if (formId === 'ntpForm') {
        if (jsonData.ntp_port && !validatePort(jsonData.ntp_port)) {
            showFieldError('ntp_port', 'Port must be between 1 and 65535');
            isValid = false;
        }
    }
    
    if (formId === 'systemForm') {
        if (jsonData.restart_interval && (jsonData.restart_interval < 1 || jsonData.restart_interval > 168)) {
            showFieldError('restart_interval', 'Restart interval must be between 1 and 168 hours');
            isValid = false;
        }
    }
    
    if (formId === 'logsForm') {
        if (jsonData.syslog_server && jsonData.syslog_server.length > 0 && !validateIP(jsonData.syslog_server)) {
            showFieldError('syslog_server', 'Invalid IP address format');
            isValid = false;
        }
        if (jsonData.syslog_port && !validatePort(jsonData.syslog_port)) {
            showFieldError('syslog_port', 'Port must be between 1 and 65535');
            isValid = false;
        }
    }
    
    return isValid;
}

// Form submission handler
function handleFormSubmit(formId, apiEndpoint) {
    const form = document.getElementById(formId);
    const formData = new FormData(form);
    const jsonData = {};
    
    // Convert FormData to JSON
    for (let [key, value] of formData.entries()) {
        const field = form.querySelector('[name="' + key + '"]');
        if (field.type === 'checkbox') {
            jsonData[key] = field.checked;
        } else if (field.type === 'number') {
            jsonData[key] = parseInt(value);
        } else {
            jsonData[key] = value;
        }
    }
    
    // Handle special cases for network form
    if (formId === 'networkForm') {
        const useDhcp = document.getElementById('use_dhcp').checked;
        if (useDhcp) {
            jsonData.ip_address = 0;
            jsonData.netmask = 0;
            jsonData.gateway = 0;
            jsonData.dns_server = 0;
        } else {
            // Convert IP addresses to integers if needed
            if (jsonData.ip_address) jsonData.ip_address = ipToInt(jsonData.ip_address);
            if (jsonData.netmask) jsonData.netmask = ipToInt(jsonData.netmask);
            if (jsonData.gateway) jsonData.gateway = ipToInt(jsonData.gateway);
            if (jsonData.dns_server) jsonData.dns_server = ipToInt(jsonData.dns_server);
        }
    }
    
    // Handle custom dropdown values and ensure proper type conversion
    if (formId === 'logsForm') {
        // Ensure log_level is converted to integer
        if (jsonData.log_level !== undefined) {
            jsonData.log_level = parseInt(jsonData.log_level);
        }
        // Ensure syslog_port is converted to integer
        if (jsonData.syslog_port !== undefined) {
            jsonData.syslog_port = parseInt(jsonData.syslog_port);
        }
    }
    
    if (formId === 'ntpForm') {
        // Ensure ntp_stratum is converted to integer
        if (jsonData.ntp_stratum !== undefined) {
            jsonData.ntp_stratum = parseInt(jsonData.ntp_stratum);
        }
        // Ensure ntp_port is converted to integer
        if (jsonData.ntp_port !== undefined) {
            jsonData.ntp_port = parseInt(jsonData.ntp_port);
        }
    }
    
    if (formId === 'gnssForm') {
        // Ensure disaster_alert_priority is converted to integer
        if (jsonData.disaster_alert_priority !== undefined) {
            jsonData.disaster_alert_priority = parseInt(jsonData.disaster_alert_priority);
        }
        // Ensure gnss_update_rate is converted to integer
        if (jsonData.gnss_update_rate !== undefined) {
            jsonData.gnss_update_rate = parseInt(jsonData.gnss_update_rate);
        }
    }
    
    if (formId === 'systemForm') {
        // Ensure restart_interval is converted to integer
        if (jsonData.restart_interval !== undefined) {
            jsonData.restart_interval = parseInt(jsonData.restart_interval);
        }
    }
    
    // Debug: Log the data being sent (remove in production)
    console.log('Form submission data for', formId, ':', jsonData);
    
    // Validate form data
    if (!validateFormData(formId, jsonData)) {
        showMessage('Please correct the highlighted errors before saving', 'error');
        return;
    }
    
    showLoading();
    showMessage('Saving configuration...', 'warning');
    
    fetch(apiEndpoint, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(jsonData)
    }).then(response => {
          if (!response.ok) {
              // Log response details for debugging
              console.error('HTTP Error:', response.status, response.statusText);
              return response.text().then(text => {
                  console.error('Response body:', text);
                  throw new Error(`HTTP ${response.status}: ${response.statusText}`);
              });
          }
          return response.json();
      })
      .then(data => {
          hideLoading();
          if (data.success) {
              showMessage(data.message || 'Configuration saved successfully', 'success');
          } else {
              console.error('Server error:', data);
              showMessage(data.error || 'Failed to save configuration', 'error');
          }
      }).catch(error => {
          hideLoading();
          console.error('Request failed:', error);
          showMessage('Network error: ' + error.message, 'error');
      });
}

// IP address conversion utilities
function ipToInt(ip) {
    return ip.split('.').reduce((acc, part) => (acc << 8) + parseInt(part), 0) >>> 0;
}

function intToIp(int) {
    return [(int >>> 24) & 255, (int >>> 16) & 255, (int >>> 8) & 255, int & 255].join('.');
}

// Message display functions
function showMessage(message, type) {
    const container = document.getElementById('messageContainer');
    container.innerHTML = '<div class="message ' + type + '">' + message + '</div>';
    setTimeout(() => container.innerHTML = '', 5000);
}

function showLoading() {
    document.getElementById('loadingIndicator').style.display = 'block';
}

function hideLoading() {
    document.getElementById('loadingIndicator').style.display = 'none';
}

// Status update function
async function updateStatus() {
    try {
        const [statusResponse, metricsResponse, logsResponse] = await Promise.all([
            fetch('/api/status'),
            fetch('/api/system/metrics'),
            fetch('/api/system/logs')
        ]);
        
        const statusData = await statusResponse.json();
        const metricsData = await metricsResponse.json();
        const logsData = await logsResponse.json();
        
        updateStatusGrid(statusData, metricsData, logsData);
    } catch (error) {
        console.error('Status update failed:', error);
        showMessage('Failed to update status: ' + error.message, 'error');
    }
}

function updateStatusGrid(status, metrics, logs) {
    const grid = document.getElementById('statusGrid');
    grid.innerHTML = `
        <div class="status-item">
            <label>GPS Status:</label>
            <span class="status-indicator ${status.gps_fix ? 'status-ok' : 'status-error'}">
                ${status.gps_fix ? 'Fixed' : 'No Fix'}
            </span>
        </div>
        <div class="status-item">
            <label>Satellites:</label>
            <span>${status.satellites || 0}</span>
        </div>
        <div class="status-item">
            <label>Network Status:</label>
            <span class="status-indicator ${status.network_connected ? 'status-ok' : 'status-error'}">
                ${status.network_connected ? 'Connected' : 'Disconnected'}
            </span>
        </div>
        <div class="status-item">
            <label>IP Address:</label>
            <span>${status.ip_address || 'N/A'}</span>
        </div>
        <div class="status-item">
            <label>NTP Requests:</label>
            <span>${metrics.ntp_requests || 0}</span>
        </div>
        <div class="status-item">
            <label>Uptime:</label>
            <span>${formatUptime(metrics.uptime_seconds || 0)}</span>
        </div>
        <div class="status-item">
            <label>Memory Usage:</label>
            <span>${((metrics.memory_used || 0) / 1024).toFixed(1)} KB</span>
        </div>
        <div class="status-item">
            <label>System Health:</label>
            <span class="status-indicator ${getHealthIndicator(metrics.health_score || 0)}">
                ${(metrics.health_score || 0).toFixed(1)}%
            </span>
        </div>
    `;
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    
    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m`;
    } else {
        return `${minutes}m`;
    }
}

function getHealthIndicator(score) {
    if (score >= 80) return 'status-ok';
    if (score >= 60) return 'status-warning';
    return 'status-error';
}

// Reset functions
function resetNetworkDefaults() {
    if (confirm('Reset network settings to default values?')) {
        document.getElementById('hostname').value = 'gps-ntp-server';
        document.getElementById('use_dhcp').checked = true;
        document.getElementById('ip_address').value = '';
        document.getElementById('netmask').value = '';
        document.getElementById('gateway').value = '';
        document.getElementById('dns_server').value = '';
        toggleStaticIpFields();
        clearFieldErrors();
        showMessage('Network settings reset to defaults', 'success');
    }
}

function resetGnssDefaults() {
    if (confirm('Reset GNSS settings to default values?')) {
        document.getElementById('gps_enabled').checked = true;
        document.getElementById('glonass_enabled').checked = true;
        document.getElementById('galileo_enabled').checked = true;
        document.getElementById('beidou_enabled').checked = true;
        document.getElementById('qzss_enabled').checked = true;
        document.getElementById('qzss_l1s_enabled').checked = true;
        document.getElementById('gnss_update_rate').value = '1';
        document.getElementById('disaster_alert_priority').value = '1';
        updateCustomDropdown('disaster_alert_priority_dropdown', 1);
        clearFieldErrors();
        showMessage('GNSS settings reset to defaults', 'success');
    }
}

function resetNtpDefaults() {
    if (confirm('Reset NTP settings to default values?')) {
        document.getElementById('ntp_enabled').checked = true;
        document.getElementById('ntp_port').value = '123';
        document.getElementById('ntp_stratum').value = '1';
        updateStratumDropdown(1);
        clearFieldErrors();
        showMessage('NTP settings reset to defaults', 'success');
    }
}

function resetSystemDefaults() {
    if (confirm('Reset system settings to default values?')) {
        document.getElementById('auto_restart_enabled').checked = false;
        document.getElementById('restart_interval').value = '24';
        document.getElementById('debug_enabled').checked = false;
        clearFieldErrors();
        showMessage('System settings reset to defaults', 'success');
    }
}

function resetLogsDefaults() {
    if (confirm('Reset logging settings to default values?')) {
        document.getElementById('syslog_server').value = '192.168.1.100';
        document.getElementById('syslog_port').value = '514';
        document.getElementById('log_level').value = '6';
        updateCustomDropdown('log_level_dropdown', 6);
        document.getElementById('prometheus_enabled').checked = true;
        clearFieldErrors();
        showMessage('Logging settings reset to defaults', 'success');
    }
}

// Factory reset function
function factoryReset() {
    if (confirm('Are you sure you want to reset all settings to factory defaults? This action cannot be undone.')) {
        showLoading();
        fetch('/api/reset', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                hideLoading();
                if (data.success) {
                    showMessage('Factory reset completed. Page will reload in 3 seconds.', 'success');
                    setTimeout(() => location.reload(), 3000);
                } else {
                    showMessage(data.error || 'Factory reset failed', 'error');
                }
            }).catch(error => {
                hideLoading();
                showMessage('Network error: ' + error, 'error');
            });
    }
}

// Static IP fields toggle
function toggleStaticIpFields() {
    const useDhcp = document.getElementById('use_dhcp').checked;
    const staticGroup = document.getElementById('staticIpGroup');
    staticGroup.style.display = useDhcp ? 'none' : 'block';
}

// Load configuration data
async function loadConfiguration() {
    if (configLoaded) return;
    
    try {
        const [networkResponse, gnssResponse, ntpResponse, systemResponse, logsResponse] = await Promise.all([
            fetch('/api/config/network'),
            fetch('/api/config/gnss'),
            fetch('/api/config/ntp'),
            fetch('/api/config/system'),
            fetch('/api/config/log')
        ]);
        
        const [networkData, gnssData, ntpData, systemData, logsData] = await Promise.all([
            networkResponse.json(),
            gnssResponse.json(),
            ntpResponse.json(),
            systemResponse.json(),
            logsResponse.json()
        ]);
        
        // Populate network form
        document.getElementById('hostname').value = networkData.hostname || 'gps-ntp-server';
        document.getElementById('use_dhcp').checked = (networkData.ip_address === 0);
        document.getElementById('ip_address').value = networkData.ip_address ? intToIp(networkData.ip_address) : '';
        document.getElementById('netmask').value = networkData.netmask ? intToIp(networkData.netmask) : '';
        document.getElementById('gateway').value = networkData.gateway ? intToIp(networkData.gateway) : '';
        document.getElementById('dns_server').value = networkData.dns_server ? intToIp(networkData.dns_server) : '';
        document.getElementById('mac_address').textContent = networkData.mac_address || 'Unknown';
        toggleStaticIpFields();
        
        // Populate GNSS form
        document.getElementById('gps_enabled').checked = gnssData.gps_enabled || false;
        document.getElementById('glonass_enabled').checked = gnssData.glonass_enabled || false;
        document.getElementById('galileo_enabled').checked = gnssData.galileo_enabled || false;
        document.getElementById('beidou_enabled').checked = gnssData.beidou_enabled || false;
        document.getElementById('qzss_enabled').checked = gnssData.qzss_enabled || false;
        document.getElementById('qzss_l1s_enabled').checked = gnssData.qzss_l1s_enabled || false;
        document.getElementById('gnss_update_rate').value = gnssData.gnss_update_rate || 1;
        document.getElementById('disaster_alert_priority').value = gnssData.disaster_alert_priority || 1;
        
        // Update custom dropdown for disaster alert priority
        updateCustomDropdown('disaster_alert_priority_dropdown', gnssData.disaster_alert_priority || 1);
        
        // Populate NTP form
        document.getElementById('ntp_enabled').checked = ntpData.ntp_enabled || false;
        document.getElementById('ntp_port').value = ntpData.ntp_port || 123;
        document.getElementById('ntp_stratum').value = ntpData.ntp_stratum || 1;
        
        // Update custom dropdown for stratum level
        updateStratumDropdown(ntpData.ntp_stratum || 1);
        
        // Populate System form
        document.getElementById('auto_restart_enabled').checked = systemData.auto_restart_enabled || false;
        document.getElementById('restart_interval').value = systemData.restart_interval || 24;
        document.getElementById('debug_enabled').checked = systemData.debug_enabled || false;
        
        // Populate Logs form
        document.getElementById('syslog_server').value = logsData.syslog_server || '';
        document.getElementById('syslog_port').value = logsData.syslog_port || 514;
        document.getElementById('log_level').value = logsData.log_level || 6;
        document.getElementById('prometheus_enabled').checked = logsData.prometheus_enabled || false;
        
        // Update custom dropdown for log level
        updateCustomDropdown('log_level_dropdown', logsData.log_level || 6);
        
        configLoaded = true;
        console.log('Configuration loaded successfully');
        
    } catch (error) {
        console.error('Failed to load configuration:', error);
        showMessage('Failed to load configuration: ' + error.message, 'error');
    }
}

// Initialize page
document.addEventListener('DOMContentLoaded', function() {
    // Load configuration data
    loadConfiguration();
    
    // Update status
    updateStatus();
    
    // Set up periodic status updates (every 30 seconds)
    setInterval(updateStatus, 30000);
    
    // Add form submit event listeners
    document.getElementById('networkForm').addEventListener('submit', function(e) {
        e.preventDefault();
        handleFormSubmit('networkForm', '/api/config/network');
    });
    
    document.getElementById('gnssForm').addEventListener('submit', function(e) {
        e.preventDefault();
        handleFormSubmit('gnssForm', '/api/config/gnss');
    });
    
    document.getElementById('ntpForm').addEventListener('submit', function(e) {
        e.preventDefault();
        handleFormSubmit('ntpForm', '/api/config/ntp');
    });
    
    document.getElementById('systemForm').addEventListener('submit', function(e) {
        e.preventDefault();
        handleFormSubmit('systemForm', '/api/config/system');
    });
    
    document.getElementById('logsForm').addEventListener('submit', function(e) {
        e.preventDefault();
        handleFormSubmit('logsForm', '/api/config/log');
    });
    
    // Add DHCP checkbox listener
    document.getElementById('use_dhcp').addEventListener('change', toggleStaticIpFields);
    
    // Add real-time validation for IP address fields
    const ipFields = ['ip_address', 'netmask', 'gateway', 'dns_server', 'syslog_server'];
    ipFields.forEach(fieldName => {
        const field = document.getElementById(fieldName);
        if (field) {
            field.addEventListener('blur', function() {
                if (this.value && !validateIP(this.value)) {
                    this.classList.add('error-field');
                } else {
                    this.classList.remove('error-field');
                }
            });
        }
    });
    
    // Add real-time validation for port fields
    const portFields = ['ntp_port', 'syslog_port'];
    portFields.forEach(fieldName => {
        const field = document.getElementById(fieldName);
        if (field) {
            field.addEventListener('blur', function() {
                if (this.value && !validatePort(this.value)) {
                    this.classList.add('error-field');
                } else {
                    this.classList.remove('error-field');
                }
            });
        }
    });
    
    // Add real-time validation for hostname
    const hostnameField = document.getElementById('hostname');
    if (hostnameField) {
        hostnameField.addEventListener('blur', function() {
            if (!validateHostname(this.value)) {
                this.classList.add('error-field');
            } else {
                this.classList.remove('error-field');
            }
        });
    }
    
    // Initialize custom dropdown for Stratum Level
    initializeCustomDropdown();
    
    console.log('Configuration page initialized');
});

// Custom Dropdown Implementation
function initializeCustomDropdown() {
    // Initialize all custom dropdowns
    const dropdownIds = ['ntp_stratum_dropdown', 'disaster_alert_priority_dropdown', 'log_level_dropdown'];
    
    dropdownIds.forEach(dropdownId => {
        const dropdown = document.getElementById(dropdownId);
        if (!dropdown) return;
        
        const button = dropdown.querySelector('.custom-dropdown-button');
        const options = dropdown.querySelector('.custom-dropdown-options');
        const arrow = dropdown.querySelector('.custom-dropdown-arrow');
        const selectedSpan = dropdown.querySelector(`#${dropdownId.replace('_dropdown', '_selected')}`);
        const hiddenInput = document.getElementById(dropdownId.replace('_dropdown', ''));
        
        if (!button || !options || !arrow || !selectedSpan || !hiddenInput) return;
        
        // Toggle dropdown
        button.addEventListener('click', function() {
            const isOpen = options.classList.contains('open');
            closeAllDropdowns();
            
            if (!isOpen) {
                options.classList.add('open');
                arrow.classList.add('open');
            }
        });
        
        // Handle keyboard navigation
        button.addEventListener('keydown', function(e) {
            if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                button.click();
            }
        });
        
        // Handle option selection
        dropdown.querySelectorAll('.custom-dropdown-option').forEach(option => {
            option.addEventListener('click', function() {
                const value = this.getAttribute('data-value');
                const text = this.textContent;
                
                // Update selected option
                dropdown.querySelectorAll('.custom-dropdown-option').forEach(opt => {
                    opt.classList.remove('selected');
                });
                this.classList.add('selected');
                
                // Update display and hidden input
                selectedSpan.textContent = text;
                hiddenInput.value = value;
                
                // Close dropdown
                options.classList.remove('open');
                arrow.classList.remove('open');
            });
        });
        
        // Close dropdown when clicking outside
        document.addEventListener('click', function(e) {
            if (!dropdown.contains(e.target)) {
                options.classList.remove('open');
                arrow.classList.remove('open');
            }
        });
    });
}

// Helper function to close all dropdowns
function closeAllDropdowns() {
    document.querySelectorAll('.custom-dropdown-options').forEach(options => {
        options.classList.remove('open');
    });
    document.querySelectorAll('.custom-dropdown-arrow').forEach(arrow => {
        arrow.classList.remove('open');
    });
}

// Update dropdown to show selected value (generic function)
function updateCustomDropdown(dropdownId, value) {
    const dropdown = document.getElementById(dropdownId);
    if (!dropdown) return;
    
    const selectedSpan = dropdown.querySelector(`#${dropdownId.replace('_dropdown', '_selected')}`);
    const hiddenInput = document.getElementById(dropdownId.replace('_dropdown', ''));
    const options = dropdown.querySelectorAll('.custom-dropdown-option');
    
    // Find and select the matching option
    options.forEach(option => {
        option.classList.remove('selected');
        if (option.getAttribute('data-value') === value.toString()) {
            option.classList.add('selected');
            selectedSpan.textContent = option.textContent;
            hiddenInput.value = value;
        }
    });
}

// Backward compatibility function for stratum dropdown
function updateStratumDropdown(value) {
    updateCustomDropdown('ntp_stratum_dropdown', value);
}