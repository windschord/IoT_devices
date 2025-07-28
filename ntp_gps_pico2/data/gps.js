// Chrome専用 Modern JavaScript (ES6+)
// GPS Satellite Display - Separated from C++ for better maintainability

// Global variables
let gpsData = null;
let previousGpsData = null;
let updateInterval = null;
let connectionStatus = 'connecting';
let connectionFailures = 0;
let lastUserInteraction = 0;
let isUserInteracting = false;

// Constants
const CONSTELLATIONS = [
  { key: 'gps', name: 'GPS', color: '#f39c12' },
  { key: 'sbas', name: 'SBAS', color: '#95a5a6' },
  { key: 'galileo', name: 'Galileo', color: '#27ae60' },
  { key: 'beidou', name: 'BeiDou', color: '#3498db' },
  { key: 'glonass', name: 'GLONASS', color: '#e74c3c' },
  { key: 'qzss', name: 'QZSS', color: '#9b59b6' }
];

const GPS_DATA_CACHE_INTERVAL = 2000; // 2 seconds
const MAX_CONNECTION_FAILURES = 5;
const USER_INTERACTION_TIMEOUT = 10000; // 10 seconds

// Utility functions
function handleUserInteraction() {
  lastUserInteraction = Date.now();
  isUserInteracting = true;
  
  // Clear user interaction flag after timeout
  setTimeout(() => {
    if (Date.now() - lastUserInteraction >= USER_INTERACTION_TIMEOUT) {
      isUserInteracting = false;
    }
  }, USER_INTERACTION_TIMEOUT);
}

function hasSignificantChange(oldData, newData) {
  if (!oldData || !newData) return true;
  
  // Check for significant position change (>1 meter)
  const positionThreshold = 0.00001; // ~1 meter
  const latDiff = Math.abs(newData.latitude - oldData.latitude);
  const lonDiff = Math.abs(newData.longitude - oldData.longitude);
  
  if (latDiff > positionThreshold || lonDiff > positionThreshold) {
    return true;
  }
  
  // Check for fix type change
  if (newData.fix_type !== oldData.fix_type) {
    return true;
  }
  
  // Check for satellite count change
  if (newData.satellites_total !== oldData.satellites_total) {
    return true;
  }
  
  return false;
}

// Network functions
async function fetchGpsData() {
  const startTime = Date.now();
  
  try {
    const response = await fetch('/api/gps', {
      method: 'GET',
      headers: {
        'Cache-Control': 'no-cache',
        'Accept': 'application/json'
      },
      signal: AbortSignal.timeout(5000) // 5 second timeout
    });
    
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    
    const text = await response.text();
    
    // Validate JSON before parsing
    if (!text || text.length === 0) {
      throw new Error('Empty response received');
    }
    
    // Parse JSON with error handling
    let data;
    try {
      data = JSON.parse(text);
    } catch (parseError) {
      console.error('JSON Parse Error:', parseError);
      console.error('Response text length:', text.length);
      console.error('Response preview:', text.substring(0, 200));
      throw new Error(`JSON parsing failed: ${parseError.message}`);
    }
    
    // Store previous data for comparison
    previousGpsData = gpsData;
    gpsData = data;
    
    // Reset connection failure counter
    connectionFailures = 0;
    connectionStatus = 'connected';
    
    // Only update display if data has changed significantly or user is not interacting
    if (!isUserInteracting && hasSignificantChange(previousGpsData, gpsData)) {
      updateDisplayWithTimestamp();
    }
    
    updateConnectionStatus();
    const responseTime = Date.now() - startTime;
    console.log(`GPS data updated in ${responseTime}ms`);
    
  } catch (error) {
    connectionFailures++;
    connectionStatus = 'error';
    console.error('GPS data fetch error:', error);
    
    updateConnectionStatus();
    
    // Attempt reconnection after multiple failures
    if (connectionFailures >= MAX_CONNECTION_FAILURES) {
      attemptReconnection();
    }
  }
}

function attemptReconnection() {
  console.log('Attempting reconnection...');
  connectionStatus = 'connecting';
  updateConnectionStatus();
  
  // Reset failure counter and try again
  connectionFailures = 0;
  
  // Immediate retry
  setTimeout(fetchGpsData, 1000);
}

