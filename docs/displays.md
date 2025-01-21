# Displays

<!--
Compatibility Icons
====================================================================================

:material-radiobox-marked:{ .support-full title="Feature Supported" }
:material-radiobox-indeterminate-variant:{ .support-partial title="Feature Partially Supported" }
:material-radiobox-blank:{ .support-none title="Feature Not Supported" }
-->

<div class="grid cards fill" markdown>

- Torch: [:material-radiobox-blank:{ .support-none }]( title ="Feature Not Supported" )
- EVK: :material-radiobox-marked:{ .support-full title="Feature Supported" }

</div>

A variety of RTK devices utilize a 0.96" or 1.3" high-contrast OLED display. While small, the display packs various situational data that can be helpful in the field. This section describes each possible display state.

![RTK EVK - 1.3" OLED](img/Displays/SparkFun_RTK_EVK_LBand_Indicator.png)

*Rover mode on the RTK EVK's 1.3" OLED display*

## Power On/Off

![Startup and Shutdown Screens](img/Displays/SparkFun_RTK_Facet_-_Display_On_Off.jpg)

*RTK Facet Startup and Shutdown Screens*

Press and hold the power button until the display illuminates to turn on the device. Similarly, press and hold the power button to turn off the device.

The device's firmware version is shown during the Power On display.

### Force Power Off

In the event that a device becomes unresponsive, the device can be completely powered off by holding the power button for 10 seconds or more. The force-power-off method is hardware-based and will therefore work regardless of what firmware the device may be running.

![Multiple COM ports shown](img/Serial/SparkFun_RTK_Facet_-_Multiple_COM_Ports.jpg)

If the power state of a device is not known (for example, because a display may be malfunctioning) the device can be connected to USB. If one or more COM ports enumerate, the device is on (shown above). If no COM ports are seen, the device is fully powered off.

## Rover Fix

![Rover with location fix](img/Displays/SparkFun_RTK_Facet_-_Main_Display_Icons.jpg)

*Rover with location fix*

![RTK EVK with location fix and Ethernet connected](img/Displays/SparkFun_RTK_EVK_-_Main_Display_Icons.jpg)

*RTK EVK with location fix and Ethernet connected*

Upon power up the device will enter either Rover mode or Base mode. Above, the Rover mode is displayed.

* **MAC:** The MAC address of the internal Bluetooth module. This is helpful knowledge when attempting to connect to the device from your phone. This will change to a Bluetooth symbol once connected.
* **HPA:** Horizontal positional accuracy is an estimate of how accurate the current positional readings are. This number will decrease rapidly after the first power-up and settle around 0.3m depending on your antenna and view of the sky. When RTK fix is achieved this icon will change to a double circle and the HPA number will decrease even further to as low as 0.014m.
* **SIV:** Satellites in view is the number of satellites used for the fix calculation. This symbol will blink before a location fix is generated and become solid when the device has a good location fix. SIV is a good indicator of how good of a view the antenna has. This number will vary but anything above 10 is adequate. We've seen as high as 31.
* **Model:** This icon will change depending on the selected dynamic model: Portable (default) Pedestrian, Sea, Bike, Stationary, etc.
* **Log:** This icon will remain animated while the log file is increasing. This is a good visual indication that you have an SD card inserted and RTK Facet can successfully record to it. There are three logging icons ![Logging icons](img/Displays/SparkFun%20RTK%20Logging%20Types.png)
	* Standard (three lines) is shown when the standard 5 NMEA messages are being logged
	* PPP (capital P) is shown when the standard 5 NMEA + RAWX and SFRBX messages are recorded. This is most often used for post process positioning (PPP) and 12 to 24-hour logs for [fixed permanent bases](permanent_base.md).
	* Custom (capital C) is shown when a custom set of messages are being recorded (not standard, and not PPP).
* **IP Address:** On the RTK EVK 1.3" OLED, the full Ethernet or WiFi IP Address is shown at the bottom left of the display.

## Rover RTK Fix

![Rover with RTK Fix and Bluetooth connected](img/Displays/SparkFun_RTK_Express_-_Display_-_Rover_RTK_Fixed.jpg)

*Rover with RTK Fix and Bluetooth connected*

![RTK EVK with RTK Fix and Ethernet connected](img/Displays/SparkFun_RTK_EVK_LBand_Indicator.png)

*RTK EVK with RTK Fix and Ethernet connected*

Once NTRIP is enabled on your phone or RTCM data is being streamed into the **Radio** port the device will gain an RTK Fix. You should see the HPA drop to 14mm with a double circle bulls-eye as shown above.

## Base Survey-In

![RTK Facet in Survey-In Mode](img/Displays/SparkFun_RTK_Express_-_Display_-_Survey-In.jpg)

*RTK device in Survey-In Mode*

Pressing the Setup button will change the device to Base mode. If the device is configured for *Survey-In* base mode, a flag icon will be shown and the survey will begin. The mean standard deviation will be shown as well as the time elapsed. For most Survey-In setups, the survey will complete when both 60 seconds have elapsed *and* a mean of 5m or less is obtained.

## Base Transmitting

![RTK Facet in Fixed Transmit Mode](img/Displays/SparkFun_RTK_Express_-_Display_-_FixedBase-Xmitting.jpg)

*RTK Facet in Fixed Transmit Mode*

Once the *survey-in* is complete the device enters RTCM Transmit mode. The number of RTCM transmissions is displayed. By default, this is one per second.

The *Fixed Base* mode is similar but uses a structure icon (shown above) to indicate a fixed base.

## Base Transmitting NTRIP

If the NTRIP server is enabled the device will first attempt to connect over WiFi. The WiFi icon will blink until a WiFi connection is obtained. If the WiFi icon continually blinks be sure to check your SSID and PW for the local WiFi.

![RTK Facet in Transmit Mode with NTRIP](img/Displays/SparkFun_RTK_Express_-_Display_-_FixedBase-Casting.jpg)

*RTK Facet in Transmit Mode with NTRIP*


Once WiFi connects the device will attempt to connect to the NTRIP mount point. Once successful the display will show 'Casting' along with a solid WiFi icon. The number of successful RTCM transmissions will increase every second.

Note: During NTRIP transmission WiFi is turned on and Bluetooth is turned off. You should not need to know the location information of the base so Bluetooth should not be needed. If necessary, USB can be connected to the USB port to view detailed location and ZED-F9P configuration information.

## L-Band

<!--
Compatibility Icons
====================================================================================

:material-radiobox-marked:{ .support-full title="Feature Supported" }
:material-radiobox-indeterminate-variant:{ .support-partial title="Feature Partially Supported" }
:material-radiobox-blank:{ .support-none title="Feature Not Supported" }
-->

<div class="grid cards fill" markdown>

- Torch: [:material-radiobox-blank:{ .support-none }]( title ="Feature Not Supported" )
- EVK: :material-radiobox-marked:{ .support-full title="Feature Supported" }

</div>

L-Band decryption keys are valid for a maximum of 56 days. During that time, the RTK Facet L-Band can operate normally without the need for WiFi access. However, when the keys are set to expire in 28 days or less, the RTK Facet L-Band will attempt to log in to the 'Home' WiFi at each power on. If WiFi is not available, it will continue normal operation.

![Display showing 14 days until L-Band Keys Expire](img/Displays/SparkFun_RTK_LBand_DayToExpire.jpg)

*Display showing 14 days until L-Band Keys Expire*

The unit will display various messages to aid the user in obtaining keys as needed.

![Three-pronged satellite dish indicating L-Band reception](img/Displays/SparkFun_RTK_LBand_Indicator.jpg)

*Three-pronged satellite dish indicating L-Band reception*

![Three-pronged satellite dish indicating L-Band reception on RTK EVK](img/Displays/SparkFun_RTK_EVK_LBand_Indicator.png)

*Three-pronged satellite dish indicating L-Band reception on RTK EVK*

Upon successful reception and decryption of L-Band corrections, the satellite dish icon will increase to a three-pronged icon. As the unit's fix increases the cross-hair will indicate a basic 3D solution, a double blinking cross-hair will indicate a floating RTK solution, and a solid double cross-hair will indicate a fixed RTK solution.

## Antenna Detection

<!--
Compatibility Icons
====================================================================================

:material-radiobox-marked:{ .support-full title="Feature Supported" }
:material-radiobox-indeterminate-variant:{ .support-partial title="Feature Partially Supported" }
:material-radiobox-blank:{ .support-none title="Feature Not Supported" }
-->

<div class="grid cards fill" markdown>

- Torch: [:material-radiobox-blank:{ .support-none }]( title ="Feature Not Supported" )
- EVK: :material-radiobox-marked:{ .support-full title="Feature Supported" }

</div>

On devices that support it, open circuit and short circuit detection is available on the GNSS antenna connection.

![RTK EVK indicating antenna open circuit](img/Displays/SparkFun-RTK-EVK-OLED_Antenna_Open_Highlighted.jpg)

*RTK EVK with the GNSS antenna disconnected (open circuit)*

![RTK EVK indicating antenna short circuit](img/Displays/SparkFun-RTK-EVK-OLED_Antenna_Short_Highlighted.jpg)

*RTK EVK with a GNSS antenna cable fault (short circuit)*
