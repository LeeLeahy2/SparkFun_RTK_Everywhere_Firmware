/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  WiFi Status Values:
    WL_CONNECTED: assigned when connected to a WiFi network
    WL_CONNECTION_LOST: assigned when the connection is lost
    WL_CONNECT_FAILED: assigned when the connection fails for all the attempts
    WL_DISCONNECTED: assigned when disconnected from a network
    WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and
                    remains active until the number of attempts expires (resulting in
                    WL_CONNECT_FAILED) or a connection is established (resulting in
                    WL_CONNECTED)
    WL_NO_SHIELD: assigned when no WiFi shield is present
    WL_NO_SSID_AVAIL: assigned when no SSID are available
    WL_SCAN_COMPLETED: assigned when the scan networks is completed

  WiFi Station States:

                                  WIFI_STATE_OFF <-------------.
                                    |                          |
                       wifiStart()  |                          |
                                    |                          | WL_CONNECT_FAILED (Bad password)
                                    |                          | WL_NO_SSID_AVAIL (Out of range)
                                    v                   Fail   |
                                  WIFI_STATE_CONNECTING ------>+
                                    |    ^                     ^
                     wifiConnect()  |    |                     | wifiShutdown()
                                    |    | WL_CONNECTION_LOST  |
                                    |    | WL_DISCONNECTED     |
                                    v    |                     |
                                  WIFI_STATE_CONNECTED --------'
  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Globals
//----------------------------------------

int wifiConnectionAttempts; // Count the number of connection attempts between restarts

#ifdef COMPILE_WIFI

//----------------------------------------
// Constants
//----------------------------------------

//----------------------------------------
// Locals
//----------------------------------------

static uint32_t wifiLastConnectionAttempt;

// Throttle the time between connection attempts
// ms - Max of 4,294,967,295 or 4.3M seconds or 71,000 minutes or 1193 hours or 49 days between attempts
static uint32_t wifiConnectionAttemptsTotal; // Count the number of connection attempts absolutely
static uint32_t wifiConnectionAttemptTimeout;

// WiFi Timer usage:
//  * Measure interval to display IP address
static unsigned long wifiDisplayTimer;

// Last time the WiFi state was displayed
static uint32_t lastWifiState;

// DNS server for Captive Portal
static DNSServer dnsServer;

//----------------------------------------
// WiFi Routines
//----------------------------------------

// Set WiFi credentials
// Enable TCP connections
void menuWiFi()
{
    bool restartWiFi = false; // Restart WiFi if user changes anything

    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: WiFi Networks");

        for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
        {
            systemPrintf("%d) SSID %d: %s\r\n", (x * 2) + 1, x + 1, settings.wifiNetworks[x].ssid);
            systemPrintf("%d) Password %d: %s\r\n", (x * 2) + 2, x + 1, settings.wifiNetworks[x].password);
        }

        systemPrint("a) Configure device via WiFi Access Point or connect to WiFi: ");
        systemPrintf("%s\r\n", settings.wifiConfigOverAP ? "AP" : "WiFi");

        systemPrint("c) Captive Portal: ");
        systemPrintf("%s\r\n", settings.enableCaptivePortal ? "Enabled" : "Disabled");

        systemPrintln("x) Exit");

        byte incoming = getUserInputCharacterNumber();

        if (incoming >= 1 && incoming <= MAX_WIFI_NETWORKS * 2)
        {
            int arraySlot = ((incoming - 1) / 2); // Adjust incoming to array starting at 0

            if (incoming % 2 == 1)
            {
                systemPrintf("Enter SSID network %d: ", arraySlot + 1);
                getUserInputString(settings.wifiNetworks[arraySlot].ssid,
                                   sizeof(settings.wifiNetworks[arraySlot].ssid));
                restartWiFi = true; // If we are modifying the SSID table, force restart of WiFi
            }
            else
            {
                systemPrintf("Enter Password for %s: ", settings.wifiNetworks[arraySlot].ssid);
                getUserInputString(settings.wifiNetworks[arraySlot].password,
                                   sizeof(settings.wifiNetworks[arraySlot].password));
                restartWiFi = true; // If we are modifying the SSID table, force restart of WiFi
            }
        }
        else if (incoming == 'a')
        {
            settings.wifiConfigOverAP ^= 1;
            restartWiFi = true;
        }
        else if (incoming == 'c')
        {
            settings.enableCaptivePortal ^= 1;
        }
        else if (incoming == 'x')
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    // Erase passwords from empty SSID entries
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) == 0)
            strcpy(settings.wifiNetworks[x].password, "");
    }

    // Restart WiFi if anything changes
    if (restartWiFi == true)
    {
        // Restart the AP webserver if we are in that state
        if (systemState == STATE_WIFI_CONFIG)
            requestChangeState(STATE_WIFI_CONFIG_NOT_STARTED);
        else
        {
            // Restart WiFi if we are not in AP config mode
            NETWORK_DATA *network = networkGet(NETWORK_TYPE_WIFI, false);
            if (network)
            {
                if (settings.debugWifiState == true)
                    systemPrintln("Menu caused restarting of WiFi");
                networkRestartNetwork(network);
                networkStop(NETWORK_TYPE_WIFI);
            }
        }
    }

    clearBuffer(); // Empty buffer of any newline chars
}

