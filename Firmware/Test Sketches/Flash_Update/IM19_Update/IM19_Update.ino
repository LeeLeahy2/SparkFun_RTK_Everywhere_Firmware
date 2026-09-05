/*
    This example shows how to update the IM19 over WiFi.

    This was written for Torch and FP hardware.

    Ported from the reference implementation in upgrade.c: a 268-byte framed
    protocol (0xAA55 header, 256-byte payload, uint32 checksum) used to push
    a firmware image to the IM19 module and confirm it booted the new image.

    To test: load this sketch onto a Torch or FP.
    Press 'u' to start the update. Allow the update to complete.
    Press 'r' to reset. The GNSS module should boot and respond to commands.

    All loaders should have similar structure:
    Given the web address of the binary to load,
    Do the WiFi stuff to begin reading the file data
    Put the target into bootload mode and malloc any necessary buffers xxxUpdateFirmwareBegin()
    Grab chunks of bytes over WiFi and throw at xxxUpdateFirmware(*data, length)
    When done, call xxxUpdateFirmwareEnd() to free buffers and exit the bootloader mode or reset the target
*/

//----------------------------------------
// Common declarations
//----------------------------------------

bool RTK_CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC = false; // Needed because of local BT TLS patch

#include <arpa/inet.h>
#include <HTTPClient.h>
#include <netdb.h>
#include <Network.h>
#include <NetworkClientSecure.h>
#include <sys/socket.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#ifndef ENABLE_DEVELOPER
#define ENABLE_DEVELOPER            true
#endif   // ENABLE_DEVELOPER
#define DMW_if if (0)

const uint8_t logoSparkFun[] = {0};
#define logoSparkFun_Height         1
#define logoSparkFun_Width          1

const uint8_t logoSparkPNT[] = {0};
#define logoSparkPNT_Height         1
#define logoSparkPNT_Width          1

#include "secrets.h"
#include "settings.h"

#define rtkMalloc(bytes, description)       malloc(bytes)
#define rtkFree(buffer, description)        free(buffer)

// Timer for firmware update duration
unsigned long firmwareUpdateStartTime = 0;
unsigned long firmwareUpdateElapsed = 0;

//----------------------------------------
// Test specific declarations
//----------------------------------------

// v11.4.1
const char * url_11_4_1 = "https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Everywhere_Firmware_Binaries/main/imu/im19/20260522185649_VH2_B2.2_A11.4.1_131b44ecee0bdad5670c7.enc";

// v11.1
const char * url_11_1 = "https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Everywhere_Firmware_Binaries/main/imu/im19/20260302210315_VH2_B2.2_A11.1_6bf04becee0bda310e65d.enc";

// v6.1
const char * url_6_1 = "https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Everywhere_Firmware_Binaries/main/imu/im19/20230419111130_VH2_B2.2_A6.1_2eea4d4c024538bf5ed52.enc";

#define OTA_FIRMWARE_GITHUB_RAW "raw.githubusercontent.com"

char imuVersion[96];

//----------------------------------------
// Test entry point
//----------------------------------------
void setup()
{
    // Common setup
    Serial.begin(115200);
    delay(250);

    identifyBoard(); // Determine what hardware platform we are running on.
    beginBoard();    // Set all pin numbers and pin initial states
    beginMux();      // Must come before I2C activity to avoid external
                     // devices from corrupting the bus. See issue 474
                     //  https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/474
    peripheralsOn(); // Enable power for the display, SD, etc
    beginI2C();      // Requires settings and peripheral power (if applicable).

    if (productVariant == RTK_TORCH)
        imuReset();
    else if (productVariant == RTK_FACET_FP)
    {
        beginGpioExpanderSwitches();
        gpioExpanderSelectImu(); // On FP, confirm SW3 is in the correct position
    }
    else
    {
        Serial.println("Product variant does not support IM19. Freezing...");
        while (true)
            delay(1000);
    }

    im19GetVersionString();

    wifiConnect();

    displayMenu();
}

void displayMenu()
{
    systemPrintln();
    systemPrintln("Menu:");
    systemPrintln("o) Update IM19 to 6.1");
    systemPrintln("p) Update IM19 to 11.1");
    systemPrintln("u) Update IM19 to 11.4.1");
    systemPrintln("r) Reset");
    systemPrintln("e) Enter URL");
    systemPrintf("d) Debug: %s\r\n", settings.debugFirmwareUpdate ? "Enabled" : "Disabled");
    systemPrintf("v) Verbose output: %s\r\n", otaDebugVerbose ? "Enabled" : "Disabled");
    systemPrint("Make selection: ");
}

void loop()
{
    if (Serial.available())
    {
        byte incoming = Serial.read();
        Serial.printf("%c\r\n", incoming);
        if (incoming == 'r')
        {
            ESP.restart();
        }
        else if (incoming == 'd')
        {
            settings.debugFirmwareUpdate ^= 1;
            otaDebugVerbose = false;
        }
        else if (incoming == 'e')
        {
            // Get the URL
            systemPrint("Enter URL: ");
            String urlString = systemGetStringFromUser();
            firmwareUpdate(urlString.c_str());
        }
        else if (incoming == 'o')
            firmwareUpdate(url_6_1);
        else if (incoming == 'p')
            firmwareUpdate(url_11_1);
        else if (incoming == 'u')
            firmwareUpdate(url_11_4_1);
        else if (incoming == 'v')
            otaDebugVerbose ^= 1;
        displayMenu();
    }
}

// Perform the firmware update
void firmwareUpdate(const char * url)
{
    // Verify the url
    if ((url == nullptr) || (strlen(url) == 0))
        systemPrintf("No URL specified\r\n");
    else
    {
        // Start timer before erase
        firmwareUpdateStartTime = millis();

        // Attempt to update the firmware
        if (im19FirmwareUpdate(url) == true)
        {
            // Stop timer and print elapsed time
            firmwareUpdateElapsed = millis() - firmwareUpdateStartTime;
            systemPrint("Firmware update time: ");
            systemPrint(firmwareUpdateElapsed / 1000.0, 3);
            systemPrintln(" seconds");
        }
    }
}

// Connects to the configured SSID and blocks until connected or the attempt times out.
bool wifiConnect()
{
    systemPrint("Connecting to WiFi SSID: ");
    systemPrintln(wifiSSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPassword);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if ((millis() - start) > 20000)
        {
            systemPrintln("WiFi connection timed out.");
            return false;
        }
        delay(250);
        systemPrint(".");
    }

    systemPrint("WiFi connected, IP address: ");
    systemPrintln(WiFi.localIP());
    return true;
}
