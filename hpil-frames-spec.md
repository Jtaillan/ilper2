# HP-IL — Remote Message Reference

## Message Glossary

### Remote Messages

---

#### AAD*n* — Auto address *n*

**Auto address group, ready class** — `101 100aaaaa`

The controller sends AAD*n* to assign simple (one byte) addresses to devices on the loop. The lower five bits represent a binary coded address number *n* which can range from 0 to 30 (31 is an illegal address). Each device accepts the incoming value *n* as its address, increments this value, and sends the modified AAD to the next device on the loop, which, in turn, does the same. Once a device has received its address in this way, it will no longer respond to any AAG (auto address group) message until after the AAU command is received or the device is powered off, then on again. The controller uses the address value which returns after going through each device around the loop to determine the number of devices, or to determine if there are too many devices.

---

#### AAG — Auto address group

**Ready class** — `101 1xxxxxxx`

This mnemonic indicates the entire group of auto address ready frames, including simple address, extended and multiple secondary address assignment frames. The controller uses these to assign addresses to devices on the loop in various ways.

---

#### AAU — Auto address unconfigure

**Universal command group, command class** — `100 10011010`

The controller uses this command to cause all devices to reset their address assignments. After an AAU, devices must respond to either address switches or a preset address. If loop devices already have addresses assigned, the controller must send the AAU message before assigning new addresses to devices. Note that IFC (interface clear) does not affect address assignments in any way.

---

#### ACG — Addressed command group

**Command class** — `100 x000xxxx` or `100 101xxxxx` or `100 110xxxxx`

This mnemonic indicates that group of commands to which a device does not respond unless it is addressed as a talker or a listener, as well as others.

---

#### AEP*n* — Auto extended primary *n*

**Auto address group, ready class** — `101 101aaaaa`

After the controller has assigned secondary addresses with the AES*n* message to a group of devices which can accept extended addresses, it uses the AEP*n* message to assign the same primary address to each device in the group. Devices do not modify this message; they merely accept the address assignment and send the message to the next device. Other devices do not respond to this message. After the AEP, the device is configured and can respond to its assigned secondary and primary addresses. AAU will reset the address assignment and ready the device to receive a new address. `aaaaa` represents the five-bit binary address *n*, which can range from 0 to 30 (31 is an illegal address; devices will not respond to 31).

---

#### AES*n* — Auto extended secondary *n*

**Auto address group, ready class** — `101 110aaaaa`

AES is used by the controller to assign secondary addresses to extended addressable devices. The lower five bits contain the binary address *n*, which can range from 0 to 30 (31 is illegal). Each device accepts the value *n* as its secondary address, increments this value, and sends the modified message on to the next device. When the value reaches 31, no other devices respond and the message simply returns to the controller. The controller can then use AEP*n* to assign the primary address to this group of devices. Once configured, the devices can no longer respond to the AES*n*, so the controller can now send it out again to assign extended addresses to the next group of devices on the loop. The primary address for each group must, of course, be unique.

---

#### AMP*n* — Auto multiple primary

**Auto address group, ready class** — `101 111aaaaa`

AMP assigns primary addresses to all devices which use multiple addressing on the loop. The lower five bits represent a binary address *n*, which can range from 0 to 30 (31 is illegal). The controller sends the AMP message and each succeeding device accepts the incoming value as its new address, increments *n*, and sends the message to the next device, which, in turn, does the same. The value which returns to the controller indicates the number of multiple address devices on the loop. Following this, the controller sends the ZES command to each device so that it can reserve the proper sized block of secondary addresses. The device is then configured and can respond to its assigned addresses. AAU is necessary before devices will respond to new address assignments.

---

#### ARG — Addressed ready group

**Ready class** — `101 01xxxxxx`

Only talkers, listeners, and controllers may respond to this group of messages. Idle devices must ignore (retransmit) these messages. - messages include SOT (start of transmission), EOT (end of transmission), and NRD (not ready for data) subgroups. With the exception of NRD, these messages do not normally travel all the way around the loop back to the sourcing device. In general, they serve a handshake function and the destination device replaces them with another message. At present, listeners do not respond to these messages, but may source the NRD message if enabled by the controller.

---

#### CMD — Command

**`100 xxxxxxxx`**

Commands are one of the major types of loop messages. They control the operation of the interface functions of each device in a major way, and to a lesser extent, the device functions also. The controller is the only device which may source command messages (except for asynchronous IFC by the system controller). Every command must be immediately followed by the RFC message to provide devices the opportunity to handshake, that is, to indicate they are ready to receive the next command. Commands are immediately retransmitted by all devices to minimize delay but a copy of the message is saved by each device to begin execution of the command.

---

#### DAB — Data byte

**Data or end class** — `00x xxxxxxxx`

Data bytes are the basic unit of the device dependent message transmission. This is the data which the interface system is designed to handle. The other messages are largely overhead for control purposes. These messages between devices may be coded in any way but it is strongly recommended that ASCII be used wherever possible for compatibility reasons. The data byte also contains the SRQ bit (C0) which devices may set to indicate to the controller a need for service.

---

#### DCL — Device clear

**Universal command group, command class** — `100 00010100`