// Display the WiFi IP address
void wifiDisplayIpAddress()
{
    systemPrintf("WiFi '%s' IP address: ", WiFi.SSID());
    systemPrint(WiFi.localIP());
    systemPrintf(" RSSI: %d\r\n", WiFi.RSSI());

    wifiDisplayTimer = millis();
}

// Get the WiFi adapter status
byte wifiGetStatus()
{
    return WiFi.status();
}

// Set AP mode
void wifiSetApMode()
{
    WiFi.mode(WIFI_AP);
}

// Update the state of the WiFi state machine
void wifiSetState(byte newState)
{
    if ((settings.debugWifiState || PERIODIC_DISPLAY(PD_WIFI_STATE)) && (wifiState == newState))
        systemPrint("*");
    wifiState = newState;

    if (settings.debugWifiState || PERIODIC_DISPLAY(PD_WIFI_STATE))
    {
        PERIODIC_CLEAR(PD_WIFI_STATE);
        switch (newState)
        {
        default:
            systemPrintf("Unknown WiFi state: %d\r\n", newState);
            break;
        case WIFI_STATE_OFF:
            systemPrintln("WIFI_STATE_OFF");
            break;
        case WIFI_STATE_START:
            systemPrintln("WIFI_STATE_START");
            break;
        case WIFI_STATE_CONNECTING:
            systemPrintln("WIFI_STATE_CONNECTING");
            break;
        case WIFI_STATE_CONNECTED:
            systemPrintln("WIFI_STATE_CONNECTED");
            break;
        }
    }
}

//----------------------------------------
// WiFi Config Support Routines - compiled out
//----------------------------------------

// Start the access point for user to connect to and configure device
// We can also start as a WiFi station and attempt to connect to local WiFi for config
bool wifiStartAP()
{
    return (wifiStartAP(false)); // Don't force AP mode
}

bool wifiStartAP(bool forceAP)
{
    if (settings.wifiConfigOverAP == true || forceAP)
    {
        // Stop any current WiFi activity
        WIFI_STOP();

        // Start in AP mode
        WiFi.mode(WIFI_AP);

        // Before starting AP mode, be sure we have default WiFi protocols enabled.
        // esp_wifi_set_protocol requires WiFi to be started
        esp_err_t response =
            esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
        if (response != ESP_OK)
            systemPrintf("wifiStartAP: Error setting WiFi protocols: %s\r\n", esp_err_to_name(response));
        else
        {
            if (settings.debugWifiState == true)
                systemPrintln("WiFi protocols set");
        }

        IPAddress local_IP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);

        WiFi.softAPConfig(local_IP, gateway, subnet);

        const char *softApSsid = "RTK Config";
        if (WiFi.softAP(softApSsid) == false) // Must be short enough to fit OLED Width
        {
            systemPrintln("WiFi AP failed to start");
            return (false);
        }
        systemPrintf("WiFi AP '%s' started with IP: ", softApSsid);
        systemPrintln(WiFi.softAPIP());

        // Start DNS Server
        if (dnsServer.start(53, "*", WiFi.softAPIP()) == false)
        {
            systemPrintln("WiFi DNS Server failed to start");
            return (false);
        }
        else
        {
            if (settings.debugWifiState == true)
                systemPrintln("DNS Server started");
        }
    }
    else
    {
        // Start webserver on local WiFi instead of AP

        // Attempt to connect to local WiFi with increasing timeouts
        int x = 0;
        const int maxTries = 2;
        for (; x < maxTries; x++)
        {
            if (wifiConnect(settings.wifiConnectTimeoutMs) == true) // Attempt to connect to any SSID on settings list
            {
                wifiPrintNetworkInfo();
                break;
            }
        }
        if (x == maxTries)
        {
            displayNoWiFi(2000);
            return (wifiStartAP(true)); // Because there is no local WiFi available, force AP mode so user can still get
                                        // access/configure it
        }
    }

    return (true);
}

