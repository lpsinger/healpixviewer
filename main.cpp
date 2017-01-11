#define GLFW_INCLUDE_GLCOREARB
#include <chealpix.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static int mousex = 0, mousey = 0, scrolly = 0;

static int64_t interleave(int64_t x, int64_t y)
{
    unsigned char ibit = 0;
    int64_t ipix = 0;

    while (x || y)
    {
        ipix |= (x & 1) << ibit;
        x >>= 1;
        ibit ++;
        ipix |= (y & 1) << ibit;
        y >>= 1;
        ibit ++;
    }

    return ipix;
}

static const float base_tile_xys[12][2] = {
    /* face  0 */ {0.25, 0},
    /* face  1 */ {0.75, 0},
    /* face  2 */ {1.25, 0},
    /* face  3 */ {1.75, 0},
    /* face  4 */ {0, -0.25},
    /* face  5 */ {0.5, -0.25},
    /* face  6 */ {1, -0.25},
    /* face  7 */ {1.5, -0.25},
    /* face  8 */ {0.25, -0.5},
    /* face  9 */ {0.75, -0.5},
    /* face 10 */ {1.25, -0.5},
    /* face 11 */ {1.75, -0.5}
};

static float squaref(float x)
{
    return x * x;
}

static void xy2zphi(float x, float y, float *z, float *phi)
{
    float abs_y = fabsf(y);
    if (abs_y <= 0.25)
    {
        *phi = M_PI * x;
        *z = 8. / 3 * y;
    } else {
        if (abs_y == 0.5)
        {
            *phi = 0;
        } else {
            *phi = M_PI * (x - (abs_y - 0.25) / (abs_y - 0.5)
                * (fmodf(x, 0.5) - 0.25));
        }
        *z = copysignf(1 - squaref(2 - 4 * abs_y) / 3, y);
    }
}

static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

static void motion(GLFWwindow *window, double x, double y)
{
    mousex = x;
    mousey = y;
}

static void scroll(GLFWwindow *window, double xoffset, double yoffset)
{
    scrolly += yoffset;
    if (scrolly < -100)
        scrolly = -100;
    else if (scrolly > 100)
        scrolly = 100;
}