function updateConnectionStatus() {
  const statusElement = document.getElementById('connectionStatus');
  if (statusElement) {
    statusElement.textContent = connectionStatus === 'connected' ? 'Connected' : 
                               connectionStatus === 'connecting' ? 'Connecting...' : 
                               `Connection Error (${connectionFailures}/5)`;
    
    statusElement.className = `connection-status ${connectionStatus}`;
  }
}

// Display functions
function updateDisplay() {
  if (!gpsData) return;
  
  drawRadarChart();
  updateConstellationStats();
  updateDateView();
}

function drawRadarChart() {
  const canvas = document.getElementById('radarChart');
  const ctx = canvas.getContext('2d');
  
  // Clear canvas
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  
  const centerX = canvas.width / 2;
  const centerY = canvas.height / 2;
  const radius = Math.min(centerX, centerY) - 20;
  
  // Get zoom level
  const zoomSlider = document.getElementById('zoomSlider');
  const zoomFactor = zoomSlider ? zoomSlider.value / 15 : 1.0;
  const adjustedRadius = radius * zoomFactor;
  
  // Draw concentric circles (every 30 degrees elevation)
  ctx.strokeStyle = 'rgba(255, 255, 255, 0.3)';
  ctx.lineWidth = 1;
  
  for (let i = 1; i <= 3; i++) {
    const r = (adjustedRadius / 3) * i;
    ctx.beginPath();
    ctx.arc(centerX, centerY, r, 0, 2 * Math.PI);
    ctx.stroke();
  }
  
  // Draw cardinal directions
  ctx.strokeStyle = 'rgba(255, 255, 255, 0.5)';
  ctx.lineWidth = 1;
  
  // North-South line
  ctx.beginPath();
  ctx.moveTo(centerX, centerY - adjustedRadius);
  ctx.lineTo(centerX, centerY + adjustedRadius);
  ctx.stroke();
  
  // East-West line
  ctx.beginPath();
  ctx.moveTo(centerX - adjustedRadius, centerY);
  ctx.lineTo(centerX + adjustedRadius, centerY);
  ctx.stroke();
  
  // Draw direction labels
  ctx.fillStyle = 'rgba(255, 255, 255, 0.8)';
  ctx.font = '14px Arial';
  ctx.textAlign = 'center';
  ctx.fillText('N', centerX, centerY - adjustedRadius - 10);
  ctx.fillText('S', centerX, centerY + adjustedRadius + 20);
  ctx.textAlign = 'left';
  ctx.fillText('E', centerX + adjustedRadius + 10, centerY + 5);
  ctx.textAlign = 'right';
  ctx.fillText('W', centerX - adjustedRadius - 10, centerY + 5);
  
  // Draw elevation circles labels
  ctx.textAlign = 'center';
  ctx.font = '12px Arial';
  ctx.fillStyle = 'rgba(255, 255, 255, 0.6)';
  ctx.fillText('30°', centerX + (adjustedRadius / 3), centerY - 5);
  ctx.fillText('60°', centerX + (adjustedRadius * 2 / 3), centerY - 5);
  ctx.fillText('90°', centerX + adjustedRadius - 15, centerY - 5);
  
  // Draw satellites
  if (gpsData && gpsData.satellites) {
    gpsData.satellites.forEach(sat => {
      if (!shouldShowSatellite(sat)) return;
      
      // Convert elevation and azimuth to canvas coordinates
      const elevationRad = (90 - sat.elevation) * Math.PI / 180;
      const azimuthRad = (sat.azimuth - 90) * Math.PI / 180;
      
      const distance = (adjustedRadius * elevationRad) / (Math.PI / 2);
      const x = centerX + distance * Math.cos(azimuthRad);
      const y = centerY + distance * Math.sin(azimuthRad);
      
      // Get constellation color
      const constellation = CONSTELLATIONS[sat.constellation] || CONSTELLATIONS[0];
      
      // Draw satellite
      ctx.fillStyle = constellation.color;
      ctx.beginPath();
      ctx.arc(x, y, sat.used_in_nav ? 8 : 6, 0, 2 * Math.PI);
      ctx.fill();
      
      // Draw signal strength indicator
      if (sat.signal_strength > 0) {
        const barHeight = (sat.signal_strength / 50) * 20;
        ctx.fillStyle = `rgba(255, 255, 255, 0.7)`;
        ctx.fillRect(x - 2, y - barHeight - 10, 4, barHeight);
      }
      
      // Draw PRN label
      ctx.fillStyle = 'white';
      ctx.font = '10px Arial';
      ctx.textAlign = 'center';
      ctx.fillText(sat.prn.toString(), x, y + 20);
    });
  }
}

