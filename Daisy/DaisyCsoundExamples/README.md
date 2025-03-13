# Daisy Csound Examples

This folder contains two examples for using Csound on the Daisy platform:

1. MIDI Example
2. Generative Example

Both examples demonstrate the use of DaisyCsound's analog and digital interfaces.

## Instructions

1. Place this folder in your `DaisyExamples` directory.
2. Run the examples from there, or specify the path to your `libdaisy` and `daisysp` installations in the example's makefile.
3. Specify the path to your Csound baremetal installation location in the example's makefile.

## Additional Information

- A custom modified linker script and the Daisy bootloader v5.4 are included in this folder to enable the Csound Daisy program to run with the required heap allocated on SDRAM.
- These examples can be used as templates or starting points for Daisy Csound applications.
