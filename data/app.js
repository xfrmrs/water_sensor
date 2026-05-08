const RESTART_FIELD_LABELS = {
  trigPin: "Trigger pin",
  echoPin: "Echo pin",
  waterPin: "Water relay pin",
  errLedPin: "Error LED pin",
  serialBaud: "Serial baud",
  httpPort: "HTTP port",
  websocketPort: "WebSocket port"
};

const FIELD_SECTIONS = [
  {
    title: "Water Control",
    description: "Main loop timing and threshold values that drive filling decisions.",
    fields: [
      { key: "waterMaxDuration", label: "Water max duration (control cycles)", type: "number", min: 1, step: 1 },
      { key: "loopDelayMs", label: "Loop delay (ms)", type: "number", min: 1, step: 1 },
      { key: "waterDelayMs", label: "Water delay (ms)", type: "number", min: 1, step: 1 },
      { key: "waterHighUs", label: "High threshold (us)", type: "number", min: 1, step: 1 },
      { key: "waterLowUs", label: "Low threshold (us)", type: "number", min: 1, step: 1 },
      { key: "waterErrUs", label: "Error threshold (us)", type: "number", min: 1, step: 1 }
    ]
  },
  {
    title: "Sampling and Filtering",
    description: "Sensor sampling cadence, glitch rejection, and Kalman filter parameters.",
    fields: [
      { key: "pulseTimeoutUs", label: "Pulse timeout (us)", type: "number", min: 1, step: 1 },
      { key: "nPings", label: "Ping count", type: "number", min: 1, step: 1 },
      { key: "maxConfigurablePings", label: "Max configurable pings", type: "number", min: 1, step: 1 },
      { key: "minValidPings", label: "Minimum valid pings", type: "number", min: 1, step: 1 },
      { key: "pingGapMs", label: "Ping gap (ms)", type: "number", min: 1, step: 1 },
      { key: "minValidEchoUs", label: "Minimum valid echo (us)", type: "number", min: 1, step: 1 },
      { key: "shortJumpUs", label: "Short jump reject window (us)", type: "number", min: 1, step: 1 },
      { key: "shortConfirmDeltaUs", label: "Short confirm delta (us)", type: "number", min: 1, step: 1 },
      { key: "shortConfirmCount", label: "Short confirm count", type: "number", min: 1, step: 1 },
      { key: "maxHeldInvalidBursts", label: "Max held invalid bursts", type: "number", min: 1, step: 1 },
      { key: "historyCapacity", label: "History capacity", type: "number", min: 1, step: 1 },
      { key: "kalmanMeasurementError", label: "Kalman measurement error", type: "number", min: 0.0001, step: 0.0001 },
      { key: "kalmanEstimateError", label: "Kalman estimate error", type: "number", min: 0.0001, step: 0.0001 },
      { key: "kalmanProcessNoise", label: "Kalman process noise", type: "number", min: 0.00001, step: 0.00001 }
    ]
  },
  {
    title: "Hardware and Runtime",
    description: "Editable runtime toggles plus low-level settings that only apply after a restart.",
    fields: [
      { key: "debugControlLogs", label: "Enable control debug logs", type: "checkbox" },
      { key: "debugMeasurementLogs", label: "Enable measurement debug logs", type: "checkbox" },
      { key: "httpPort", label: "HTTP port", type: "number", min: 1, step: 1, restartField: true },
      { key: "websocketPort", label: "WebSocket port", type: "number", min: 1, step: 1, restartField: true },
      { key: "dnsPort", label: "DNS port", type: "number", min: 1, step: 1 },
      { key: "trigPin", label: "Trigger pin", type: "select", optionsKey: "safePins", restartField: true },
      { key: "echoPin", label: "Echo pin", type: "select", optionsKey: "safePins", restartField: true },
      { key: "waterPin", label: "Water relay pin", type: "select", optionsKey: "safePins", restartField: true },
      { key: "errLedPin", label: "Error LED pin", type: "select", optionsKey: "safePins", restartField: true },
      { key: "serialBaud", label: "Serial baud", type: "select", optionsKey: "supportedBauds", restartField: true }
    ]
  },
  {
    title: "Wi-Fi Station",
    description: "LAN join settings that are applied live after save.",
    fields: [
      { key: "wifiStaSsid", label: "Station SSID", type: "text", placeholder: "Your LAN SSID" },
      { key: "enableStationDhcp", label: "Enable station DHCP", type: "checkbox" },
      {
        key: "wifiStaPassword",
        label: "Station password",
        type: "password",
        clearKey: "clearStaPassword",
        storedFlag: "hasStaPassword"
      },
      { key: "wifiStaIp", label: "Enter Station IP", type: "text", placeholder: "10.0.0.53", dependsOn: { key: "enableStationDhcp", value: false } },
      { key: "wifiStaGateway", label: "Station gateway", type: "text", placeholder: "10.0.0.1", dependsOn: { key: "enableStationDhcp", value: false } },
      { key: "wifiStaSubnet", label: "Station subnet", type: "text", placeholder: "255.0.0.0", dependsOn: { key: "enableStationDhcp", value: false } },
      { key: "wifiStaConnectTimeoutMs", label: "Station connect timeout (ms)", type: "number", min: 1, step: 1 }
    ]
  },
  {
    title: "Setup Access Point",
    description: "Recovery AP settings used when station join is unavailable.",
    fields: [
      { key: "wifiApSsid", label: "Setup AP SSID", type: "text", placeholder: "WaterSensorSetup" },
      {
        key: "wifiApPassword",
        label: "Setup AP password",
        type: "password",
        clearKey: "clearApPassword",
        storedFlag: "hasApPassword"
      },
      { key: "enableApDhcp", label: "Enable AP DHCP", type: "checkbox" },
      { key: "wifiApIp", label: "Setup AP IP", type: "text", placeholder: "10.0.0.47" },
      { key: "wifiApGateway", label: "Setup AP gateway", type: "text", placeholder: "10.0.0.47" },
      { key: "wifiApSubnet", label: "Setup AP subnet", type: "text", placeholder: "255.0.0.0" },
      { key: "wifiApChannel", label: "Setup AP channel", type: "number", min: 1, step: 1 },
      { key: "wifiApHidden", label: "Hide setup AP SSID", type: "checkbox" },
      { key: "wifiApMaxConnections", label: "Setup AP max clients", type: "number", min: 1, step: 1 }
    ]
  },
  {
    title: "Security",
    description: "Manage administrative access to the device settings.",
    fields: [
      {
        key: "adminPassword",
        label: "Admin password",
        type: "password",
        clearKey: "clearAdminPassword",
        storedFlag: "hasAdminPassword"
      }
    ]
  }
];

