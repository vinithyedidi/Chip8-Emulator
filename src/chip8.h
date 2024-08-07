#ifndef CHIP8_H
#define CHIP8_H

#include <stdlib.h>

class chip8 {
public:

	chip8() {
		// Default start conditions
		pc = 0x200;
		opcode = 0;
		I = 0;
		sp = 0;
		drawflag = false;

		// clear stack, registers, and keys
		for (int i = 0; i < 16; ++i) {
			stack[i] = 0;
			V[i] = 0;
			key[i] = 0;
		}
		// clear memory
		for (int i = 0; i < 4096; ++i) {
			memory[i] = 0;
		}

		//clear display
		for (int i = 0; i < 32*64; ++i) {
			gfx[i] = 0;
		}

		// load fontset into memory
		unsigned char fontset[80] = {
			0xF0, 0x90, 0x90, 0x90, 0xF0, //0
			0x20, 0x60, 0x20, 0x20, 0x70, //1
			0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
			0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
			0x90, 0x90, 0xF0, 0x10, 0x10, //4
			0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
			0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
			0xF0, 0x10, 0x20, 0x40, 0x40, //7
			0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
			0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
			0xF0, 0x90, 0xF0, 0x90, 0x90, //A
			0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
			0xF0, 0x80, 0x80, 0x80, 0xF0, //C
			0xE0, 0x90, 0x90, 0x90, 0xE0, //D
			0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
			0xF0, 0x80, 0xF0, 0x80, 0x80  //F
		};
		for (int i = 0; i < 80; ++i) {
			memory[i] = fontset[i];
		}

		// reset timers
		delay_timer = 60;
		sound_timer = 60;
	}


	bool loadGame(const char * filename)
	{
		printf("Loading: %s\n", filename);

		// Open file
		FILE * pFile = fopen(filename, "rb");
		if (pFile == NULL) {
			fputs ("File error", stderr);
			return false;
		}

		// Check file size
		fseek(pFile , 0 , SEEK_END);
		long lSize = ftell(pFile);
		rewind(pFile);
		printf("Filesize: %d\n", (int)lSize);

		// Allocate memory to contain the whole file
		char * buffer = (char*)malloc(sizeof(char) * lSize);
		if (buffer == NULL) {
			fputs ("Memory error", stderr);
			return false;
		}

		// Copy the file into the buffer
		size_t result = fread (buffer, 1, lSize, pFile);
		if (result != lSize) {
			fputs("Reading error",stderr);
			return false;
		}

		// Copy buffer to Chip8 memory
		if((4096-512) > lSize)
		{
			for(int i = 0; i < lSize; ++i)
				memory[i + 512] = buffer[i];
		}
		else
			printf("Error: ROM too big for memory");

		// Close file, free buffer
		fclose(pFile);
		free(buffer);

		return true;
	}


