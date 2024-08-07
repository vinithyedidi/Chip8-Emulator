#include <iostream>
#include <chrono>
#include <thread>

#include <glad/glad.h>
#include <glad/glad.c>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "chip8.h"


// settings
const unsigned int SCR_WIDTH = 64;
const unsigned int SCR_HEIGHT = 32;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, chip8 *chip);


void run_window() {
    // set up window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Chip8 Emulator -Vinith Yedidi, 2024",
        NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    // Use Shader class to compile shaders
    Shader ourShader("/Users/vinithyedidi/CodingProjects/CLion/Chip8 Emulator/src/vt.glsl",
        "/Users/vinithyedidi/CodingProjects/CLion/Chip8 Emulator/src/ft.glsl");

    // Generate rectangle across entire screen — this is the canvas for displaying the pixels
    float vertices[] = {
        // positions          // colors           // texture coords
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   1.0f, 1.0f, // top right
         1.0f, -1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    // more OpenGL setup
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Initialize pixel array to be rendered onto
    unsigned char pixels[SCR_WIDTH*SCR_HEIGHT*3];
    unsigned int texture;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // set up Chip 8 emulator
    chip8 chip;
    chip.loadGame("/Users/vinithyedidi/CodingProjects/CLion/Chip8 Emulator/pong2.c8");

    // used to ensure game runs at 60hz
    std::chrono::time_point<std::chrono::steady_clock> t0, t1;
    long long dt;

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        t0 = std::chrono::high_resolution_clock::now();

        glClear(GL_COLOR_BUFFER_BIT);

        // Use Chip 8 cycle to generate array of pixels, gfx, to display on screen
        chip.emulateCycle();
        if(chip.drawflag) {
            // transfer gfx to RGB pixels
            for (int i = 0; i < 32*64; ++i) {
                if(chip.gfx[i] == 0) {
                    pixels[3*i]   = (unsigned char)0;
                    pixels[3*i+1] = (unsigned char)0;
                    pixels[3*i+2] = (unsigned char)0;
                }
                else {
                    pixels[3*i]   = (unsigned char)255;
                    pixels[3*i+1] = (unsigned char)255;
                    pixels[3*i+2] = (unsigned char)255;
                }
            }
        }
        processInput(window, &chip);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT,
           0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        ourShader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // ensures chip8 runs at 60Hz
        t1 = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        auto sleepfor = std::chrono::milliseconds(int(1000.0/60.0 - dt));
        std::this_thread::sleep_for(sleepfor);
    }

    // De-allocate all resources once they've outlived their purpose:
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
}


// Process all user input
void processInput(GLFWwindow *window, chip8 *chip) {

    // reset key input
    for (int i = 0; i < 16; ++i) {
        chip->key[i] = 0;
    }
    // Misc keys: ESC to quit, L to increase window size
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        glfwSetWindowSize(window, 1024, 512);
        glfwSetWindowPos( window , 300 , 200 );
    }

    // ugly if statement block that adds all pressed keys to chip8 as follows:
    //
    // Keyboard:                Keypad:
    // +-+-+-+-+                +-+-+-+-+
    // |1|2|3|4|                |1|2|3|C|
    // +-+-+-+-+                +-+-+-+-+
    // |Q|W|E|R|                |4|5|6|D|
    // +-+-+-+-+       =>       +-+-+-+-+
    // |A|S|D|F|                |7|8|9|E|
    // +-+-+-+-+                +-+-+-+-+
    // |Z|X|C|V|                |A|0|B|F|
    // +-+-+-+-+                +-+-+-+-+
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {chip->key[0x1] = 1;}
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {chip->key[0x2] = 1;}
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {chip->key[0x3] = 1;}
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {chip->key[0xC] = 1;}
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {chip->key[0x4] = 1;}
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {chip->key[0x5] = 1;}
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {chip->key[0x6] = 1;}
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {chip->key[0xD] = 1;}
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {chip->key[0x7] = 1;}
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {chip->key[0x8] = 1;}
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {chip->key[0x9] = 1;}
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {chip->key[0xE] = 1;}
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {chip->key[0xA] = 1;}
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {chip->key[0x0] = 1;}
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {chip->key[0xB] = 1;}
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {chip->key[0xF] = 1;}
}


// React to change in window size.
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions
    glViewport(0, 0, width, height);
}


int main(int argc, char** argv) {
    run_window();
    return 0;
}