const state = {
  config: null,
  status: null,
  history: [],
  options: null,
  socket: null,
  socketConnected: false,
  reconnectTimer: null,
  formRendered: false,
  manualBanner: null
};

const refs = {};
let saveButtons = [];

document.addEventListener("DOMContentLoaded", () => {
  cacheRefs();
  bindEvents();
  initialize();
});

function cacheRefs() {
  refs.connectionSummary = document.getElementById("connectionSummary");
  refs.socketBadge = document.getElementById("socketBadge");
  refs.statusBanner = document.getElementById("statusBanner");
  refs.runtimeSummary = document.getElementById("runtimeSummary");
  refs.configSectionTabs = document.getElementById("configSectionTabs");
  refs.formSections = document.getElementById("formSections");
  refs.configForm = document.getElementById("configForm");
  refs.saveButton = document.getElementById("saveButton");
  refs.restartButton = document.getElementById("restartButton");
  refs.emergencyStopButton = document.getElementById("emergencyStopButton");
  refs.restartNowButton = document.getElementById("restartNowButton");
  refs.restartLaterButton = document.getElementById("restartLaterButton");
  refs.restartModal = document.getElementById("restartModal");
  refs.restartModalBody = document.getElementById("restartModalBody");
  refs.showRaw = document.getElementById("showRaw");
  refs.showAccepted = document.getElementById("showAccepted");
  refs.showFiltered = document.getElementById("showFiltered");
  refs.historyCanvas = document.getElementById("historyCanvas");
  refs.rawValue = document.getElementById("rawValue");
  refs.rawSubvalue = document.getElementById("rawSubvalue");
  refs.acceptedValue = document.getElementById("acceptedValue");
  refs.acceptedSubvalue = document.getElementById("acceptedSubvalue");
  refs.filteredValue = document.getElementById("filteredValue");
  refs.filteredSubvalue = document.getElementById("filteredSubvalue");
  refs.stateValue = document.getElementById("stateValue");
  refs.stateSubvalue = document.getElementById("stateSubvalue");
  refs.pumpValue = document.getElementById("pumpValue");
  refs.pumpSubvalue = document.getElementById("pumpSubvalue");
  refs.networkValue = document.getElementById("networkValue");
  refs.networkSubvalue = document.getElementById("networkSubvalue");
  saveButtons = Array.from(document.querySelectorAll('button[type="submit"]'));
}

