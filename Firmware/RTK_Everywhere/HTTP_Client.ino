/*------------------------------------------------------------------------------
HTTP_Client.ino

  The HTTP client sits on top of the network layer and handles Zero Touch
  Provisioning (ZTP) via a HTTP POST.

  Previously this functionality was in pointperfectProvisionDevice and
  pointperfectTryZtpToken but was problematic as it used a separate instance
  of NetworkClientSecure and had no control over the underlying network.

------------------------------------------------------------------------------*/

#ifdef COMPILE_HTTP_CLIENT

//----------------------------------------
// Constants
//----------------------------------------

// Give up connecting after this number of attempts
// Note: pointperfectProvisionDevice implements its own timeout.
// MAX_HTTP_CLIENT_CONNECTION_ATTEMPTS * httpClientConnectionAttemptTimeout should
// be less than the pointperfectProvisionDevice timeout.
static const int MAX_HTTP_CLIENT_CONNECTION_ATTEMPTS = 3;

// Available in menuPP.ino
extern const uint8_t ppLbandToken[16];
extern const uint8_t ppIpToken[16];
extern const uint8_t ppLbandIpToken[16];
extern const uint8_t ppRtcmToken[16];

// Define the HTTP client states
enum HTTPClientState
{
    HTTP_CLIENT_OFF = 0,
    HTTP_CLIENT_ON,                  // WIFI_STATE_START state
    HTTP_CLIENT_NETWORK_STARTED,     // Connecting to WiFi access point or Ethernet
    HTTP_CLIENT_CONNECTING_2_SERVER, // Connecting to the HTTP server
    HTTP_CLIENT_CONNECTED,           // Connected to the HTTP services
    HTTP_CLIENT_COMPLETE,            // Complete. Can not or do not need to continue
    // Insert new states here
    HTTP_CLIENT_STATE_MAX // Last entry in the state list
};

const char *const httpClientStateName[] = {
    "HTTP_CLIENT_OFF",       "HTTP_CLIENT_ON",       "HTTP_CLIENT_NETWORK_STARTED", "HTTP_CLIENT_CONNECTING_2_SERVER",
    "HTTP_CLIENT_CONNECTED", "HTTP_CLIENT_COMPLETE",
};

const int httpClientStateNameEntries = sizeof(httpClientStateName) / sizeof(httpClientStateName[0]);

// PointPerfect offers are variety of services
// Here we capture the various service levels and types

enum PP_ModelType
{
    PP_MODEL_SSR = 0, // State Space Representation style model (mathematical model covering large geographic area)
    PP_MODEL_OSR,     // Observation Space Representation (lots of static reference stations)
    PP_MODEL_NONE,
};

enum PP_DeliveryMethod
{
    PP_DELIVERY_NTRIP = 0,    // Delivery over an internet connection (essentially TCP)
    PP_DELIVERY_MQTT,         // Delivery over an internet connection using MQTT (deprecated)
    PP_DELIVERY_LBAND_NA,     // Delivery over L-Band signal, North America coverage
    PP_DELIVERY_LBAND_GLOBAL, // Delivery over L-Band signal, global coverage
    PP_DELIVERY_NONE,
};

enum PP_Encoding
{
    PP_ENCODING_SPARTN = 0, // Low bit rate, u-blox proprietary
    PP_ENCODING_RTCM,       // Classic RTCM encoding that nearly all RTK receivers can understand
    PP_ENCODING_NONE,
};

// PointPerfect offers are variety of services
// Each service will have a printable name
typedef struct
{
    const char serviceName[23];
    PP_ModelType modelType;
    PP_DeliveryMethod deliveryMethod;
    PP_Encoding encoding;
} PP_Service;

