# Updating UM980 Firmware

Torch: ![Feature Supported](img/Icons/GreenDot.png) / EVK: ![Feature Not Supported](img/Icons/RedDot.png)

The UM980 is the GNSS receiver inside the RTK Torch. The following describes how to update the firmware on the UM980.

1. Download the latest UM980 firmware. As of writing, the UM980 firmware is [v11833](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Torch/main/UM980_Firmware/UM980_R4.10Build11833.pkg). See the [RTK Torch repo](https://github.com/sparkfun/SparkFun_RTK_Torch) for the latest firmware.

2. Download and install [UPrecise](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Torch/main/UM980_Firmware/uprecise-v2-0.exe). UPrecise is the software created by Unicore to configure and view output from Unicore GNSS modules. v2 of the software can be downloaded [here](https://raw.githubusercontent.com/sparkfun/SparkFun_RTK_Torch/main/UM980_Firmware/uprecise-v2-0.exe) but newer versions may be available.

    ![Hardware menu showing pass through option](<img/UPrecise/SparkFun RTK Everywhere - UM980 Passthrough.png>)

    *Entering direct connect via Serial Configuration* 

3. Put the device into passthrough mode. From the main menu navigate to System 's'->Hardware 'h'->UM980 Direct Connect '13'. This will put the device into a pass-through mode, but with special character monitoring that allows the UM980 to be reset when bootloading is detected. Close the terminal connection so that UPrecise can connect to the COM port. 
    
    ![Startup message showing passthrough mode](<img/UPrecise/SparkFun RTK Everywhere - UM980 Passthrough Startup.png>)

    *Startup message showing passthrough mode*

    **Note:** The RTK device will stay in this passthrough mode until the external power button is pressed and released. The device will beep when exiting passthrough mode.

    ![Connect button and connection settings in UPrecise](<img/UPrecise/SparkFun RTK Everywhere - UPrecise Inteface Connect.png>)

    *Connect button and connection settings in UPrecise*

4. Select the COM port and set the baud rate to 115200, then press 'Connect'

    ![Receiver Upgrade Button](<img/UPrecise/SparkFun RTK Everywhere - UPrecise Inteface Receiver Upgrade.png>)

    *Receiver Upgrade Button*

5. Once connected, press the **Receiver Upgrade** button.

    ![Firmware selection window](<img/UPrecise/SparkFun RTK Everywhere - UPrecise Inteface Firmware Selection.png>)

    *Firmware selection window*

6. Select the latest firmware file that was previously downloaded (See the [RTK Torch repo](https://github.com/sparkfun/SparkFun_RTK_Torch) for the latest firmware). Then press Start.

    ![Firmware update process](<img/UPrecise/SparkFun RTK Everywhere - UPrecise Inteface Firmware Upload.png>)

    *Firmware update process*

7. The update process can take multiple minutes. Once completed, close UPrecise, and power cycle the RTK Torch.

    ![UM980 Firmware version shown in System Menu](<img/Terminal/SparkFun RTK Everywhere - System Menu UM980 Firmware.png>)

    *UM980 Firmware version shown in System Menu*

8. Upon power-up, the firmware will be updated. Open the System Menu to confirm the firmware version.
