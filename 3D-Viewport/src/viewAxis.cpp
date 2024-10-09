/*
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct Color {
    float r, g, b;
};

void drawAxis(const Color &color, float length) {
    glBegin(GL_LINES);
    glColor3f(color.r, color.g, color.b);

    glVertex2f(-length / 2.0f, 0);
    glVertex2f(length / 2.0f, 0);

    glEnd();
}

void drawAxes() {
    Color xColor = {1.0f, 0.0f, 0.0f};
    Color yColor = {0.0f, 1.0f, 0.0f};
    Color zColor = {0.0f, 0.0f, 1.0f};

    float axisLength = 100.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawAxis(xColor, axisLength);
    drawAxis(yColor, axisLength);
    drawAxis(zColor, axisLength);
}

int main() {
    if (!glfwInit()) {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Axis", NULL, NULL);
    if (!window) {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW" << endl;
        return -1;
    }

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        drawAxes();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
*/