// Static array containing all the compatible messages
// Rate = Output once every N position fix(es).
const PP_Service ppServices[] = {
    {"Disabled", PP_MODEL_NONE, PP_DELIVERY_NONE, PP_ENCODING_NONE},            // Do not use PointPerfect corrections
    {"Flex RTCM", PP_MODEL_SSR, PP_DELIVERY_NTRIP, PP_ENCODING_RTCM},           // Uses "ZTP-RTCM-100" profile
    {"Flex L-Band NA", PP_MODEL_SSR, PP_DELIVERY_LBAND_NA, PP_ENCODING_SPARTN}, // Uses "ZTP-LBand" profile
    {"Global", PP_MODEL_SSR, PP_DELIVERY_LBAND_GLOBAL, PP_ENCODING_SPARTN},     // Uses "ZTP-Global" profile
    {"Live", PP_SERVICE_OSR, PP_DELIVERY_NTRIP, PP_ENCODING_RTCM},              // Uses "ZTP-Live" profile
    {"Flex MQTT (Deprecated)", PP_MODEL_SSR, PP_DELIVERY_MQTT, PP_ENCODING_SPARTN}, // Deprecated
    // "ZTP-IP" profile deprecated - This was used for
    // "ZTP-RTCM-100-Trial" profile deprecated
    // "ZTP-LBand+IP" profile deprecated
};

enum PP_NickName
{
    PP_NICKNAME_DISABLED = 0,
    PP_NICKNAME_FLEX_RTCM,
    PP_NICKNAME_FLEX_LBAND_NA,
    PP_NICKNAME_GLOBAL,
    PP_NICKNAME_LIVE,
    PP_NICKNAME_IP_MQTT,
};

const int ppServiceCount = sizeof(ppServices) / sizeof(ppServices[0]);

int ztpPlatformMaxProfiles = 0; // Depending on the platform there are a different number of ZTP profiles to test
ZtpResponse ztpResponse =
    ZTP_NOT_STARTED; // Used in menuPP. This is the overall result of the ZTP process of testing multiple tokens
static ZtpResponse ztpInterimResponse[4] = {
    ZTP_NOT_STARTED}; // Individual responses to token attempts. Local only. Size is manual count of max tokens.

//----------------------------------------
// Locals
//----------------------------------------

static HTTPClient *httpClient = nullptr;

JsonDocument *jsonZtp = nullptr;
char *tempHolderPtr = nullptr;

// Throttle the time between connection attempts
static int httpClientConnectionAttempts; // Count the number of connection attempts between restarts
static uint32_t httpClientConnectionAttemptTimeout = 5 * 1000L; // Wait 5s
static int httpClientConnectionAttemptsTotal;                   // Count the number of connection attempts absolutely

static volatile uint32_t httpClientLastDataReceived; // Last time data was received via HTTP

static NetworkClientSecure *httpSecureClient;

static volatile uint8_t httpClientState = HTTP_CLIENT_OFF;

// HTTP client timer usage:
//  * Reconnection delay
//  * Measure the connection response time
//  * Receive HTTP data timeout
static uint32_t httpClientStartTime; // For calculating uptime
static uint32_t httpClientTimer;

//----------------------------------------
// HTTP Client Routines
//----------------------------------------

// Determine if another connection is possible or if the limit has been reached
bool httpClientConnectLimitReached()
{
    bool limitReached;
    int seconds;

    // Retry the connection a few times
    limitReached = (httpClientConnectionAttempts >= MAX_HTTP_CLIENT_CONNECTION_ATTEMPTS);

    bool enableHttpClient = true;
    if (!settings.enablePointPerfectCorrections)
        enableHttpClient = false;

    // Restart the HTTP client
    httpClientStop(limitReached || (!enableHttpClient));

    httpClientConnectionAttempts++;
    httpClientConnectionAttemptsTotal++;
    if (settings.debugHttpClientState)
        httpClientPrintStatus();

    if (limitReached == false)
    {
        // Display the delay before starting the HTTP client
        if (settings.debugHttpClientState && httpClientConnectionAttemptTimeout)
        {
            seconds = httpClientConnectionAttemptTimeout / 1000;
            systemPrintf("HTTP Client trying again in %d seconds.\r\n", seconds);
        }
    }
    else
        // No more connection attempts
        systemPrintln("HTTP Client connection attempts exceeded!");

    return limitReached;
}

