# ESP32-I2S Multi DAC

## Up to 32 16-bit output channels on a single ESP32
* Output an I2s stream to multiple 16-bit DACs (PT8211) enabling a maximum of 16 channels by demultiplexing BCLK, WS and DATA.
* Using common (cheap) off the shelf logic building blocks that are readily avaiable.
* Samplerates from 8kHz up to 96 kHz depending on number of DAC channels.
* Uses only 1 extra GPIO to synchronize up to 8 connected DACs.

---
## 74-series chips needed for 16 mono or 8 stereo channels
* 1 x level converter 74HCT125
* 1 x 4-bit binary counter 74HC161
* 3 x 3-to-8 line decoder/demultiplexer 74HC138
* 1 x HEX buffer/inverter 74HC04
* 4 x PT8211 16-bit DAC

According to the datasheet of the 74HC138 3-to-8 line decoder/demultiplexer:
> The '138 can be used as an eight output demultiplexer by using one of the active LOW enable inputs as the data input and the remaining enable inputs as strobes.

This chip is used to demultiplex the three I2S signals and feed them to the individual DACs after the polarity is reversed by the HEX-buffer/inverter.



