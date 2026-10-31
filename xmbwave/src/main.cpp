#include "spline.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <string>
#include <vector>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#define namespace _wlns
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
#include "xdg-shell-client-protocol.h"

namespace {

constexpr int GridRes = 100;
constexpr int ParticleCount = 1000;
constexpr double WaveScroll = 0.0;

struct App {
    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    wl_compositor *compositor = nullptr;
    zwlr_layer_shell_v1 *layer_shell = nullptr;
    wl_output *output = nullptr;
    wl_surface *surface = nullptr;
    zwlr_layer_surface_v1 *layer_surface = nullptr;
    wl_egl_window *egl_window = nullptr;
    EGLDisplay egl_display = EGL_NO_DISPLAY;
    EGLContext egl_context = EGL_NO_CONTEXT;
    EGLSurface egl_surface = EGL_NO_SURFACE;
    int32_t width = 0;
    int32_t height = 0;
    bool configured = false;
    bool size_dirty = true;
    XmbSpline spline;
    GLuint prog_bg = 0, prog_wave = 0, prog_part = 0;
    GLuint vbo_bg = 0, vbo_wave = 0, ibo_wave = 0, vbo_part = 0, ibo_part = 0;
    std::vector<float> wave_verts;
    int wave_index_count = 0;
    int fps = 30;
    float gain = 1.0f;
    float ink[3] = {1.0f, 1.0f, 1.0f};
    double time_s = 0.0;
    double particle_offset = 0.0;
    double last_frame_time = -1;
    float top_rgb[3] = {0.0f, 0.0f, 0.0f};
    float bottom_rgb[3] = {0.0f, 0.0f, 0.0f};
};

const char *bg_vs =
#include "bg.vert.inc"
;
const char *bg_fs =
#include "bg.frag.inc"
;
const char *wave_vs =
#include "wave.vert.inc"
;
const char *wave_fs =
#include "wave.frag.inc"
;
const char *part_vs =
#include "particles.vert.inc"
;
const char *part_fs =
#include "particles.frag.inc"
;

void parse_hex(const char *c, float out[3]) {
    if (*c == '#') c++;
    unsigned long v = strtoul(c, nullptr, 16);
    out[0] = ((v >> 16) & 0xff) / 255.0f;
    out[1] = ((v >> 8) & 0xff) / 255.0f;
    out[2] = (v & 0xff) / 255.0f;
}

double now_s() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

GLuint make_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        fprintf(stderr, "shader compile fail: %s\n", log);
        exit(1);
    }
    return s;
}
GLuint make_program(const char *vs, const char *fs) {
    GLuint v = make_shader(GL_VERTEX_SHADER, vs);
    GLuint f = make_shader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        fprintf(stderr, "link fail: %s\n", log);
        exit(1);
    }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

