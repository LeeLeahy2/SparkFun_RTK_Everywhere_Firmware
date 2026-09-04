/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Begin.ino

  This module implements the initial startup functions for GNSS, SD, display,
  radio, etc.
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

#include <esp_mac.h> // required - exposes esp_mac_type_t values

//----------------------------------------
// Constants
//----------------------------------------

#define MAX_ADC_VOLTAGE 3300 // Millivolts

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Hardware initialization functions
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//----------------------------------------
// Compute the upper and lower threshold values
//----------------------------------------
float computeThreshold(float r1, float r2, float tolerance)
{
    return MAX_ADC_VOLTAGE * (r2 * (1.0 + (tolerance / 100.0))) /
           ((r1 * (1.0 + (-tolerance / 100.0))) + (r2 * (1.0 + (tolerance / 100.0))));
}

//----------------------------------------
// Determine if the measured value matches the product ID value
// idWithAdc applies resistor tolerance using worst-case tolerances:
// Upper threshold: R1 down by TOLERANCE, R2 up by TOLERANCE
// Lower threshold: R1 up by TOLERANCE, R2 down by TOLERANCE
// Testing shows the combined ADC+resistors is under a 1% window
// But the internal ESP32 VRef fuse is not always set correctly
//----------------------------------------
bool idWithAdc(uint16_t mvMeasured, float r1, float r2, float tolerance)
{
    float lowerThreshold;
    float upperThreshold;

    //                                ADC input
    //                       r1 KOhms     |     r2 KOhms
    //  MAX_ADC_VOLTAGE -----/\/\/\/\-----+-----/\/\/\/\----- Ground

    // Return true if the mvMeasured value is within the tolerance range
    // of the mvProduct value
    upperThreshold = ceil(computeThreshold(r1, r2, tolerance));
    lowerThreshold = floor(computeThreshold(r1, r2, -tolerance));

    bool result = (upperThreshold > mvMeasured) && (mvMeasured > lowerThreshold);
    if (result)
        systemPrintf("R1: %0.2f R2: %0.2f lowerThreshold: %0.0f mvMeasured: %d upperThreshold: %0.0f\r\n", r1, r2,
                     lowerThreshold, mvMeasured, upperThreshold);

    return result;
}

//----------------------------------------
// Configure UART2 serial port shared between LoRa and Tilt
// This only applies to the FP. The Torch has tilt connected direct to ESP UART0 (shared with USB)
//----------------------------------------
bool beginUart2Serial()
{
    // Determine if serial port is already configured
    if (uart2Serial)
        return true;

    // Allocate the serial port object
    uart2Serial = new HardwareSerial(2);

    // Determine if the allocation failed
    if (uart2Serial == nullptr)
    {
        systemPrintf("ERROR: Failed to allocate the uart2Serial port!\r\n");
        return false;
    }

    // Configure the serial port
    //uart2Serial->setRxBufferSize(settings.uartReceiveBufferSize);
    //uart2Serial->setTimeout(settings.serialTimeoutGNSS); // Requires serial traffic on the UART pins for detection
    uart2Serial->begin(115200, SERIAL_8N1, pin_IMU_RX, pin_IMU_TX);
    return true;
}