	void emulateCycle() {

		// two byte opcode from memory at location of program counter
		opcode = memory[pc] << 8 | memory[pc + 1];

		// decode and execute current opcode
		switch (opcode & 0xF000) {
			case(0x0000): {
				switch (opcode & 0x000F) {
					case(0x0000): {
						// 0x00E0: clears display
						for (unsigned char pixel : gfx) {
							pixel = 0;
						}
						break;
					}

					case(0x000E): {
						// 0x00EE: returns from subroutine
						--sp;
						pc = stack[sp];
						pc += 2;
						break;
					}

					default: {
						printf("Unknown opcode: 0x%X\n", opcode);
					}
				}
			}
			break;

			case(0x1000): {
				// 0x1NNN: jumps to address NNN
				pc = opcode & 0x0FFF;
				break;
			}

			case(0x2000): {
				// 0x2NNN: calls subroutine at address NNN
				stack[sp] = pc;
				++sp;
				pc = opcode & 0x0FFF;
				break;
			}

			case (0x3000): {
				// 0x3XNN: skips next instruction if VX != NN
				if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
					pc += 4;
				else
					pc += 2;
				break;
			}

			case (0x4000): {
				// 0x4XNN: skips next instruction if VX == NN
				if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
					pc += 4;
				else
					pc += 2;
				break;
			}

			case (0x5000): {
				// 0x5XYN: skips next instruction if VX == VY
				if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0)] >> 4)
					pc += 4;
				else
					pc +=2;
				break;
			}

			case (0x6000): {
				// 0x6XNN: sets register VX to value NN
				V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
				pc += 2;
				break;
			}

			case (0x7000): {
				// 0x7XNN: register VX += NN
				V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
				pc += 2;
				break;
			}

			case(0x8000): {
				switch (opcode & 0x000F) {
					case (0x0000): {
						// 0x8XY0: Sets VX to VY
						V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;
					}

					case (0x0001): {
						// 0x8XY1: sets VX to VX or VY
						V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;
					}

					case (0x0002): {
						// 0x8XY2: sets VX to VX and VY
						V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;
					}

					case (0x0003): {
						// 0x8XY3: sets VX to VX xor VY
						V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;
					}

					case (0x0004): {
						// 0x8XY4: adds VX by VY. VF = 1 if overflow, 0 if not.
						if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
							V[0xF] = 1; //carry
						else
							V[0xF] = 0; // no carry
						V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;
					}

					case (0x0005): {
						// 0x8XY5: subtracts VX by VY. VF = 1 if underflow, 0 if not.
						if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
							V[0xF] = 0; // borrow
						else
							V[0xF] = 1;
						V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
						pc += 2;
						break;
					}

					case (0x0006): {
						// 0x8XY6: shifts VX to the right, stores least significant bit in VF.
						V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
						V[(opcode & 0x0F00) >> 8] >>= 1;
						pc += 2;
						break;
					}

					case (0x0007): {
						// 0x8XY7: sets VX to VY - VX. VF = 0 if underflow, 1 if not.
						if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
							V[0xF] = 0; // borrow
						else
							V[0xF] = 1;
						V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
						pc += 2;
						break;
					}

					case (0x000E): {
						// 0x8XYE: shifts VX one to left, stores least significant bit in VF
						V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
						V[(opcode & 0x0F00) >> 8] <<= 1;
						pc += 2;
						break;
					}

					default:{
						printf("Unknown opcode: 0x%X\n", opcode);
					}
				}
				break;
			}

			case (0x9000): {
				// 0x9XY0: skips next instruction if VX != VY
				if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0 >> 4)])
					pc += 4;
				else
					pc += 2;
				break;
			}

			case (0xA000): {
				// 0xANNN: Sets I to NNN
				I = opcode & 0x0FFF;
				pc += 2;
				break;
			}

			case (0xB000): {
				// 0xBNNN: jumps to adress V0 + NNN
				pc = V[0] + (opcode & 0x0FFF);
				break;
			}

			case (0xC000): {
				// 0xCXNN: sets VX to NN and random number
				V[(opcode & 0x0F00 >> 8)] = (rand() & 0xFF) & (opcode & 0x00FF);
				pc += 2;
				break;
			}

			case 0xD000: {
				// DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
				// Each row of 8 pixels is read as bit-coded starting from memory location I;
				// I value doesn't change after the execution of this instruction.
				// VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn,
				// and to 0 if that doesn't happen
				unsigned short x = V[(opcode & 0x0F00) >> 8];
				unsigned short y = V[(opcode & 0x00F0) >> 4];
				unsigned short height = opcode & 0x000F;
				unsigned short pixel;

				V[0xF] = 0;
				for (int yline = 0; yline < height; yline++) {
					pixel = memory[I + yline];
					for(int xline = 0; xline < 8; xline++) {
						if((pixel & (0x80 >> xline)) != 0) {
							if(gfx[(x + xline + ((y + yline) * 64))] == 1) {
								V[0xF] = 1;
							}
							gfx[x + xline + ((y + yline) * 64)] ^= 1;
						}
					}
				}
				drawflag = true;
				pc += 2;
				break;
			}

			case (0xE000): {
				switch(opcode & 0x00FF) {
					case (0x009E): {
						// 0xEX9E: skips next instruction if key stored in VX is pressed
						if (key[V[(opcode & 0x0F00) >> 8]] != 0)
							pc += 4;
						else
							pc += 2;
						break;
					}

					case (0x00A1): {
						// 0xEXA1: skips next instruction if key stored in VX is not pressed
						if (key[V[(opcode & 0x0F00) >> 8]] == 0)
							pc += 4;
						else
							pc += 2;
						break;
					}

					default: {
						printf("Unknown opcode: 0x%X\n", opcode);
					}
				}
				break;
			}

			case (0xF000): {
				switch (opcode & 0x00FF) {
					case (0x0007): {
						// 0xFX07: sets VX to value of delay timer
						V[(opcode & 0x0F00) >> 8] = delay_timer;
						pc += 2;
						break;
					}

					case (0x000A): {
						// 0xFX0A: wait for key press, store result in VX.
						bool keypress = false;
						for (int i = 0; i < 16; ++i) {
							if (key[i] != 0) {
								keypress = true;
								V[(opcode & 0x0F00) >> 8] = i;
							}
						}
						if (keypress)
							pc += 2;
						else
							return;
						break;
					}

					case (0x0015): {
						// 0xFX15: sets delay timer to VX.
						delay_timer = V[(opcode & 0x0F00) >> 8];
						pc += 2;
						break;
					}

					case (0x0018): {
						// 0xFX18: sets sound timer to VX.
						sound_timer = V[(opcode & 0x0F00) >> 8];
						pc += 2;
						break;
					}

					case (0x001E): {
						// 0xFX1E: Adds VX to I
						if(I + V[(opcode & 0x0F00) >> 8] > 0xFFF)	// VF is set to 1 if  overflow and 0 if nott.
							V[0xF] = 1;
						else
							V[0xF] = 0;
						I += V[(opcode & 0x0F00) >> 8];
						pc += 2;
						break;
					}

					case (0x0029): {
						// 0xFX29: Sets I to the location of the sprite for the character in VX.
						I = V[(opcode & 0x0F00) >> 8] * 0x5;
						pc += 2;
						break;
					}

					case (0x0033): {
						// 0xFX33: stores binary-coded decimal VX at addresses I, I + 1, and I + 2
						memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
						memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
						memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
						pc += 2;
						break;
					}

					case (0x0055): {
						// 0xFX55: stores V0-VX in memory starting at index I
						for (int i = 0; i <= (opcode & 0x0F00) >> 8; ++i) {
							memory[I + i] = V[i];
						}
						I += ((opcode & 0x0F00) >> 8) + 1;
						pc += 2;
						break;
					}

					case (0x0065): {
						// 0xFX65: fills V0-VX with data from memory starting at index I
						for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
							V[i] = memory[I + i];

						I += ((opcode & 0x0F00) >> 8) + 1;
						pc += 2;
						break;
					}

					default:{
						printf("Unknown opcode: 0x%X\n", opcode);
					}
				}
				break;
			}

			default: {
				printf("Unknown opcode: 0x%X\n", opcode);
			}
		}

		// update timers
		if(delay_timer > 0)
			--delay_timer;
		if(sound_timer > 0) {
			if(sound_timer == 1)
				printf("BEEP!\n");
			--sound_timer;
		}
	}


    // elements of the Chip8 virtual machine
    unsigned short opcode;          // current opcode
    unsigned char memory[4096];     // 4kb memory
    unsigned char V[16];            // 15 general registers, V1-VE. VF is carry flag
    unsigned short I;               // index register
    unsigned short pc;              // program counter
    unsigned char gfx[64 * 32];     // pixel array, 2048 pixels in total
    unsigned char delay_timer;      // 60Hz timers, counts down til 0
    unsigned char sound_timer;      //
    unsigned short stack[16];       // store program counter in stack before calling subroutine
    unsigned short sp;              // stack pointer, to remember which level of stack
    unsigned char key[16];          // stores current state of the 16 keys, 1-F
    unsigned char fontset[80];      // font set to be stored in ram.
    bool drawflag;                  // flag to see if screen needs to be redraw

};
#endif //CHIP8_H
