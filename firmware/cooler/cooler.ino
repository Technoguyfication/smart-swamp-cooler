#include "cooler.h"

void setup()
{
    // set up pins
    pinMode(FAN_LO_PIN, INPUT);
    pinMode(FAN_HI_PIN, INPUT);
    pinMode(PUMP_PIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    attachInterrupt(FAN_LO_PIN, loFanInterrupt, RISING);
    attachInterrupt(FAN_HI_PIN, hiFanInterrupt, RISING);
    attachInterrupt(PUMP_PIN, pumpOnInterrupt, RISING);

    Serial.begin(115200);
    delay(1000); // wait for serial monitor to connect since the ESP32 disconnects on reset

    // connect to wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);

    debugf("Connecting to %s", wifi_ssid);

    // blink led while waiting for wifi
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    debugf("Connected to wifi! IP: %s", WiFi.localIP().toString());

    // set up mDNS
    if (!MDNS.begin(hostname))
    {
        debug("Error setting up mDNS");
    }
    else
    {
        debug("mDNS responder started");
    }

    // start web server
    httpUpdater.setup(&webServer);  // sets up at /update
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/api/cooler", HTTP_GET, handleCoolerGet);
    webServer.on("/api/cooler", HTTP_PATCH, handlePatch);
    webServer.begin();

    debugf("Server ready! Open http://%s.local/ in your browser", hostname);
}

void loop()
{
    webServer.handleClient();
}

void debug(String message)
{
    Serial.println(message);
    // future: write to a log file so we can pull logs over wifi
}

void handleRoot()
{
    debugf("GET / from %s", webServer.client().remoteIP().toString());

    webServer.send(200, "text/html", indexHtml);
}

void handleCoolerGet()
{
    debugf("GET /api/cooler from %s", webServer.client().remoteIP().toString());

    auto fanState = pollFanState();
    auto pumpState = pollPumpState();

    String response = "{";
    response += "\"fan\": \"" + String(fanStateNames[(int)fanState]) + "\",";
    response += "\"pump\": \"" + String(pumpStateNames[(int)pumpState]) + "\"";
    response += "}";

    webServer.send(200, "application/json", response);
}

void handlePatch()
{
    debugf("PATCH /api/cooler from %s", webServer.client().remoteIP().toString());

    auto fanState = webServer.arg("fan");
    auto pumpState = webServer.arg("pump");

    if (fanState == "lo")
    {
        setFanSpeed(FanState::LO);
    }
    else if (fanState == "hi")
    {
        setFanSpeed(FanState::HI);
    }
    else if (fanState == "off")
    {
        setFanSpeed(FanState::OFF);
    }

    if (pumpState == "on")
    {
        setPumpState(PumpState::ON);
    }
    else if (pumpState == "off")
    {
        setPumpState(PumpState::OFF);
    }

    handleCoolerGet();    // echo the new state
}

void setPumpState(PumpState state)
{
    auto currentState = pollPumpState();

    if (currentState == state)
    {
        debugf("Pump is already %s", String(pumpStateNames[(int)state]));
        return;
    }

    debug("Turning pump " + String(pumpStateNames[(int)state]));
    pressButton(PUMP_BTN_PIN);
}

void setFanSpeed(FanState state)
{
    // first row is current state, second row is desired state, value is number of presses required
    static PROGMEM int fanStateMatrix[3][3] = {
        { 0, 2, 1 },
        { 1, 0, 2 },
        { 2, 1, 0 }
    };
    
    auto currentState = pollFanState();
    int moves = (int)fanStateMatrix[(int)currentState][(int)state];

    debug("Request to change fan speed from " + String(fanStateNames[(int)currentState]) + " to " + String(fanStateNames[(int)state]) + " requires " + String(moves) + " moves.");

    for (int i = 0; i < moves; i++)
    {
        pressButton(FAN_BTN_PIN);
        delay(BUTTON_HOLD_MILLIS);
    }
}

void pressButton(int pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(BUTTON_HOLD_MILLIS);
    pinMode(pin, INPUT);
}

FanState pollFanState()
{
    // poll low fan speed first
    loFanState = false;
    delay(LED_POLL_MILLIS);
    if (loFanState)
    {
        return FanState::LO;
    }

    // poll high fan speed
    hiFanState = false;
    delay(LED_POLL_MILLIS);
    if (hiFanState)
    {
        return FanState::HI;
    }

    return FanState::OFF;
}

PumpState pollPumpState()
{
    pumpState = false;
    delay(LED_POLL_MILLIS);
    
    return pumpState ? PumpState::ON : PumpState::OFF;
}