//----------------------------------------
// WiFi Routines
//----------------------------------------

// Advance the WiFi state from off to connected
// Throttle connection attempts as needed
void wifiUpdate()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // if(settings.debugWifiState == true)
        //     systemPrintln("configureViaEthernet: skipping wifiUpdate");
        return;
    }

    // Periodically display the WiFi state
    if (settings.debugWifiState && ((millis() - lastWifiState) > 15000))
    {
        wifiSetState(wifiState);
        lastWifiState = millis();
    }

    DMW_st(wifiSetState, wifiState);
    switch (wifiState)
    {
    default:
        systemPrintf("Unknown wifiState: %d\r\n", wifiState);
        break;

    case WIFI_STATE_OFF:
        // Any service that needs WiFi will call wifiStart()
        break;

    case WIFI_STATE_CONNECTING:
        // Pause until connection timeout has passed
        if (millis() - wifiLastConnectionAttempt > wifiConnectionAttemptTimeout)
        {
            wifiLastConnectionAttempt = millis();

            if (wifiConnect(settings.wifiConnectTimeoutMs) == true) // Attempt to connect to any SSID on settings list
            {
                // Restart ESPNow if it was previously on
                if (espnowState > ESPNOW_OFF)
                    espnowStart();

                wifiSetState(WIFI_STATE_CONNECTED);
            }
            else
            {
                // We failed to connect
                if (wifiConnectLimitReached() == false) // Increases wifiConnectionAttemptTimeout
                {
                    if (wifiConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("Next WiFi attempt in %d seconds.\r\n", wifiConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("Next WiFi attempt in %d minutes.\r\n", wifiConnectionAttemptTimeout / 1000 / 60);
                }
                else
                {
                    systemPrintln("WiFi connection failed. Giving up.");
                    displayNoWiFi(2000);
                    WIFI_STOP(); // Move back to WIFI_STATE_OFF
                }
            }
        }

        break;

    case WIFI_STATE_CONNECTED:
        // Verify link is still up
        if (wifiIsConnected() == false)
        {
            systemPrintln("WiFi link lost");
            wifiConnectionAttempts = 0; // Reset the timeout
            wifiSetState(WIFI_STATE_CONNECTING);
        }

        // If WiFi is connected, and no services require WiFi, shut it off
        else if (wifiIsNeeded() == false)
            WIFI_STOP();

        break;
    }

    // Process DNS when we are in AP mode or AP+STA mode for captive portal
    if (((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() == WIFI_AP_STA)) && settings.enableCaptivePortal)
    {
        dnsServer.processNextRequest();
    }
}

// Starts the WiFi connection state machine (moves from WIFI_STATE_OFF to WIFI_STATE_CONNECTING)
// Sets the appropriate protocols (WiFi + ESP-Now)
// If radio is off entirely, start WiFi
// If ESP-Now is active, only add the LR protocol
void wifiStart()
{
    if (wifiNetworkCount() == 0)
    {
        systemPrintln("Error: Please enter at least one SSID before using WiFi");
        displayNoSSIDs(2000);
        WIFI_STOP();
        return;
    }

    if (wifiIsConnected() == true)
        return; // We don't need to do anything

    if (wifiState > WIFI_STATE_OFF)
        return; // We're in the midst of connecting

    if (settings.debugWifiState == true)
        systemPrintln("Starting WiFi");

    wifiSetState(WIFI_STATE_CONNECTING); // This starts the state machine running

    // Display the heap state
    reportHeapNow(settings.debugWifiState);
}

// Stop WiFi and release all resources
void wifiStop()
{
    // Stop the web server
    stopWebServer();

    // Stop the multicast domain name server
    networkStopMulticastDNS();

    // Stop the DNS server if we were using the captive portal
    if (((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() == WIFI_AP_STA)) && settings.enableCaptivePortal)
        dnsServer.stop();

    // Stop the other network clients and then WiFi
    NETWORK_STOP(NETWORK_TYPE_WIFI);
}

// Stop WiFi and release all resources
// If ESP NOW is active, leave WiFi on enough for ESP NOW
void wifiShutdown()
{
    wifiSetState(WIFI_STATE_OFF);

    wifiConnectionAttempts = 0; // Reset the timeout

    // If ESP-Now is active, change protocol to only Long Range
    if (espnowState > ESPNOW_OFF)
    {
        if (WiFi.getMode() != WIFI_STA)
            WiFi.mode(WIFI_STA);

        // Enable long range, PHY rate of ESP32 will be 512Kbps or 256Kbps
        // esp_wifi_set_protocol requires WiFi to be started
        esp_err_t response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
        if (response != ESP_OK)
            systemPrintf("wifiShutdown: Error setting ESP-Now lone protocol: %s\r\n", esp_err_to_name(response));
        else
        {
            if (settings.debugWifiState == true)
                systemPrintln("WiFi disabled, ESP-Now left in place");
        }
    }
    else
    {
        WiFi.mode(WIFI_OFF);
        if (settings.debugWifiState == true)
            systemPrintln("WiFi Stopped");
    }

    // Display the heap state
    reportHeapNow(settings.debugWifiState);
}

bool wifiIsConnected()
{
    return (wifiGetStatus() == WL_CONNECTED);
}

// Attempts a connection to all provided SSIDs
// Returns true if successful
// Gives up if no SSID detected or connection times out
// If useAPSTAMode is true, do an extra check and go from WIFI_AP mode to WIFI_AP_STA mode
bool wifiConnect(unsigned long timeout)
{
    return wifiConnect(timeout, false, nullptr);
}

bool wifiConnect(unsigned long timeout, bool useAPSTAMode, bool *wasInAPmode)
{
    // If WiFi is already connected and AP_STA mode is not needed, then return true now
    if (wifiIsConnected() && !useAPSTAMode)
    {
        return (true); // Nothing to do
    }

    displayWiFiConnect();

    // If otaUpdate or otaCheckVersion wants to use WIFI_AP_STA mode
    if (useAPSTAMode && (wasInAPmode != nullptr))
    {
        *wasInAPmode = (WiFi.getMode() == WIFI_AP);

        if (*wasInAPmode)
        {
            systemPrintln("wifiConnect: changing from WIFI_AP to WIFI_AP_STA");
            WiFi.mode(WIFI_AP_STA); // Change mode from WIFI_AP to WIFI_AP_STA
        }
        else
        {
            systemPrintln("wifiConnect: was not in WIFI_AP mode. Going to WIFI_STA");
            WiFi.mode(WIFI_STA); // Must have been off - or already in STA mode?
        }
    }
    else
    {
        // Before we can issue esp_wifi_() commands WiFi must be started
        if (WiFi.getMode() != WIFI_STA)
            WiFi.mode(WIFI_STA);
    }

    // Verify that the necessary protocols are set
    uint8_t protocols = 0;
    esp_err_t response = esp_wifi_get_protocol(WIFI_IF_STA, &protocols);
    if (response != ESP_OK)
        systemPrintf("wifiConnect: Failed to get protocols: %s\r\n", esp_err_to_name(response));

    // If ESP-NOW is running, blend in ESP-NOW protocol.
    if (espnowState > ESPNOW_OFF)
    {
        if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR))
        {
            esp_err_t response =
                esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N |
                                                       WIFI_PROTOCOL_LR); // Enable WiFi + ESP-Now.
            if (response != ESP_OK)
                systemPrintf("wifiConnect: Error setting WiFi + ESP-NOW protocols: %s\r\n", esp_err_to_name(response));
        }
    }
    else
    {
        // Make sure default WiFi protocols are in place
        if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N))
        {
            esp_err_t response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                                                                        WIFI_PROTOCOL_11N); // Enable WiFi.
            if (response != ESP_OK)
                systemPrintf("wifiConnect: Error setting WiFi protocols: %s\r\n", esp_err_to_name(response));
        }
    }

    int wifiResponse = WL_DISCONNECTED;

    systemPrint("Connecting WiFi... ");

    static WiFiMulti wifiMulti;

    // Load SSIDs
    wifiMulti.APlistClean();
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) > 0)
            wifiMulti.addAP((const char *)&settings.wifiNetworks[x].ssid, (const char *)&settings.wifiNetworks[x].password);
    }

    wifiResponse = wifiMulti.run(timeout);

    if (wifiResponse == WL_CONNECTED)
    {
        if (settings.enableTcpClient == true || settings.enableTcpServer == true || settings.enableUdpServer == true)
            networkStartMulticastDNS();

        systemPrintln();
        return true;
    }
    else if (wifiResponse == WL_DISCONNECTED)
        systemPrint("No friendly WiFi networks detected.\r\n");
    else
        systemPrintf("WiFi failed to connect: error #%d.\r\n", wifiResponse);

    return false;
}

