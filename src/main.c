/*
 * Corrupt World
 * -------------
 * A small walkable 3D "corrupted" terrain demo written in C using GLFW
 * and legacy (fixed-function) OpenGL, so it builds with nothing more
 * than GLFW + the system GL/GLU libraries.
 *
 * The world is a heightfield grid whose vertices are displaced and
 * colored by a cheap pseudo-noise function that occasionally spikes,
 * producing a "glitchy / corrupted" look. The noise seed slowly drifts
 * over time so the corruption crawls across the terrain while you walk.
 *
 * Controls:
 *   W / A / S / D  - move
 *   Mouse          - look around
 *   Space / Shift  - up / down (fly a little, useful for exploring)
 *   R              - re-roll the corruption seed
 *   Esc            - quit
 */

#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GRID_SIZE      96      /* vertices per side              */
#define CELL_SCALE     1.5f    /* world units between vertices   */
#define MOVE_SPEED     8.0f
#define MOUSE_SENS     0.0022f

static int   g_width = 1280, g_height = 720;
static float g_seed = 0.0f;
static float g_time = 0.0f;

/* camera */
static float cam_x = 0.0f, cam_y = 6.0f, cam_z = 0.0f;
static float yaw = -90.0f, pitch = -15.0f;
static double last_mouse_x, last_mouse_y;
static int    mouse_captured = 1;

/* ---- cheap hash-based pseudo noise, deterministic per (x,z,seed) ---- */
static float hash2(float x, float z, float seed) {
    float n = sinf(x * 12.9898f + z * 78.233f + seed * 37.719f) * 43758.5453f;
    return n - floorf(n);
}

static float corrupt_noise(float x, float z) {
    float base = hash2(floorf(x), floorf(z), g_seed);
    float fx = x - floorf(x), fz = z - floorf(z);
    float n00 = hash2(floorf(x), floorf(z), g_seed);
    float n10 = hash2(floorf(x) + 1, floorf(z), g_seed);
    float n01 = hash2(floorf(x), floorf(z) + 1, g_seed);
    float n11 = hash2(floorf(x) + 1, floorf(z) + 1, g_seed);
    float nx0 = n00 + (n10 - n00) * fx;
    float nx1 = n01 + (n11 - n01) * fx;
    float smooth = nx0 + (nx1 - nx0) * fz;

    /* occasional "glitch spikes": rare cells jump wildly */
    float spike_roll = hash2(floorf(x) * 3.1f, floorf(z) * 7.7f, g_seed + 91.0f);
    float spike = (spike_roll > 0.965f) ? (hash2(x, z, g_seed + 5.0f) * 4.0f - 2.0f) : 0.0f;

    (void)base;
    return smooth + spike;
}

/* height of the terrain at world (x,z), including slow drift over time */
static float terrain_height(float x, float z) {
    float t = g_time * 0.15f;
    float n = corrupt_noise(x * 0.12f + t, z * 0.12f - t * 0.5f);
    return n * 3.0f;
}

/* color for a given height/noise value: greens/blues that corrupt into
 * hot magenta/red when a glitch spike is present */
static void corrupt_color(float x, float z, float h, float out[3]) {
    float glitch = fabsf(h) > 2.4f ? 1.0f : 0.0f;
    float base_r = 0.10f + 0.05f * sinf(x * 0.3f);
    float base_g = 0.35f + 0.15f * (h * 0.2f);
    float base_b = 0.45f + 0.10f * cosf(z * 0.3f);
    if (glitch > 0.5f) {
        float flicker = 0.5f + 0.5f * sinf(g_time * 40.0f + x * 3.0f + z * 3.0f);
        out[0] = 0.8f + 0.2f * flicker;
        out[1] = 0.05f;
        out[2] = 0.7f;
    } else {
        out[0] = base_r;
        out[1] = base_g < 0.05f ? 0.05f : base_g;
        out[2] = base_b;
    }
}

static void error_callback(int error, const char *desc) {
    fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

static void framebuffer_size_callback(GLFWwindow *win, int w, int h) {
    (void)win;
    g_width = w; g_height = h;
    glViewport(0, 0, w, h);
}

static void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, GLFW_TRUE);
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        g_seed = (float)(rand() % 100000);
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        mouse_captured = !mouse_captured;
        glfwSetInputMode(win, GLFW_CURSOR,
            mouse_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}

