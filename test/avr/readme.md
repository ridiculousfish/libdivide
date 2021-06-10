# AVR Testing & Benchmarking

[Libdivide](https://github.com/ridiculousfish/libdivide) test & benchmarking for embedded microcontrollers. E.g. AtMega2560.

## Getting Started

 1. Install [vscode & platformIO](https://docs.platformio.org/en/latest/integration/ide/vscode.html#installation)
 2. Open `libdivide/avr_test.code-workspace` in vscode (File -> Open Workspace)
 3. Wait for the PlatformIO extension to start
 
## Running the Test program

There are 2 test programs:
 1. Slow. Will test all combinations, same as the Windows/Linux tests. 
 2. Fast. Runs a more sparse test suite. Good for most purposes
   
The slow option is the default

To run the test program in a simulator:
 1. On the activity bar, select PlatformIO 
 2. Run Project Tasks -> megaatmega2560_TestFast -> Custom -> Simulate
    a. This will build the test program & launch it in the simulator (this might download )supporting packages)
    b. **NOTE** Once running it can take a **long** time for ouput to appear in the terminal. **Be patient**
     * Or copy the simavr command line from the terminal to a command prompt (or another vscode terminal)