DCL is sent by the controller to cause all devices which recognize this command to set their device functions to a preset state, whether they are addressed or not. This command does not affect the interface functions. The preset state is defined by the device designer and is normally the same as the power-on state.

---

#### DDL*n* — Device dependent listener command *n*

**Addressed command group, command class** — `100 101xxxxx`

A device must be addressed as a listener in order to respond to any one of the 32 possible DDL commands. The particular effect of the specific command is designer determined but it must not directly affect any interface functions.

---

#### DDT*n* — Device dependent talker command *n*

**Addressed command group, command class** — `100 110xxxxx`

A device must be addressed as a talker in order to respond to any one of the 32 possible DDT commands. The particular effect of the specific command is designer determined but it must not directly affect any interface functions.

---

#### DOE — Data or end

**`0xx xxxxxxxx`**

This major frame classification includes all the device dependent messages for the interface system. This is the data which is communicated from one device to another and for which the system was designed. DOE frames include the END bit (C1) to indicate an end-of-record condition without terminating the transmission, and the SRQ bit (C0) for devices to indicate a need for service to the active controller. These messages are sourced by the active talker and are received by the active listener(s) on the loop.

---

#### EAR — Enable asynchronous requests

**Universal command group, command class** — `100 00011000`

This command is used by the active controller to put all devices which have the capability in a mode where they can source their own IDY message (with service request bit set) to indicate a need for service to the controller. The controller is the only device to source IDY frames normally. After the controller sends the EAR–RFC sequence, it will allow the loop to go idle, until such time as an asynchronous IDY from one of the devices arrives or until the controller must perform some other operation. To disable the asynchronous request mode, the controller must send out a universal command which disables the mode and then resume normal operation. All universal commands except EAR and LPD disable the mode. However, it is recommended that controllers use the NOP command as it has no affect on the other interface functions. LPD does not disable the mode so that it is possible to have some device other than the active controller wake up a powered down loop.

---

#### ELN — Enable listener not ready

**Addressed command group, command class** — `100 00001111`

This command enables listeners to halt data transfers when necessary. Any device that is active to listen may respond to the ELN command.

---

#### END — End data byte

**Data or end class** — `01x xxxxxxxx`

The end byte is the same as a data byte except that bit C1 is set to indicate an end-of-record condition to the listener. This has no affect on the interface functions and the end byte is treated exactly the same as any other data byte. Most ASCII transmissions will indicate end-of-line with a CR, LF pair, for example, but binary data will probably use the END message for this function. The END byte does not terminate the transmission.

---

#### EOT — End of transmission

**Addressed ready group, ready class** — `101 0100000x`

This is a subgroup including two messages, ETE (end of transmission with error) and ETO (end of transmission, OK). The active talker sources these messages to indicate the end of a data transfer to the controller. The controller replaces the EOT message with the next interface message. Listeners must ignore (retransmit) the EOT messages.

---

#### ETE — End of transmission, error

**Addressed ready group, ready class** — `101 01000001`

This message is sourced by the active talker to indicate to the controller that a data message sent by the talker has returned in error. This error checking capability is strongly recommended but not required. If the device does not perform error checking, it may not source this message. The controller replaces this message with its next operation on the loop, possibly an attempt to restart the transmission.

---

#### ETO — End of transmission, OK

**Addressed ready group, ready class** — `101 01000000`

This message is sourced by the active talker to indicate to the active controller that it is no longer actively sourcing data. Though not required, error checking is strongly recommended. If error checking is not performed, the talker must assume that no errors have occurred and end its data transfer with the ETO message. The ETO message does not return to the talker; instead the controller replaces it with the next interface message. Active listeners must ignore (retransmit) the ETO message.

---

#### GET — Group execute trigger

**Addressed command group, command class** — `100 00001000`

GET is a command used by the controller to cause all devices which are listener addressed to begin their particular device operation. This operation for each device is designer specified. The controller may use the GET command to start an operation in several devices as nearly at the same time as is possible given the loop architecture of the system.

---

#### GTL — Go to local

**Addressed command group, command class** — `100 00000001`

The controller uses this command to put all devices which are listener addressed under control of their local (front panel) controls. Programming data for the device will, in general, be ignored while the device is in this state, if it is received from the interface.

---

#### IAA — Illegal auto address

**Auto address group, ready class** — `101 10011111`

If the controller receives this message as a result of assigning addresses with the AAD message, there may be exactly the maximum or too many devices on the loop. Since devices do not respond to the IAA message, one or more devices may not have been assigned an address. To test whether all devices have been assigned an address, the controller should send the AAD30 message. If AAD30 is returned, then exactly the maximum number of devices are on the loop and the controller may begin normal operations. If IAA is returned again, then too many devices are present and more than one device may respond to a particular address. Before normal operations may begin, all extra devices must either be removed from the loop or assigned to addresses from an unused address range. For example, the extra devices can all be assigned address 30 by repeatedly sending AAD30 until it returns unchanged.

---

#### IDY — Identify

**`11x xxxxxxxx`**

This major classification of messages is used by the controller to perform parallel poll or to check for service request. Bit C0 is set by devices which need service. If the controller has configured devices to respond to parallel poll, these devices set designated data bits in the IDY message as it passes through the device. With this capability, the controller can rapidly identify which device needs service. During normal operations IDY messages may only be sourced by the active controller, but other devices can be enabled to send IDY frames (with the service request bit set) with the EAR command.