function shouldShowSatellite(sat) {
  const showNotTracked = document.getElementById('showNotTracked')?.checked ?? true;
  const showUsedOnly = document.getElementById('showUsedOnly')?.checked ?? false;
  const showHighSignal = document.getElementById('showHighSignal')?.checked ?? false;
  
  // Check constellation filter
  const constellation = CONSTELLATIONS[sat.constellation];
  if (constellation) {
    const filterCheckbox = document.getElementById(`filter_${constellation.key}`);
    if (filterCheckbox && !filterCheckbox.checked) {
      return false;
    }
  }
  
  // Check tracking filter
  if (!showNotTracked && !sat.tracked) {
    return false;
  }
  
  // Check navigation usage filter
  if (showUsedOnly && !sat.used_in_nav) {
    return false;
  }
  
  // Check signal strength filter
  if (showHighSignal && sat.signal_strength <= 35) {
    return false;
  }
  
  return true;
}

function updateConstellationStats() {
  const statsContainer = document.getElementById('constellationStats');
  if (!statsContainer || !gpsData) return;
  
  const stats = gpsData.constellation_stats;
  if (!stats) return;
  
  let html = `
    <div class="stat-card">
      <strong>Total</strong>
      ${stats.satellites_total}
    </div>
    <div class="stat-card">
      <strong>Used</strong>
      ${stats.satellites_used}
    </div>
  `;
  
  // Add constellation-specific stats
  const constellationStats = {
    gps: stats.gps || { total: 0, used: 0 },
    glonass: stats.glonass || { total: 0, used: 0 },
    galileo: stats.galileo || { total: 0, used: 0 },
    beidou: stats.beidou || { total: 0, used: 0 },
    sbas: stats.sbas || { total: 0, used: 0 },
    qzss: stats.qzss || { total: 0, used: 0 }
  };
  
  Object.entries(constellationStats).forEach(([key, data]) => {
    const constellation = CONSTELLATIONS.find(c => c.key === key);
    if (constellation && data.total > 0) {
      html += `
        <div class="stat-card" style="border-left: 4px solid ${constellation.color}">
          <strong>${constellation.name}</strong>
          ${data.used}/${data.total}
        </div>
      `;
    }
  });
  
  statsContainer.innerHTML = html;
}

function updateGnssControls() {
  const controlsContainer = document.getElementById('gnssControls');
  if (!controlsContainer || !gpsData) return;
  
  const enables = gpsData.constellation_enables || {};
  
  let html = '';
  CONSTELLATIONS.forEach(constellation => {
    const enabled = enables[constellation.key] ?? true;
    const checked = document.getElementById(`filter_${constellation.key}`)?.checked ?? true;
    
    html += `
      <div style="margin: 5px 0; display: flex; align-items: center;">
        <input type="checkbox" id="filter_${constellation.key}" ${checked ? 'checked' : ''} onchange="handleUserInteraction(); updateDisplay();">
        <div style="width: 16px; height: 16px; background: ${constellation.color}; margin: 0 8px; border-radius: 2px;"></div>
        <label for="filter_${constellation.key}" style="font-size: 12px;">
          ${constellation.name} ${enabled ? '(Enabled)' : '(Disabled)'}
        </label>
      </div>
    `;
  });
  
  controlsContainer.innerHTML = html;
}

