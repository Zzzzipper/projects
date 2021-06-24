copy /Y ..\Debug\modem_arm_firmware2.binary ..\Debug\firmware.bin
"D:\devtool\stm32_st-link_utility\ST-LINK Utility\ST-LINK_CLI.exe" -P "..\Debug\firmware.bin" 0x08040000 -V
"D:\devtool\stm32_st-link_utility\ST-LINK Utility\ST-LINK_CLI.exe" -Rst
pause