---

#### IEP — Illegal extended primary

**Auto address group, ready class** — `101 10111111`

This message is the same as the AEP message except that the address value is 31, an illegal value. Devices will not respond to this message. Furthermore, since AEP is not incremented by the loop devices, it will not be received by the controller and it is included here only for consistency.

---

#### IES — Illegal extended secondary

**Auto address group, ready class** — `101 11011111`

Extended address devices receive their secondary address assignments via the AES message, which they accept, increment, and send on to the next device. When the address value reaches 31, it is defined as the IES message and is no longer accepted by other devices, which pass it unchanged back to the controller. The controller then assigns primary addresses with the AEP message, which is not incremented. Only devices which have received their secondary address (and have not yet received the AEP message) will respond. After receiving both secondary and primary addresses, devices will no longer respond to AAG messages and the controller may configure the next group of devices. If the controller generates the IEP message internally and the AES message returns incremented, there are too many devices on the loop.

---

#### IFC — Interface clear

**Universal command group, command class** — `100 10010000`

IFC may be sourced only by the system controller. It may be sent at any time to take control of the interface system. IFC resets all talker, listener, and controller functions on the loop to their idle state, but does not affect any other interface or device functions. IFC also must not affect the parallel poll or address assignment.

---

#### IMP — Illegal multiple primary

**Auto address group, ready class** — `101 11111111`

The controller uses the AMP message to assign primary addresses to those devices which have multiple address capability. If there are exactly the maximum number or too many devices of this type on the loop, the AMP message will be incremented to 31, which is defined as the IMP message. No further devices will respond and the message will return to the controller. The controller should send AMP30 at this point. If it returns unchanged, the loop has exactly the maximum number of devices. If it returns modified, there are too many. The controller should signal an error condition and not attempt normal operations.

---

#### LAD*n* — Listen address *n* command

**Listen address group, command class** — `100 001aaaaa`

This is the command that the controller uses to cause a particular device to become the active listener, that is, able to receive and interpret data messages from the loop. The lower five bits represent a binary address *n*, which can range from 0 to 30. 31 is an illegal address which causes all listeners to go to the idle state; it is called the unlisten command. For devices which use a two byte address, LAD provides the primary address only. The MSA command must be received to make these devices active listeners. Multiple LAD messages will activate multiple listeners.

---

#### LAG — Listen address group

**Command class** — `100 001xxxxx`

This group of commands includes all the listen address commands and also the unlisten command (address 31). Secondary address commands are in a separate group; only primary addresses are included in LAG.

---

#### LLO — Local lockout

**Universal command group, command class** — `100 00010001`

With this command the controller can cause all devices which respond to this command to lock out, or not respond to, their return to local control buttons on the instruments. This will prevent an operator from changing a device's control settings inadvertently at a critical time.

---

#### LPD — Loop power down

**Universal command group, command class** — `100 10011011`

The controller uses this command to place the loop, or rather all devices which respond to this command, in a power-down state to conserve power. The controller remains powered up so that it can wake up the loop at a later time and continue normal operations. If the controller has enabled the asynchronous request mode prior to sending the LPD, other devices with the proper capability can also initiate the loop wake-up sequence.

---

#### MLA — My listen address

**Listen address group, command class**

This is the particular LAD*n* command which happens to match in the least significant five bits with the address (primary address in the case of devices that have a two byte address) which is assigned to this specific device. This command causes the device to become the active listener and able to receive device dependent messages. In the case of devices which require two byte addresses, the MSA code is also required before the device becomes active.

---

#### MSA — My secondary address

**Secondary address group, command class** — `100 011mmmmm`

This is the particular SAD*n* command which matches the secondary address assigned to this specific device. This command causes the device to become the active talker or to become an active listener. This command must immediately follow the MTA or MLA command.

---

#### MTA — My talk address

**Talk address group, command class** — `100 010mmmmm`

This is the particular TAD*n* command which matches the talk address (primary address in the case of devices that have a two byte address) which is assigned to this specific device. When the device receives MTA, it becomes the active talker on the loop, and, when enabled, will source device dependent data on the loop. For devices which have a two byte address, the MSA command is also required before the device becomes addressed to talk. This command causes the previous talker to become idle.

---

#### NAA — Next auto address

**Auto address group, ready class** — `101 100nnnnn`

This mnemonic represents the incremented auto address message that a device sends on to the next device on the loop. The value of the lower five bits might range from 1 to 31, depending on the value of the AAD message before incrementing. The value 31 is also called IAA; values less than 31 are also called AAD.

---

#### NES — Next extended secondary

**Auto address group, ready class** — `101 110nnnnn`

The NES frame is the incremented secondary address assignment which devices send to the next loop device. It is used by devices with extended or multiple address capability. The address bits (`nnnnn`) can range from 1 to 31.

---

#### NMP — Next multiple primary

**Auto address group, ready class** — `101 111nnnnn`