function updateDateView() {
  const container = document.getElementById('datePositionInfo');
  if (!container || !gpsData) return;
  
  const fixTypeNames = ['No Fix', 'Dead Reckoning', '2D Fix', '3D Fix', 'GNSS + DR', 'Time Only'];
  const fixTypeName = fixTypeNames[gpsData.fix_type] || 'Unknown';
  
  // Format UTC time
  const utcTime = gpsData.utc_time ? new Date(gpsData.utc_time * 1000).toISOString() : 'N/A';
  
  // Calculate speed in different units
  const speedKmh = (gpsData.speed * 3.6).toFixed(1);
  const speedMph = (gpsData.speed * 2.237).toFixed(1);
  
  const html = `
    <div class="info-item">
      <strong>Fix Status</strong>
      ${fixTypeName} (Type ${gpsData.fix_type})
    </div>
    <div class="info-item">
      <strong>Position</strong>
      ${gpsData.latitude.toFixed(6)}°, ${gpsData.longitude.toFixed(6)}°
    </div>
    <div class="info-item">
      <strong>Altitude</strong>
      ${gpsData.altitude.toFixed(1)} m
    </div>
    <div class="info-item">
      <strong>UTC Time</strong>
      ${utcTime}
    </div>
    <div class="info-item">
      <strong>Speed</strong>
      ${gpsData.speed.toFixed(1)} m/s (${speedKmh} km/h, ${speedMph} mph)
    </div>
    <div class="info-item">
      <strong>Course</strong>
      ${gpsData.course.toFixed(1)}°
    </div>
    <div class="info-item">
      <strong>HDOP</strong>
      ${gpsData.hdop.toFixed(2)}
    </div>
    <div class="info-item">
      <strong>VDOP</strong>
      ${gpsData.vdop.toFixed(2)}
    </div>
    <div class="info-item">
      <strong>3D Accuracy</strong>
      ${gpsData.accuracy_3d.toFixed(1)} m
    </div>
    <div class="info-item">
      <strong>2D Accuracy</strong>
      ${gpsData.accuracy_2d.toFixed(1)} m
    </div>
    <div class="info-item">
      <strong>TTFF</strong>
      ${gpsData.ttff} seconds
    </div>
  `;
  
  container.innerHTML = html;
}

// Control functions
function updateZoomLabel() {
  const slider = document.getElementById('zoomSlider');
  const label = document.getElementById('zoomValue');
  if (slider && label) {
    const zoomValue = (slider.value / 15).toFixed(1);
    label.textContent = `${zoomValue}x`;
  }
  updateDisplay();
}

function resetView() {
  // Reset zoom
  const zoomSlider = document.getElementById('zoomSlider');
  if (zoomSlider) {
    zoomSlider.value = 15;
    updateZoomLabel();
  }
  
  // Reset filters
  document.getElementById('showNotTracked').checked = true;
  document.getElementById('showUsedOnly').checked = false;
  document.getElementById('showHighSignal').checked = false;
  
  // Reset constellation filters
  const constellationFilters = document.querySelectorAll('[id^="filter_"]');
  constellationFilters.forEach(filter => filter.checked = true);
  
  updateDisplay();
}

function updateDisplayWithTimestamp() {
  updateDisplay();
  
  // Update last update timestamp
  const now = new Date();
  const timeString = now.toLocaleTimeString();
  const timestampElement = document.getElementById('lastUpdateTime');
  if (timestampElement) {
    timestampElement.textContent = timeString;
  }
}

// Initialize everything when page loads
document.addEventListener('DOMContentLoaded', function() {
  console.log('GPS Satellite Display initialized');
  
  // Initial data fetch
  fetchGpsData();
  
  // Set up periodic updates
  updateInterval = setInterval(fetchGpsData, GPS_DATA_CACHE_INTERVAL);
  
  // Add event listeners for all interactive controls
  document.getElementById('showNotTracked')?.addEventListener('change', () => {
    handleUserInteraction();
    updateDisplay();
  });
  
  document.getElementById('showUsedOnly')?.addEventListener('change', () => {
    handleUserInteraction();
    updateDisplay();
  });
  
  document.getElementById('showHighSignal')?.addEventListener('change', () => {
    handleUserInteraction();
    updateDisplay();
  });
  
  document.getElementById('zoomSlider')?.addEventListener('input', () => {
    handleUserInteraction();
    updateZoomLabel();
  });
  
  // Add interaction detection for buttons
  document.querySelectorAll('.btn').forEach(btn => {
    btn.addEventListener('click', handleUserInteraction);
  });
  
  // Initial update of GNSS controls when data loads
  setTimeout(() => {
    if (gpsData) {
      updateGnssControls();
    }
  }, 1000);
});

// Clean up on page unload
window.addEventListener('beforeunload', function() {
  if (updateInterval) {
    clearInterval(updateInterval);
  }
});