void setup_bg(App &a) {
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo_bg);
    float verts[8] = {
        0, 0,
        (float)a.width, 0,
        0, (float)a.height,
        (float)a.width, (float)a.height,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
}
void setup_wave(App &a) {
    a.wave_verts.resize(GridRes * GridRes * 8);
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo_wave);
    glBufferData(GL_ARRAY_BUFFER, a.wave_verts.size() * sizeof(float), nullptr, GL_STREAM_DRAW);
    std::vector<uint16_t> idx;
    idx.reserve((GridRes - 1) * GridRes * 2 + (GridRes - 2) * 2);
    for (int y = 0; y < GridRes - 1; ++y) {
        if (y > 0) idx.push_back((y + 1) * GridRes);
        for (int x = 0; x < GridRes; ++x) {
            idx.push_back((y + 1) * GridRes + x);
            idx.push_back(y * GridRes + x);
        }
        if (y < GridRes - 2) idx.push_back(y * GridRes + GridRes - 1);
    }
    a.wave_index_count = (int)idx.size();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a.ibo_wave);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(uint16_t), idx.data(), GL_STATIC_DRAW);
}
void setup_particles(App &a) {
    uint32_t s = 1u;
    auto rnd = [&]() { s = s * 1664525u + 1013904223u; return (s >> 8) & 0xffffff; };
    std::vector<float> seeds(ParticleCount * 4 * 4);
    for (int i = 0; i < ParticleCount; ++i) {
        float sx = rnd() / float(0xffffff);
        float sy = rnd() / float(0xffffff) * 2.0f - 1.0f;
        float sz = rnd() / float(0xffffff);
        for (int c = 0; c < 4; ++c) {
            float *o = seeds.data() + (i * 4 + c) * 4;
            o[0] = sx; o[1] = sy; o[2] = sz; o[3] = (float)c;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo_part);
    glBufferData(GL_ARRAY_BUFFER, seeds.size() * sizeof(float), seeds.data(), GL_STATIC_DRAW);
    std::vector<uint16_t> idx(ParticleCount * 6);
    for (int i = 0; i < ParticleCount; ++i) {
        uint16_t b = i * 4;
        idx[i * 6 + 0] = b + 0;
        idx[i * 6 + 1] = b + 1;
        idx[i * 6 + 2] = b + 2;
        idx[i * 6 + 3] = b + 2;
        idx[i * 6 + 4] = b + 1;
        idx[i * 6 + 5] = b + 3;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a.ibo_part);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(uint16_t), idx.data(), GL_STATIC_DRAW);
}

