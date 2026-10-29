# audioanalyzer

Real-time audio spectrum analyzer for the Raspberry Pi Pico W (RP2040).
Audio from a 3.5 mm jack is sampled at 48 kHz and shown as 32 frequency bars on an SSD1306 OLED.

- ADC sampling with DMA double buffering, no samples lost
- 256-point DFT, 187.5 Hz per bin, 0–24 kHz range
- 32 log-spaced bars in dB with smooth fall-off
- Uses [SSD1306-lib](../fund/SSD1306) for the display

## Hardware

| Part | Notes |
|---|---|
| Pico W | |
| SSD1306 OLED | 128×64, I2C, address `0x3C` |
| 3.5 mm audio cable or jack | Tip (left) and sleeve (ground) |
| 1 µF capacitor (`105`) | Coupling capacitor |
| 2× 100 kΩ | Bias divider |
| 1 kΩ | Series resistor into the ADC pin |

### Input circuit

The ADC reads 0–3.3 V, but audio swings around 0 V. The capacitor blocks the source's DC and the divider re-centers the signal at about 1.65 V.

```
                     ADC_VREF (pin 35)
                          |
                       [100k]
                          |
TIP  ------||-------------+--------[1k]-------- GP26 (pin 31)
          1µF             |
                       [100k]
                          |
SLEEVE -------------------+-------------------- AGND (pin 33)
```

With no audio playing, the point between the resistors should read about 1.5–1.65 V.

### Wiring

| Pico W | Connects to |
|---|---|
| GP26 (pin 31) | Input circuit (1k) |
| AGND (pin 33) | Input circuit ground, jack sleeve |
| ADC_VREF (pin 35) | Top 100k |
| GP4 (SDA) | OLED SDA |
| GP5 (SCL) | OLED SCL |
| 3V3 | OLED VCC |
| GND | OLED GND |

## Building

The SSD1306 library is expected at `../fund/SSD1306`. Override with `-DSSD1306_PATH=...`.

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build
cmake --build build -j
```

Flash `build/audioanalyzer.uf2` by holding BOOTSEL while plugging in the Pico and copying the file to the drive.

## Testing

Generated signals are more reliable than YouTube tracks, which are often filtered or compressed.

| Command | Expected display |
|---|---|
| `ffplay -nodisp -f lavfi "anoisesrc=color=white:amplitude=0.3"` | Roughly flat bars, lower at the far right |
| `ffplay -nodisp -f lavfi "anoisesrc=color=pink:amplitude=0.3"` | High on the left, sloping down |
| `speaker-test -t sine -f 1875 -c 1` | One spike at bar 9 |
| `speaker-test -t sine -f 15000 -c 1` | One spike at bar 28 |
| `ffplay -nodisp -autoexit -f lavfi "aevalsrc='0.3*sin(2*PI*(100*t+995*t*t/2))':d=20"` | A spike sweeping left to right |

If extra spikes appear at evenly spaced bars when a tone is playing, the input is clipping. Lower the volume.

## How it works

1. **ADC:** samples GP26 at 48 MHz / (1 + `CLKDIV`) = 48 kHz into its FIFO.
2. **DMA:** two chained channels move samples into `first_array` and `second_array`, alternating without CPU help. The interrupt marks a buffer as ready and resets its write address.
3. **DFT** (`src/dft.c`): removes the DC bias, then correlates the 256 samples with precomputed sine and cosine tables for each of the 128 bins.
4. **Bars** (`main.cpp`): bins are grouped into 32 log-spaced bars, the average power is converted to dB and mapped to 0–64 px. Bars jump up immediately and fall 1 px per frame.
5. **Display:** only the framebuffer pages that changed are sent over I2C.

Each frame processes one freshly completed buffer and drops older ready flags, so DMA can't overwrite samples while they are being read.

## Tuning

| Define | Default | Effect |
|---|---|---|
| `FLOOR_DB` | 42 | Level drawn as 0 px. Set just above the noise you see at silence |
| `RANGE_DB` | 48 | dB span of the full display height. Loudest signal ≈ `FLOOR_DB + RANGE_DB` |
| `BAR_COUNT` / `BAR_WIDTH` / `BAR_STEP` | 32 / 3 / 4 | Bar layout, `BAR_COUNT × BAR_STEP` should be 128 |
| `CLKDIV` | 999 | Sample rate, 48 MHz / (1 + `CLKDIV`) |
| `BUFFER_SIZE` | 256 | Samples per DFT, bin width = sample rate / `BUFFER_SIZE` |

## Limitations

- Everything below about 94 Hz lands in bin 0 (DC), which is not drawn, so deep bass and kick drums are mostly invisible.
- The lowest 20 bars each cover one 187.5 Hz bin, so the layout is linear up to about 4 kHz and logarithmic above it.
- The DFT runs in software floating point, so most captured buffers are skipped. A larger `BUFFER_SIZE` would need an FFT to keep the frame rate.
- Only the left channel (tip) is used.

## License

MIT, see [LICENSE](LICENSE).
