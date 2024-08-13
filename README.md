# Chip8-Emulator
This was a fun project for me to learn the ins and outs of how a computer really works, down to the level of registers and memory. I made an emulator for the Chip8 machine, a virtual machine with 4Kb memory and 16 main registers. I made a class for the CPU with these elements as well as the stack, other registers, timers, and graphics output. Then I implemented the various opcodes for instructions between the memory, stack, and the registers. And then finally, I implemented an OpenGL window (taken from my Raytracing project) that displays each frame pixel by pixel. Finally, I downloaded a Chip8 version of Pong and enabled the machine to read it.

Overall, I learned a lot about low-level computing. In the future, I'd like to explore more emulators, such as for the NES or Gameboy. I'd also like to explore audio, as in my version, the sound timer merely causes the system to print "BEEP" instead of handling audio. This was a really fun project for me to make in C++ and I found it really educational.

# Sources
https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/

- Vinith Yedidi, 2024