This mnemonic represents the incremented primary address message that multiple address devices send on to the next device on the loop. The lower five bits can range from 1 to 31 depending on the value of the received AMP message. The address value 31 is also known as IMP and values less than 31 are also called AMP.

---

#### NOP — No operation

**Universal command group, command class** — `100 00010000`

This is the universal no operation command. It is useful for disabling asynchronous request mode since any universal command disables it and the NOP command has no other effects.

---

#### NRD — Not ready for data

**Addressed ready group, ready class** — `101 01000010`

When the controller or a device enabled by the controller needs to interrupt an active talker during a data transmission, it does so by holding the next data byte and replacing it with the NRD message. This message signals the talker that it should terminate its transmission. When the NRD returns to the sourcing device, it will then send the held data message. When this data message is received by the talker, it sends the EOT message to the controller. If an active talker is directed to continue the data transfer (with a send data message), it must continue at the point of interruption unless directed otherwise in a device dependent manner defined for that purpose.

---

#### NRE — Not remote enable

**Universal command group, command class** — `100 10010011`

With this command, the controller causes all devices to be placed under local control; that is, they will respond to their front panel controls and not to programming information received from the loop. Devices which do not implement the remote local interface function will simply ignore this command.

---

#### NUL — Null command

**Addressed command group, command class** — `100 00000000`

This is the addressed no operation command. Devices do not perform any action in response to this command so it is useful for such operations as testing the loop handshaking.

---

#### OSA — Other secondary address

**Secondary address group, command class** — `100 011ttttt`

This mnemonic represents any secondary address command whose address bits (`ttttt`) do not match the address assigned to this particular device. It causes talker functions to become unaddressed.

---

#### OTA — Other talk address

**Talk address group, command class** — `100 010ttttt`

This mnemonic represents any talk address command that contains an address (`ttttt`) that does not match the address assigned to this particular device. Since there can only be one active talker on the loop at a time, this device will return its talker function to the idle state.

---

#### PPD — Parallel poll disable

**Addressed command group, command class** — `100 00000101`

This command is used by the controller to cause the devices which are listen addressed to no longer respond to parallel polls. The parallel poll function returns to its idle state.

---

#### PPE*n* — Parallel poll enable *n*

**Addressed command group, command class** — `100 1000sbbb`

This command allows the controller to configure devices that are addressed to listen to respond to parallel polls in various ways. The `s` bit indicates the sense of the device's response. If `s` is 1 the device will set the assigned bit of an IDY frame if it needs service; if `s` is 0 the device will set the assigned bit if it does not need service. The lower three bits (`bbb`) indicate the binary bit number on which the device must respond: `000` indicates bit D0, `001` indicates D1, ..., `111` indicates D7.

---

#### PPU — Parallel poll unconfigure

**Universal command group, command class** — `100 00010101`

The controller uses this command to disable all devices on the loop from responding to parallel polls. The parallel poll function in each device will return to its idle state.

---

#### RDY — Ready

**`101 xxxxxxxx`**

This major class of messages is used for several different purposes, including device handshake functions and address configuration. Most are sourced by the active controller, but some may be sourced by other devices under certain conditions.

---

#### REN — Remote enable

**Universal command group, command class** — `100 10010010`

With this command, the controller enables the loop for remote operation. When a device is addressed to listen, it enters its remote control state and will no longer respond to its front panel controls. Devices which do not implement this function will simply retransmit this command and take no other action.

---

#### RFC — Ready for command

**Ready class** — `101 00000000`

The controller uses this ready frame as the handshake after each command so that it knows that all devices on the loop have received the command and are ready to receive the next. Every command must be immediately followed by the RFC message.

---

#### SAD*n* — Secondary address *n* command

**Secondary address group, command class** — `100 011aaaaa`

This command provides the secondary address to enable talkers and listeners which respond to two byte addresses. The lower five bits represent the binary address *n*, which can range from 0 to 30. 31 is an illegal address. The secondary address must follow the talk or listen address command immediately to enable the device to talk or listen.

---

#### SAG — Secondary address group

**Command class** — `100 011xxxxx`

This group of commands contains the secondary addresses, that is, SAD commands. These are only used for devices which respond to extended or multiple address modes.

---

#### SAI — Send accessory identification

**Addressed ready group, ready class** — `101 01100011`

This message is used by the controller to cause the addressed talker to begin sending its accessory ID byte(s). The talker replaces the SAI with the accessory ID on the loop, and terminates with the proper EOT message, just as in a data transmission. If the device does not have accessory ID capability, it merely sends the SAI back to the controller to indicate that the device cannot respond. Accessory ID consists of a single byte whose high order four bits indicate the device class (e.g. printer, mass storage, etc.) and low order four bits represent the device type.

---

#### SDA — Send data

**Addressed ready group, ready class** — `101 01100000`

This message is used by the controller to direct the addressed talker to begin sending its data. The talker replaces the SDA with its first byte of data and continues to send data until no more is available. The talker follows its last data byte with the proper EOT message. If the device cannot source data, it merely returns the SDA to the controller. If the talker has no data ready, it sends ETO.

---

#### SDC — Selected device clear

**Addressed command group, command class** — `100 00000100`

The SDC command causes active listeners to perform their device dependent clear function. SDC has no affect on other interface functions.

---

#### SDI — Send device ID

