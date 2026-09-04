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
