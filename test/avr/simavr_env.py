import os

Import('env')

# Dump construction environment
# print(env.Dump())

platform = env.PioPlatform()

simavr = f'{os.path.join(os.path.join(platform.get_package_dir("tool-simavr") or "", "bin"), "simavr.exe")}'

# Single action/command per 1 target
env.AddCustomTarget(
    'Simulate', 
    '$PROG_PATH', 
    f'{simavr} -f $BOARD_F_CPU -m $BOARD_MCU $PROG_PATH')