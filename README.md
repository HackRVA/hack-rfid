# hack_rfid
Please do contribute if you can.  
This is a candidate to replace the current rfid solution at hackrva.

The main problem this is trying to solve is that there's no verification step between the server and the current rfid reader.

## high level
### evaluate access
When someone scans their rfid fob, the mcu will compare it to the ACL (access control list) that it currently has stored.

Access Granted -> activate the relay. -> log the event
Access Denied -> log the event

## dev environment
> TODO: add some notes on setting up dev environment -- it's similar to [badge](https://github.com/HackRVA/badge2024)

Essentially:
* disconnect the Board
* hold down BOOTSEL button
* connect the Board

```bash
bash run_cmake.sh # you should only have to run this the first time
cd build
make
```
copy the firmware to the Board
```bash
cp hack_rfid.uf2 /media/$USER/RPI-RP2/
```

## WIP

| feature | status |
| --- | --- |
| hashable ACL | ✅ |
| rfid reader | ✅ |
| persistent storage | ❌ |
| network (wifi) | ❌ |
| update mechanism (mqtt events) | ❌ |

### hardware

* [rpi pico w](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)
* [mfrc522 rfid reader](https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf)
* [1591B Relay Board](https://thepihut.com/products/1591b-relay-board-for-raspberry-pi-pico)
* `12v power supply`

> note: there's some interest in switching to [pn532](https://www.elechouse.com/product/pn532-nfc-rfid-module-v4/) for the rfid reader 

### update lifecycle
The rfid reader will subscribe to specific mqtt events so that the server can report changes to the ACL.
The server should be able to request the current ACL hash to determine if the reader holds an ACL that is out dated. If the ACL is outdated, the server should initiate a sync.

The mcu should publish events and heartbeats via mqtt.

## Wiring
The Raspberry Pi Pico connects to the RC522 module via the SPI interface. The default pinouts are provided below:

### RFID Reader

| RFID Signal | Pico Pin |
|-------------|:--------:|
| SCK         |   GP2    |
| MOSI        |   GP3    |
| MISO        |   GP4    |
| RST         |   GP0    |
| SDA         |   GP1    |

### 12v Relay

> note: pin TBD

## MQTT Events
mqtt is currently not implemented.  However the existing mqtt topics are as follows:

### Subscriptions

| Topic | Payload | Notes |
|-------|---------|-------|
| `<topic_prefix>/acl` | n/a | The device will publish a `<topic_prefix>/acl_response` message. |
| `<topic_prefix>/adduser` | `uid of the RFID fob to add` | Adds the specified fob to the device's Access Control List (ACL). |
| `<topic_prefix>/removeuser` | `uid of the RFID fob to remove` | Removes the specified fob from the device's ACL. |
| `<topic_prefix>/open` | n/a | Opens the door. |

### Publish

| Topic | Payload | Notes |
|-------|---------|-------|
| `<topic_prefix>/acl_response` | `{'acl': '<SHA256 hash of the current ACL stored>'}` | Allows the server to verify if the device has the correct ACL. |
| `<topic_prefix>/heartbeat` | `OK` | Allows the server to verify the device's network connection. |
| `<topic_prefix>/access_granted` | `uid of the fob that is granted access` | Used for logging purposes. |
| `<topic_prefix>/access_denied` | `uid of the fob that is denied access` | Used for logging purposes. |
