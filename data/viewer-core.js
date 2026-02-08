// Binary data decoder for MPULogRecord format
class MPULogDecoder {
  constructor() {
    this.RECORD_SIZE = 32; // bytes per record
  }

  async decodeFile(arrayBuffer) {
    const dataView = new DataView(arrayBuffer);
    const records = [];
    
    const recordCount = Math.floor(arrayBuffer.byteLength / this.RECORD_SIZE);
    
    // Only iterate while there's enough data for a complete record
    for (let offset = 0; offset <= arrayBuffer.byteLength - this.RECORD_SIZE; offset += this.RECORD_SIZE) {
      try {
        const record = {
          timestamp: dataView.getUint32(offset, true), // little-endian
          accel_x: dataView.getFloat32(offset + 4, true),
          accel_y: dataView.getFloat32(offset + 8, true),
          accel_z: dataView.getFloat32(offset + 12, true),
          yaw: dataView.getFloat32(offset + 16, true),
          pitch: dataView.getFloat32(offset + 20, true),
          roll: dataView.getFloat32(offset + 24, true),
          flags: dataView.getUint8(offset + 31)  // Correct offset for flags field
        };
        records.push(record);
      } catch (error) {
        console.error('Error decoding record at offset', offset, ':', error);
        // Stop decoding on error to prevent corrupt data
        break;
      }
    }
    
    const hasPartialRecord = (arrayBuffer.byteLength % this.RECORD_SIZE) !== 0;
    
    return {
      records: records,
      recordCount: recordCount,
      expectedCount: recordCount,
      corrupted: records.length < recordCount || hasPartialRecord
    };
  }
}

// Helper function to calculate optimal Y-axis range with proper rounding
function calculateOptimalYRange(dataSeries) {
  if (!dataSeries || dataSeries.length === 0) {
    return { min: 0, max: 1 };
  }

  // Find the true min and max values across all series
  let globalMin = Infinity;
  let globalMax = -Infinity;
  
  dataSeries.forEach(series => {
    if (!series || series.length === 0) return;
    
    const seriesMin = Math.min(...series);
    const seriesMax = Math.max(...series);
    
    if (seriesMin < globalMin) globalMin = seriesMin;
    if (seriesMax > globalMax) globalMax = seriesMax;
  });

  // If all values are the same, create a small range around that value
  if (Math.abs(globalMax - globalMin) < 0.001) {
    const center = globalMin;
    const padding = Math.max(1, Math.abs(center) * 0.1);
    return { 
      min: roundToNice(center - padding), 
      max: roundToNice(center + padding) 
    };
  }

  // Check if data spans both positive and negative values (bidirectional)
  const hasPositiveValues = globalMax > 0;
  const hasNegativeValues = globalMin < 0;
  
  if (hasPositiveValues && hasNegativeValues) {
    // For bidirectional data, create symmetric range around zero
    const maxAbsValue = Math.max(Math.abs(globalMin), Math.abs(globalMax));
    
    // Add 10% padding to the maximum absolute value
    const paddedMaxAbs = maxAbsValue * 1.1;
    
    // Apply nice rounding and create symmetric range
    const niceMax = roundToNice(paddedMaxAbs);
    
    return {
      min: -niceMax,
      max: niceMax
    };
  } else {
    // For unidirectional data, use asymmetric range with padding
    const range = globalMax - globalMin;
    const padding = range * 0.1;
    
    const minWithPadding = globalMin - padding;
    const maxWithPadding = globalMax + padding;

    return {
      min: roundToNice(minWithPadding),
      max: roundToNice(maxWithPadding)
    };
  }
}

// Helper function to round to "nice" numbers for clean grid lines
function roundToNice(value) {
  const absValue = Math.abs(value);
  
  if (absValue === 0) return 0;
  
  // Determine the order of magnitude
  const exponent = Math.floor(Math.log10(absValue));
  const base = Math.pow(10, exponent);
  
  // Normalize the value to 1-10 range
  const normalized = absValue / base;
  
  // Find the nearest "nice" multiple (1, 2, 5, or 10)
  let niceMultiple;
  if (normalized <= 1.5) {
    niceMultiple = 1;
  } else if (normalized <= 3) {
    niceMultiple = 2;
  } else if (normalized <= 7) {
    niceMultiple = 5;
  } else {
    niceMultiple = 10;
  }
  
  // Calculate the nice rounded value
  const niceValue = niceMultiple * base;
  
  // Apply the correct sign
  return value < 0 ? -niceValue : niceValue;
}

