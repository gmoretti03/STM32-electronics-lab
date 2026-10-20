# STM32-electronics-lab

Firmware, MATLAB scripts, measurements and analysis for the *Esperimentazioni di Elettronica*
laboratory course (A.A. 2024/2025), built around an STM32 Nucleo-H743ZI board
(STM32H743ZITx).

Authors: Giacomo Moretti, Emma Pellizzoni, Leonardo Zambusi.

## Repository layout

| Folder | Content |
|---|---|
| `firmware/` | STM32CubeIDE project (`STM32-electronics-lab.ioc`) with all experiments |
| `firmware/legacy/` | Earlier single-experiment projects (DMA acquisition, exponentials, ADC temperature/voltage snippet), kept for reference |
| `matlab/` | MATLAB live scripts that talk to the board over the serial port and plot the results |
| `matlab/extra/` | Additional MATLAB scripts and figures (Emma Pellizzoni) |
| `analysis/` | Spreadsheets, the `Overvoltage.ipynb` fit notebook and photos of the setups |
| `data/` | Oscilloscope screenshots and small data files for the AD848, OP27 and SiPM experiments |
| `report/` | The lab report (PDF, in Italian) |

## Experiments

All experiments live in `firmware/Core/Src/main.c`. Enable one by uncommenting its
initialisation calls (the SiPM chain is active by default):

| Module | Experiment | MATLAB script |
|---|---|---|
| `LED.c` | Traffic light on GPIO with TIM6 and a push-button interrupt | - |
| `USART_RT.c` | USART echo | `matlab/USART_RT.mlx` |
| `exponential.c` | Fixed-point difference of two exponentials sent over USART | `matlab/esponenziale.mlx` |
| `temp_volt.c` | ADC3 reading of the internal temperature sensor and voltage reference | `matlab/temp_volt.mlx` |
| `SiPM.c` | SiPM waveform acquisition: comparator trigger, ADC + DMA, USART transfer | `matlab/SiPM.mlx`, `matlab/Oscilloscopio.mlx` |

The protocol with MATLAB is simple: MATLAB sends the character `w` and the board answers
with the acquired data.

## Building

1. Open STM32CubeIDE and import `firmware/` as an existing project.
2. Build the `Debug` configuration and flash the board over ST-LINK.
3. Run the MATLAB script of the selected experiment and set its serial port.

The `Debug/` build output is not tracked; regenerate it by building.

## Data

Raw captures larger than 2 MB are not stored in the repository. See
[`data/LARGE_FILES.md`](data/LARGE_FILES.md) for the list.

## License

- Code (firmware, MATLAB scripts, notebook): [MIT](LICENSE).
- Lab report, measurements, figures and spreadsheets: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- STM32 HAL/CMSIS drivers in `firmware/Drivers` keep their original STMicroelectronics/Arm licenses.

## Notes

- Identifiers in the firmware (`*_iniz`, `ricezione`, ...) are still Italian; comments are in English.
- The MATLAB scripts and the report are in Italian.