// Based on the current settings and system states, determine if we need WiFi on or not
// This function does not start WiFi. Any service that needs it should call wifiStart().
// This function is used to turn WiFi off if nothing needs it.
//
// TODO: Check if this is still required or needs to be updated. As a minimum, it should be networkIsNeeded ?
bool wifiIsNeeded()
{
    if (settings.enablePointPerfectCorrections)
        return true;
    if (settings.enableTcpClient == true)
        return true;
    if (settings.enableTcpServer == true)
        return true;
    if (settings.enableUdpServer == true)
        return true;
    if (settings.enableAutoFirmwareUpdate)
        return true;

    // Handle WiFi within systemStates
    if (systemState <= STATE_ROVER_RTK_FIX && settings.enableNtripClient == true)
        return true;

    if (inBaseMode() == true && settings.enableNtripServer == true)
        return true;

    // If the user has enabled NTRIP Client for an Assisted Survey-In, and Survey-In is running, keep WiFi on.
    if (systemState >= STATE_BASE_NOT_STARTED && systemState <= STATE_BASE_TEMP_SURVEY_STARTED &&
        settings.enableNtripClient == true && settings.fixedBase == false)
        return true;

    // If WiFi is on while we are in the following states, allow WiFi to continue to operate
    if (systemState >= STATE_DISPLAY_SETUP && systemState <= STATE_PROFILE)
    {
        // Keep WiFi on if user presses setup button, enters bubble level, is in AP config mode, etc
        return true;
    }

    return false;
}