// Global variables
let decoder = new MPULogDecoder();
let currentData = null;
let accelPlot = null;
let orientationPlot = null;
let fileList = [];
let currentZoomRange = { accel: null, orientation: null };
let defaultZoomRanges = { accel: { min: -10, max: 10 }, orientation: { min: -180, max: 180 } };
let currentView = 'charts'; // 'charts' or 'table'
let currentLocalFileName = null; // Track current file name for local files
let isOfflineMode = false; // Track whether we're in offline mode
let standAloneMode = false; //True if not fetched from a logger device captive portal

// Offline mode management functions
function enableOfflineMode() {
  isOfflineMode = true;
  document.body.classList.add('offline-mode');
  if(standAloneMode){
    updateStatus('Use "Load From..." to open files.', 'warning');
  }else{
    updateStatus('Offline Mode - Logger not connected. Use "Load From..." to open local files.', 'warning');
  }
}

function disableOfflineMode() {
  isOfflineMode = false;
  document.body.classList.remove('offline-mode');
  updateStatus('Online Mode - Logger connected.', 'success');
}

// Retry server connection
async function retryServerConnection() {
  updateStatus('Attempting to reconnect to logger...', 'info');
  
  try {
    const response = await fetch('/api/files');
    if (response.ok) {
      const data = await response.json();
      if (data.files && Array.isArray(data.files)) {
        fileList = data.files;
        updateFileSelect();
        disableOfflineMode();
        updateStatus('Server connection restored successfully', 'success');
      }
    }
  } catch (error) {
    updateStatus('Server still unavailable. Remaining in offline mode.', 'warning');
  }
}

// Load file list from server
async function loadFileList() {
  try {
    const response = await fetch('/api/files');
    const data = await response.json();
    
    if (data.files && Array.isArray(data.files)) {
      fileList = data.files;
      updateFileSelect();
      
      // If we were in offline mode, switch back to online mode
      if (isOfflineMode) {
        disableOfflineMode();
      }
    } else {
      updateStatus('Invalid response format from server', 'error');
    }
  } catch (error) {
    // If server is unavailable, enable offline mode
    enableOfflineMode();
  }
}

// Update file selection dropdown
function updateFileSelect() {
  const select = document.getElementById('file-select');
  select.innerHTML = '<option value="">Select a file...</option>';
  
  // Filter to only show .bin files
  fileList.filter(file => file.name.endsWith('.bin')).forEach(file => {
    const option = document.createElement('option');
    option.value = file.name;
    option.textContent = `${file.name} (${formatFileSize(file.size)})`;
    select.appendChild(option);
  });
  
  document.getElementById('delete-btn').disabled = true;
}

// Handle file selection change
function handleFileSelection() {
  const filename = document.getElementById('file-select').value;
  const deleteBtn = document.getElementById('delete-btn');
  const downloadBinBtn = document.getElementById('download-bin-btn');
  const loadBtn = document.getElementById('load-btn');
  
  // Enable delete and download buttons when a file is selected, regardless of whether it's loaded
  deleteBtn.disabled = !filename;
  downloadBinBtn.disabled = !filename;
  
  // Load button remains disabled until user explicitly clicks it
  loadBtn.disabled = false;
}

// Local file loading functions
function loadLocalFile() {
  document.getElementById('local-file-input').click();
}

async function handleLocalFileSelection(event) {
  const file = event.target.files[0];
  if (!file) return;
  
  // Check if it's a .bin file
  if (!file.name.toLowerCase().endsWith('.bin')) {
    updateStatus('Please select a .bin file', 'warning');
    return;
  }

  currentLocalFileName = file.name;
  updateStatus(`Loading ${file.name}...`, 'info');
  showProgress();

  try {
    // Read the file as ArrayBuffer
    const arrayBuffer = await file.arrayBuffer();
    updateProgress(50);
    
    // Decode binary data using existing decoder
    const decodedData = await decoder.decodeFile(arrayBuffer);
    updateProgress(75);
    
    if (decodedData.records.length === 0) {
      throw new Error('No valid data found in file');
    }

    currentData = decodedData;
    updateProgress(100);
    
    // Create a mock file object for UI consistency
    const mockFile = {
      name: file.name,
      size: file.size,
      lastModified: file.lastModified
    };
    
    // Update UI
    updateFileInfo(mockFile, decodedData);
    displayData(decodedData);
    
    // Reset to chart view by default
    currentView = 'charts';
    document.getElementById('table-btn').disabled = false;
    document.getElementById('chart-btn').disabled = true;
    
    // Update button states for local file
    updateLocalFileControls(true, true);
    
    updateStatus(`Successfully loaded ${decodedData.records.length} records from ${file.name}`, 'success');
    
  } catch (error) {
    updateStatus('Error loading local file: ' + error.message, 'error');
    console.error('Local file loading error:', error);
    currentLocalFileName = null;
  } finally {
    hideProgress();
    // Clear the file input to allow selecting the same file again
    event.target.value = '';
  }
}