void update_wave(App &a, double t) {
    a.spline.compute(t, 1.0 / a.fps, t * WaveScroll);
    float *v = a.wave_verts.data();
    for (int y = 0; y < GridRes; ++y) {
        float fy = y / float(GridRes - 1);
        int r0, r1;
        float rt;
        XmbSpline::rowWeights(fy, r0, r1, rt);
        const float *a1 = a.spline.d1(r0);
        const float *b1 = a.spline.d1(r1);
        const float *a2 = a.spline.d2(r0);
        const float *b2 = a.spline.d2(r1);
        for (int x = 0; x < GridRes; ++x) {
            float *o = v + (y * GridRes + x) * 8;
            o[0] = x / float(GridRes - 1) * 2 - 1;
            o[1] = fy * 2 - 1;
            o[2] = a1[x] + (b1[x] - a1[x]) * rt;
            o[3] = a2[x] + (b2[x] - a2[x]) * rt;
        }
    }
    for (int y = 0; y < GridRes; ++y) {
        int ny = y < GridRes - 1 ? y + 1 : y - 1;
        for (int x = 0; x < GridRes; ++x) {
            int nx = x < GridRes - 1 ? x + 1 : x - 1;
            float *o = v + (y * GridRes + x) * 8;
            const float *px = v + (y * GridRes + nx) * 8;
            const float *py = v + (ny * GridRes + x) * 8;
            o[4] = px[2]; o[5] = px[3];
            o[6] = py[2]; o[7] = py[3];
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo_wave);
    glBufferData(GL_ARRAY_BUFFER, a.wave_verts.size() * sizeof(float), a.wave_verts.data(), GL_STREAM_DRAW);
}

void ortho(float out[16], float w, float h) {
    memset(out, 0, 64);
    out[0] = 2.0f / w;
    out[5] = -2.0f / h;
    out[10] = -1.0f;
    out[12] = -1.0f;
    out[13] = 1.0f;
    out[15] = 1.0f;
}

void draw(App &a) {
    double t = a.time_s;
    glViewport(0, 0, a.width, a.height);
    glClearColor(a.top_rgb[0], a.top_rgb[1], a.top_rgb[2], 1);
    glClear(GL_COLOR_BUFFER_BIT);

    float u_matrix[16];
    ortho(u_matrix, (float)a.width, (float)a.height);
    float u_view[4] = {(float)a.width, (float)a.height, (float)t, 1.0f};
    float ratio = std::clamp(a.height ? (float)a.width / a.height : 1.0f, 1.0f, 2.0f) * 0.375f;
    float u_top[4]    = {a.top_rgb[0], a.top_rgb[1], a.top_rgb[2], (float)(t + a.particle_offset)};
    float u_bottom[4] = {a.bottom_rgb[0], a.bottom_rgb[1], a.bottom_rgb[2], ratio};
    float u_opacity = 1.0f;

    auto set_commons = [&](GLuint p) {
        glUseProgram(p);
        glUniformMatrix4fv(glGetUniformLocation(p, "u_matrix"), 1, GL_FALSE, u_matrix);
        glUniform4fv(glGetUniformLocation(p, "u_view"), 1, u_view);
        glUniform4fv(glGetUniformLocation(p, "u_top"), 1, u_top);
        glUniform4fv(glGetUniformLocation(p, "u_bottom"), 1, u_bottom);
        glUniform1f(glGetUniformLocation(p, "u_opacity"), u_opacity);
        glUniform1f(glGetUniformLocation(p, "u_gain"), a.gain);
        glUniform3fv(glGetUniformLocation(p, "u_ink"), 1, a.ink);
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    set_commons(a.prog_bg);
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo_bg);
    GLint loc_pos = glGetAttribLocation(a.prog_bg, "pos");
    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_pos, 2, GL_FLOAT, GL_FALSE, 8, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(loc_pos);

    update_wave(a, t);
    set_commons(a.prog_wave);
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo_wave);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a.ibo_wave);
    GLint loc_data = glGetAttribLocation(a.prog_wave, "data");
    GLint loc_next = glGetAttribLocation(a.prog_wave, "next");
    glEnableVertexAttribArray(loc_data);
    glEnableVertexAttribArray(loc_next);
    glVertexAttribPointer(loc_data, 4, GL_FLOAT, GL_FALSE, 32, 0);
    glVertexAttribPointer(loc_next, 4, GL_FLOAT, GL_FALSE, 32, (const void *)16);
    glDrawElements(GL_TRIANGLE_STRIP, a.wave_index_count, GL_UNSIGNED_SHORT, 0);
    glDisableVertexAttribArray(loc_data);
    glDisableVertexAttribArray(loc_next);

    set_commons(a.prog_part);
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo_part);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a.ibo_part);
    GLint loc_seed = glGetAttribLocation(a.prog_part, "seed");
    glEnableVertexAttribArray(loc_seed);
    glVertexAttribPointer(loc_seed, 4, GL_FLOAT, GL_FALSE, 16, 0);
    glDrawElements(GL_TRIANGLES, ParticleCount * 6, GL_UNSIGNED_SHORT, 0);
    glDisableVertexAttribArray(loc_seed);
}

void layer_surface_configure(void *data, zwlr_layer_surface_v1 *ls, uint32_t serial, uint32_t w, uint32_t h) {
    auto *a = (App *)data;
    zwlr_layer_surface_v1_ack_configure(ls, serial);
    if ((int)w != a->width || (int)h != a->height) {
        a->width = w; a->height = h;
        if (a->egl_window) wl_egl_window_resize(a->egl_window, w, h, 0, 0);
        a->size_dirty = true;
    }
    a->configured = true;
}
void layer_surface_closed(void *data, zwlr_layer_surface_v1 *) {
    exit(0);
}
const zwlr_layer_surface_v1_listener layer_surface_listener = {
    layer_surface_configure, layer_surface_closed,
};

void registry_global(void *data, wl_registry *r, uint32_t name, const char *iface, uint32_t ver) {
    auto *a = (App *)data;
    if (!strcmp(iface, wl_compositor_interface.name))
        a->compositor = (wl_compositor *)wl_registry_bind(r, name, &wl_compositor_interface, 4);
    else if (!strcmp(iface, zwlr_layer_shell_v1_interface.name))
        a->layer_shell = (zwlr_layer_shell_v1 *)wl_registry_bind(r, name, &zwlr_layer_shell_v1_interface, 1);
    else if (!strcmp(iface, wl_output_interface.name) && !a->output)
        a->output = (wl_output *)wl_registry_bind(r, name, &wl_output_interface, 2);
}
void registry_remove(void *, wl_registry *, uint32_t) {}
const wl_registry_listener registry_listener = { registry_global, registry_remove };

} // namespace