static void error(int error, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static const GLubyte colormap[][4] = {
    /* OrRd */
    // {255, 247, 236, 255},
    // {255, 247, 235, 255},
    // {255, 246, 234, 255},
    // {255, 246, 233, 255},
    // {255, 245, 231, 255},
    // {255, 245, 230, 255},
    // {255, 244, 229, 255},
    // {255, 244, 228, 255},
    // {255, 243, 227, 255},
    // {255, 243, 226, 255},
    // {255, 242, 225, 255},
    // {255, 242, 224, 255},
    // {255, 241, 222, 255},
    // {255, 241, 221, 255},
    // {255, 240, 220, 255},
    // {255, 240, 219, 255},
    // {254, 239, 218, 255},
    // {254, 239, 217, 255},
    // {254, 239, 216, 255},
    // {254, 238, 215, 255},
    // {254, 238, 213, 255},
    // {254, 237, 212, 255},
    // {254, 237, 211, 255},
    // {254, 236, 210, 255},
    // {254, 236, 209, 255},
    // {254, 235, 208, 255},
    // {254, 235, 207, 255},
    // {254, 234, 206, 255},
    // {254, 234, 204, 255},
    // {254, 233, 203, 255},
    // {254, 233, 202, 255},
    // {254, 232, 201, 255},
    // {254, 232, 200, 255},
    // {254, 231, 199, 255},
    // {254, 231, 197, 255},
    // {254, 230, 196, 255},
    // {254, 229, 195, 255},
    // {254, 229, 193, 255},
    // {254, 228, 192, 255},
    // {254, 228, 191, 255},
    // {254, 227, 189, 255},
    // {254, 226, 188, 255},
    // {254, 226, 187, 255},
    // {254, 225, 185, 255},
    // {254, 224, 184, 255},
    // {254, 224, 183, 255},
    // {254, 223, 181, 255},
    // {254, 223, 180, 255},
    // {253, 222, 179, 255},
    // {253, 221, 177, 255},
    // {253, 221, 176, 255},
    // {253, 220, 175, 255},
    // {253, 219, 173, 255},
    // {253, 219, 172, 255},
    // {253, 218, 171, 255},
    // {253, 217, 170, 255},
    // {253, 217, 168, 255},
    // {253, 216, 167, 255},
    // {253, 216, 166, 255},
    // {253, 215, 164, 255},
    // {253, 214, 163, 255},
    // {253, 214, 162, 255},
    // {253, 213, 160, 255},
    // {253, 212, 159, 255},
    // {253, 212, 158, 255},
    // {253, 211, 157, 255},
    // {253, 210, 156, 255},
    // {253, 209, 155, 255},
    // {253, 209, 155, 255},
    // {253, 208, 154, 255},
    // {253, 207, 153, 255},
    // {253, 206, 152, 255},
    // {253, 206, 151, 255},
    // {253, 205, 150, 255},
    // {253, 204, 150, 255},
    // {253, 203, 149, 255},
    // {253, 202, 148, 255},
    // {253, 202, 147, 255},
    // {253, 201, 146, 255},
    // {253, 200, 146, 255},
    // {253, 199, 145, 255},
    // {253, 198, 144, 255},
    // {253, 198, 143, 255},
    // {253, 197, 142, 255},
    // {253, 196, 141, 255},
    // {253, 195, 141, 255},
    // {253, 195, 140, 255},
    // {253, 194, 139, 255},
    // {253, 193, 138, 255},
    // {253, 192, 137, 255},
    // {253, 191, 137, 255},
    // {253, 191, 136, 255},
    // {253, 190, 135, 255},
    // {253, 189, 134, 255},
    // {253, 188, 133, 255},
    // {253, 187, 133, 255},
    // {253, 186, 131, 255},
    // {253, 185, 130, 255},
    // {253, 184, 129, 255},
    // {253, 182, 127, 255},
    // {253, 181, 126, 255},
    // {253, 179, 125, 255},
    // {253, 178, 123, 255},
    // {253, 176, 122, 255},
    // {253, 175, 121, 255},
    // {253, 173, 119, 255},
    // {253, 172, 118, 255},
    // {253, 171, 117, 255},
    // {253, 169, 115, 255},
    // {253, 168, 114, 255},
    // {253, 166, 113, 255},
    // {253, 165, 111, 255},
    // {252, 163, 110, 255},
    // {252, 162, 109, 255},
    // {252, 160, 107, 255},
    // {252, 159, 106, 255},
    // {252, 158, 105, 255},
    // {252, 156, 103, 255},
    // {252, 155, 102, 255},
    // {252, 153, 100, 255},
    // {252, 152, 99, 255},
    // {252, 150, 98, 255},
    // {252, 149, 96, 255},
    // {252, 147, 95, 255},
    // {252, 146, 94, 255},
    // {252, 145, 92, 255},
    // {252, 143, 91, 255},
    // {252, 142, 90, 255},
    // {252, 140, 89, 255},
    // {251, 139, 88, 255},
    // {251, 138, 88, 255},
    // {251, 137, 87, 255},
    // {250, 135, 87, 255},
    // {250, 134, 86, 255},
    // {249, 133, 86, 255},
    // {249, 132, 85, 255},
    // {249, 130, 84, 255},
    // {248, 129, 84, 255},
    // {248, 128, 83, 255},
    // {247, 127, 83, 255},
    // {247, 125, 82, 255},
    // {246, 124, 82, 255},
    // {246, 123, 81, 255},
    // {246, 122, 81, 255},
    // {245, 120, 80, 255},
    // {245, 119, 80, 255},
    // {244, 118, 79, 255},
    // {244, 117, 79, 255},
    // {244, 115, 78, 255},
    // {243, 114, 78, 255},
    // {243, 113, 77, 255},
    // {242, 112, 76, 255},
    // {242, 110, 76, 255},
    // {242, 109, 75, 255},
    // {241, 108, 75, 255},
    // {241, 106, 74, 255},
    // {240, 105, 74, 255},
    // {240, 104, 73, 255},
    // {240, 103, 73, 255},
    // {239, 101, 72, 255},
    // {239, 100, 71, 255},
    // {238, 98, 70, 255},
    // {237, 97, 69, 255},
    // {236, 95, 67, 255},
    // {236, 93, 66, 255},
    // {235, 92, 65, 255},
    // {234, 90, 63, 255},
    // {233, 88, 62, 255},
    // {233, 87, 61, 255},
    // {232, 85, 60, 255},
    // {231, 83, 58, 255},
    // {230, 82, 57, 255},
    // {229, 80, 56, 255},
    // {229, 78, 54, 255},
    // {228, 77, 53, 255},
    // {227, 75, 52, 255},
    // {226, 73, 51, 255},
    // {226, 72, 49, 255},
    // {225, 70, 48, 255},
    // {224, 68, 47, 255},
    // {223, 67, 45, 255},
    // {223, 65, 44, 255},
    // {222, 63, 43, 255},
    // {221, 62, 42, 255},
    // {220, 60, 40, 255},
    // {220, 58, 39, 255},
    // {219, 57, 38, 255},
    // {218, 55, 36, 255},
    // {217, 53, 35, 255},
    // {217, 52, 34, 255},
    // {216, 50, 33, 255},
    // {215, 48, 31, 255},
    // {214, 47, 30, 255},
    // {213, 45, 29, 255},
    // {212, 44, 28, 255},
    // {211, 42, 27, 255},
    // {210, 41, 26, 255},
    // {209, 39, 25, 255},
    // {207, 38, 24, 255},
    // {206, 36, 23, 255},
    // {205, 35, 22, 255},
    // {204, 33, 22, 255},
    // {203, 32, 21, 255},
    // {202, 30, 20, 255},
    // {201, 29, 19, 255},
    // {199, 27, 18, 255},
    // {198, 26, 17, 255},
    // {197, 24, 16, 255},
    // {196, 23, 15, 255},
    // {195, 21, 14, 255},
    // {194, 20, 13, 255},
    // {193, 18, 12, 255},
    // {192, 17, 11, 255},
    // {190, 15, 10, 255},
    // {189, 14, 9, 255},
    // {188, 12, 8, 255},
    // {187, 11, 7, 255},
    // {186, 9, 6, 255},
    // {185, 8, 5, 255},
    // {184, 6, 4, 255},
    // {183, 5, 3, 255},
    // {181, 3, 2, 255},
    // {180, 2, 1, 255},
    // {179, 0, 0, 255},
    // {178, 0, 0, 255},
    // {176, 0, 0, 255},
    // {174, 0, 0, 255},
    // {173, 0, 0, 255},
    // {171, 0, 0, 255},
    // {169, 0, 0, 255},
    // {168, 0, 0, 255},
    // {166, 0, 0, 255},
    // {165, 0, 0, 255},
    // {163, 0, 0, 255},
    // {161, 0, 0, 255},
    // {160, 0, 0, 255},
    // {158, 0, 0, 255},
    // {156, 0, 0, 255},
    // {155, 0, 0, 255},
    // {153, 0, 0, 255},
    // {151, 0, 0, 255},
    // {150, 0, 0, 255},
    // {148, 0, 0, 255},
    // {147, 0, 0, 255},
    // {145, 0, 0, 255},
    // {143, 0, 0, 255},
    // {142, 0, 0, 255},
    // {140, 0, 0, 255},
    // {138, 0, 0, 255},
    // {137, 0, 0, 255},
    // {135, 0, 0, 255},
    // {134, 0, 0, 255},
    // {132, 0, 0, 255},
    // {130, 0, 0, 255},
    // {129, 0, 0, 255},
    // {127, 0, 0, 255}

    /* Jet */
    {0, 0, 128, 255},
    {0, 0, 132, 255},
    {0, 0, 137, 255},
    {0, 0, 141, 255},
    {0, 0, 146, 255},
    {0, 0, 150, 255},
    {0, 0, 155, 255},
    {0, 0, 159, 255},
    {0, 0, 164, 255},
    {0, 0, 168, 255},
    {0, 0, 173, 255},
    {0, 0, 178, 255},
    {0, 0, 182, 255},
    {0, 0, 187, 255},
    {0, 0, 191, 255},
    {0, 0, 196, 255},
    {0, 0, 200, 255},
    {0, 0, 205, 255},
    {0, 0, 209, 255},
    {0, 0, 214, 255},
    {0, 0, 218, 255},
    {0, 0, 223, 255},
    {0, 0, 227, 255},
    {0, 0, 232, 255},
    {0, 0, 237, 255},
    {0, 0, 241, 255},
    {0, 0, 246, 255},
    {0, 0, 250, 255},
    {0, 0, 255, 255},
    {0, 0, 255, 255},
    {0, 0, 255, 255},
    {0, 0, 255, 255},
    {0, 1, 255, 255},
    {0, 4, 255, 255},
    {0, 9, 255, 255},
    {0, 13, 255, 255},
    {0, 17, 255, 255},
    {0, 20, 255, 255},
    {0, 25, 255, 255},
    {0, 29, 255, 255},
    {0, 33, 255, 255},
    {0, 36, 255, 255},
    {0, 41, 255, 255},
    {0, 45, 255, 255},
    {0, 49, 255, 255},
    {0, 52, 255, 255},
    {0, 57, 255, 255},
    {0, 61, 255, 255},
    {0, 65, 255, 255},
    {0, 68, 255, 255},
    {0, 73, 255, 255},
    {0, 77, 255, 255},
    {0, 81, 255, 255},
    {0, 84, 255, 255},
    {0, 89, 255, 255},
    {0, 93, 255, 255},
    {0, 97, 255, 255},
    {0, 100, 255, 255},
    {0, 105, 255, 255},
    {0, 109, 255, 255},
    {0, 113, 255, 255},
    {0, 116, 255, 255},
    {0, 121, 255, 255},
    {0, 125, 255, 255},
    {0, 129, 255, 255},
    {0, 133, 255, 255},
    {0, 136, 255, 255},
    {0, 141, 255, 255},
    {0, 145, 255, 255},
    {0, 149, 255, 255},
    {0, 153, 255, 255},
    {0, 157, 255, 255},
    {0, 161, 255, 255},
    {0, 165, 255, 255},
    {0, 168, 255, 255},
    {0, 173, 255, 255},
    {0, 177, 255, 255},
    {0, 181, 255, 255},
    {0, 185, 255, 255},
    {0, 189, 255, 255},
    {0, 193, 255, 255},
    {0, 197, 255, 255},
    {0, 200, 255, 255},
    {0, 205, 255, 255},
    {0, 209, 255, 255},
    {0, 213, 255, 255},
    {0, 217, 255, 255},
    {0, 221, 254, 255},
    {0, 225, 251, 255},
    {0, 229, 248, 255},
    {2, 232, 244, 255},
    {6, 237, 241, 255},
    {9, 241, 238, 255},
    {12, 245, 235, 255},
    {15, 249, 231, 255},
    {19, 253, 228, 255},
    {22, 255, 225, 255},
    {25, 255, 222, 255},
    {28, 255, 219, 255},
    {31, 255, 215, 255},
    {35, 255, 212, 255},
    {38, 255, 209, 255},
    {41, 255, 206, 255},
    {44, 255, 202, 255},
    {48, 255, 199, 255},
    {51, 255, 196, 255},
    {54, 255, 193, 255},
    {57, 255, 190, 255},
    {60, 255, 186, 255},
    {64, 255, 183, 255},
    {67, 255, 180, 255},
    {70, 255, 177, 255},
    {73, 255, 173, 255},
    {77, 255, 170, 255},
    {80, 255, 167, 255},
    {83, 255, 164, 255},
    {86, 255, 160, 255},
    {90, 255, 157, 255},
    {93, 255, 154, 255},
    {96, 255, 151, 255},
    {99, 255, 148, 255},
    {102, 255, 144, 255},
    {106, 255, 141, 255},
    {109, 255, 138, 255},
    {112, 255, 135, 255},
    {115, 255, 131, 255},
    {119, 255, 128, 255},
    {122, 255, 125, 255},
    {125, 255, 122, 255},
    {128, 255, 119, 255},
    {131, 255, 115, 255},
    {135, 255, 112, 255},
    {138, 255, 109, 255},
    {141, 255, 106, 255},
    {144, 255, 102, 255},
    {148, 255, 99, 255},
    {151, 255, 96, 255},
    {154, 255, 93, 255},
    {157, 255, 90, 255},
    {160, 255, 86, 255},
    {164, 255, 83, 255},
    {167, 255, 80, 255},
    {170, 255, 77, 255},
    {173, 255, 73, 255},
    {177, 255, 70, 255},
    {180, 255, 67, 255},
    {183, 255, 64, 255},
    {186, 255, 60, 255},
    {190, 255, 57, 255},
    {193, 255, 54, 255},
    {196, 255, 51, 255},
    {199, 255, 48, 255},
    {202, 255, 44, 255},
    {206, 255, 41, 255},
    {209, 255, 38, 255},
    {212, 255, 35, 255},
    {215, 255, 31, 255},
    {219, 255, 28, 255},
    {222, 255, 25, 255},
    {225, 255, 22, 255},
    {228, 255, 19, 255},
    {231, 255, 15, 255},
    {235, 255, 12, 255},
    {238, 255, 9, 255},
    {241, 252, 6, 255},
    {244, 248, 2, 255},
    {248, 245, 0, 255},
    {251, 241, 0, 255},
    {254, 237, 0, 255},
    {255, 234, 0, 255},
    {255, 230, 0, 255},
    {255, 226, 0, 255},
    {255, 222, 0, 255},
    {255, 219, 0, 255},
    {255, 215, 0, 255},
    {255, 211, 0, 255},
    {255, 208, 0, 255},
    {255, 204, 0, 255},
    {255, 200, 0, 255},
    {255, 196, 0, 255},
    {255, 193, 0, 255},
    {255, 189, 0, 255},
    {255, 185, 0, 255},
    {255, 182, 0, 255},
    {255, 178, 0, 255},
    {255, 174, 0, 255},
    {255, 171, 0, 255},
    {255, 167, 0, 255},
    {255, 163, 0, 255},
    {255, 159, 0, 255},
    {255, 156, 0, 255},
    {255, 152, 0, 255},
    {255, 148, 0, 255},
    {255, 145, 0, 255},
    {255, 141, 0, 255},
    {255, 137, 0, 255},
    {255, 134, 0, 255},
    {255, 130, 0, 255},
    {255, 126, 0, 255},
    {255, 122, 0, 255},
    {255, 119, 0, 255},
    {255, 115, 0, 255},
    {255, 111, 0, 255},
    {255, 108, 0, 255},
    {255, 104, 0, 255},
    {255, 100, 0, 255},
    {255, 96, 0, 255},
    {255, 93, 0, 255},
    {255, 89, 0, 255},
    {255, 85, 0, 255},
    {255, 82, 0, 255},
    {255, 78, 0, 255},
    {255, 74, 0, 255},
    {255, 71, 0, 255},
    {255, 67, 0, 255},
    {255, 63, 0, 255},
    {255, 59, 0, 255},
    {255, 56, 0, 255},
    {255, 52, 0, 255},
    {255, 48, 0, 255},
    {255, 45, 0, 255},
    {255, 41, 0, 255},
    {255, 37, 0, 255},
    {255, 34, 0, 255},
    {255, 30, 0, 255},
    {255, 26, 0, 255},
    {255, 22, 0, 255},
    {255, 19, 0, 255},
    {250, 15, 0, 255},
    {246, 11, 0, 255},
    {241, 8, 0, 255},
    {237, 4, 0, 255},
    {232, 0, 0, 255},
    {228, 0, 0, 255},
    {223, 0, 0, 255},
    {218, 0, 0, 255},
    {214, 0, 0, 255},
    {209, 0, 0, 255},
    {205, 0, 0, 255},
    {200, 0, 0, 255},
    {196, 0, 0, 255},
    {191, 0, 0, 255},
    {187, 0, 0, 255},
    {182, 0, 0, 255},
    {178, 0, 0, 255},
    {173, 0, 0, 255},
    {168, 0, 0, 255},
    {164, 0, 0, 255},
    {159, 0, 0, 255},
    {155, 0, 0, 255},
    {150, 0, 0, 255},
    {146, 0, 0, 255},
    {141, 0, 0, 255},
    {137, 0, 0, 255},
    {132, 0, 0, 255},
    {128, 0, 0, 255}
};

#define GLSL(source) "#version 330\n" #source

static const char *vertex_shader_source = GLSL(
    in vec3 position;
    in vec2 datacoord;

    uniform mat4 proj;
    uniform mat4 view;
    uniform mat4 model;

    out vec2 Datacoord;
    out float lum;

    void main(void) {
        vec4 normal_cam = view * model * vec4(position, 0);
        lum = 0.25+normalize(normal_cam).z;
        gl_Position = proj * view * model * vec4(position, 1);
        Datacoord = datacoord;
    }
);

static const char *fragment_shader_source = GLSL(
    in vec2 Datacoord;
    in float lum;

    uniform sampler2D datamap;
    uniform sampler1D colormap;

    out vec4 outcolor;

    void main(void) {
        outcolor = lum * texture(colormap, texture(datamap, Datacoord).r);
    }
);

int main(int argc, char **argv)
{
    /* Validate input */
    if (argc != 2)
    {
        puts("usage: healpixviewer INPUT.fits[.gz]");
        exit(EXIT_FAILURE);
    }

    const char *fitsfilename = argv[1];
    long nside;
    char coordsys[80];
    char ordering[80];

    /* Read sky map from FITS file */
    float *hp = read_healpix_map(
        fitsfilename, &nside, coordsys, ordering);
    if (!hp)
    {
        fputs("error: read_healpix_map\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Determine total number of pixels */
    int64_t npix = nside2npix64(nside);

    /* Convert to NEST ordering if necessary */
    if (ordering[0] == 'R') /* Ring indexing */
    {
        float *new_hp = (float *) malloc(npix * sizeof(*new_hp));
        if (!new_hp)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        int64_t ipix_nest;
        for (ipix_nest = 0; ipix_nest < npix; ipix_nest ++)
        {
            int64_t ipix_ring;
            nest2ring64(nside, ipix_nest, &ipix_ring);
            new_hp[ipix_nest] = hp[ipix_ring];
        }
        free(hp);
        hp = new_hp;
    }

    /* Rearrange into base tiles */
    {
        float *tile = (float *)malloc(nside * nside * sizeof(*hp));
        if (!tile)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        for (unsigned char ibase = 0; ibase < 12; ibase ++)
        {
            float *base = hp + nside * nside * ibase;
            long x, y;
            for (x = 0; x < nside; x ++)
                for (y = 0; y < nside; y ++)
                    tile[x * nside + y] = base[interleave(y, x)];
            memcpy(base, tile, nside * nside * sizeof(*tile));
        }
        free(tile);
    }

    /* Rescale to range [0, 255] */
    GLushort *pix = (GLushort *) malloc(npix * sizeof(GLushort));
    if (!pix)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    {
        float max = hp[0];
        for (int64_t i = 1; i < npix; i ++)
            if (hp[i] > max)
                max = hp[i];
        for (int64_t i = 0; i < npix; i ++)
            pix[i] = hp[i] / max * 65535;
    }
    free(hp);

    /* Initialize window system */
    glfwSetErrorCallback(error);
    if (!glfwInit())
        exit(EXIT_FAILURE);

    /* Necessary to get modern OpenGL context on Mavericks */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);

    /* 4x antialiasing */
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(600, 600, "HEALPix Viewer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    /* Load textures */
    GLuint textures[13];
    glGenTextures(13, textures);
    {
        /* Load textures for faces */
        unsigned char ibase;
        for (ibase = 0; ibase < 12; ibase ++)
        {
            glBindTexture(GL_TEXTURE_2D, textures[ibase]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, nside, nside, 0, GL_RED,
                GL_UNSIGNED_SHORT, pix + nside * nside * ibase);
        }
    
        /* Load texture for color map, make active in slot 1 */
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, textures[ibase]);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, sizeof(colormap)/4, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, colormap);
    }
    free(pix);

    /* Compile shaders */
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    {
        GLint status;

        glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE)
        {
            char buffer[512];
            glGetShaderInfoLog(vertex_shader, sizeof(buffer), NULL, buffer);
            fprintf(stderr, "error compiling vertex shader: %s\n", buffer);
            exit(EXIT_FAILURE);
        }

        glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE)
        {
            char buffer[512];
            glGetShaderInfoLog(fragment_shader, sizeof(buffer), NULL, buffer);
            fprintf(stderr, "error compiling fragment shader: %s\n", buffer);
            exit(EXIT_FAILURE);
        }
    }

    /* Link shaders */
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glBindFragDataLocation(shader_program, 0, "outcolor");
    glLinkProgram(shader_program);
    glUseProgram(shader_program);

    /* Get all attribs and uniforms */
    GLint position_attrib = glGetAttribLocation(shader_program, "position");
    GLint datacoord_attrib = glGetAttribLocation(shader_program, "datacoord");
    GLint proj_uniform = glGetUniformLocation(shader_program, "proj");
    GLint view_uniform = glGetUniformLocation(shader_program, "view");
    GLint model_uniform = glGetUniformLocation(shader_program, "model");
    GLint datamap_uniform = glGetUniformLocation(shader_program, "datamap");
    GLint colormap_uniform = glGetUniformLocation(shader_program, "colormap");

    /* Upload vertices, face by face */
    GLuint vaos[12];
    glGenVertexArrays(12, vaos);
    static const unsigned int ndiv = 16;
    {
        GLuint ebo;
        {
            GLushort elements[ndiv * ndiv * 3 * 2];
            GLushort *el = elements;
            for (int i = 0; i < ndiv; i ++)
            {
                for (int j = 0; j < ndiv; j ++)
                {
                    *el++ = (i+0) * (ndiv + 1) + (j+0);
                    *el++ = (i+0) * (ndiv + 1) + (j+1);
                    *el++ = (i+1) * (ndiv + 1) + (j+0);
                    *el++ = (i+1) * (ndiv + 1) + (j+0);
                    *el++ = (i+0) * (ndiv + 1) + (j+1);
                    *el++ = (i+1) * (ndiv + 1) + (j+1);
                }
            }

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
        }

        for (unsigned char ibase = 0; ibase < 12; ibase ++)
        {
            glBindVertexArray(vaos[ibase]);

            /* Upload vertex buffer */
            {
                GLfloat vertices[ndiv+1][ndiv+1][5];
                const float x0 = base_tile_xys[ibase][0];
                const float y0 = base_tile_xys[ibase][1];
                for (int i = 0; i <= ndiv; i ++)
                {
                    for (int j = 0; j <= ndiv; j ++)
                    {
                        float x, y, z, phi;
                        xy2zphi(x0 - 0.25 * (j - i) / ndiv, y0 + 0.25 * (i + j) / ndiv, &z, &phi);
                        x = cosf(phi) * sqrtf(1 - squaref(z));
                        y = sinf(phi) * sqrtf(1 - squaref(z));
                        float s = (float)i / ndiv;
                        float t = (float)j / ndiv;
                        vertices[i][j][0] = x;
                        vertices[i][j][1] = y;
                        vertices[i][j][2] = z;
                        vertices[i][j][3] = s;
                        vertices[i][j][4] = t;
                    }
                }

                GLuint vbo;
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            }

            /* We'll reuse the same ebo for each face. */
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

            /* Specify attribute layout */
            glEnableVertexAttribArray(position_attrib);
            glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE,
                5 * sizeof(GLfloat), 0);

            glEnableVertexAttribArray(datacoord_attrib);
            glVertexAttribPointer(datacoord_attrib, 2, GL_FLOAT, GL_FALSE,
                5 * sizeof(GLfloat), (void *) (3 * sizeof(GLfloat)));
        }
    }

    /* Pass active textures */
    glUniform1i(datamap_uniform, 0);
    glUniform1i(colormap_uniform, 1);

    /* Switch to active texture 0 so that we can cycle through tiles */
    glActiveTexture(GL_TEXTURE0);

    glfwSetKeyCallback(window, keyboard);
    glfwSetCursorPosCallback(window, motion);
    glfwSetScrollCallback(window, scroll);

    glClearColor(0, 0, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::mat4 model;
        model = glm::rotate(model, 0.5f*mousex, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, -0.5f*mousey, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(45.0f, (float)width/height, 1.0f, 10.0f);
        glViewport(0, 0, width, height);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(proj_uniform, 1, GL_FALSE, glm::value_ptr(proj));
        glm::mat4 view = glm::lookAt(
            glm::vec3(3.0f + 0.01f*scrolly, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        unsigned char iface;
        for (iface = 0; iface < 12; iface ++)
        {
            glBindVertexArray(vaos[iface]);
            glBindTexture(GL_TEXTURE_2D, textures[iface]);
            glDrawElements(GL_TRIANGLES, ndiv*ndiv*3*2, GL_UNSIGNED_SHORT, 0);
        }
        glFlush();
        glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