static void mouse_callback(GLFWwindow *win, double xpos, double ypos) {
    (void)win;
    if (!mouse_captured) { last_mouse_x = xpos; last_mouse_y = ypos; return; }
    double dx = xpos - last_mouse_x;
    double dy = ypos - last_mouse_y;
    last_mouse_x = xpos;
    last_mouse_y = ypos;
    yaw   += (float)(dx * MOUSE_SENS * 57.29578);
    pitch -= (float)(dy * MOUSE_SENS * 57.29578);
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

static void process_input(GLFWwindow *win, float dt) {
    float yaw_r = yaw * (float)M_PI / 180.0f;
    float fx = cosf(yaw_r), fz = sinf(yaw_r);
    float rx = -fz, rz = fx;
    float speed = MOVE_SPEED * dt;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) { cam_x += fx * speed; cam_z += fz * speed; }
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) { cam_x -= fx * speed; cam_z -= fz * speed; }
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) { cam_x += rx * speed; cam_z += rz * speed; }
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) { cam_x -= rx * speed; cam_z -= rz * speed; }
    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) cam_y += speed;
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam_y -= speed;

    /* keep the camera walking on the corrupted ground, with a little
     * headroom, unless the player has flown up on purpose */
    float ground = terrain_height(cam_x, cam_z) + 2.2f;
    if (cam_y < ground) cam_y = ground;
}

static void draw_terrain(void) {
    float half = (GRID_SIZE * CELL_SCALE) * 0.5f;

    for (int gz = 0; gz < GRID_SIZE - 1; gz++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int gx = 0; gx < GRID_SIZE; gx++) {
            for (int row = 0; row < 2; row++) {
                int zz = gz + row;
                float wx = gx * CELL_SCALE - half;
                float wz = zz * CELL_SCALE - half;
                float h = terrain_height(wx, wz);
                float col[3];
                corrupt_color(wx, wz, h, col);
                glColor3f(col[0], col[1], col[2]);
                glVertex3f(wx, h, wz);
            }
        }
        glEnd();
    }
}

static void draw_sky_glitch(void) {
    /* a few floating corrupted "shards" drifting above the terrain */
    for (int i = 0; i < 40; i++) {
        float fx = hash2((float)i, 1.0f, g_seed) * 200.0f - 100.0f;
        float fz = hash2((float)i, 2.0f, g_seed) * 200.0f - 100.0f;
        float fy = 8.0f + hash2((float)i, 3.0f, g_seed) * 14.0f
                   + sinf(g_time * 0.7f + i) * 0.6f;
        float s = 0.4f + hash2((float)i, 4.0f, g_seed) * 1.2f;
        float flick = 0.5f + 0.5f * sinf(g_time * 6.0f + i * 2.1f);

        glPushMatrix();
        glTranslatef(fx, fy, fz);
        glRotatef(g_time * 30.0f + i * 15.0f, 0.3f, 1.0f, 0.2f);
        glColor3f(0.9f, 0.1f * flick, 0.9f);
        glBegin(GL_TRIANGLES);
            glVertex3f(0, s, 0);
            glVertex3f(-s, -s, s);
            glVertex3f(s, -s, -s);
        glEnd();
        glPopMatrix();
    }
}

int main(void) {
    srand((unsigned)time(NULL));
    g_seed = (float)(rand() % 100000);

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow *win = glfwCreateWindow(g_width, g_height, "Corrupt World", NULL, NULL);
    if (!win) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetKeyCallback(win, key_callback);
    glfwSetCursorPosCallback(win, mouse_callback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(win, &last_mouse_x, &last_mouse_y);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.03f, 0.0f, 0.05f, 1.0f);

    double last_frame = glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - last_frame);
        last_frame = now;
        g_time = (float)now;

        process_input(win, dt);

        glViewport(0, 0, g_width, g_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = g_height > 0 ? (float)g_width / (float)g_height : 1.0f;
        float fov = 70.0f, znear = 0.1f, zfar = 400.0f;
        float top = znear * tanf(fov * 0.5f * (float)M_PI / 180.0f);
        float right = top * aspect;
        glFrustum(-right, right, -top, top, znear, zfar);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        float yaw_r = yaw * (float)M_PI / 180.0f;
        float pitch_r = pitch * (float)M_PI / 180.0f;
        float lx = cosf(yaw_r) * cosf(pitch_r);
        float ly = sinf(pitch_r);
        float lz = sinf(yaw_r) * cosf(pitch_r);

        gluLookAt(cam_x, cam_y, cam_z,
                  cam_x + lx, cam_y + ly, cam_z + lz,
                  0.0f, 1.0f, 0.0f);

        draw_terrain();
        draw_sky_glitch();

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
