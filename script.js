function submitTask() {
    fetch('/submit');
}

function cancelTasks() {
    fetch('/cancel');
}

function updateStats() {
    fetch('/stats')
    .then(res => res.text())
    .then(data => {
        let lines = data.split("\n");
        document.getElementById("active").innerText = lines[0];
        document.getElementById("queued").innerText = lines[1];
        document.getElementById("completed").innerText = lines[2];
    });
}

setInterval(updateStats, 1000);
updateStats();