**Addressed ready group, ready class** — `101 01100010`

SDI is used by the controller to cause the talker addressed device to begin sending its device ID string. The talker replaces the SDI with its ID string on the loop and terminates the transmission with the proper EOT message. The ID string consists of ASCII characters followed by carriage return and linefeed. Typically, the ID has the following form: two characters for the manufacturer code, up to five characters for the model number, one model number revision character, and optionally other information that describes options or capabilities. If a device does not implement device ID, the SDI message will simply be returned to the controller.

---

#### SOT — Start of transmission

**Addressed ready group, ready class** — `101 01100xxx`

This subgroup of messages includes the SDA, SST, SDI, SAI, and TCT messages. They all serve to enable the beginning of transmission from a device other than the controller (except for TCT, which enables the new controller). The other device will source the proper EOT message when finished to signal the controller to take over once again. In the case of TCT (take control) the new controller remains in control of the loop and does not source the EOT. This group of messages does not go completely around the loop, but is replaced by the first message from the enabled device.

---

#### SRQ — Service request

**Data or end class, or identify class** — `0x1 xxxxxxxx` or `111 xxxxxxxx`

The SRQ bit is bit C0 of data, end, and identify messages. Devices may set this bit when they have a need for service from the controller. The bit then represents the logical OR of the various devices' individual service bits. The controller will generally need to perform a serial poll operation to find out which device needs service. The command and ready classes of messages do not have a service request bit and, therefore, do not transmit this message.

---

#### SST — Send status

**Addressed ready group, ready class** — `101 01100001`

The controller uses this message to cause the addressed talker to begin sending its status byte(s). The talker replaces the SST with its status information on the loop. When finished, the talker then transmits the proper EOT message. Two bits of the first byte of status are reserved for specific purposes. If bit D7 (msb) is set, then the first byte of status represents a coded system status message. If it is clear, the lower 6 bits (D5–D0) are device dependent. Bit D6 of the first status byte is always equal to the device's local message SRQ. If the device does not implement serial poll (status), the SST frame is simply returned to the controller.

---

#### TAD*n* — Talk address *n* command

**Talk address group, command class** — `100 010aaaaa`

The controller uses this command to enable one device on the loop to be the active talker. The lower five bits (`aaaaa`) represent the device's address (primary address in the case of those devices which have a two byte address). Address values can range from 0 to 30. TAD with address 31 is the UNT (untalk) command. If an addressed talker receives UNT or a TAD with an address that does not match its own, it must become untalked.

---

#### TAG — Talk address group

**Command class** — `100 010xxxxx`

This group includes all primary talk address commands and the untalk command. Secondary addresses are contained in the SAG group.

---

#### TCT — Take control

**Addressed ready group, ready class** — `101 01100100`

The active controller uses this message to pass control of the loop to another controller. The device to which control is passed must first be talk addressed, then the current controller sends the TCT message. Upon receipt of the TCT, the talker addressed device becomes the active controller, and replaces the TCT with its first interface message. If the device cannot accept control of the loop, it merely retransmits the TCT which returns to the current controller, which resumes active control of the loop.

---

#### UCG — Universal command group

**Command class** — `100 x001xxxx`

This group of commands includes all those to which all devices respond whether they are currently addressed or not.

---

#### UNL — Unlisten

**Listen address group, command class** — `100 00111111`

This command causes all addressed listeners to return to the idle state. The controller normally will use this command to reset the listeners before addressing a new listener(s) for the next data transmission.

---

#### UNT — Untalk

**Talk address group, command class** — `100 01011111`

This command causes the addressed talker to return to the idle state. It is sometimes necessary to have no talker addressed device on the loop.

---

#### ZES — Zero extended secondary

**Auto address group, ready class** — `101 11000000`

This message is used by the controller to assign secondary addresses to those devices which have multiple address capability. After each device has received its primary address via the AMP frame, it waits to recognize the ZES frame. When it is received, the low order five bits are incremented by the number of addresses reserved for this device and the frame is then sent back to the controller which now knows how many addresses there are in that device. Since the device will only respond to the ZES message, the controller sends it once for each device in turn. After this, all devices are configured and can respond normally to their assigned primary and secondary addresses.

---

## Message Coding

### Command Coding

Command frames have class bits `100`. The table below maps the lower eight data bits (D7–D0) to commands. Rows are D7–D4, columns are D3–D0 (hex).

