# Pubsub Bridge Component

Bridges local pubsub messages to remote ESP32 boards via transport layer (UART, CAN, etc).

## Features

- **Transport abstraction**: Swap UART/CAN/RS-485 without changing bridge logic
- **Robust framing**: STX/ETX markers with CRC16-CCITT error detection
- **Topic filtering**: Only forward configured topics
- **Bidirectional**: Automatically publishes remote messages locally

## Usage

### Heater Board Example

```cpp
#include "pubsub_bridge.hpp"
#include "uart_transport.hpp"

using namespace toothless;

// Configure UART
UartTransportConfig uart_config;
uart_config.port = UART_NUM_1;
uart_config.tx_pin = GPIO_NUM_17;
uart_config.rx_pin = GPIO_NUM_18;
uart_config.baud_rate = 921600;

UartTransport transport(uart_config);

// Configure bridge
BridgeConfig bridge_config;
bridge_config.forward_topics = {
  "sensor.temperature.*",    // Forward all temp sensors
  "heater.chamber.actual",   // Forward actual temp
  "heater.mode",             // Forward heater mode
  "health"                   // Forward health data
};

PubsubBridge bridge(&transport);
bridge.Init(bridge_config);

// In main loop
void loop() {
  bridge.Loop();  // Call frequently (10ms typical)
}
```

### UI Board Example

```cpp
// UI board mirrors the setup, but forwards UI commands back
BridgeConfig ui_bridge_config;
ui_bridge_config.forward_topics = {
  "heater.chamber.target.set",   // Forward target temp commands
  "heater.mode.set",             // Forward mode changes
  "heater.profile.set"           // Forward profile selections
};
```

## Frame Format

```
[STX:1][LEN:2][TOPIC:n][NULL:1][DATA:n][CRC16:2][ETX:1]

STX  = 0x02 (Start of Text)
LEN  = Payload length (topic + null + data)
TOPIC = Null-terminated topic string
DATA = Message payload (ps_msg_t data)
CRC16 = CRC16-CCITT over payload
ETX  = 0x03 (End of Text)
```

## Wiring

```
Heater Board          UI Board
GPIO_17 (TX) ────────> GPIO_18 (RX)
GPIO_18 (RX) <──────── GPIO_17 (TX)
GND ──────────────────> GND
```

Recommended: Use twisted pair or shielded cable for noise immunity.

## Adding CAN Transport

Create `can_transport.cpp`:

```cpp
class CanTransport : public ITransport {
  // Implement Init/Send/Receive using ESP32 TWAI peripheral
};
```

Then swap:
```cpp
CanTransport transport(can_config);  // Instead of UartTransport
PubsubBridge bridge(&transport);     // Same bridge code
```

## Performance

- UART @ 921600 baud: ~92 KB/s theoretical, ~70 KB/s practical
- Typical message overhead: 8 bytes + topic length
- CRC validation adds ~10μs per message