function bindEvents() {
  refs.configForm.addEventListener("submit", saveConfig);
  refs.restartButton.addEventListener("click", requestRestart);
  refs.emergencyStopButton.addEventListener("click", requestEmergencyStop);
  refs.restartNowButton.addEventListener("click", requestRestart);
  refs.restartLaterButton.addEventListener("click", closeRestartModal);
  document.addEventListener("click", handleTabClick);
  refs.showRaw.addEventListener("change", drawGraph);
  refs.showAccepted.addEventListener("change", drawGraph);
  refs.showFiltered.addEventListener("change", drawGraph);
  window.addEventListener("resize", () => window.requestAnimationFrame(drawGraph));
}

async function initialize() {
  await loadBootstrap();
  connectSocket();
}

async function loadBootstrap() {
  try {
    const response = await fetch("/api/bootstrap", { cache: "no-store" });
    const payload = await response.json();
    if (!response.ok) {
      throw new Error(payload.message || "Bootstrap request failed.");
    }

    state.config = payload.config;
    state.status = payload.status;
    state.history = Array.isArray(payload.history) ? payload.history.slice() : [];
    state.options = payload.options;
    state.manualBanner = null;

    renderForm();
    populateForm();
    renderStatus();
    drawGraph();
  } catch (error) {
    setManualBanner(error.message || "Failed to load device state.", "error");
    renderBanner();
  }
}

function connectSocket() {
  if (state.reconnectTimer) {
    window.clearTimeout(state.reconnectTimer);
    state.reconnectTimer = null;
  }

  if (state.socket) {
    state.socket.close();
  }

  const protocol = window.location.protocol === "https:" ? "wss" : "ws";
  const websocketPort = state.config?.websocketPort || 81;
  const socketUrl = `${protocol}://${window.location.hostname}:${websocketPort}/`;
  const socket = new WebSocket(socketUrl);
  state.socket = socket;

  socket.addEventListener("open", () => {
    if (state.socket !== socket) {
      return;
    }
    state.socketConnected = true;
    renderSocketBadge();
    renderBanner();
  });

  socket.addEventListener("message", (event) => {
    if (state.socket !== socket) {
      return;
    }
    try {
      const payload = JSON.parse(event.data);
      if (payload.type === "telemetry") {
        handleTelemetry(payload.data);
      } else if (payload.type === "status") {
        handleStatus(payload.data);
      }
    } catch (error) {
      console.error("WebSocket payload parse failed:", error);
    }
  });

  socket.addEventListener("close", () => {
    if (state.socket !== socket) {
      return;
    }
    state.socketConnected = false;
    renderSocketBadge();
    renderBanner();
    state.reconnectTimer = window.setTimeout(connectSocket, 1500);
  });

  socket.addEventListener("error", () => {
    socket.close();
  });
}

function handleTelemetry(snapshot) {
  if (!state.status) {
    return;
  }

  state.status.latestMeasurement = snapshot;
  pushHistorySample(snapshot);
  renderStatus();
  drawGraph();
}

function handleStatus(statusPayload) {
  state.status = statusPayload;
  renderStatus();
}

function pushHistorySample(sample) {
  state.history.push(sample);
  const capacity = state.config?.historyCapacity || 120;
  if (state.history.length > capacity) {
    state.history.splice(0, state.history.length - capacity);
  }
}

