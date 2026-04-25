let chart;
let labels = [];
let dataCompleted = [];

function log(msg) {
  const box = document.getElementById("logBox");
  const time = new Date().toLocaleTimeString();
  box.innerHTML = `[${time}] ${msg}<br>` + box.innerHTML;
}

function submitTask() {
  fetch('/submit');
  log("Task submitted");
}

function cancelTasks() {
  fetch('/cancel');
  log("Cancelled pending tasks");
}

function shutdown() {
  fetch('/shutdown');
  log("ThreadPool shutdown");
}

function updateStats() {
  fetch('/stats')
    .then(res => res.text())
    .then(data => {
      let lines = data.split("\n");

      let active = parseInt(lines[0].split(": ")[1]);
      let queued = parseInt(lines[1].split(": ")[1]);
      let completed = parseInt(lines[2].split(": ")[1]);

      document.getElementById("active").innerText = active;
      document.getElementById("queued").innerText = queued;
      document.getElementById("completed").innerText = completed;

      // chart update
      labels.push(new Date().toLocaleTimeString());
      dataCompleted.push(completed);

      if (labels.length > 10) {
        labels.shift();
        dataCompleted.shift();
      }

      chart.update();
    });
}

function initChart() {
  const ctx = document.getElementById("chart").getContext("2d");

  chart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [{
        label: 'Completed Tasks',
        data: dataCompleted,
        borderWidth: 2
      }]
    },
    options: {
      responsive: true
    }
  });
}

initChart();
setInterval(updateStats, 1000);