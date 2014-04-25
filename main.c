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

/* Load textures */
static GLuint textures[12];

typedef struct {
    float x;
    float y;
    float z;
    float u;
    float v;
} texturevec;

static texturevec bisect(texturevec v0, texturevec v1)
{
    texturevec r = {v0.x + v1.x, v0.y + v1.y, v0.z + v1.z,
        0.5 * (v0.u + v1.u), 0.5 * (v0.v + v1.v)};
    float norm = 1.f / sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
    r.x *= norm;
    r.y *= norm;
    r.z *= norm;
    return r;
}

static void draw_patch(
    texturevec v0,
    texturevec v1,
    texturevec v2,
    unsigned int lod
) {
    if (lod == 0)
    {
        glBegin(GL_TRIANGLES);
        glTexCoord2f(v0.u, v0.v);
        glNormal3f(v0.x, v0.y, v0.z);
        glVertex3f(v0.x, v0.y, v0.z);

        glTexCoord2f(v1.u, v1.v);
        glNormal3f(v1.x, v1.y, v1.z);
        glVertex3f(v1.x, v1.y, v1.z);

        glTexCoord2f(v2.u, v2.v);
        glNormal3f(v2.x, v2.y, v2.z);
        glVertex3f(v2.x, v2.y, v2.z);
        glEnd();
    } else {
        lod --;
        draw_patch(v0, bisect(v0, v1), bisect(v0, v2), lod);
        draw_patch(bisect(v0, v1), v1, bisect(v1, v2), lod);
        draw_patch(bisect(v0, v2), bisect(v1, v2), v2, lod);
        draw_patch(bisect(v0, v1), bisect(v1, v2), bisect(v2, v0), lod);
    }
}

static void display(void) {
    const float a = sqrt(5)/3;
    const float b = 2./3;
    const float c = 1/sqrt(2);
    const float vertices[14][3] = {
        { 0,  0,  1},
        { a,  0,  b},
        { 0,  a,  b},
        {-a,  0,  b},
        { 0, -a,  b},
        { c,  c,  0},
        {-c,  c,  0},
        {-c, -c,  0},
        { c, -c,  0},
        { a,  0, -b},
        { 0,  a, -b},
        {-a,  0, -b},
        { 0, -a, -b},
        { 0,  0, -1}
    };
    const unsigned int faces[12][4] = {
        {5, 1, 0, 2},
        {6, 2, 0, 3},
        {7, 3, 0, 4},
        {8, 4, 0, 1},
        {9, 8, 1, 5},
        {10, 5, 2, 6},
        {11, 6, 3, 7},
        {12, 7, 4, 8},
        {13, 9, 5, 10},
        {13, 10, 6, 11},
        {13, 11, 7, 12},
        {13, 12, 8, 9}
    };
    const float uvs[4][2] = {
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0}
    };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    unsigned char iface;
    for (iface = 0; iface < 12; iface ++)
    {
        unsigned char ivert;
        glBindTexture(GL_TEXTURE_2D, textures[iface]);
        texturevec tv[4];
        for (ivert = 0; ivert < 4; ivert ++)
        {
            const float *v = vertices[faces[iface][ivert]];
            const float *uv = uvs[ivert];
            tv[ivert].x = v[0];
            tv[ivert].y = v[1];
            tv[ivert].z = v[2];
            tv[ivert].u = uv[0];
            tv[ivert].v = uv[1];
        }
        draw_patch(tv[0], tv[1], tv[2], 5);
        draw_patch(tv[0], tv[2], tv[3], 5);
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
            pix[4*i] = hp[i] / max * 255;
            pix[4*i+1] = 0;//hp[i] / max * 255;
            pix[4*i+2] = 0;
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
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