function renderForm() {
  if (state.formRendered || !state.options) {
    return;
  }

  const tabsFragment = document.createDocumentFragment();
  const fragment = document.createDocumentFragment();

  FIELD_SECTIONS.forEach((section, index) => {
    const sectionId = configSectionTabId(index);

    const tabButton = document.createElement("button");
    tabButton.className = index === 0 ? "tab-button is-active" : "tab-button";
    tabButton.type = "button";
    tabButton.dataset.tabGroup = "configSections";
    tabButton.dataset.tabTarget = sectionId;
    tabButton.textContent = section.title;
    tabsFragment.appendChild(tabButton);

    const wrapper = document.createElement("section");
    wrapper.className = index === 0 ? "form-section tab-panel is-active" : "form-section tab-panel";
    wrapper.dataset.tabGroup = "configSections";
    wrapper.dataset.tabId = sectionId;

    const head = document.createElement("div");
    head.className = "form-section__head";
    head.innerHTML = `<h3>${section.title}</h3><p class="helper-copy">${section.description}</p>`;
    wrapper.appendChild(head);

    const grid = document.createElement("div");
    grid.className = "form-grid";

    section.fields.forEach((field) => {
      grid.appendChild(renderField(field));
    });

    wrapper.appendChild(grid);
    fragment.appendChild(wrapper);
  });

  refs.configSectionTabs.innerHTML = "";
  refs.configSectionTabs.appendChild(tabsFragment);
  refs.formSections.innerHTML = "";
  refs.formSections.appendChild(fragment);
  state.formRendered = true;
}

function renderField(field) {
  const wrapper = document.createElement("div");
  wrapper.className = field.type === "checkbox" ? "field--checkbox" : field.type === "password" ? "field--password" : "field";
  wrapper.dataset.fieldKey = field.key;
  if (field.dependsOn) {
    wrapper.dataset.dependsOnKey = field.dependsOn.key;
    wrapper.dataset.dependsOnValue = String(field.dependsOn.value);
  }

  const inputId = fieldId(field.key);

  if (field.type === "checkbox") {
    wrapper.innerHTML = `
      <label for="${inputId}">
        <input id="${inputId}" name="${field.key}" type="checkbox">
        <span>${field.label}</span>
      </label>
    `;

    const checkbox = wrapper.querySelector("input");
    checkbox.addEventListener("change", updateDependentFields);

    return wrapper;
  }

  const label = document.createElement("label");
  label.setAttribute("for", inputId);
  label.textContent = field.label;
  wrapper.appendChild(label);

  let input;
  if (field.type === "select") {
    input = document.createElement("select");
    resolveSelectOptions(field).forEach((optionValue) => {
      const option = document.createElement("option");
      option.value = String(optionValue);
      option.textContent = String(optionValue);
      input.appendChild(option);
    });
  } else {
    input = document.createElement("input");
    input.type = field.type === "password" ? "password" : field.type;
    if (field.min !== undefined) input.min = String(field.min);
    if (field.step !== undefined) input.step = String(field.step);
    if (field.placeholder) input.placeholder = field.placeholder;
  }

  input.id = inputId;
  input.name = field.key;
  input.autocomplete = "off";
  wrapper.appendChild(input);

  if (field.type === "password") {
    const meta = document.createElement("div");
    meta.className = "password-meta";
    meta.innerHTML = `
      <span id="${storedPasswordLabelId(field.key)}">Stored: unknown</span>
      <label for="${clearFieldId(field.clearKey)}">
        <input id="${clearFieldId(field.clearKey)}" type="checkbox">
        <span>Clear stored password</span>
      </label>
    `;
    wrapper.appendChild(meta);

    const helper = document.createElement("p");
    helper.className = "helper-copy";
    helper.textContent = "Leave this blank to keep the current password. Enter a value to store it.";
    wrapper.appendChild(helper);

    input.addEventListener("input", () => {
      if (input.value.length > 0) {
        const clearToggle = document.getElementById(clearFieldId(field.clearKey));
        clearToggle.checked = false;
      }
    });
  }

  return wrapper;
}

function resolveSelectOptions(field) {
  const values = state.options?.[field.optionsKey] || [];
  return Array.isArray(values) ? values : [];
}