// Print the HTTP client state summary
void httpClientPrintStateSummary()
{
    switch (httpClientState)
    {
    default:
        systemPrintf("Unknown: %d", httpClientState);
        break;
    case HTTP_CLIENT_OFF:
        systemPrint("Off");
        break;

    case HTTP_CLIENT_ON:
    case HTTP_CLIENT_NETWORK_STARTED:
        systemPrint("Disconnected");
        break;

    case HTTP_CLIENT_CONNECTING_2_SERVER:
        systemPrint("Connecting");
        break;

    case HTTP_CLIENT_CONNECTED:
        systemPrint("Connected");
        break;

    case HTTP_CLIENT_COMPLETE:
        systemPrint("Complete");
        break;
    }
}

// Print the HTTP Client status
void httpClientPrintStatus()
{
    systemPrint("HTTP Client ");
    httpClientPrintStateSummary();
}

// Restart the HTTP client
void httpClientRestart()
{
    // Save the previous uptime value
    if (httpClientState == HTTP_CLIENT_CONNECTED)
        httpClientStartTime = httpClientTimer - httpClientStartTime;
    httpClientConnectLimitReached();
}

// Update the state of the HTTP client state machine
void httpClientSetState(uint8_t newState)
{
    if (settings.debugHttpClientState || PERIODIC_DISPLAY(PD_HTTP_CLIENT_STATE))
    {
        if (httpClientState == newState)
            systemPrint("*");
        else
            systemPrintf("%s --> ", httpClientStateName[httpClientState]);
    }
    httpClientState = newState;
    if (settings.debugHttpClientState || PERIODIC_DISPLAY(PD_HTTP_CLIENT_STATE))
    {
        PERIODIC_CLEAR(PD_HTTP_CLIENT_STATE);
        if (newState >= HTTP_CLIENT_STATE_MAX)
        {
            systemPrintf("Unknown HTTP Client state: %d\r\n", newState);
            reportFatalError("Unknown HTTP Client state");
        }
        else
            systemPrintln(httpClientStateName[httpClientState]);
    }
}

// Shutdown the HTTP client
void httpClientShutdown()
{
    httpClientStop(true);
}

// Start the HTTP client
void httpClientStart()
{
    // Display the heap state
    reportHeapNow(settings.debugHttpClientState);

    // Start the HTTP client
    systemPrintln("HTTP Client start");
    httpClientStop(false);
}

// Shutdown or restart the HTTP client
void httpClientStop(bool shutdown)
{
    // Free the httpClient resources
    if (httpClient)
    {
        if (settings.debugHttpClientState)
            systemPrintln("Freeing httpClient");

        delete httpClient;
        httpClient = nullptr;
        reportHeapNow(settings.debugHttpClientState);
    }

    // Free the httpSecureClient resources
    if (httpSecureClient)
    {
        if (settings.debugHttpClientState)
            systemPrintln("Freeing httpSecureClient");
        // delete httpSecureClient; // Don't. This causes issue #335
        httpSecureClient = nullptr;
        reportHeapNow(settings.debugHttpClientState);
    }

    // Increase timeouts if we started the network
    if (httpClientState > HTTP_CLIENT_ON)
        // Mark the Client stop so that we don't immediately attempt re-connect to Caster
        httpClientTimer = millis();

    // Determine the next HTTP client state
    online.httpClient = false;
    if (shutdown)
    {
        httpClientSetState(HTTP_CLIENT_OFF);
        // settings.enablePointPerfectCorrections = false;
        //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Why? This means PointPerfect Corrections
        // cannot be restarted without opening the menu or web configuration page...
        httpClientConnectionAttempts = 0;
        httpClientConnectionAttemptTimeout = 0;
    }
    else
        httpClientSetState(HTTP_CLIENT_ON);
}