|  | `x0` | `x1` | `x2` | `x3` | `x4` | `x5` | `x6` | `x7` | `x8` | `x9` | `xA` | `xB` | `xC` | `xD` | `xE` | `xF` |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `0x` | NUL | GTL | — | — | SDC | PPD | — | — | GET | — | — | — | — | — | — | ELN |
| `1x` | NOP | LLO | — | — | DCL | PPU | — | — | EAR | — | — | — | — | — | — | — |
| `2x` | LAD 0 | LAD 1 | LAD 2 | LAD 3 | LAD 4 | LAD 5 | LAD 6 | LAD 7 | LAD 8 | LAD 9 | LAD 10 | LAD 11 | LAD 12 | LAD 13 | LAD 14 | LAD 15 |
| `3x` | LAD 16 | LAD 17 | LAD 18 | LAD 19 | LAD 20 | LAD 21 | LAD 22 | LAD 23 | LAD 24 | LAD 25 | LAD 26 | LAD 27 | LAD 28 | LAD 29 | LAD 30 | UNL |
| `4x` | TAD 0 | TAD 1 | TAD 2 | TAD 3 | TAD 4 | TAD 5 | TAD 6 | TAD 7 | TAD 8 | TAD 9 | TAD 10 | TAD 11 | TAD 12 | TAD 13 | TAD 14 | TAD 15 |
| `5x` | TAD 16 | TAD 17 | TAD 18 | TAD 19 | TAD 20 | TAD 21 | TAD 22 | TAD 23 | TAD 24 | TAD 25 | TAD 26 | TAD 27 | TAD 28 | TAD 29 | TAD 30 | UNT |
| `6x` | SAD 0 | SAD 1 | SAD 2 | SAD 3 | SAD 4 | SAD 5 | SAD 6 | SAD 7 | SAD 8 | SAD 9 | SAD 10 | SAD 11 | SAD 12 | SAD 13 | SAD 14 | SAD 15 |
| `7x` | SAD 16 | SAD 17 | SAD 18 | SAD 19 | SAD 20 | SAD 21 | SAD 22 | SAD 23 | SAD 24 | SAD 25 | SAD 26 | SAD 27 | SAD 28 | SAD 29 | SAD 30 | — |
| `8x` | PPE0(0) | PPE0(1) | PPE0(2) | PPE0(3) | PPE0(4) | PPE0(5) | PPE0(6) | PPE0(7) | PPE1(0) | PPE1(1) | PPE1(2) | PPE1(3) | PPE1(4) | PPE1(5) | PPE1(6) | PPE1(7) |
| `9x` | IFC | — | REN | NRE | — | — | — | — | — | — | AAU | LPD | — | — | — | — |
| `Ax` | DDL 0 | DDL 1 | DDL 2 | DDL 3 | DDL 4 | DDL 5 | DDL 6 | DDL 7 | DDL 8 | DDL 9 | DDL 10 | DDL 11 | DDL 12 | DDL 13 | DDL 14 | DDL 15 |
| `Bx` | DDL 16 | DDL 17 | DDL 18 | DDL 19 | DDL 20 | DDL 21 | DDL 22 | DDL 23 | DDL 24 | DDL 25 | DDL 26 | DDL 27 | DDL 28 | DDL 29 | DDL 30 | DDL 31 |
| `Cx` | DDT 0 | DDT 1 | DDT 2 | DDT 3 | DDT 4 | DDT 5 | DDT 6 | DDT 7 | DDT 8 | DDT 9 | DDT 10 | DDT 11 | DDT 12 | DDT 13 | DDT 14 | DDT 15 |
| `Dx` | DDT 16 | DDT 17 | DDT 18 | DDT 19 | DDT 20 | DDT 21 | DDT 22 | DDT 23 | DDT 24 | DDT 25 | DDT 26 | DDT 27 | DDT 28 | DDT 29 | DDT 30 | DDT 31 |
| `Ex` | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| `Fx` | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — |


---

### Ready Coding

Ready frames have class bits `101`. The table below maps the lower eight data bits (D7–D0) to ready messages. Rows are D7–D4, columns are D3–D0 (hex).

|  | `x0` | `x1` | `x2` | `x3` | `x4` | `x5` | `x6` | `x7` | `x8` | `x9` | `xA` | `xB` | `xC` | `xD` | `xE` | `xF` |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `0x` | RFC | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| `1x` | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| `2x` | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| `3x` | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — | — |
| `4x` | ETO | ETE | NRD | - | - | - | - | - | - | - | - | - | - | - | - | - |
| `5x` | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - |
| `6x` | SDA | SST | SDI | SAI | TCT | - | - | - | - | - | - | - | - | - | - | - |
| `7x` | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - |
| `8x` | AAD 0 | AAD 1 | AAD 2 | AAD 3 | AAD 4 | AAD 5 | AAD 6 | AAD 7 | AAD 8 | AAD 9 | AAD 10 | AAD 11 | AAD 12 | AAD 13 | AAD 14 | AAD 15 |
| `9x` | AAD 16 | AAD 17 | AAD 18 | AAD 19 | AAD 20 | AAD 21 | AAD 22 | AAD 23 | AAD 24 | AAD 25 | AAD 26 | AAD 27 | AAD 28 | AAD 29 | AAD 30 | IAA |
| `Ax` | AEP 0 | AEP 1 | AEP 2 | AEP 3 | AEP 4 | AEP 5 | AEP 6 | AEP 7 | AEP 8 | AEP 9 | AEP 10 | AEP 11 | AEP 12 | AEP 13 | AEP 14 | AEP 15 |
| `Bx` | AEP 16 | AEP 17 | AEP 18 | AEP 19 | AEP 20 | AEP 21 | AEP 22 | AEP 23 | AEP 24 | AEP 25 | AEP 26 | AEP 27 | AEP 28 | AEP 29 | AEP 30 | IEP |
| `Cx` | AES 0 | AES 1 | AES 2 | AES 3 | AES 4 | AES 5 | AES 6 | AES 7 | AES 8 | AES 9 | AES 10 | AES 11 | AES 12 | AES 13 | AES 14 | AES 15 |
| `Dx` | AES 16 | AES 17 | AES 18 | AES 19 | AES 20 | AES 21 | AES 22 | AES 23 | AES 24 | AES 25 | AES 26 | AES 27 | AES 28 | AES 29 | AES 30 | IES |
| `Ex` | AMP 0 | AMP 1 | AMP 2 | AMP 3 | AMP 4 | AMP 5 | AMP 6 | AMP 7 | AMP 8 | AMP 9 | AMP 10 | AMP 11 | AMP 12 | AMP 13 | AMP 14 | AMP 15 |
| `Fx` | AMP 16 | AMP 17 | AMP 18 | AMP 19 | AMP 20 | AMP 21 | AMP 22 | AMP 23 | AMP 24 | AMP 25 | AMP 26 | AMP 27 | AMP 28 | AMP 29 | AMP 30 | IMP |