function populateForm() {
  if (!state.config) {
    return;
  }

  FIELD_SECTIONS.forEach((section) => {
    section.fields.forEach((field) => {
      const input = document.getElementById(fieldId(field.key));
      if (!input) {
        return;
      }

      if (field.type === "checkbox") {
        input.checked = Boolean(state.config[field.key]);
      } else if (field.type === "password") {
        input.value = "";
        const statusLabel = document.getElementById(storedPasswordLabelId(field.key));
        const clearToggle = document.getElementById(clearFieldId(field.clearKey));
        if (statusLabel) {
          statusLabel.textContent = state.config[field.storedFlag] ? "Stored: yes" : "Stored: no";
        }
        if (clearToggle) {
          clearToggle.checked = false;
        }
      } else {
        input.value = state.config[field.key];
      }
    });
  });

  updateDependentFields();
}

function updateDependentFields() {
  const controllers = {};
  FIELD_SECTIONS.forEach((section) => {
    section.fields.forEach((field) => {
      if (field.type === "checkbox") {
        const control = document.getElementById(fieldId(field.key));
        if (control) {
          controllers[field.key] = control.checked;
        }
      }
    });
  });

  document.querySelectorAll("[data-depends-on-key]").forEach((wrapper) => {
    const dependsKey = wrapper.dataset.dependsOnKey;
    const dependsValue = wrapper.dataset.dependsOnValue === "true";
    const currentValue = controllers[dependsKey];
    if (currentValue === undefined) {
      return;
    }
    wrapper.style.display = currentValue === dependsValue ? "" : "none";
  });
}

async function saveConfig(event) {
  event.preventDefault();

  try {
    setSaveButtonsDisabled(true);
    setManualBanner("Saving settings...", "info");
    renderBanner();

    const payload = collectConfigPayload();
    const response = await fetch("/api/config", {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify(payload)
    });
    const data = await response.json();

    if (!response.ok || !data.ok) {
      throw new Error(data.message || "Saving settings failed.");
    }

    state.config = data.config;
    state.status = data.status;
    state.manualBanner = null;
    populateForm();
    renderStatus();
    drawGraph();

    if (data.restartRequired) {
      openRestartModal(data.restartFields || []);
    } else {
      closeRestartModal();
    }
  } catch (error) {
    setManualBanner(error.message || "Saving settings failed.", "error");
    renderBanner();
  } finally {
    setSaveButtonsDisabled(false);
  }
}

function collectConfigPayload() {
  const payload = {};

  FIELD_SECTIONS.forEach((section) => {
    section.fields.forEach((field) => {
      const input = document.getElementById(fieldId(field.key));
      if (!input) {
        return;
      }

      if (field.type === "checkbox") {
        payload[field.key] = input.checked;
      } else if (field.type === "password") {
        const clearToggle = document.getElementById(clearFieldId(field.clearKey));
        const shouldClear = Boolean(clearToggle?.checked);
        payload[field.clearKey] = shouldClear;
        if (input.value.length > 0) {
          payload[field.key] = input.value;
        }
      } else if (field.type === "number" || field.type === "select") {
        const numericValue = Number(input.value);
        if (Number.isNaN(numericValue)) {
          throw new Error(`Invalid numeric value for ${field.label}.`);
        }
        payload[field.key] = numericValue;
      } else {
        payload[field.key] = input.value.trim();
      }
    });
  });

  return payload;
}

async function requestEmergencyStop() {
  const confirmed = window.confirm("Activate emergency shutoff and force the water relay off until restart?");
  if (!confirmed) {
    return;
  }

  try {
    refs.emergencyStopButton.disabled = true;
    setManualBanner("Activating emergency shutoff...", "error");
    renderBanner();

    const response = await fetch("/api/emergency-stop", {
      method: "POST"
    });
    const data = await response.json();
    if (!response.ok || !data.ok) {
      throw new Error(data.message || "Emergency shutoff request failed.");
    }

    state.status = data.status;
    state.manualBanner = null;
    renderStatus();
  } catch (error) {
    setManualBanner(error.message || "Emergency shutoff request failed.", "error");
    renderBanner();
  } finally {
    refs.emergencyStopButton.disabled = Boolean(state.status?.emergencyStopActive);
  }
}

