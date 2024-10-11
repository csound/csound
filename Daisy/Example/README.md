# DaisyCsound

A basic example demonstrating the use of baremetal Csound on the Daisy platform with connections between Csound and Daisy's Analog and Digital ins, as well as MIDI. 

Once the Daisy toolchain is installed, replace the file at ~/DaisyExamples/libDaisy/core/STM32H750IB_qspi.lds with the custom linker script from this folder named STM32H750IB_qspi.lds. This provides the required memory allocation on SDRAM for baremetal Csound to run.