function updateLocalFileControls(fileLoaded, isLocalFile = false) {
  // Don't disable server controls in offline mode since they're already hidden
  if (!isOfflineMode) {
    document.getElementById('file-select').disabled = fileLoaded;
    document.getElementById('load-btn').disabled = fileLoaded;
    document.getElementById('delete-btn').disabled = fileLoaded;
    // Only disable BIN download button for local files, not server files
    document.getElementById('download-bin-btn').disabled = isLocalFile && fileLoaded;
  }
  
  // Enable/disable other controls based on file loaded state
  document.getElementById('download-btn').disabled = !fileLoaded;
  document.getElementById('download-csv-btn').disabled = !fileLoaded;
  document.getElementById('local-load-btn').disabled = fileLoaded;
  
  if (!fileLoaded) {
    // Re-enable server controls when no file is loaded (only in online mode)
    if (!isOfflineMode) {
      document.getElementById('file-select').disabled = false;
      document.getElementById('load-btn').disabled = false;
      document.getElementById('delete-btn').disabled = true;
      document.getElementById('download-bin-btn').disabled = true;
    }
    document.getElementById('local-load-btn').disabled = false;
    currentLocalFileName = null;
  }
}

// Download binary file
function downloadBinFile() {
  const filename = document.getElementById('file-select').value;
  if (!filename) return;
  
  // Create a direct link to the binary file
  const url = filename.startsWith('/') ? filename : '/' + filename;
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  
  updateStatus(`Downloading ${filename}`, 'success');
}

// Load selected file
async function loadSelectedFile() {
  const filename = document.getElementById('file-select').value;
  if (!filename) {
    updateStatus('Please select a file to load', 'warning');
    return;
  }

  const file = fileList.find(f => f.name === filename);
  if (!file) {
    updateStatus('File not found: ' + filename, 'error');
    return;
  }

  updateStatus(`Loading ${filename}...`, 'info');
  showProgress();
  document.getElementById('load-btn').disabled = true;
  document.getElementById('delete-btn').disabled = true;

  try {
    const response = await fetch(`${filename.startsWith('/') ? filename : '/' + filename}`);
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    const arrayBuffer = await response.arrayBuffer();
    updateProgress(50);
    
    // Decode binary data
    const decodedData = await decoder.decodeFile(arrayBuffer);
    updateProgress(75);
    
    if (decodedData.records.length === 0) {
      throw new Error('No valid data found in file');
    }

    currentData = decodedData;
    currentLocalFileName = null; // Clear local file name when loading server file
    updateProgress(100);
    
    // Update UI
    updateFileInfo(file, decodedData);
    displayData(decodedData);
    
    // Reset to chart view by default
    currentView = 'charts';
    document.getElementById('table-btn').disabled = false;
    document.getElementById('chart-btn').disabled = true;
    
    // Reset local file controls when loading server file, but re-enable BIN download
    updateLocalFileControls(false, false);
    // Explicitly re-enable BIN download for server files
    document.getElementById('download-bin-btn').disabled = false;
    
    updateStatus(`Successfully loaded ${decodedData.records.length} records from ${filename}`, 'success');
    document.getElementById('delete-btn').disabled = false;
    document.getElementById('download-btn').disabled = false;
    document.getElementById('download-csv-btn').disabled = false;
    
  } catch (error) {
    updateStatus('Error loading file: ' + error.message, 'error');
    console.error('File loading error:', error);
  } finally {
    hideProgress();
    document.getElementById('load-btn').disabled = false;
  }
}

