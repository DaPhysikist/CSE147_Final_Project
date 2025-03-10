document.addEventListener("DOMContentLoaded", function () {
  const fetchButton = document.getElementById("fetch-button");
  const dateSelect = document.getElementById("date-select");
  const applianceSelect = document.getElementById("appliance-select");

  let totalEnergyChart = null;
  let energyPresenceChart = null;
  let presencePercentageChart = null;

  async function loadApplianceNames() {
    try {
      const response = await fetch("/appliance_names");
      const data = await response.json();

      applianceSelect.innerHTML = "";
      const defaultOption = document.createElement("option");
      defaultOption.value = "";
      defaultOption.textContent = "Select Appliance";
      applianceSelect.appendChild(defaultOption);

      data.appliance_names.forEach(name => {
        const option = document.createElement("option");
        option.value = name;
        option.textContent = name;
        applianceSelect.appendChild(option);
      });
    } catch (error) {
      console.error("Error fetching appliance names:", error);
    }
  }

  async function loadDatesForAppliance(applianceName) {
    try {
      const response = await fetch(`/available_dates/${applianceName}`);
      const data = await response.json();

      dateSelect.innerHTML = "";
      const defaultOption = document.createElement("option");
      defaultOption.value = "";
      defaultOption.textContent = "Select Date";
      dateSelect.appendChild(defaultOption);

      data.dates.forEach(date => {
        const option = document.createElement("option");
        option.value = date;
        option.textContent = date;
        dateSelect.appendChild(option);
      });
    } catch (error) {
      console.error("Error fetching dates:", error);
    }
  }

  async function fetchData() {
    const selectedDate = dateSelect.value;
    const applianceName = applianceSelect.value;

    if (!applianceName || !selectedDate) {
      alert("Please select an appliance and a date.");
      return;
    }

    try {
      const response = await fetch(`/shuteye_periodic_measurement_data/${applianceName}`);
      const data = await response.json();

      const filteredData = Object.values(data).filter(entry => entry.local_time.includes(selectedDate));

      const hours = Array.from({ length: 24 }, (_, i) => `${String(i).padStart(2, "0")}:00`);

      console.log("Hours Array:", hours);
      console.log("Filtered Data:", filteredData);
      filteredData.forEach(entry => console.log(entry.local_time));

      const totalPowerData = [];
      const userPresenceEnergyData = [];
      const noUserPresenceEnergyData = [];
      const userPresencePercentageData = [];

      hours.forEach(hour => {
        const hourData = filteredData.filter(data => data.local_time.includes(`${selectedDate} ${hour}`));

        let totalWattHours = 0;
        let userPresenceWattHours = 0;
        let noUserPresenceWattHours = 0;
        let totalPresenceTime = 0;
        let totalTimeInHour = 0;

        if (hourData.length > 0) { 
          for (let i = 1; i < hourData.length; i++) {
            const previousMeasurement = hourData[i - 1];
            const currentMeasurement = hourData[i];

            const previousTime = new Date(previousMeasurement.local_time);
            const currentTime = new Date(currentMeasurement.local_time);
            const timeDifferenceInSeconds = (currentTime - previousTime) / 1000;

            const wattSeconds = timeDifferenceInSeconds * previousMeasurement.current_power;
            const wattHours = wattSeconds / 3600;

            totalWattHours += wattHours;
            totalTimeInHour += timeDifferenceInSeconds;

            if (previousMeasurement.user_presence_detected) {
              userPresenceWattHours += wattHours;
              totalPresenceTime += timeDifferenceInSeconds;
            } else {
              noUserPresenceWattHours += wattHours;
            }
          }
        }

        totalPowerData.push(totalWattHours);
        userPresenceEnergyData.push(userPresenceWattHours);
        noUserPresenceEnergyData.push(noUserPresenceWattHours);
        userPresencePercentageData.push(totalTimeInHour > 0 ? (totalPresenceTime / totalTimeInHour) * 100 : 0);
      });

      updateTotalEnergyChart(hours, totalPowerData);
      updateEnergyPresenceChart(hours, userPresenceEnergyData, noUserPresenceEnergyData);
      updatePresencePercentageChart(hours, userPresencePercentageData);

    } catch (error) {
      console.error("Error fetching data:", error);
    }
  }

  function updateTotalEnergyChart(labels, totalData) {
    if (totalEnergyChart) {
      totalEnergyChart.destroy();
    }

    const ctx = document.getElementById("totalEnergyChart").getContext("2d");
    totalEnergyChart = new Chart(ctx, {
      type: "bar",
      data: {
        labels: labels,
        datasets: [{
          label: "Total Energy Consumption (Wh)",
          data: totalData,
          backgroundColor: "rgba(75, 192, 192, 0.6)",
          borderColor: "rgba(75, 192, 192, 1)",
          borderWidth: 1
        }]
      },
      options: {
        scales: {
          y: { beginAtZero: true }
        }
      }
    });
  }

  function updateEnergyPresenceChart(labels, userPresenceData, noUserPresenceData) {
    if (energyPresenceChart) {
      energyPresenceChart.destroy();
    }

    const ctx = document.getElementById("energyPresenceChart").getContext("2d");
    energyPresenceChart = new Chart(ctx, {
      type: "bar",
      data: {
        labels: labels,
        datasets: [{
          label: "With User Presence",
          data: userPresenceData,
          backgroundColor: "rgba(153, 102, 255, 0.6)",
          borderColor: "rgba(153, 102, 255, 1)",
          borderWidth: 1
        }, {
          label: "Without User Presence",
          data: noUserPresenceData,
          backgroundColor: "rgba(255, 159, 64, 0.6)",
          borderColor: "rgba(255, 159, 64, 1)",
          borderWidth: 1
        }]
      },
      options: {
        scales: {
          y: { beginAtZero: true, stacked: true },
          x: { stacked: true }
        }
      }
    });
  }

  function updatePresencePercentageChart(labels, presencePercentageData) {
    if (presencePercentageChart) {
      presencePercentageChart.destroy();
    }

    const ctx = document.getElementById("presencePercentageChart").getContext("2d");
    presencePercentageChart = new Chart(ctx, {
      type: "bar",
      data: {
        labels: labels,
        datasets: [{
          label: "User Presence (%)",
          data: presencePercentageData,
          backgroundColor: "rgba(54, 162, 235, 0.6)",
          borderColor: "rgba(54, 162, 235, 1)",
          borderWidth: 1
        }]
      },
      options: {
        scales: {
          y: { beginAtZero: true, max: 100 }
        }
      }
    });
  }

  applianceSelect.addEventListener("change", function () {
    const applianceName = applianceSelect.value;
    if (applianceName) {
      loadDatesForAppliance(applianceName);
    } else {
      dateSelect.innerHTML = "<option>Select Date</option>";
    }
  });

  fetchButton.addEventListener("click", fetchData);
  loadApplianceNames();
});