async function requestRestart() {
  try {
    refs.restartButton.disabled = true;
    refs.restartNowButton.disabled = true;
    setManualBanner("Requesting restart...", "info");
    renderBanner();

    const response = await fetch("/api/restart", {
      method: "POST"
    });
    const data = await response.json();
    if (!response.ok || !data.ok) {
      throw new Error(data.message || "Restart request failed.");
    }

    state.status = data.status;
    state.manualBanner = null;
    closeRestartModal();
    renderStatus();
  } catch (error) {
    setManualBanner(error.message || "Restart request failed.", "error");
    renderBanner();
  } finally {
    refs.restartButton.disabled = false;
    refs.restartNowButton.disabled = false;
  }
}

function renderStatus() {
  renderSocketBadge();
  renderBanner();
  renderConnectionSummary();
  renderRuntimeSummary();
  renderMetrics();
  if (refs.emergencyStopButton) {
    refs.emergencyStopButton.disabled = Boolean(state.status?.emergencyStopActive);
    refs.emergencyStopButton.textContent = state.status?.emergencyStopActive ? "Emergency Shutoff Active" : "Emergency Shutoff";
  }
}

function renderSocketBadge() {
  refs.socketBadge.textContent = state.socketConnected ? "Socket live" : "Socket reconnecting";
  refs.socketBadge.classList.toggle("socket-badge--online", state.socketConnected);
  refs.socketBadge.classList.toggle("socket-badge--offline", !state.socketConnected);
}

function renderConnectionSummary() {
  if (!state.status) {
    refs.connectionSummary.textContent = "Loading device status...";
    return;
  }

  const name = state.status.networkName || "Unnamed network";
  const ip = state.status.networkIp || "IP unavailable";
  refs.connectionSummary.textContent = `${state.status.networkMode} on ${ip} (${name})`;
}

function renderRuntimeSummary() {
  if (!state.status?.activeRuntime) {
    refs.runtimeSummary.textContent = "No runtime information available.";
    return;
  }

  const active = state.status.activeRuntime;
  const restartText = state.status.restartRequired
    ? ` Pending restart: ${formatRestartFields(state.status.restartFields)}.`
    : "";
  refs.runtimeSummary.textContent =
    `Active GPIOs T${active.trigPin} / E${active.echoPin} / W${active.waterPin} / LED${active.errLedPin} | Serial ${active.serialBaud}.${restartText}`;
}

function renderMetrics() {
  const measurement = state.status?.latestMeasurement;
  if (!measurement) {
    return;
  }

  refs.rawValue.textContent = `${measurement.rawUs} us`;
  refs.rawSubvalue.textContent = `${measurement.rawCm} cm`;
  refs.acceptedValue.textContent = `${measurement.acceptedUs} us`;
  refs.acceptedSubvalue.textContent = `${measurement.acceptedCm} cm`;
  refs.filteredValue.textContent = `${measurement.filteredUs} us`;
  refs.filteredSubvalue.textContent = `${measurement.filteredCm} cm`;
  const emergencyActive = Boolean(state.status?.emergencyStopActive || measurement.emergencyStopActive);
  refs.stateValue.textContent = emergencyActive ? "EMERGENCY" : measurement.state;
  refs.stateSubvalue.textContent = emergencyActive ? "Shutoff latch active" : measurement.valid ? "Reading valid" : "Sensor invalid";
  refs.pumpValue.textContent = emergencyActive ? "OFF" : measurement.waterOutputOn ? "ON" : "OFF";
  refs.pumpSubvalue.textContent = emergencyActive
    ? "Emergency shutoff latch active"
    : measurement.filling ? "Filling cycle active" : "Pump idle";
  refs.networkValue.textContent = state.status.networkIp || "Offline";
  refs.networkSubvalue.textContent = `${state.status.networkMode} / ${state.status.networkName || "unknown"}`;
}

function renderBanner() {
  const banner = determineBanner();
  if (!banner) {
    refs.statusBanner.classList.add("hidden");
    refs.statusBanner.textContent = "";
    return;
  }

  refs.statusBanner.className = `banner banner--${banner.kind}`;
  refs.statusBanner.textContent = banner.message;
}

