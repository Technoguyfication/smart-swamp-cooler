#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <ESPmDNS.h>
#include <cJSON.h>

#include "secrets.h"

#define FAN_LO_PIN D11
#define FAN_HI_PIN D12
#define PUMP_PIN D10

#define FAN_BTN_PIN D2
#define PUMP_BTN_PIN D3

#define LED_POLL_MILLIS 10
#define BUTTON_HOLD_MILLIS 100

WebServer webServer(80);
HTTPUpdateServer httpUpdater;

enum class FanState
{
    OFF = 0,
    LO  = 1,
    HI  = 2
};

enum class PumpState
{
    OFF = 0,
    ON  = 1
};

static char* fanStateNames[] = {
    "OFF",
    "LO",
    "HI"
};

static char* pumpStateNames[] = {
    "OFF",
    "ON"
};

// state variables
bool loFanState = false;
bool hiFanState = false;
bool pumpState = false;

// interrupt service routines
void ICACHE_RAM_ATTR loFanInterrupt()
{
    loFanState = true;
}

void ICACHE_RAM_ATTR hiFanInterrupt()
{
    hiFanState = true;
}

void ICACHE_RAM_ATTR pumpOnInterrupt()
{
    pumpState = true;
}

// utilities
void debug(String message);
void debugf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    debug(buffer);
}

// internal API
void setFanSpeed(FanState state);
void setPumpState(PumpState state);
void pressButton(int pin);
FanState pollFanState();
PumpState pollPumpState();

// external API
void handleRoot();
void handleCoolerGet();
void handlePatch();

const char* PROGMEM indexHtml = R"=====(
<html>
    <title>Cooler Control</title>
    <script>
    function updateCooler(settingType, settingValue) {
        fetch('/api/cooler', {
            method: 'PATCH',
            headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: new URLSearchParams({ [settingType]: settingValue })
        })
        .then(response => response.json())
        .then(data => updateUi(data, settingType));
    }

    function updateUi(data, settingType) {
        document.getElementById(settingType + '-state').textContent = data[settingType];
    }

    function setup() {
        // Add click event listener to each button
        const buttons = document.querySelectorAll('button');
        buttons.forEach(button => {
            button.addEventListener('click', function() {
            const settingType = this.dataset.type;
            const settingValue = this.dataset.value;
            updateCooler(settingType, settingValue);
            });
        });

        // Fetch initial state
        fetch('/api/cooler')
        .then(response => response.json())
        .then(data => {
            updateUi(data, 'fan');
            updateUi(data, 'pump');
        });
    }

    // Call setup on window load
    window.onload = setup;
    </script>
    <body>
    <div id="cooler-control">
    <h1>Cooler Control</h1>
    <div>
        <strong>Fan: <span id="fan-state">OFF</span></strong>
        <button data-type="fan" data-value="off">OFF</button>
        <button data-type="fan" data-value="lo">LOW</button>
        <button data-type="fan" data-value="hi">HIGH</button>
    </div>
    <div>
        <strong>Pump: <span id="pump-state">OFF</span></strong>
        <button data-type="pump" data-value="off">OFF</button>
        <button data-type="pump" data-value="on">ON</button>
    </div>
    <a href="/update">Update</a>
    </div>
    </body>
</html>
)=====";
