#include <chealpix.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OpenGL/gl.h>
#include <GLUT/glut.h>

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
static GLuint textures[12];

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
            // printf("x=%.3f y=%.3f z=%.3f s=%.3f t=%.3f\n", x, y, z, s, t);
            tiles[i][j][0] = x;
            tiles[i][j][1] = y;
            tiles[i][j][2] = z;
            tiles[i][j][3] = s;
            tiles[i][j][4] = t;
        }
    }
    // puts("");

    for (i = 0; i < n; i ++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (j = 0; j <= n; j ++)
        {
            for (k = i; k < i + 2; k ++)
            {
                glTexCoord2f(tiles[k][j][3], tiles[k][j][4]);
                // glNormal3f(tiles[k][j][0], tiles[k][j][1], tiles[k][j][2]);
                glVertex3f(tiles[k][j][0], tiles[k][j][1], tiles[k][j][2]);
            }
        }
        glEnd();
    }
}

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    unsigned char iface;
    for (iface = 0; iface < 12; iface ++)
    {
        // printf("face %d\n", iface);
        glBindTexture(GL_TEXTURE_2D, textures[iface]);
        draw_patch(base_tile_xys[iface][0], base_tile_xys[iface][1], 16);
    }
    glDisable(GL_TEXTURE_2D);
    glFlush();
}

static void keyboard(unsigned char key, int x, int y) {
    switch (key)
    {
        case 27: /* escape */
        case 'q':
            exit(EXIT_SUCCESS);
        default:
            break;
    }
}

static void reproject()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -2.5);
    glRotatef(mousex, 0, 1, 0);
    glRotatef(mousey, 0, 0, 1);
}

static void motion(int x, int y)
{
    mousex = x;
    mousey = y;
    reproject();
    glutPostRedisplay();
}

static void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60., (GLfloat)w/(GLfloat)h, 1., 30.);
    reproject();
}

void OrRd(float x, float xmin, float xmax, GLubyte *rgb)
{
    static GLubyte colormap[3][3] = {
        {0xfe, 0xe8, 0xc8},
        {0xfd, 0xbb, 0x84},
        {0xe3, 0x4a, 0x33}
    };
    GLubyte *a, *b;
    x = (x - xmin) / (xmax - xmin);

    if (x < 0)
        memcpy(rgb, colormap[0], 3);
    else if (x > 1)
        memcpy(rgb, colormap[2], 3);
    else if (x != x) /* nan */
        memset(rgb, 0, 3);
    else {
        if (x < 0.5)
        {
            a = colormap[0];
            b = colormap[1];
        } else {
            a = colormap[1];
            b = colormap[2];
            x -= 0.5;
        }
        x *= 2;
        int i;
        for (i = 0; i < 3; i ++)
            rgb[i] = (1 - x) * a[i] + x * b[i];
    }
}

int main(int argc, char **argv)
{
    /* Load GLUT */
    glutInit(&argc, argv);

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

    GLubyte *pix = malloc(npix * 3 * sizeof(GLubyte));
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
        {
            OrRd(hp[i], 0, max, &pix[4*i]);
            pix[4*i+3] = 255;
        }
    }
    free(hp);

    glutInitWindowSize(600, 600);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("HEALPix Viewer");

    glClearColor(1, 1, 1, 0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glGenTextures(12, textures);
    {
        unsigned char ibase;
        for (ibase = 0; ibase < 12; ibase ++)
        {
            glBindTexture(GL_TEXTURE_2D, textures[ibase]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nside, nside, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, pix + 4 * nside * nside * ibase);
        }
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}