function handleTabClick(event) {
  const button = event.target.closest(".tab-button[data-tab-group]");
  if (!button) {
    return;
  }

  activateTab(button.dataset.tabGroup, button.dataset.tabTarget);
}

function activateTab(group, targetId) {
  const buttons = Array.from(document.querySelectorAll(`.tab-button[data-tab-group="${group}"]`));
  const panels = Array.from(document.querySelectorAll(`.tab-panel[data-tab-group="${group}"]`));

  buttons.forEach((button) => {
    button.classList.toggle("is-active", button.dataset.tabTarget === targetId);
  });

  panels.forEach((panel) => {
    panel.classList.toggle("is-active", panel.dataset.tabId === targetId);
  });

  if (isGraphVisible()) {
    window.requestAnimationFrame(drawGraph);
  }
}

function isGraphVisible() {
  const mainPanel = document.querySelector('.tab-panel[data-tab-group="main"][data-tab-id="liveTelemetry"]');
  const graphPanel = document.querySelector('.tab-panel[data-tab-group="telemetry"][data-tab-id="liveGraph"]');
  return Boolean(mainPanel?.classList.contains("is-active") && graphPanel?.classList.contains("is-active"));
}

function determineBanner() {
  if (state.manualBanner) {
    return state.manualBanner;
  }

  if (!state.status) {
    return null;
  }

  const parts = [];
  let kind = "info";

  if (state.status.message) {
    parts.push(state.status.message);
  }

  if (state.status.emergencyStopActive) {
    kind = "error";
    parts.push("Emergency shutoff is active. Water output remains off until restart.");
  }

  if (state.status.restartRequired) {
    kind = kind === "error" ? "error" : "warn";
    parts.push(`Restart pending for ${formatRestartFields(state.status.restartFields)}.`);
  }

  if (!state.socketConnected) {
    kind = kind === "error" ? "error" : "warn";
    parts.push("Live socket is reconnecting.");
  }

  if (state.status.reconnectHint) {
    parts.push(state.status.reconnectHint);
  }

  if (parts.length === 0) {
    return null;
  }

  return {
    message: parts.join(" "),
    kind
  };
}

function drawGraph() {
  const canvas = refs.historyCanvas;
  const ctx = canvas.getContext("2d");
  const bounds = canvas.getBoundingClientRect();
  const pixelRatio = window.devicePixelRatio || 1;

  canvas.width = Math.max(1, Math.floor(bounds.width * pixelRatio));
  canvas.height = Math.max(1, Math.floor(bounds.height * pixelRatio));
  ctx.setTransform(pixelRatio, 0, 0, pixelRatio, 0, 0);

  const width = bounds.width;
  const height = bounds.height;
  ctx.clearRect(0, 0, width, height);

  const history = state.history || [];
  if (!history.length || !state.config) {
    drawEmptyGraph(ctx, width, height, "No telemetry yet.");
    return;
  }

  const padding = { top: 20, right: 18, bottom: 28, left: 44 };
  const plotWidth = width - padding.left - padding.right;
  const plotHeight = height - padding.top - padding.bottom;

  const visibleKeys = [];
  if (refs.showFiltered.checked) visibleKeys.push("filteredUs");
  if (refs.showAccepted.checked) visibleKeys.push("acceptedUs");
  if (refs.showRaw.checked) visibleKeys.push("rawUs");
  if (!visibleKeys.length) {
    drawEmptyGraph(ctx, width, height, "Enable at least one telemetry line.");
    return;
  }

  const allValues = history.flatMap((sample) => visibleKeys.map((key) => Number(sample[key] || 0)));
  allValues.push(state.config.waterHighUs, state.config.waterLowUs, state.config.waterErrUs);
  const yMax = Math.max(...allValues, 1) * 1.08;

  drawGrid(ctx, padding, plotWidth, plotHeight, width, yMax);
  drawThreshold(ctx, padding, plotWidth, plotHeight, yMax, state.config.waterHighUs, "#f7a868", "High");
  drawThreshold(ctx, padding, plotWidth, plotHeight, yMax, state.config.waterLowUs, "#38d5a0", "Low");
  drawThreshold(ctx, padding, plotWidth, plotHeight, yMax, state.config.waterErrUs, "#ff7a67", "Err");

  if (refs.showFiltered.checked) {
    drawSeries(ctx, history, "filteredUs", "#38d5a0", padding, plotWidth, plotHeight, yMax, 2.8);
  }
  if (refs.showAccepted.checked) {
    drawSeries(ctx, history, "acceptedUs", "#f8f0b4", padding, plotWidth, plotHeight, yMax, 2.0);
  }
  if (refs.showRaw.checked) {
    drawSeries(ctx, history, "rawUs", "#8fb8ff", padding, plotWidth, plotHeight, yMax, 1.6);
  }
}

