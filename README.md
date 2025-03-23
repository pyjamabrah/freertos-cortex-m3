## This repository hosts our custom port of FreeRTOS for QEMU target of ARM CORTEX-M3 processor.

NOTE: The codebase here is to be treated as a supplement of the of FreeRTOS playlist.

## Dependencies:
* Toolchain:- `gcc-arm-none-eabi` :  `sudo apt install gcc-arm-none-eabi`
* QEMU:- `qemu-system-arm` : `sudo apt install qemu-system`

## Compilation:
* Fresh compilation : `make`
* Clean artifacts : `make clean`

## Running on QEMU:
* `qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -kernel ./output/freertos-cortex-m3.out -monitor none -nographic -serial stdio`