---

### Frame Hierarchy

```
DOE ──── DAB                         IDY ──── IDY
         ├── DAB(SRQ)                         └── IDY(SRQ)
         ├── END
         └── END(SRQ)                RDY ──── RFC
                                              ├── ARG ──── EOT ──── ETO
CMD ──── ACG ──── NUL                         │            │        └── ETE
         │        ├── GTL                     │            ├── NRD
         │        ├── SDC                     │            └── SOT ──── SDA
         │        ├── PPD                     │                     ├── SST
         │        ├── GET                     │                     ├── SDI
         │        ├── ELN                     │                     ├── SAI
         │        ├── PPE0(0–7)               │                     └── TCT
         │        ├── PPE1(0–7)               │
         │        ├── DDL(0–31)               └── AAG ──── AAD(0–30)
         │        └── DDT(0–31)                            ├── NAA(1–31)
         │                                                 ├── IAA
         ├── UCG ──── NOP                                  ├── AEP(0–30)
         │        ├── LLO                                  │   └── IEP
         │        ├── DCL                                  ├── ZES
         │        ├── PPU                                  ├── AES(0–30)
         │        ├── EAR                                  │   ├── NES(1–31)
         │        ├── IFC                                  │   └── IES
         │        ├── REN                                  └── AMP(0–31)
         │        ├── NRE                                      ├── NMP(1–31)
         │        ├── AAU                                      └── IMP
         │        └── LPD
         │
         ├── LAG ──── LAD(0–30)
         │        ├── MLA(0–30)
         │        └── UNL
         │
         ├── TAG ──── TAD(0–30)
         │        ├── MTA(0–30)
         │        ├── OTA(0–30)
         │        └── UNT
         │
         └── SAG ──── SAD(0–30)
                  ├── MSA(0–30)
                  └── OSA(0–30)
```

---

### Accessory Identification

Accessory ID provides HP-IL controllers with the ability to quickly identify the devices on the loop according to device functions. The accessory ID consists of a four-bit class descriptor and a four-bit type field. The class descriptor indicates what main function the device provides (such as printer, mass storage, etc.). The type field indicates specific attributes about the device.

If a device functions as other devices within a particular class and type, the device should respond to accessory ID with that class and type. If no type exists within a device's class that closely matches its attributes, the device should respond with type `E` of that class. The extended class (`Fx`) and extended type (`xF`) are reserved to allow new classes and types. It is very strongly recommended that all future devices designed for HP-IL systems respond to accessory ID.

#### Classes and Types (hex)

**`0x` — Controllers**

| Code | Description |
|------|-------------|
| `00` | Limited controller capability; mostly automatic system I/O functions. Example: HP-41C |
| `01` | Full instrumentation controller; completely manual (programmatic) control. Example: HP Series 80 |
| `02` | Full interface controller; completely automatic including control passing during I/O operations |
| `03` | Full interface controller; partially automatic |
| `0E` | General controller |
| `0F` | Extended (reserved) |

**`1x` — Mass Storage Devices**

| Code | Description |
|------|-------------|
| `10` | Seek/read/write protocol using device dependent commands as defined by the HP 82161A Digital Cassette Drive. Example: HP 82161A Digital Cassette Drive |
| `1E` | General mass storage |
| `1F` | Extended (reserved) |

**`2x` — Printers**

| Code | Description |
|------|-------------|
| `20` | 24 column; HP escape sequences; column dot graphics. Example: HP 82162A Thermal Printer |
| `21` | 80 column; HP escape sequences; column dot graphics. Example: HP 82905B Impact Printer |
| `22` | 80 column; HP escape sequences; no graphics. Example: HP 2671A Thermal Printer |
| `23` | 80 column; HP escape sequences; HP raster graphics. Example: HP 2671G Thermal Printer |
| `24` | 80 column; HP escape sequences; HP raster graphics. Example: HP 2673A Thermal Printer |
| `2E` | General printer |
| `2F` | Extended (reserved) |

**`3x` — Displays**

| Code | Description |
|------|-------------|
| `30` | 32 column; HP escape sequences; no graphics. Example: HP 82163A Video Interface |
| `3E` | General display |
| `3F` | Extended (reserved) |

**`4x` — Interfaces**