// Delete selected file
async function deleteSelectedFile() {
  const filename = document.getElementById('file-select').value;
  if (!filename) return;

  if (!confirm(`Are you sure you want to delete ${filename}?`)) {
    return;
  }

  updateStatus(`Deleting ${filename}...`, 'info');
  
  try {
    const response = await fetch(`${filename.startsWith('/') ? filename : '/' + filename}`, { method: 'DELETE' });
    const data = await response.json();
    
    if (data.status === 'ok') {
      updateStatus(`Successfully deleted ${filename}`, 'success');
      await loadFileList(); // Refresh file list
      
      // Clear the current data if the deleted file was loaded
      if (currentData && document.getElementById('file-select').value === filename) {
        clearCharts();
        currentData = null;
      }
      
      // Reset delete button state
      document.getElementById('delete-btn').disabled = true;
    } else {
      updateStatus('Error deleting file: ' + (data.error || 'Unknown error'), 'error');
    }
  } catch (error) {
    updateStatus('Network error deleting file: ' + error.message, 'error');
  }
}

// Convert absolute timestamps to relative time in seconds from start
function convertToRelativeTime(timestamps) {
  if (timestamps.length === 0) return [];
  
  const startTime = timestamps[0];
  return timestamps.map(timestamp => (timestamp - startTime) / 1000); // Convert to seconds
}

// Display decoded data in charts
function displayData(decodedData) {
  const records = decodedData.records;
  
  // Prepare data arrays
  const timestamps = records.map(r => r.timestamp);
  const accelX = records.map(r => r.accel_x);
  const accelY = records.map(r => r.accel_y);
  const accelZ = records.map(r => r.accel_z);
  const yaw = records.map(r => r.yaw);
  const pitch = records.map(r => r.pitch);
  const roll = records.map(r => r.roll);

  // Convert timestamps to relative time for charts
  const relativeTime = convertToRelativeTime(timestamps);
  const chartData = {
    relativeTime: relativeTime,
    accelX: accelX,
    accelY: accelY,
    accelZ: accelZ,
    yaw: yaw,
    pitch: pitch,
    roll: roll,
    originalTimestamps: timestamps // Keep for reference
  };

  // Initialize or update plots
  if (!accelPlot) {
    initPlots(chartData);
    
    setTimeout(()=>{
      handleResize();
    }, 100);        
  } else {
    updatePlots(chartData);

    setTimeout(()=>{
      handleResize();
    }, 100);
  }

  document.getElementById('charts').style.display = 'grid';
}


function getSize(elementId) {
  const el = document.getElementById(elementId);
  return {
    width: el.offsetWidth,
    height: 300
  };
}

// Initialize uPlot charts
function initPlots(data) {
  const dims = getSize('accel-plot');

  // Calculate auto-scaled Y-axis range for acceleration data
  const accelRange = calculateOptimalYRange([data.accelX, data.accelY, data.accelZ]);

  // Acceleration plot
  accelPlot = new uPlot({
    width: dims.width,
    height: dims.height,
    scales: {
      x: {
        time: false
      },
      y: {
        range: [accelRange.min, accelRange.max],
        align: 1
      }
    },
    series: [
      {
        label: "Time (seconds)",
        value: (u, v) => v != null ? v.toFixed(2) + "s" : "",
      },
      {
        label: "Accel X",
        stroke: "#ff6b6b",
        width: 1,
        points: { show: false }
      },
      {
        label: "Accel Y", 
        stroke: "#4ecdc4",
        width: 1,
        points: { show: false }
      },
      {
        label: "Accel Z",
        stroke: "#45b7d1", 
        width: 1,
        points: { show: false }
      }
    ],
    axes: [
      {
        stroke: "#666",
        grid: { show: false },
        label: "Time (seconds)",
        size: 60
      },
      {
        stroke: "#666",
        grid: { show: true },
        size: 50
      }
    ],
    cursor: {
      sync: {
        key: "cursor",
        setSeries: true,
      },
      focus: {
        prox: 30,
      },
    }
  }, [
    data.relativeTime,
    data.accelX,
    data.accelY,
    data.accelZ
  ], document.getElementById('accel-plot'));

  // Calculate auto-scaled Y-axis range for orientation data
  const orientationRange = calculateOptimalYRange([data.yaw, data.pitch, data.roll]);

  // Orientation plot
  orientationPlot = new uPlot({
    width: dims.width,
    height: dims.height,
    scales: {
      x: {
        time: false
      },
      y: {
        range: [orientationRange.min, orientationRange.max],
        align: 1
      }
    },
    series: [
      {
        label: "Time (seconds)",
        value: (u, v) => v != null ? v.toFixed(2) + "s" : "",
      },
      {
        label: "Yaw",
        stroke: "#ff9f43",
        width: 1,
        points: { show: false }
      },
      {
        label: "Pitch",
        stroke: "#10ac84",
        width: 1,
        points: { show: false }
      },
      {
        label: "Roll",
        stroke: "#5f27cd",
        width: 1,
        points: { show: false }
      }
    ],
    axes: [
      {
        stroke: "#666",
        grid: { show: false },
        label: "Time (seconds)",
        size: 60
      },
      {
        stroke: "#666",
        grid: { show: true },
        size: 50
      }
    ],
    cursor: {
      sync: {
        key: "cursor",
        setSeries: true,
      },
      focus: {
        prox: 30,
      },
    }
  }, [
    data.relativeTime,
    data.yaw,
    data.pitch,
    data.roll
  ], document.getElementById('gyros-plot'));
}

