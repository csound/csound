
# Daisy Csound Examples

This folder contains two example projects demonstrating how to use Csound on the Daisy platform:

1. **MIDI Example**  
2. **Generative Example**  
3. **Input Processing Example**  
4. **Pod Synth**  

Both examples showcase the use of DaisyCsound's analog and digital interfaces.

## Instructions

1. Either use the released binary package or build libcsound from
sources  
2. Place the `DaisyCsoundExamples` (this folder),  as well as the `lib` and `include` folders inside your `DaisyExamples` directory (assuming
you've already downloaded and installed the Daisy tools from
Electrosmith).  
3. From one of the example folders, run `make` to build that example.  
4. Connect your Daisy board to your computer via USB. Put the board into bootloader mode by:
   - Holding down the **BOOT** button  
   - Pressing and releasing the **RESET** button  
   - Releasing the **BOOT** button  
6. Run `make program-boot` to flash the bootloader program.  
7. After the board is flashed, press the RESET button once more (you’ll see an LED slowly fading in and out), then press and release the **BOOT** button again.  
8. Run `make program-dfu` to upload the firmware.  

Once the last step is complete, your Csound-based firmware should be running on the Daisy board.

## Additional Information

- A custom-modified linker script and the Daisy bootloader version 5.4 are included to ensure the Csound application uses SDRAM for its required heap allocation.
- These examples can serve as templates or starting points for your own Daisy + Csound projects.
