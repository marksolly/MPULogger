// Table-related variables
let tableData = [];
let filteredTableData = [];
let currentPage = 1;
let rowsPerPage = 100;
let sortColumn = -1;
let sortDirection = 'asc';

// Table functions
function updateTableData() {
  if (!currentData) return;
  
  const records = currentData.records;
  const timestamps = records.map(r => r.timestamp);
  const accelX = records.map(r => r.accel_x);
  const accelY = records.map(r => r.accel_y);
  const accelZ = records.map(r => r.accel_z);
  const yaw = records.map(r => r.yaw);
  const pitch = records.map(r => r.pitch);
  const roll = records.map(r => r.roll);

  // Convert timestamps to relative time for table
  const relativeTime = convertToRelativeTime(timestamps);

  // Calculate inter-sample delay (first record has 0.000 delay)
  tableData = [];
  for (let i = 0; i < timestamps.length; i++) {
    const interSampleDelay = i === 0 ? 0.000 : 
      (timestamps[i] - timestamps[i - 1]) / 1000;

    tableData.push({
      relativeTime: relativeTime[i],
      interSampleDelay: interSampleDelay.toFixed(3),
      accel_x: accelX[i],
      accel_y: accelY[i],
      accel_z: accelZ[i],
      yaw: yaw[i],
      pitch: pitch[i],
      roll: roll[i],
      flags: records[i].flags
    });
  }

  filteredTableData = [...tableData];
  currentPage = 1;
  sortColumn = -1;
  sortDirection = 'asc';
  clearSortIndicators();
}

function updateTable() {
  const tbody = document.getElementById('table-body');
  const startIndex = (currentPage - 1) * rowsPerPage;
  const endIndex = Math.min(startIndex + rowsPerPage, filteredTableData.length);
  
  tbody.innerHTML = '';
  
  for (let i = startIndex; i < endIndex; i++) {
    const row = filteredTableData[i];
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td>${row.relativeTime.toFixed(3)}</td>
      <td>${row.interSampleDelay}</td>
      <td>${row.accel_x.toFixed(4)}</td>
      <td>${row.accel_y.toFixed(4)}</td>
      <td>${row.accel_z.toFixed(4)}</td>
      <td>${row.yaw.toFixed(2)}</td>
      <td>${row.pitch.toFixed(2)}</td>
      <td>${row.roll.toFixed(2)}</td>
      <td>${row.flags}</td>
    `;
    tbody.appendChild(tr);
  }
  
  updatePagination();
}

function updatePagination() {
  const totalPages = Math.ceil(filteredTableData.length / rowsPerPage);
  const pageInfo = document.getElementById('page-info');
  
  pageInfo.textContent = `Page ${currentPage} of ${totalPages} (${filteredTableData.length} records)`;
  
  document.getElementById('first-btn').disabled = currentPage === 1;
  document.getElementById('prev-btn').disabled = currentPage === 1;
  document.getElementById('next-btn').disabled = currentPage === totalPages;
  document.getElementById('last-btn').disabled = currentPage === totalPages;
}

function firstPage() {
  currentPage = 1;
  updateTable();
}

function previousPage() {
  if (currentPage > 1) {
    currentPage--;
    updateTable();
  }
}

function nextPage() {
  const totalPages = Math.ceil(filteredTableData.length / rowsPerPage);
  if (currentPage < totalPages) {
    currentPage++;
    updateTable();
  }
}

function lastPage() {
  currentPage = Math.ceil(filteredTableData.length / rowsPerPage);
  updateTable();
}

function sortTable(column) {
  if (sortColumn === column) {
    sortDirection = sortDirection === 'asc' ? 'desc' : 'asc';
  } else {
    sortColumn = column;
    sortDirection = 'asc';
  }

  clearSortIndicators();
  const indicator = document.getElementById(`sort-${column}`);
  indicator.textContent = sortDirection === 'asc' ? '↑' : '↓';

  const keys = ['relativeTime', 'interSampleDelay', 'accel_x', 'accel_y', 'accel_z', 'yaw', 'pitch', 'roll', 'flags'];
  const key = keys[column];

  filteredTableData.sort((a, b) => {
    let aVal = a[key];
    let bVal = b[key];
    
    if (typeof aVal === 'string') {
      aVal = aVal.toLowerCase();
      bVal = bVal.toLowerCase();
    }
    
    if (sortDirection === 'asc') {
      return aVal > bVal ? 1 : aVal < bVal ? -1 : 0;
    } else {
      return aVal < bVal ? 1 : aVal > bVal ? -1 : 0;
    }
  });

  currentPage = 1;
  updateTable();
}

function clearSortIndicators() {
  for (let i = 0; i < 9; i++) {
    document.getElementById(`sort-${i}`).textContent = '';
  }
}

function searchTable() {
  const searchTerm = document.getElementById('search-box').value.toLowerCase();
  
  if (searchTerm === '') {
    filteredTableData = [...tableData];
  } else {
    filteredTableData = tableData.filter(row => {
      return Object.values(row).some(value => 
        value.toString().toLowerCase().includes(searchTerm)
      );
    });
  }
  
  currentPage = 1;
  updateTable();
}

// Clear table state - called from core module
function clearTableState() {
  tableData = [];
  filteredTableData = [];
  currentPage = 1;
  sortColumn = -1;
  sortDirection = 'asc';
}