void httpClientUpdate()
{
    // Shutdown the HTTP client when the mode or setting changes
    DMW_st(httpClientSetState, httpClientState);

    if (!httpClientModeNeeded)
    {
        if (httpClientState > HTTP_CLIENT_OFF)
        {
            systemPrintln("HTTP Client stopping");
            httpClientStop(true); // Was false - #StopVsRestart
            httpClientConnectionAttempts = 0;
            httpClientConnectionAttemptTimeout = 0;
            httpClientSetState(HTTP_CLIENT_OFF);
        }
    }

    // Enable the network and the HTTP client if requested
    switch (httpClientState)
    {
    default:
    case HTTP_CLIENT_OFF: {
        if (httpClientModeNeeded)
            httpClientStart();
        break;
    }

    // Start the network
    case HTTP_CLIENT_ON: {
        if ((millis() - httpClientTimer) > httpClientConnectionAttemptTimeout)
        {
            httpClientSetState(HTTP_CLIENT_NETWORK_STARTED);
        }
        break;
    }

    // Wait for a network media connection
    case HTTP_CLIENT_NETWORK_STARTED: {
        // Determine if the HTTP client was turned off
        if (!httpClientModeNeeded)
            httpClientStop(true);

        // Wait until the network is connected
        else if (networkHasInternet())
            httpClientSetState(HTTP_CLIENT_CONNECTING_2_SERVER);
        break;
    }

    // Connect to the HTTP server
    case HTTP_CLIENT_CONNECTING_2_SERVER: {
        // Determine if the network has failed
        if (networkHasInternet() == false)
        {
            // Failed to connect to the network, attempt to restart the network
            httpClientStop(true); // Was httpClientRestart(); - #StopVsRestart
            break;
        }

        // Allocate the httpSecureClient structure
        httpSecureClient = new NetworkClientSecure();
        if (!httpSecureClient)
        {
            systemPrintln("ERROR: Failed to allocate the httpSecureClient structure!");
            httpClientShutdown();
            break;
        }

        // Set the Amazon Web Services public certificate
        httpSecureClient->setCACert(AWS_PUBLIC_CERT);

        // Connect to the server
        if (!httpSecureClient->connect(THINGSTREAM_SERVER, HTTPS_PORT))
        {
            // Failed to connect to the server
            systemPrintln("ERROR: Failed to connect to the Thingstream server!");
            httpClientRestart(); // I _think_ we want to restart here - i.e. retry after the timeout?
            break;
        }

        // Allocate the httpClient structure
        httpClient = new HTTPClient();
        if (!httpClient)
        {
            // Failed to allocate the httpClient structure
            systemPrintln("ERROR: Failed to allocate the httpClient structure!");
            httpClientShutdown();
            break;
        }

        if (settings.debugHttpClientState)
            systemPrintf("HTTP client connecting to %s\r\n", THINGSTREAM_ZTPURL);

        // Begin the HTTP client
        if (!httpClient->begin(*httpSecureClient, THINGSTREAM_ZTPURL))
        {
            systemPrintln("ERROR: Failed to start httpClient!\r\n");
            httpClientRestart(); // I _think_ we want to restart here - i.e. retry after the timeout?
            break;
        }

        reportHeapNow(settings.debugHttpClientState);
        online.httpClient = true;
        httpClientSetState(HTTP_CLIENT_CONNECTED);

        break;
    }

    case HTTP_CLIENT_CONNECTED: {
        // Determine if the network has failed
        if (networkHasInternet() == false)
        {
            // Failed to connect to the network, attempt to restart the network
            httpClientStop(true); // Was httpClientRestart(); - #StopVsRestart
            break;
        }

        String ztpRequest;
        createZtpRequest(ztpRequest);

        if (settings.debugCorrections || settings.debugHttpClientData)
        {
            systemPrintf("Sending JSON, %d bytes\r\n", ztpRequest.length());
            // systemPrintln(ztpRequest.c_str());
        }

        httpClient->addHeader(F("Content-Type"), F("application/json"));
        int httpResponseCode = httpClient->POST(ztpRequest.c_str());
        String response = httpClient->getString();
        if (settings.debugCorrections || settings.debugHttpClientData)
        {
            systemPrint("HTTP response: ");
            systemPrintln(response.c_str());
        }

        if (httpResponseCode != 200) // Connection failed
        {
            if (settings.debugCorrections || settings.debugHttpClientData)
            {
                systemPrintf("HTTP response error %d: ", httpResponseCode);
                systemPrintln(response.c_str());
            }

            // "HTTP response error -11 " = 411 which is length required
            // https://stackoverflow.com/questions/19227142/http-status-code-411-length-required
            if (httpResponseCode == -11)
            {
                if (settings.debugCorrections || settings.debugHttpClientData)
                {
                    systemPrintln("HTTP response error 411: Length Required. Retrying...");
                    systemPrintln(response.c_str());
                }

                httpClientRestart();
                break;
            }

            // If a device has already been registered on a different ZTP profile, response will be:
            // "HTTP response error 403: Device already registered"
            else if (response.indexOf("Device already registered") >= 0)
            {
                if (settings.debugCorrections || settings.debugHttpClientData)
                    systemPrintln("Device already registered to different profile");

                // ztpInterimResponse[ztpAttempt] = ZTP_ALREADY_REGISTERED;

                break;
            }
            // If a device has been deactivated, response will be: "HTTP response error 403: No plan for device
            // device:9f19e97f-e6a7-4808-8d58-ac7ecac90e23"
            else if (response.indexOf("No plan for device") >= 0)
            {
                if (settings.debugCorrections || settings.debugHttpClientData)
                    systemPrintln("Device has been deactivated.");

                // ztpInterimResponse[ztpAttempt] = ZTP_DEACTIVATED;

                // ztpNextToken(); // Move to the next ZTP profile. Exit client as needed.

                httpClientSetState(HTTP_CLIENT_COMPLETE);
                break;
            }
            // If a device is not whitelisted, response will be: "HTTP response error 403: Device hardware code not
            // whitelisted"
            else if (response.indexOf("not whitelisted") >= 0)
            {
                if (settings.debugCorrections || settings.debugHttpClientData)
                    systemPrintln("Device not whitelisted.");

                // ztpInterimResponse[ztpAttempt] = ZTP_NOT_WHITELISTED;

                // ztpNextToken(); // Move to the next ZTP profile. Exit client as needed.

                break;
            }
            else
            {
                systemPrintf("HTTP response error %d: ", httpResponseCode);
                systemPrintln(response.c_str());
                // ztpInterimResponse[ztpAttempt] = ZTP_UNKNOWN_ERROR;

                // ztpNextToken(); // Move to the next ZTP profile. Exit client as needed.

                break;
            }
        }

        else
        {
            // Device is now active with ThingStream
            // Pull pertinent values from response
            jsonZtp = new JsonDocument;
            if (!jsonZtp)
            {
                systemPrintln("ERROR - Failed to allocate jsonZtp!\r\n");
                httpClientShutdown(); // Try again?
                break;
            }

            DeserializationError error = deserializeJson(*jsonZtp, response);
            if (DeserializationError::Ok != error)
            {
                systemPrintln("JSON error");
                httpClientShutdown(); // Try again
                break;
            }
            else
            {
                if (online.psram == true)
                    tempHolderPtr = (char *)ps_malloc(MQTT_CERT_SIZE);
                else
                    tempHolderPtr = (char *)malloc(MQTT_CERT_SIZE);

                if (!tempHolderPtr)
                {
                    systemPrintln("ERROR - Failed to allocate tempHolderPtr buffer!\r\n");
                    httpClientShutdown(); // Try again?
                    break;
                }

                if (response.indexOf("rtcmCredentials") >= 0)
                {
                    // Handle a PointPerfect RTCM credentials response
                    systemPrintf("PointPerfect response: %s\r\n", response.c_str());

                    strncpy(settings.ntripClient_CasterHost, (const char *)((*jsonZtp)["rtcmCredentials"]["endpoint"]),
                            sizeof(settings.ntripClient_CasterHost));
                    settings.ntripClient_CasterPort = (*jsonZtp)["rtcmCredentials"]["httpPort"];

                    // If region is determined, override NTRIP Settings
                    settings.enableNtripClient = true;
                    settings.ntripClient_TransmitGGA = true;

                    strncpy(settings.ntripClient_CasterUser, (const char *)((*jsonZtp)["rtcmCredentials"]["userName"]),
                            sizeof(settings.ntripClient_CasterUser));
                    strncpy(settings.ntripClient_CasterUserPW,
                            (const char *)((*jsonZtp)["rtcmCredentials"]["password"]),
                            sizeof(settings.ntripClient_CasterUserPW));
                    strncpy(settings.ntripClient_MountPoint,
                            (const char *)((*jsonZtp)["rtcmCredentials"]["mountPoint"]),
                            sizeof(settings.ntripClient_MountPoint));

                    if (settings.debugCorrections || settings.debugHttpClientData)
                        pointperfectPrintNtripInformation("HTTP Client");

                    ztpInterimResponse[ztpAttempt] = ZTP_SUCCESS;

                    ztpResponse = ZTP_SUCCESS; // Report success to provisioningUpdate()

                    httpClientSetState(HTTP_CLIENT_COMPLETE);
                }
                else
                {
                    // Handle a PointPerfect 'keys' response
                    strncpy(tempHolderPtr, (const char *)((*jsonZtp)["certificate"]), MQTT_CERT_SIZE - 1);
                    recordFile("certificate", tempHolderPtr, strlen(tempHolderPtr));

                    strncpy(tempHolderPtr, (const char *)((*jsonZtp)["privateKey"]), MQTT_CERT_SIZE - 1);
                    recordFile("privateKey", tempHolderPtr, strlen(tempHolderPtr));

                    free(tempHolderPtr); // Clean up. Done with tempHolderPtr

                    // Validate the keys
                    if (!checkCertificates())
                    {
                        systemPrintln("ERROR - Failed to validate the Point Perfect certificates!");
                    }
                    else
                    {
                        if (settings.debugPpCertificate || settings.debugHttpClientData)
                            systemPrintln("Certificates recorded successfully.");

                        strncpy(settings.pointPerfectClientID, (const char *)((*jsonZtp)["clientId"]),
                                sizeof(settings.pointPerfectClientID));
                        strncpy(settings.pointPerfectBrokerHost, (const char *)((*jsonZtp)["brokerHost"]),
                                sizeof(settings.pointPerfectBrokerHost));

                        // Note: from the ZTP documentation:
                        // ["subscriptions"][0] will contain the key distribution topic
                        // But, assuming the key distribution topic is always ["subscriptions"][0] is potentially
                        // brittle It is safer to check the "description" contains "key distribution topic" If we are on
                        // an IP-only plan, the path will be /pp/ubx/0236/ip If we are on a L-Band-only or L-Band+IP
                        // plan, the path will be /pp/ubx/0236/Lb These 0236 key distribution topics provide the keys in
                        // UBX format, ready to be pushed to a ZED. There are also /pp/key/ip and /pp/key/Lb topics
                        // which provide the keys in JSON format - but we don't use those.
                        int subscription =
                            findZtpJSONEntryTnT("subscriptions", "description", "key distribution topic", jsonZtp);
                        if (subscription >= 0)
                            strncpy(settings.pointPerfectKeyDistributionTopic,
                                    (const char *)((*jsonZtp)["subscriptions"][subscription]["path"]),
                                    sizeof(settings.pointPerfectKeyDistributionTopic));

                        // "subscriptions" will also contain the correction topics for all available regional areas -
                        // for IP-only or L-Band+IP We should store those too, and then allow the user to select the one
                        // for their regional area
                        for (int r = 0; r < numRegionalAreas; r++)
                        {
                            char findMe[40];
                            snprintf(findMe, sizeof(findMe), "correction topic for %s",
                                     Regional_Information_Table[r].name); // Search for "US" etc.
                            subscription =
                                findZtpJSONEntryTnT("subscriptions", "description", (const char *)findMe, jsonZtp);
                            if (subscription >= 0)
                                strncpy(settings.regionalCorrectionTopics[r],
                                        (const char *)((*jsonZtp)["subscriptions"][subscription]["path"]),
                                        sizeof(settings.regionalCorrectionTopics[0]));
                            else
                                settings.regionalCorrectionTopics[r][0] =
                                    0; // Erase any invalid (non-plan) correction topics. Just in case the plan has
                                       // changed.
                        }

                        // "subscriptions" also contains the geographic area definition topic for each region for
                        // localized distribution. We can cheat by appending "/gad" to the correction topic. TODO: think
                        // about doing this properly.

                        // Now we extract the current and next key pair
                        strncpy(settings.pointPerfectCurrentKey,
                                (const char *)((*jsonZtp)["dynamickeys"]["current"]["value"]),
                                sizeof(settings.pointPerfectCurrentKey));
                        settings.pointPerfectCurrentKeyDuration = (*jsonZtp)["dynamickeys"]["current"]["duration"];
                        settings.pointPerfectCurrentKeyStart = (*jsonZtp)["dynamickeys"]["current"]["start"];

                        strncpy(settings.pointPerfectNextKey,
                                (const char *)((*jsonZtp)["dynamickeys"]["next"]["value"]),
                                sizeof(settings.pointPerfectNextKey));
                        settings.pointPerfectNextKeyDuration = (*jsonZtp)["dynamickeys"]["next"]["duration"];
                        settings.pointPerfectNextKeyStart = (*jsonZtp)["dynamickeys"]["next"]["start"];

                        if (settings.debugCorrections || settings.debugHttpClientData)
                            pointperfectPrintKeyInformation("HTTP Client");

                        // displayKeysUpdated();

                        ztpInterimResponse[ztpAttempt] = ZTP_SUCCESS;

                        ztpResponse = ZTP_SUCCESS; // Report success to provisioningUpdate()

                        httpClientSetState(HTTP_CLIENT_COMPLETE);
                    } // Valid certificates
                } // Handle keys type response
            } // JSON Deserialized correctly
        } // HTTP Response was 200
        break;
    }

    // The ZTP HTTP POST is complete. We either can not or do not want to continue.
    // Hang here until httpClientModeNeeded is set to false by updateProvisioning
    case HTTP_CLIENT_COMPLETE: {
        // Determine if the network has failed
        if (networkHasInternet() == false)
            // Failed to connect to the network, attempt to restart the network
            httpClientStop(true); // Was httpClientRestart(); - #StopVsRestart
        break;
    }

    } // switch (httpClientState)

    // Periodically display the HTTP client state
    if (PERIODIC_DISPLAY(PD_HTTP_CLIENT_STATE))
        httpClientSetState(httpClientState);
}

