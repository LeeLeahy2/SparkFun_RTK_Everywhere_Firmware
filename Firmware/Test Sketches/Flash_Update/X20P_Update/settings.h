// Product Variant used as part of device ID and whitelists. Do not reorder.
typedef enum
{
    RTK_EVK = 0, // 0x00
    // RTK_FACET_V2 = 1, // 0x01 - No L-Band
    RTK_FACET_MOSAIC = 2, // 0x02
    RTK_TORCH = 3, // 0x03
    // RTK_FACET_V2_LBAND = 4, // 0x04
    RTK_POSTCARD = 5, // 0x05
    RTK_FACET_FP = 6, // 0x06
    RTK_TORCH_X2 = 7, // 0x07
    // Add new values above this line
    RTK_UNKNOWN
} ProductVariant;
ProductVariant productVariant = RTK_UNKNOWN;

// This is all the settings that can be set on RTK Product. It's recorded to NVM and the config file.
// Avoid reordering. The order of these variables is mimicked in NVM/record/parse/create/update/get
struct Settings
{
    bool debugFirmwareUpdate = false;
} settings;

// Indicate which peripherals are present on a given platform
struct struct_present
{
    bool psram_2mb = false;
    bool psram_4mb = false;

    bool cellular_lara = false;
    bool ethernet_ws5500 = false;
    bool radio_lora = false;
    bool gnss_to_uart = false;
    bool gnss_to_uart2 = false;

    bool gnss_um980 = false;
    bool gnss_zedf9p = false;
    bool gnss_mosaicX5 = false; // L-Band is implicit
    bool gnss_lg290p = false;
    bool gnss_zedx20p = false;

    // A GNSS TP interrupt - for accurate clock setting
    // The GNSS UBX PVT message is sent ahead of the top-of-second
    // The rising edge of the TP signal indicates the true top-of-second
    bool timePulseInterrupt = false;

    bool imu_im19 = false;
    bool imu_zedf9r = false;

    bool microSd = false;
    bool mosaicMicroSd = false;
    bool microSdCardDetectLow = false; // Card detect low = SD in place
    bool microSdCardDetectHigh = false; // Card detect high = SD in place
    bool microSdCardDetectGpioExpanderHigh = false; // Card detect on GPIO5, high = SD in place

    bool i2c0BusSpeed_400 = false;
    bool i2c1BusSpeed_400 = false;
    bool i2c1 = false;
    bool display_i2c0 = false;
    bool display_i2c1 = false;
    bool displayInverted = false;

    bool fuelgauge_max17048 = false;
    bool fuelgauge_bq40z50 = false;
    bool charger_mp2762a = false;
    bool charger_mcp73833 = false;

    bool beeper = false;
    bool encryption_atecc608a = false;
    bool portDataMux = false;
    bool peripheralPowerControl = false;
    bool laraPowerControl = false;
    bool antennaShortOpen = false;

    bool button_mode = false; // EVK has a dedicated Mode button but no power
    bool button_function = false; // Facet FP has both power and Function buttons
    bool button_powerHigh = false; // Button is pressed when high
    bool button_powerLow = false; // Button is pressed when low
    bool gpioExpanderButtons = false; // Available on Portability shield
    bool fastPowerOff = false;
    bool invertedFastPowerOff = false; // Needed for Facet mosaic v11

    bool needsExternalPpl = false;

    bool pppCapable = false; // Device has the capability to do PPP corrections, currently B2b or E6 HAS
    bool multipathMitigation = false; // UM980 has MPM, other platforms do not
    bool minCN0 = false; // ZED, mosaic, UM980 have minCN0. LG290P does on version >= v5.
    bool minElevation = false; // ZED, mosaic, UM980 have minElevation. LG290P does on versions >= v5.
    bool dynamicModel = false; // ZED, mosaic, UM980 have dynamic models. LG290P does with firmware v2.01.
    bool gpioExpanderSwitches = false; // Used on Facet FP
    bool loraDedicatedUart = false; // Platforms may have a dedicated or shared UART interface to the LoRa radio

    const char *gnssUpdatePort = ""; // "CH342 Channel A" etc.

    bool rtcm1033AntennaDescription = false; // RTCM 1033 Antenna Descriptor - supported on X5, LG290P and UM980
} present;

// Monitor which devices on the device are on or offline.
struct struct_online
{
    bool batteryCharger_mp2762a = false;
    bool batteryFuelGauge = false;
    bool bluetooth = false;
    bool powerButton = false;
    bool functionButton = false;
    bool display = false;
    bool ethernetNTPServer = false; // EthernetUDP
    bool fs = false;
    bool gnss = false;
    bool gpioExpanderButtons = false;
    bool gpioExpanderSwitches = false;
    bool httpClient = false;
    bool i2c = false;
    bool lband_gnss = false;
    bool pointPerfectKeysApplied = false;
    bool logging = false;
    bool microSD = false;
    bool mqttClient = false;
    bool ntripClient = false;
    bool otaClient = false;
    bool ppl = false;
    bool psram = false;
    bool radio_lora = false;
    bool rtc = false;
    bool serialOutput = false;
    bool tcpClient = false;
    bool tcpServer = false;
    bool udpServer = false;
    bool webServer = false;
    bool authenticationCoPro = false; // MFi authentication
    bool imu_im19 = false;
} online;

// ISRG Root X1 (Let's Encrypt). Used to validate raw.githubusercontent.com's server cert chain.
static const char GITHUB_RAW_PUBLIC_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";