// Update existing plots with new data
function updatePlots(data) {
  if (accelPlot) {
    // Calculate new Y-axis range for acceleration data
    const accelRange = calculateOptimalYRange([data.accelX, data.accelY, data.accelZ]);
    
    // Update the Y-axis scale
    accelPlot.setScale('y', {
      min: accelRange.min,
      max: accelRange.max
    });
    
    // Update the data
    accelPlot.setData([
      data.relativeTime,
      data.accelX,
      data.accelY,
      data.accelZ
    ]);
  }
  
  if (orientationPlot) {
    // Calculate new Y-axis range for orientation data
    const orientationRange = calculateOptimalYRange([data.yaw, data.pitch, data.roll]);
    
    // Update the Y-axis scale
    orientationPlot.setScale('y', {
      min: orientationRange.min,
      max: orientationRange.max
    });
    
    // Update the data
    orientationPlot.setData([
      data.relativeTime,
      data.yaw,
      data.pitch,
      data.roll
    ]);
  }
}

// Zoom functions
// Helper to get the absolute start and end time of the current data
function getDataExtents(plot) {
  // plot.data[0] always contains the x-axis values (timestamps/relative time)
  const xData = plot.data[0];
  if (!xData || xData.length === 0) return { min: 0, max: 10 };
  
  return {
    min: xData[0],
    max: xData[xData.length - 1]
  };
}

function zoomIn(chartType) {
  const plot = chartType === 'accel' ? accelPlot : orientationPlot;
  if (!plot) return;

  const currentMin = plot.scales.x.min;
  const currentMax = plot.scales.x.max;
  const range = currentMax - currentMin;
  const center = currentMin + (range / 2);

  // Zoom in by reducing the time window to 60% of current
  const newRange = range * 0.6;

  plot.setScale('x', {
    min: center - (newRange / 2),
    max: center + (newRange / 2)
  });
}

function zoomOut(chartType) {
  const plot = chartType === 'accel' ? accelPlot : orientationPlot;
  if (!plot) return;

  const currentMin = plot.scales.x.min;
  const currentMax = plot.scales.x.max;
  const range = currentMax - currentMin;
  const center = currentMin + (range / 2);
  const extents = getDataExtents(plot);

  // Zoom out by increasing the time window
  const newRange = range * 1.5;
  
  let newMin = center - (newRange / 2);
  let newMax = center + (newRange / 2);

  // Optional: Clamp so we don't zoom out infinitely past the actual data
  if (newMin < extents.min) newMin = extents.min;
  if (newMax > extents.max) newMax = extents.max;

  plot.setScale('x', {
    min: newMin,
    max: newMax
  });
}

function resetZoom(chartType) {
  const plot = chartType === 'accel' ? accelPlot : orientationPlot;
  if (!plot) return;

  // "Zoom Extents": Set the scale to the absolute first and last data points
  const extents = getDataExtents(plot);

  plot.setScale('x', {
    min: extents.min,
    max: extents.max
  });
}

// View toggle functions
function showTableView() {
  if (!currentData) return;
  
  currentView = 'table';
  document.getElementById('charts').style.display = 'none';
  document.getElementById('table-container').style.display = 'block';
  document.getElementById('table-btn').disabled = true;
  document.getElementById('chart-btn').disabled = false;
  
  updateTableData();
  updateTable();
}

