# Configure with Bluetooth

<!--
Compatibility Icons
====================================================================================

:material-radiobox-marked:{ .support-full title="Feature Supported" }
:material-radiobox-indeterminate-variant:{ .support-partial title="Feature Partially Supported" }
:material-radiobox-blank:{ .support-none title="Feature Not Supported" }
-->

<div class="grid cards fill" markdown>

- Torch: :material-radiobox-marked:{ .support-full title="Feature Supported" }
- EVK: :material-radiobox-marked:{ .support-full title="Feature Supported" }

</div>

![Configuration menu open over Bluetooth](img/Bluetooth/SparkFun%20RTK%20BEM%20-%20Config%20Menu.png)

*Configuration menu via Bluetooth*

Bluetooth-based configuration provides a quick and easy way to navigate the serial menus as if you were connected over a USB cable. For regular users, this is often the preferred configuration method.

The RTK device will be a discoverable Bluetooth device (both BT SPP and BLE are supported). For information about Bluetooth pairing, please see [Connecting Bluetooth](connecting_bluetooth.md).

## Entering Bluetooth Echo Mode

![Entering the BEM escape characters](img/Bluetooth/SparkFun%20RTK%20BEM%20-%20EscapeCharacters.png)

Once connected, the RTK device will report a large amount of NMEA data over the link. To enter Bluetooth Echo Mode send the characters +++.

!!! note
	There must be a 2 second gap between any serial sent from a phone to the RTK device, and any escape characters. In almost all cases this is not a problem. Just be sure it's been 2 seconds since an NTRIP source has been turned off and attempting to enter Bluetooth Echo Mode.

![The GNSS message menu over BEM](img/Bluetooth/SparkFun%20RTK%20BEM%20-%20Config%20Menu.png)

*The GNSS Messages menu shown over Bluetooth Echo Mode*

Once in Bluetooth Echo Mode, any character sent from the RTK unit will be shown in the Bluetooth app, and any character sent from the connected device (cell phone, laptop, etc) will be received by the RTK device. This allows the opening of the config menu as well as the viewing of all regular system output.

For more information about the Serial Config menu please see [Configure with Serial](configure_with_serial.md).

![System output over Bluetooth](img/Bluetooth/SparkFun%20RTK%20BEM%20-%20System%20Output.png)

*Exit from the Serial Config Menu*

Bluetooth can also be used to view system status and output. Simply exit the config menu using option 'x' and the system output can be seen.

## Exit Bluetooth Echo Mode

To exit Bluetooth Echo Mode simply disconnect Bluetooth. In the Bluetooth Serial Terminal app, this is done by pressing the 'two plugs' icon in the upper right corner.

![Exiting Bluetooth Echo Mode](img/Bluetooth/SparkFun%20RTK%20BEM%20-%20Exit%20BEM.png)

*Menu option 'b' for exiting Bluetooth Echo Mode*

Alternatively, if you wish to stay connected over Bluetooth but need to exit Bluetooth Echo Mode, use the 'b' menu option from the main menu.

## Serial Bluetooth Terminal Settings

Here we provide some settings recommendations to make the terminal navigation of the RTK device a bit easier.

![Disable Time stamps](img/Bluetooth/SparkFun%20RTK%20BEM%20-%20Settings%20Terminal.png)

*Terminal Settings with Timestamps disabled*

Disable timestamps to make the window a bit wider, allowing the display of longer menu items without wrapping.

![Clear on send](img/Bluetooth/SparkFun%20RTK%20BEM%20-%20Settings.png)

*Clear on send and echo off*

Clearing the input box when sending is very handy as well as turning off local echo.