int main() {
    App a;
    if (const char *g = getenv("XMB_GAIN")) a.gain = strtof(g, nullptr);
    if (const char *c = getenv("XMB_INK")) parse_hex(c, a.ink);
    if (const char *c = getenv("XMB_BG")) { parse_hex(c, a.top_rgb); parse_hex(c, a.bottom_rgb); }
    a.display = wl_display_connect(nullptr);
    if (!a.display) { fprintf(stderr, "wl_display_connect failed\n"); return 1; }
    a.registry = wl_display_get_registry(a.display);
    wl_registry_add_listener(a.registry, &registry_listener, &a);
    wl_display_roundtrip(a.display);
    if (!a.compositor || !a.layer_shell || !a.output) {
        fprintf(stderr, "missing globals: compositor=%p layer=%p output=%p\n", a.compositor, a.layer_shell, a.output);
        return 1;
    }

    a.surface = wl_compositor_create_surface(a.compositor);
    a.layer_surface = zwlr_layer_shell_v1_get_layer_surface(a.layer_shell, a.surface, a.output,
                        ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "xmbwave");
    zwlr_layer_surface_v1_add_listener(a.layer_surface, &layer_surface_listener, &a);
    zwlr_layer_surface_v1_set_size(a.layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_anchor(a.layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(a.layer_surface, -1);
    wl_surface_commit(a.surface);
    wl_display_roundtrip(a.display);
    while (!a.configured) wl_display_dispatch(a.display);

    a.egl_display = eglGetDisplay((EGLNativeDisplayType)a.display);
    EGLint ma, mi;
    eglInitialize(a.egl_display, &ma, &mi);
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint cfg_attribs[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 0, EGL_NONE };
    EGLConfig cfg;
    EGLint num;
    eglChooseConfig(a.egl_display, cfg_attribs, &cfg, 1, &num);
    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    a.egl_context = eglCreateContext(a.egl_display, cfg, EGL_NO_CONTEXT, ctx_attribs);
    a.egl_window = wl_egl_window_create(a.surface, a.width, a.height);
    a.egl_surface = eglCreateWindowSurface(a.egl_display, cfg, (EGLNativeWindowType)a.egl_window, nullptr);
    eglMakeCurrent(a.egl_display, a.egl_surface, a.egl_surface, a.egl_context);
    eglSwapInterval(a.egl_display, 1);

    a.prog_bg = make_program(bg_vs, bg_fs);
    a.prog_wave = make_program(wave_vs, wave_fs);
    a.prog_part = make_program(part_vs, part_fs);
    glGenBuffers(1, &a.vbo_bg);
    glGenBuffers(1, &a.vbo_wave);
    glGenBuffers(1, &a.ibo_wave);
    glGenBuffers(1, &a.vbo_part);
    glGenBuffers(1, &a.ibo_part);
    setup_wave(a);
    setup_particles(a);

    const double frame_s = 1.0 / a.fps;
    double start = now_s();
    while (true) {
        wl_display_dispatch_pending(a.display);
        if (a.size_dirty) { setup_bg(a); a.size_dirty = false; }
        a.time_s = now_s() - start;
        draw(a);
        eglSwapBuffers(a.egl_display, a.egl_surface);
        wl_display_flush(a.display);
        double sleep = frame_s - (now_s() - start) + std::floor((now_s() - start) / frame_s) * frame_s;
        if (sleep > 0) {
            struct timespec ts = { 0, (long)(sleep * 1e9) };
            nanosleep(&ts, nullptr);
        }
    }
    return 0;
}
