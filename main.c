#define GLFW_INCLUDE_GLCOREARB
#include <chealpix.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>
// #include <GL/glu.h>

static int mousex = 0, mousey = 0;

static hpint64 interleave(hpint64 x, hpint64 y)
{
    unsigned char ibit = 0;
    hpint64 ipix = 0;

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

/* Load textures */
static GLuint textures[13];

static void draw_patch(
    float x0,
    float y0,
    unsigned int n
) {
    float tiles[n+1][n+1][5];
    int i, j, k;

    for (i = 0; i <= n; i ++)
    {
        for (j = 0; j <= n; j ++)
        {
            float x, y, z, phi;
            xy2zphi(x0 - 0.25 * (j - i) / n, y0 + 0.25 * (i + j) / n, &z, &phi);
            x = cosf(phi) * sqrtf(1 - squaref(z));
            y = sinf(phi) * sqrtf(1 - squaref(z));
            float s = (float)i / n;
            float t = (float)j / n;
            tiles[i][j][0] = x;
            tiles[i][j][1] = y;
            tiles[i][j][2] = z;
            tiles[i][j][3] = s;
            tiles[i][j][4] = t;
        }
    }

    for (i = 0; i < n; i ++)
    {
        // glBegin(GL_TRIANGLE_STRIP);
        // for (j = 0; j <= n; j ++)
        // {
        //     for (k = i; k < i + 2; k ++)
        //     {
        //         glTexCoord2f(tiles[k][j][3], tiles[k][j][4]);
        //         glNormal3f(tiles[k][j][0], tiles[k][j][1], tiles[k][j][2]);
        //         glVertex3f(tiles[k][j][0], tiles[k][j][1], tiles[k][j][2]);
        //     }
        // }
        // glEnd();
    }
}

static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

static void reproject()
{
    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();
    // glTranslatef(0, 0, -2.5);
    // glRotatef(mousex*0.5, 0, 1, 0);
    // glRotatef(mousey*0.5, 0, 0, 1);
}

static void motion(GLFWwindow *window, double x, double y)
{
    mousex = x;
    mousey = y;
    reproject();
}

static void resize(GLFWwindow *window, int w, int h)
{
    glViewport(0, 0, w, h);
    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();
    // gluPerspective(60., (GLfloat)w/(GLfloat)h, 1., 30.);
    reproject();
}

static void error(int error, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static const GLubyte colormap[][4] = {
    {0xfe, 0xe8, 0xc8, 0xFF},
    {0xfd, 0xbb, 0x84, 0xFF},
    {0xe3, 0x4a, 0x33, 0xFF}
};

#define GLSL(source) "#version 330\n" #source

static const char *vertex_shader_source = GLSL(
    in vec3 position;
    in vec2 datacoord;

    out vec2 Datacoord;

    void main(void) {
        gl_Position = vec4(position, 1);
        Datacoord = datacoord;
    }
);

static const char *fragment_shader_source = GLSL(
    in vec2 Datacoord;

    uniform sampler2D datamap;
    uniform sampler1D colormap;

    out vec4 outcolor;

    void main(void) {
        outcolor = texture(colormap, texture(datamap, Datacoord).r);
        // float r = texture(datamap, Datacoord).r;
        // outcolor = vec4(texture(datamap, Datacoord).r, 0, 0, 1);
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
    hpint64 npix = nside2npix64(nside);

    /* Convert to NEST ordering if necessary */
    if (ordering[0] == 'R') /* Ring indexing */
    {
        float *new_hp = malloc(npix * sizeof(*new_hp));
        if (!new_hp)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        hpint64 ipix_nest;
        for (ipix_nest = 0; ipix_nest < npix; ipix_nest ++)
        {
            hpint64 ipix_ring;
            nest2ring64(nside, ipix_nest, &ipix_ring);
            new_hp[ipix_nest] = hp[ipix_ring];
        }
        free(hp);
        hp = new_hp;
    }

    /* Rearrange into base tiles */
    {
        unsigned char ibase;
        for (ibase = 0; ibase < 12; ibase ++)
        {
            float *base = hp + nside * nside * ibase;
            float tile[nside][nside];
            long x, y;
            for (x = 0; x < nside; x ++)
                for (y = 0; y < nside; y ++)
                    tile[x][y] = base[interleave(y, x)];
            memcpy(base, tile, sizeof(tile));
        }
    }

    /* Rescale to range [0, 255] */
    GLubyte *pix = malloc(npix * sizeof(GLubyte));
    if (!pix)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    {
        hpint64 i;
        float max = hp[0];
        for (i = 1; i < npix; i ++)
            if (hp[i] > max)
                max = hp[i];
        for (i = 0; i < npix; i ++)
            pix[i] = hp[i] / max * 255;
    }
    free(hp);

    glfwSetErrorCallback(error);
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    GLFWwindow *window = glfwCreateWindow(600, 600, "HEALPix Viewer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    glClearColor(1, 1, 1, 0);
    glEnable(GL_DEPTH_TEST);
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
                GL_UNSIGNED_BYTE, pix + nside * nside * ibase);
        }
    
        /* Load texture for color map, make active in slot 1 */
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, textures[ibase]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, sizeof(colormap)/3, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, colormap);
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    static const float vertices[][5] = {
        {0, 0, 0, 0, 0},
        {0, 1, 0, 0, 1},
        {1, 0, 0, 1, 0},
        {1, 1, 0, 1, 1}
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

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

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glBindFragDataLocation(shader_program, 0, "outcolor");
    glLinkProgram(shader_program);
    glUseProgram(shader_program);

    GLint position_attrib = glGetAttribLocation(shader_program, "position");
    glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
    glEnableVertexAttribArray(position_attrib);

    GLint datacoord_attrib = glGetAttribLocation(shader_program, "datacoord");
    glVertexAttribPointer(datacoord_attrib, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(datacoord_attrib);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[9]);
    GLint datamap_uniform = glGetUniformLocation(shader_program, "datamap");
    glUniform1i(datamap_uniform, 0);

    GLint colormap_uniform = glGetUniformLocation(shader_program, "colormap");
    glUniform1i(colormap_uniform, 1);

    glfwSetKeyCallback(window, keyboard);
    glfwSetCursorPosCallback(window, motion);
    glfwSetFramebufferSizeCallback(window, resize);
    {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        resize(window, w, h);
    }
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        // unsigned char iface;
        // for (iface = 0; iface < 12; iface ++)
        // {
        //     glBindTexture(GL_TEXTURE_2D, textures[iface]);
        //     draw_patch(base_tile_xys[iface][0], base_tile_xys[iface][1], 16);
        // }
        glFlush();
        glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