| Code | Description |
|------|-------------|
| `40` | HP-IL/GPIO interface. Example: HP 82165A, HP 82166A |
| `41` | HP-IL modem. Example: HP 82168A |
| `42` | HP-IL/RS-232-C interface. Example: HP 82164A |
| `43` | HP-IL/HP-IB interface. Example: HP 82169A |
| `4E` | General interface |
| `4F` | Extended (reserved) |

**`5x` — Electronic Instrumentation**

| Code | Description |
|------|-------------|
| `51`–`57` | If bit D3 is 0, the lower three bits (D2–D0) represent the functions contained in this device. D2, D1, and D0 are defined as signal source, signal conditioning, and signal measurement functions respectively. For example, a device that can act as both a signal source and measurement device would respond `55`. |
| `5E` | General electronic instrument |
| `5F` | Extended (reserved) |

**`6x` — Graphic I/O**

| Code | Description |
|------|-------------|
| `60` | HP-GL compatible. Example: HP 7470A Plotter |
| `6E` | General graphic I/O device |
| `6F` | Extended (reserved) |

**`7x` — Analytical and Scientific Instrumentation**

| Code | Description |
|------|-------------|
| `7E` | General analytical or scientific instrument |
| `7F` | Extended (reserved) |

**`Ex` — General Devices** *(devices that do not easily fit into other classes)*

| Code | Description |
|------|-------------|
| `E0` | EPROM programmer |
| `EE` | General |
| `EF` | Extended (reserved) |

**`Fx` — Extended Class** *(usage not currently defined)*

---

### System Status Messages

HP-IL has the capability for a device to report status to the controller in an interface-defined code called system status. All devices on HP-IL must reserve the most significant bit (D7) of the first byte of status to indicate system status. Bit 7 is required to be zero when the device sources device dependent status, and one when the device sources system status. More than one byte of status may be sent, but all bytes after the first are designer specified.

It is very strongly recommended that devices use system status messages whenever possible, as this allows generalized controllers to understand status messages from any device on the loop.

There are two types of status messages:
- **State messages** represent the overall status of the device. A state message remains true within a device until another message of higher priority replaces it. *All OK* is an example of a state status message.
- **Event messages** indicate an occurrence within a device that does not necessarily affect the current state of the device. Once the controller has been informed of the situation through a serial poll, the device removes the event system status message. *Data Error* is an example of an event status message.

Except for the *All OK* message, event and state system status messages can be distinguished by the value of bit D5. If bit D5 is set, the message is a state or condition. If bit D5 is clear, the message is an event. The *All OK* message is a state, but the value of this message is all zeros.

Devices must select the most important system status message to report during each serial poll. Messages are listed below in priority order (highest first).

#### Device Events

| D5–D0 | Message | Definition |
|--------|---------|------------|
| `000110` | **Self test failure** | The device has discovered a condition that makes proper operation impossible to guarantee. Highest priority of all system status messages. |
| `001000` | **Powering Down** | The device indicates to the controller that it is about to power down and therefore the loop will become inoperative shortly. The message is only useful if the device can delay powering down long enough to report the condition. This message must not be used if the device is powering down in response to an LPD command. |
| `001001` | **External Service Request** | The device indicates to the controller that an external input (such as a switch) has been activated. |
| `000010` | **Manual intervention required** | The device cannot function as designed until an action has been performed by the operator. |
| `000011` | **Data error** | The device has detected a situation that caused information to be lost in a device dependent manner not associated with the talker function. If data is lost due to a loop transmission error, the End Of Transmission with Error (ETE) ready frame will be sourced by the talker. |
| `000111` | **Command Error** | This message indicates to the controller that an invalid device command was received. |
| `000101` | **No room** | The device does not have sufficient space for the data being received from the loop and the situation will not be resolved without intervention. |
| `000100` | **Device error** | The device is in an invalid state or cannot perform the desired operation. This status may be the result of an incorrect sequence or combination of device commands or may be due to an error condition within the device. |
| `000001` | **Low battery** | The device has detected that the battery power has reached a critical level. Future performance of the loop is endangered if this message is ignored. |
| `001010` | **Device Dependent Service Request** | The device has a need for service. This message should be used only if no other system status message is appropriate. |
| `011111` | **ASCII Follows** | The bytes following this message (until an ETO) represent an ASCII message for the user or operator. Lowest priority of device event messages. |

#### Device States

| D5–D0 | Message | Definition |
|--------|---------|------------|
| `100000` | **Request control of loop** | A device may indicate to the current controller that it has need for control of the loop. Highest priority of device state messages (lower priority than event messages). |
| `100010` | **Ready to send data** | The device has data available and is currently ready to source it on the loop. |
| `100001` | **Ready to receive data** | The device indicates that it is currently ready to accept data from the loop. |
| `100011` | **Not ready to receive or send data** | The device indicates to the controller that it has no data ready to send and that it is not able or ready to accept data. Without any intervention, the device will eventually correct the situation and become ready. |
| `000000` | **All OK** | This message indicates that the device has no need for attention and is currently ready to perform the operations for which it was designed. Lowest priority of all system status messages. |

---

*From The HP-IL Interface Specification (Appendix C) HP82166-90017*