// Verify the HTTP client tables
void httpClientValidateTables()
{
    if (httpClientStateNameEntries != HTTP_CLIENT_STATE_MAX)
        reportFatalError("Fix httpClientStateNameEntries to match HTTPClientState");
}

// Given a token buffer and an attempt number, decide which token to use for ZTP
// Not all platforms can handle all service levels so this skips service levels that don't apply
// There are multiple service levels:
//   L-Band
//   RTCM
//   L-Band+IP
//   IP
//   RTCM Trial
void ztpGetToken(char *tokenString)
{
    // pointperfectCreateTokenString() convert uint8_t array into string with dashes in spots
    // We must assume u-blox will not change the position of their dashes or length of their token

    if (settings.pointPerfectService == PP_NICKNAME_FLEX_RTCM)
        pointperfectCreateTokenString(tokenString, (uint8_t *)ppRtcmToken, sizeof(ppRtcmToken));

    else if (settings.pointPerfectService == PP_NICKNAME_FLEX_LBAND_NA)
        pointperfectCreateTokenString(tokenString, (uint8_t *)ppLbandToken, sizeof(ppLbandToken));

    else if (settings.pointPerfectService == PP_NICKNAME_GLOBAL)
        pointperfectCreateTokenString(tokenString, (uint8_t *)ppGlobalToken, sizeof(ppGlobalToken));

    else if (settings.pointPerfectService == PP_NICKNAME_LIVE)
        pointperfectCreateTokenString(tokenString, (uint8_t *)ppLiveToken, sizeof(ppLiveToken));

    else if (settings.pointPerfectService == PP_NICKNAME_IP_MQTT)
        pointperfectCreateTokenString(tokenString, (uint8_t *)ppIpToken, sizeof(ppIpToken)); // Deprecated

    // ppLbandIpToken //Deprecated - remove from use once all LBand+IP users are moved over to L-Band

    else
    {
        systemPrintln("Unknown hardware for GetToken");
        return;
    }
}

// Determine if this service type uses keys
bool ppServiceUsesKeys()
{
    if (settings.pointPerfectService == PP_NICKNAME_FLEX_LBAND_NA ||
        settings.pointPerfectService == PP_NICKNAME_GLOBAL || settings.pointPerfectService == PP_NICKNAME_IP_MQTT)
        return true;
    return false;
}

#endif // COMPILE_HTTP_CLIENT
