#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>

// Configuration
#define WIFI_SSID "fnet_N"
#define WIFI_PASS "29280117"
#define PZEM_RX D6  // With voltage divider
#define PZEM_TX D7

SoftwareSerial pzemSWSerial(PZEM_RX, PZEM_TX);
PZEM004Tv30 pzem(pzemSWSerial);
ESP8266WebServer server(80);

// PROGMEM HTML
const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Energy Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@3.7.0/dist/chart.min.js"></script>
<style>
:root{--card-bg:#f8f9fa;--text-dark:#2c3e50;}
body{font-family:sans-serif;margin:0;padding:20px;background:#f0f2f5;}
.grid{display:grid;gap:10px;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));}
.card{background:var(--card-bg);border-radius:8px;padding:15px;box-shadow:0 2px 4px rgba(0,0,0,0.05);transition:all 0.2s;}
.chart-container{width:100%;height:300px;margin:20px 0;background:white;border-radius:8px;will-change:transform;}
.value{font-size:24px;font-weight:600;color:var(--text-dark);margin:5px 0;}
.unit{color:#7f8c8d;font-size:14px;}
#status{color:#666;font-size:14px;text-align:center;}
</style>
</head>
<body>
<div class="container">
 <h1 style="text-align:center;margin-bottom:25px;">Energy Monitor</h1>
 
 <div class="grid">
  <div class="card"><h3>Voltage</h3><div class="value" id="voltage">--</div><span class="unit">V</span></div>
  <div class="card"><h3>Current</h3><div class="value" id="current">--</div><span class="unit">A</span></div>
  <div class="card"><h3>Power</h3><div class="value" id="power">--</div><span class="unit">W</span></div>
  <div class="card"><h3>Energy</h3><div class="value" id="energy">--</div><span class="unit">kWh</span></div>
  <div class="card"><h3>Power Factor</h3><div class="value" id="pf">--</div></div>
  <div class="card"><h3>Frequency</h3><div class="value" id="frequency">--</div><span class="unit">Hz</span></div>
 </div>

 <select id="paramSelect" style="width:100%;padding:8px;margin:10px 0;">
  <option value="voltage">Voltage</option>
  <option value="current">Current</option>
  <option value="power">Power</option>
  <option value="energy">Energy</option>
 </select>

 <div class="chart-container">
  <canvas id="chart"></canvas>
 </div>
 <div id="status">Initializing...</div>
</div>

<script>
let chart, maxPoints=15, labels=[], data=[], currentParam='voltage';
const colors = {
  voltage: 'rgb(231, 76, 60)',
  current: 'rgb(52, 152, 219)',
  power: 'rgb(46, 204, 113)',
  energy: 'rgb(241, 196, 15)'
};

function initChart() {
  chart = new Chart(document.getElementById('chart'), {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        data: data,
        borderColor: colors.voltage,
        borderWidth: 2,
        tension: 0.3,
        pointRadius: 3,
        cubicInterpolationMode: 'monotone',
        fill: false
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: {
        duration: 400,
        easing: 'linear',
        lazy: true
      },
      plugins: {legend: {display: false}},
      scales: {
        x: {display: false},
        y: {
          beginAtZero: false,
          grace: '15%',
          grid: {display: false}
        }
      }
    }
  });
}

async function updateData() {
  try {
    const res = await fetch('/data');
    const json = await res.json();
    
    // Update all cards
    ['v','c','p','e','pf','f'].forEach((k,i) => {
      const val = json[k];
      const elem = document.getElementById(['voltage','current','power','energy','pf','frequency'][i]);
      elem.textContent = val?.toFixed([1,3,1,3,2,1][i]) || '--';
    });

    // Update chart data
    const param = document.getElementById('paramSelect').value;
    const value = json[param[0]];
    
    if(typeof value === 'number') {
      const now = new Date();
      labels.push(`${now.getMinutes()}:${now.getSeconds().toString().padStart(2,'0')}`);
      data.push(value);
      
      // Maintain buffer size
      if(labels.length > maxPoints) {
        labels = labels.slice(-maxPoints);
        data = data.slice(-maxPoints);
      }
      
      // Update chart
      chart.data.labels = labels;
      chart.data.datasets[0].data = data;
      chart.data.datasets[0].borderColor = colors[param];
      chart.update({duration: 300});
      document.getElementById('status').textContent = 
        `Showing ${data.length} ${param} readings | Updated: ${labels.slice(-1)[0]}`;
    }
    
  } catch(err) {
    document.getElementById('status').textContent = 'Update failed: ' + err.message;
  }
}

window.addEventListener('load', () => {
  initChart();
  setInterval(updateData, 2000);
  updateData();
  document.getElementById('paramSelect').addEventListener('change', updateData);
});
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(9600);
  pzemSWSerial.begin(9600);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) delay(500);
  
  MDNS.begin("pzem");
  
  server.on("/", []() {
    server.send_P(200, "text/html", HTML);
  });

  server.on("/data", []() {
    String json = "{";
    json += "\"v\":" + String(pzem.voltage(),1) + ",";
    json += "\"c\":" + String(pzem.current(),3) + ",";
    json += "\"p\":" + String(pzem.power(),1) + ",";
    json += "\"e\":" + String(pzem.energy(),3) + ",";
    json += "\"pf\":" + String(pzem.pf(),2) + ",";
    json += "\"f\":" + String(pzem.frequency(),1) + "}";
    
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
}