//----------------------------------------
// Assign I2C interrupts to the core that started the task. See: https://github.com/espressif/arduino-esp32/issues/3386
//----------------------------------------
bool i2cBusEnumerate(TwoWire *i2cBus, int i2cBusNumber)
{
    bool deviceFound;
    uint32_t timer;

    // Display the device addresses
    deviceFound = false;
    for (uint8_t addr = 0; addr < 127; addr++)
    {
        // begin/end wire transmission to see if the bus is responding correctly
        // All good: 0ms, response 2
        // SDA/SCL shorted: 1000ms timeout, response 5
        // SCL/VCC shorted: 14ms, response 5
        // SCL/GND shorted: 1000ms, response 5
        // SDA/VCC shorted: 1000ms, response 5
        // SDA/GND shorted: 14ms, response 5
        timer = millis();

        // The authentication coprocessor can be asleep. It needs special treatment
        if (addr == 0x10)
        {
            // This takes longer than 3ms to complete
            // Don't allow the code to reach else if ((millis() - timer) > 3)
            if (i2cIsDeviceRegisterPresent(i2cBus, addr, 0x00, 0x07))
            {
                if (deviceFound == false)
                {
                    systemPrintf("I2C-%d Devices:\r\n", i2cBusNumber);
                    deviceFound = true;
                }

                systemPrintf("  0x%02X - Authentication Coprocessor\r\n", addr);
            }
        }
        else if (i2cIsDevicePresent(i2cBus, addr))
        {
            if (deviceFound == false)
            {
                systemPrintf("I2C-%d Devices:\r\n", i2cBusNumber);
                deviceFound = true;
            }

            switch (addr)
            {
            default: {
                systemPrintf("  0x%02X - Unknown\r\n", addr);
                break;
            }

            case 0x08: {
                systemPrintf("  0x%02X - HUSB238 Power Delivery Sink Controller\r\n", addr);
                break;
            }

            case 0x0B: {
                systemPrintf("  0x%02X - BQ40Z50 Battery Pack Manager / Fuel gauge\r\n", addr);
                break;
            }

            case 0x18: {
                systemPrintf("  0x%02X - PCA9557 GPIO Expander with Reset\r\n", addr);
                break;
            }

            case 0x19: {
                systemPrintf("  0x%02X - LIS2DH12 Accelerometer\r\n", addr);
                break;
            }

            case 0x20: {
                systemPrintf("  0x%02X - PCA9554 GPIO Expander with Interrupt (Postcard)\r\n", addr);
                break;
            }

            case 0x21: {
                systemPrintf("  0x%02X - PCA9554 GPIO Expander with Interrupt (Facet FP)\r\n", addr);
                break;
            }

            case 0x2C: {
                systemPrintf("  0x%02X - USB251xB USB Hub\r\n", addr);
                break;
            }

            case 0x36: {
                systemPrintf("  0x%02X - MAX17048 Fuel Gauge\r\n", addr);
                break;
            }

            case 0x3C: {
                systemPrintf("  0x%02X - SSD1306 OLED Driver (Facet FP)\r\n", addr);
                break;
            }

            case 0x3D: {
                systemPrintf("  0x%02X - SSD1306 OLED Driver (Postcard/EVK/mosaic)\r\n", addr);
                break;
            }

            case 0x42: {
                systemPrintf("  0x%02X - u-blox GNSS Receiver\r\n", addr);
                break;
            }

            case 0x5C: {
                systemPrintf("  0x%02X - MP27692A Power Management / Charger\r\n", addr);
                break;
            }

            case 0x60: {
                systemPrintf("  0x%02X - ATECC608A Cryptographic Coprocessor\r\n", addr);
                break;
            }
            }
        }
        else if ((millis() - timer) > 3)
        {
            systemPrintln("ERROR: I2C bus not responding!");
            return false;
        }
    }

    // Determine if any devices are on the bus
    if (deviceFound == false)
    {
        systemPrintln("No devices found on this I2C bus");
        return false;
    }
    return true;
}

//----------------------------------------
// Read an I2C device register and check for an expected value
//----------------------------------------
bool i2cIsDeviceRegisterPresent(TwoWire *i2cBus, uint8_t deviceAddress, uint8_t registerAddress, uint8_t expectedValue)
{
    int maxRetries = 3;

    while (maxRetries > 0)
    {
        maxRetries--;
        delay(1);

        i2cBus->beginTransmission(deviceAddress);
        i2cBus->write(registerAddress);
        if (i2cBus->endTransmission() != 0)
            continue;

        i2cBus->requestFrom(deviceAddress, (uint8_t)1);
        if (i2cBus->available())
        {
            return (i2cBus->read() == expectedValue);
        }
    }

    return false;
}
