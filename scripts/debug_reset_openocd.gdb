target remote tcp:localhost:3333

set breakpoint pending on

monitor reset halt

jump Reset_Handler
