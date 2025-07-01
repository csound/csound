
# Daisy Csound Examples

This folder contains two example projects demonstrating how to use Csound on the Daisy platform:

1. **MIDI Example**  
2. **Generative Example**

Both examples showcase the use of DaisyCsound's analog and digital interfaces.

## Instructions

1. Place this folder inside your `DaisyExamples` directory (assuming
you've already downloaded and installed the Daisy tools from Electrosmith)
3. Open the included `Makefile` and update the following variables to match your system’s Csound installation:
   - `CSOUND_INCLUDE_DIR`  
   - `CSOUND_LIB_DIR`  
   - `CSOUND_STATIC_LIB`  
4. Run `make` to build the example.  
5. Connect your Daisy board to your computer via USB. Put the board into bootloader mode by:
   - Holding down the **BOOT** button  
   - Pressing and releasing the **RESET** button  
   - Releasing the **BOOT** button  
6. Run `make program-boot` to flash the bootloader program.  
7. After the board is flashed, press the RESET button once more (you’ll see an LED slowly fading in and out), then press and release the **BOOT** button again.  
8. Run `make program-dfu` to upload the firmware.

Once complete, your Csound-based firmware should be running on the Daisy board.

## Additional Information

- A custom-modified linker script and the Daisy bootloader version 5.4 are included to ensure the Csound application uses SDRAM for its required heap allocation.
- These examples can serve as templates or starting points for your own Daisy + Csound projects.