function showChartView() {
  if (!currentData) return;
  
  currentView = 'charts';
  document.getElementById('table-container').style.display = 'none';
  document.getElementById('charts').style.display = 'grid';
  document.getElementById('table-btn').disabled = false;
  document.getElementById('chart-btn').disabled = true;
}

// JSON download function
function downloadAsJSON() {
  if (!currentData) return;

  // Use current file name (works for both server and local files)
  const filename = currentLocalFileName || document.getElementById('file-select').value;
  
  // Get the current data based on view
  let exportData;
  if (currentView === 'table') {
    exportData = filteredTableData;
  } else {
    // Export all chart data
    exportData = currentData.records;
  }

  const duration = exportData.length > 0 ? 
    (exportData[exportData.length - 1].timestamp - exportData[0].timestamp) / 1000 : 0;

  const jsonData = {
    metadata: {
      filename: filename,
      recordCount: exportData.length,
      duration: duration,
      sampleRate: (exportData.length / Math.max(duration, 1)).toFixed(2),
      exportTime: new Date().toISOString(),
      corrupted: currentData.corrupted
    },
    records: exportData
  };

  const blob = new Blob([JSON.stringify(jsonData, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename.replace('.bin', '_data.json');
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);

  updateStatus(`Downloaded ${exportData.length} records as JSON`, 'success');
}

// CSV download function
function downloadAsCSV() {
  if (!currentData) return;

  // Use current file name (works for both server and local files)
  const filename = currentLocalFileName || document.getElementById('file-select').value;
  
  // Always export all records (same as JSON export behavior)
  const records = currentData.records;
  
  if (records.length === 0) {
    updateStatus('No data available to export', 'warning');
    return;
  }

  // Convert timestamps to relative time for CSV export
  const timestamps = records.map(r => r.timestamp);
  const relativeTime = convertToRelativeTime(timestamps);

  // Build CSV content with header row
  let csvContent = 'Time (s),Delay (s),Accel X (G),Accel Y (G),Accel Z (G),Yaw (°),Pitch (°),Roll (°),Flags\n';
  
  // Add data rows
  for (let i = 0; i < records.length; i++) {
    const record = records[i];
    const interSampleDelay = i === 0 ? 0.000 : (timestamps[i] - timestamps[i - 1]) / 1000;
    
    const row = [
      relativeTime[i].toFixed(3),                    // Time (s) - 3 decimal places
      interSampleDelay.toFixed(3),                    // Delay (s) - 3 decimal places
      record.accel_x.toFixed(4),                      // Accel X (G) - 4 decimal places
      record.accel_y.toFixed(4),                      // Accel Y (G) - 4 decimal places
      record.accel_z.toFixed(4),                      // Accel Z (G) - 4 decimal places
      record.yaw.toFixed(2),                          // Yaw (°) - 2 decimal places
      record.pitch.toFixed(2),                        // Pitch (°) - 2 decimal places
      record.roll.toFixed(2),                         // Roll (°) - 2 decimal places
      record.flags                                     // Flags - integer
    ];
    
    csvContent += row.join(',') + '\n';
  }

  // Create and download CSV file
  const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename.replace('.bin', '_data.csv');
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);

  updateStatus(`Downloaded ${records.length} records as CSV`, 'success');
}

// Utility functions
function formatFileSize(bytes) {
  if (bytes === 0) return '0 Bytes';
  const k = 1024;
  const sizes = ['Bytes', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function updateStatus(message, type) {
  const statusEl = document.getElementById('status');
  statusEl.textContent = message;
  statusEl.className = 'status ' + type;
}

function updateFileInfo(file, decodedData) {
  const infoEl = document.getElementById('file-info');
  const duration = decodedData.records.length > 0 ? 
    (decodedData.records[decodedData.records.length - 1].timestamp - decodedData.records[0].timestamp) / 1000 : 0;
  
  infoEl.innerHTML = `
    File: ${file.name} | 
    Size: ${formatFileSize(file.size)} | 
    Records: ${decodedData.records.length} | 
    Duration: ${duration.toFixed(1)}s |
    Sample Rate: ${(decodedData.records.length / Math.max(duration, 1)).toFixed(1)} Hz
    ${decodedData.corrupted ? ' | ⚠️ Some data may be corrupted' : ''}
  `;
}

function showProgress() {
  document.getElementById('loading-progress').style.display = 'block';
  document.getElementById('loading-progress-bar').style.width = '0%';
}

function updateProgress(percent) {
  document.getElementById('loading-progress-bar').style.width = percent + '%';
}

function hideProgress() {
  setTimeout(() => {
    document.getElementById('loading-progress').style.display = 'none';
  }, 500);
}

function clearCharts() {
  if (accelPlot) {
    accelPlot.destroy();
    accelPlot = null;
  }
  if (orientationPlot) {
    orientationPlot.destroy();
    orientationPlot = null;
  }
  document.getElementById('charts').style.display = 'none';
  document.getElementById('table-container').style.display = 'none';
  document.getElementById('file-info').innerHTML = '';
  
  // Reset table-related variables (call table module function if available)
  if (typeof clearTableState === 'function') {
    clearTableState();
  }
  
  // Reset buttons
  document.getElementById('table-btn').disabled = true;
  document.getElementById('chart-btn').disabled = true;
  document.getElementById('download-btn').disabled = true;
  document.getElementById('download-csv-btn').disabled = true;
  document.getElementById('download-bin-btn').disabled = true;
  
  // Reset local file controls
  updateLocalFileControls(false);
  
  // Clear table body
  document.getElementById('table-body').innerHTML = '';
  document.getElementById('search-box').value = '';
  
  // Reset sort indicators
  for (let i = 0; i < 9; i++) {
    const indicator = document.getElementById(`sort-${i}`);
    if (indicator) indicator.textContent = '';
  }
  
  // Reset zoom state
  currentZoomRange = { accel: null, orientation: null };
  
  // Reset view state
  currentView = 'charts';
}

// Handle window resize
function handleResize() {
  const accelDims = getSize('accel-plot');
  const orientDims = getSize('gyros-plot');
  
  if (accelPlot) {
    accelPlot.setSize(accelDims);
  }
  
  if (orientationPlot) {
    orientationPlot.setSize(orientDims);
  }
}

// Initialize page
async function init() {
  
  // Hide retry button if protocol is not file or host is not IP address
  const protocol = window.location.protocol;
  const hostname = window.location.hostname;
  
  if (protocol == 'file:' || !isValidIPAddress(hostname)) {
    const retryBtn = document.getElementById('retry-btn');
    if (retryBtn) {
      retryBtn.style.display = 'none';
    }
    standAloneMode = true;
  }
  
  await loadFileList();
  
  // Verify record size consistency
  await verifyRecordSize();
  
  // Handle window resize
  window.addEventListener('resize', handleResize);
}

// Check if hostname is a valid IP address
function isValidIPAddress(hostname) {
  // IPv4 regex pattern
  const ipv4Pattern = /^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
  
  // IPv6 regex pattern (simplified)
  const ipv6Pattern = /^(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$|^::1$|^::$/;

  return ipv4Pattern.test(hostname) || 
         ipv6Pattern.test(hostname);
}

// Verify record size consistency between client and server
async function verifyRecordSize() {
  // Skip verification if we're in offline mode
  if (isOfflineMode) {
    return;
  }
  
  try {
    const response = await fetch('/api/meta');
    if (!response.ok) {
      console.warn('Could not verify record size - server returned:', response.status, response.statusText);
      return;
    }
    
    const meta = await response.json();
    const serverRecordSize = meta.recordSize;
    
    // Compare with local constant
    if (serverRecordSize !== decoder.RECORD_SIZE) {
      const errorMsg = `⚠️ Record size mismatch detected! Server reports ${serverRecordSize} bytes, but client expects ${decoder.RECORD_SIZE} bytes. This may cause data corruption.`;
      updateStatus(errorMsg, 'error');
      console.error(errorMsg);
      
      // Show warning in file info as well
      const infoEl = document.getElementById('file-info');
      if (infoEl.innerHTML) {
        infoEl.innerHTML += `<br><span style="color: #ff6b6b; font-weight: bold;">${errorMsg}</span>`;
      }
    }
  } catch (error) {
    console.error('Error verifying record size:', error);
    // Don't show error message in offline mode as it's expected
    if (!isOfflineMode) {
      updateStatus('Error verifying record size: ' + error.message, 'error');
    }
  }
}

// Start when page loads
document.addEventListener('DOMContentLoaded', init);