function drawEmptyGraph(ctx, width, height, message) {
  ctx.fillStyle = "rgba(255, 255, 255, 0.72)";
  ctx.font = "16px Trebuchet MS";
  ctx.textAlign = "center";
  ctx.fillText(message, width / 2, height / 2);
}

function drawGrid(ctx, padding, plotWidth, plotHeight, width, yMax) {
  ctx.strokeStyle = "rgba(255,255,255,0.08)";
  ctx.lineWidth = 1;

  for (let i = 0; i <= 4; i += 1) {
    const y = padding.top + (plotHeight / 4) * i;
    ctx.beginPath();
    ctx.moveTo(padding.left, y);
    ctx.lineTo(width - padding.right, y);
    ctx.stroke();

    const value = Math.round(yMax - (yMax / 4) * i);
    ctx.fillStyle = "rgba(255,255,255,0.55)";
    ctx.font = "12px Trebuchet MS";
    ctx.textAlign = "left";
    ctx.fillText(String(value), 8, y + 4);
  }
}

function drawThreshold(ctx, padding, plotWidth, plotHeight, yMax, value, color, label) {
  const y = padding.top + plotHeight - (value / yMax) * plotHeight;
  ctx.save();
  ctx.strokeStyle = color;
  ctx.setLineDash([6, 6]);
  ctx.beginPath();
  ctx.moveTo(padding.left, y);
  ctx.lineTo(padding.left + plotWidth, y);
  ctx.stroke();
  ctx.setLineDash([]);
  ctx.fillStyle = color;
  ctx.font = "12px Trebuchet MS";
  ctx.textAlign = "right";
  ctx.fillText(label, padding.left + plotWidth - 4, y - 6);
  ctx.restore();
}

function drawSeries(ctx, history, key, color, padding, plotWidth, plotHeight, yMax, lineWidth) {
  if (history.length === 0) {
    return;
  }

  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = lineWidth;
  ctx.beginPath();

  history.forEach((sample, index) => {
    const x = padding.left + (history.length === 1 ? plotWidth / 2 : (plotWidth * index) / (history.length - 1));
    const y = padding.top + plotHeight - ((Number(sample[key]) || 0) / yMax) * plotHeight;
    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });

  ctx.stroke();
  ctx.restore();
}

function openRestartModal(fields) {
  const friendly = formatRestartFields(fields);
  refs.restartModalBody.textContent =
    `The device saved these low-level settings: ${friendly}. They are used by the firmware on the next restart.`;
  refs.restartModal.classList.remove("hidden");
  refs.restartModal.setAttribute("aria-hidden", "false");
}

function closeRestartModal() {
  refs.restartModal.classList.add("hidden");
  refs.restartModal.setAttribute("aria-hidden", "true");
}

function formatRestartFields(fields) {
  if (!Array.isArray(fields) || !fields.length) {
    return "trigger pin, echo pin, water relay pin, error LED pin, or serial baud";
  }

  return fields.map((field) => RESTART_FIELD_LABELS[field] || field).join(", ");
}

function setManualBanner(message, kind) {
  state.manualBanner = { message, kind };
}

function setSaveButtonsDisabled(disabled) {
  saveButtons.forEach((button) => {
    button.disabled = disabled;
  });
}

function configSectionTabId(index) {
  return `configSection${index}`;
}

function fieldId(key) {
  return `field-${key}`;
}

function clearFieldId(key) {
  return `clear-${key}`;
}

function storedPasswordLabelId(key) {
  return `stored-${key}`;
}
