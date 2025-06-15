# AVR Testing & Benchmarking

[Libdivide](https://github.com/ridiculousfish/libdivide) test & benchmarking for embedded microcontrollers. E.g. AtMega2560.

## Getting Started

 1. Install [vscode & platformIO](https://docs.platformio.org/en/latest/integration/ide/vscode.html#installation)
 2. Open `libdivide/avr_test.code-workspace` in vscode (File -> Open Workspace)
 3. Wait for the PlatformIO extension to start
 
## Running the Test program

The test program is the 'megaatmega2560_sim_unittest' environment.

To run the test program in a simulator (no hardware required!):

1. On the activity bar, select PlatformIO 
2. Run Project Tasks -> megaatmega2560_sim_unittest -> Advanced -> Test
    1. This will build the test program & launch it in the simulator (this might download supporting packages)
    2. **NOTE** Once running it can take a **long** time for output to appear in the terminal. **Be patient**