// Counts the number of entered SSIDs
int wifiNetworkCount()
{
    // Count SSIDs
    int networkCount = 0;
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) > 0)
            networkCount++;
    }
    return networkCount;
}

// Determine if another connection is possible or if the limit has been reached
bool wifiConnectLimitReached()
{
    // Retry the connection a few times
    bool limitReached = false;
    if (wifiConnectionAttempts++ >= wifiMaxConnectionAttempts)
        limitReached = true;

    wifiConnectionAttemptsTotal++;

    if (limitReached == false)
    {
        wifiConnectionAttemptTimeout =
            wifiConnectionAttempts * 15 * 1000L; // Wait 15, 30, 45, etc seconds between attempts

        reportHeapNow(settings.debugWifiState);
    }
    else
    {
        // No more connection attempts
        systemPrintln("WiFi connection attempts exceeded!");
    }
    return limitReached;
}

void wifiPrintNetworkInfo()
{
    systemPrintln("\nNetwork Configuration:");
    systemPrintln("----------------------");
    systemPrint("         SSID: ");
    systemPrintln(WiFi.SSID());
    systemPrint("  WiFi Status: ");
    systemPrintln(WiFi.status());
    systemPrint("WiFi Strength: ");
    systemPrint(WiFi.RSSI());
    systemPrintln(" dBm");
    systemPrint("          MAC: ");
    systemPrintln(WiFi.macAddress());
    systemPrint("           IP: ");
    systemPrintln(WiFi.localIP());
    systemPrint("       Subnet: ");
    systemPrintln(WiFi.subnetMask());
    systemPrint("      Gateway: ");
    systemPrintln(WiFi.gatewayIP());
    systemPrint("        DNS 1: ");
    systemPrintln(WiFi.dnsIP(0));
    systemPrint("        DNS 2: ");
    systemPrintln(WiFi.dnsIP(1));
    systemPrint("        DNS 3: ");
    systemPrintln(WiFi.dnsIP(2));
    systemPrintln();
}

// Return the gateway IP address for the WiFi controller
IPAddress wifiGetGatewayIpAddress()
{
    return WiFi.gatewayIP();
}

// Return the IP address for the WiFi controller
IPAddress wifiGetIpAddress()
{
    return WiFi.localIP();
}

// Return the subnet mask for the WiFi controller
IPAddress wifiGetSubnetMask()
{
    return WiFi.subnetMask();
}

// Return the WiFi signal signal strength
int wifiGetRssi()
{
    return WiFi.RSSI();
}

// Return the Wifi station ID
String wifiGetSsid()
{
    return WiFi.SSID();
}

#endif // COMPILE_WIFI
