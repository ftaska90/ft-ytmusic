/*
 * FT-YT Music Panel
 * Created by ft_aska.90
 * Copyright (c) 2026 ft_aska.90
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TITLE_WIDTH 14
#define SPECTRUM_WIDTH 24
#define SAMPLE_RATE 44100
#define FFT_FRAME 1024
#define SPECTRUM_FPS 20
#define META_INTERVAL_MS 250

static const char *DEFAULT_PLAYLIST =
    "https://www.youtube.com/watch?v=ChukpOHfAI8&list=RDChukpOHfAI8&start_radio=1";

static const char *LEVELS[] = {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};

static const char *SPECTRUM_COLORS[SPECTRUM_WIDTH] = {
    "#3b82f6", "#4389ed", "#4b90e4", "#5397db", "#5b9ed2", "#63a5c9",
    "#6bacc0", "#73b3b7", "#7bbaae", "#83c1a5", "#8bc89c", "#93cf93",
    "#a4d680", "#b5dd6d", "#c6e45a", "#d7db47", "#e8d234", "#f3c82e",
    "#f5b42d", "#f79f2c", "#f98a2b", "#fb752a", "#fd6029", "#ef4444"
};

static volatile sig_atomic_t g_running = 1;

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static void sleep_ms(int ms) {
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000L };
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR && g_running) {}
}

static const char *runtime_dir(void) {
    const char *p = getenv("XDG_RUNTIME_DIR");
    return (p && *p) ? p : "/tmp";
}

static void path_join(char *out, size_t n, const char *name) {
    snprintf(out, n, "%s/%s", runtime_dir(), name);
}

static void trim(char *s) {
    size_t n;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static void xml_escape(const char *src, char *dst, size_t dstn) {
    size_t j = 0;
    for (size_t i = 0; src && src[i] && j + 8 < dstn; i++) {
        const char *r = NULL;
        switch (src[i]) {
            case '&': r = "&amp;"; break;
            case '<': r = "&lt;"; break;
            case '>': r = "&gt;"; break;
            case '\"': r = "&quot;"; break;
            case '\'': r = "&apos;"; break;
            default: break;
        }
        if (r) {
            size_t rn = strlen(r);
            memcpy(dst + j, r, rn);
            j += rn;
        } else {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}

static int atomic_write(const char *path, const char *fmt, ...) {
    char tmp[768];
    snprintf(tmp, sizeof(tmp), "%s.tmp.%ld", path, (long)getpid());
    FILE *f = fopen(tmp, "w");
    if (!f) return -1;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (rename(tmp, path) != 0) {
        unlink(tmp);
        return -1;
    }
    return 0;
}

static int command_read(char *out, size_t outn, const char *cmd) {
    FILE *p = popen(cmd, "r");
    if (!p) {
        if (outn) out[0] = '\0';
        return -1;
    }
    size_t used = 0;
    while (used + 1 < outn) {
        size_t r = fread(out + used, 1, outn - used - 1, p);
        used += r;
        if (r == 0) break;
    }
    out[used] = '\0';
    int rc = pclose(p);
    trim(out);
    return rc;
}

static int command_ok(const char *cmd) {
    int rc = system(cmd);
    return rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) == 0;
}

static void marquee(const char *title, long long tick, char out[TITLE_WIDTH + 1]) {
    const char *fallback = "FT-YT Music";
    if (!title || !*title) title = fallback;

    size_t len = strlen(title);
    if (len <= TITLE_WIDTH) {
        memset(out, ' ', TITLE_WIDTH);
        memcpy(out, title, len);
        out[TITLE_WIDTH] = '\0';
        return;
    }

    size_t cycle = len + 3;
    size_t offset = (size_t)((tick / 500LL) % (long long)cycle);

    for (int i = 0; i < TITLE_WIDTH; i++) {
        size_t pos = (offset + (size_t)i) % cycle;
        out[i] = pos < len ? title[pos] : ' ';
    }
    out[TITLE_WIDTH] = '\0';
}

static int read_first_line(const char *path, char *out, size_t n) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(out, (int)n, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    trim(out);
    return 0;
}

static void spectrum_markup_from_levels(const int levels[SPECTRUM_WIDTH], char *out, size_t outn) {
    size_t used = 0;
    out[0] = '\0';
    for (int i = 0; i < SPECTRUM_WIDTH; i++) {
        int lv = levels[i];
        if (lv < 0) lv = 0;
        if (lv > 8) lv = 8;
        int n = snprintf(out + used, outn - used,
                         "<span foreground=\"%s\">%s</span>",
                         SPECTRUM_COLORS[i], LEVELS[lv]);
        if (n < 0 || (size_t)n >= outn - used) break;
        used += (size_t)n;
    }
}

static int parse_levels(const char *s, int out[SPECTRUM_WIDTH]) {
    for (int i = 0; i < SPECTRUM_WIDTH; i++) out[i] = 1;
    if (!s || !*s) return -1;
    int i = 0;
    const char *p = s;
    while (*p && i < SPECTRUM_WIDTH) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (!*p) break;
        char *end = NULL;
        long v = strtol(p, &end, 10);
        if (end == p) break;
        if (v < 0) v = 0;
        if (v > 8) v = 8;
        out[i++] = (int)v;
        p = end;
    }
    return i == SPECTRUM_WIDTH ? 0 : -1;
}

static int emit_panel(void) {
    char meta_path[512], spec_path[512];
    path_join(meta_path, sizeof(meta_path), "ftyt-meta-state");
    path_join(spec_path, sizeof(spec_path), "ftyt-spectrum-state");

    char title[512] = "FT-YT Music";
    char raw[4096] = {0};
    char state[32] = "Stopped";

    FILE *m = fopen(meta_path, "r");
    if (m) {
        if (fgets(title, sizeof(title), m)) trim(title);
        if (fgets(state, sizeof(state), m)) trim(state);
        fclose(m);
    }

    int levels[SPECTRUM_WIDTH];
    for (int i = 0; i < SPECTRUM_WIDTH; i++) levels[i] = 1;
    if (read_first_line(spec_path, raw, sizeof(raw)) == 0) parse_levels(raw, levels);

    char visible[TITLE_WIDTH + 1];
    marquee(title, now_ms(), visible);

    char escaped[512];
    xml_escape(visible, escaped, sizeof(escaped));

    char spec_markup[8192];
    spectrum_markup_from_levels(levels, spec_markup, sizeof(spec_markup));

    const char *title_color = strcmp(state, "Playing") == 0 ? "#e5e7eb" : "#9ca3af";

    printf("<txt><span font_family=\"monospace\"><span foreground=\"%s\">%s</span>  %s</span></txt>\n",
           title_color, escaped, spec_markup);
    printf("<tool>%s — click: play/pause</tool>\n", strcmp(state, "Playing") == 0 ? "Playing" : "Paused/Stopped");
    printf("<click>%s/.local/bin/ftyt-panel toggle</click>\n", getenv("HOME") ? getenv("HOME") : "~");
    return 0;
}

static int playerctl(const char *action) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "playerctl -p mpv %s >/dev/null 2>&1", action);
    return command_ok(cmd) ? 0 : 1;
}

static int cmd_start(void) {
    char status[64] = {0};
    if (command_read(status, sizeof(status), "playerctl -p mpv status 2>/dev/null") == 0 && status[0]) {
        if (strcmp(status, "Paused") == 0) playerctl("play");
        return 0;
    }

    const char *playlist = getenv("FTYT_PLAYLIST");
    if (!playlist || !*playlist) playlist = DEFAULT_PLAYLIST;

    char quoted[2048];
    size_t q = 0;
    quoted[q++] = '\'';
    for (size_t i = 0; playlist[i] && q + 5 < sizeof(quoted); i++) {
        if (playlist[i] == '\'') {
            memcpy(quoted + q, "'\\''", 4);
            q += 4;
        } else quoted[q++] = playlist[i];
    }
    quoted[q++] = '\'';
    quoted[q] = '\0';

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "nohup mpv --no-video --ytdl=yes --force-window=no --idle=no --really-quiet %s "
             ">/dev/null 2>&1 &",
             quoted);
    return system(cmd) == -1 ? 1 : 0;
}

static void metadata_daemon(void) {
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);

    char path[512];
    path_join(path, sizeof(path), "ftyt-meta-state");

    while (g_running) {
        char title[512] = "FT-YT Music";
        char state[64] = "Stopped";
        char tmp[1024] = {0};

        if (command_read(tmp, sizeof(tmp),
                         "playerctl -p mpv metadata --format '{{title}}' 2>/dev/null") == 0 && tmp[0]) {
            snprintf(title, sizeof(title), "%s", tmp);
        }
        tmp[0] = '\0';
        if (command_read(tmp, sizeof(tmp), "playerctl -p mpv status 2>/dev/null") == 0 && tmp[0]) {
            snprintf(state, sizeof(state), "%s", tmp);
        }

        for (char *p = title; *p; p++) if (*p == '\n' || *p == '\r') *p = ' ';
        atomic_write(path, "%s\n%s\n", title, state);
        sleep_ms(META_INTERVAL_MS);
    }
}

static double goertzel(const short *x, int n, double freq) {
    double w = 2.0 * M_PI * freq / SAMPLE_RATE;
    double coeff = 2.0 * cos(w);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < n; i++) {
        double win = 0.5 - 0.5 * cos(2.0 * M_PI * i / (n - 1));
        s0 = (double)x[i] * win + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    return power > 0.0 ? sqrt(power) / n : 0.0;
}

static void spectrum_daemon(void) {
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);

    char sink[512] = {0};
    if (command_read(sink, sizeof(sink), "pactl get-default-sink 2>/dev/null") != 0 || !sink[0]) {
        snprintf(sink, sizeof(sink), "@DEFAULT_MONITOR@");
    } else {
        strncat(sink, ".monitor", sizeof(sink) - strlen(sink) - 1);
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "parec --raw --format=s16le --rate=%d --channels=1 --device='%s' 2>/dev/null",
             SAMPLE_RATE, sink);

    char path[512];
    path_join(path, sizeof(path), "ftyt-spectrum-state");

    short frame[FFT_FRAME];
    double smoothed[SPECTRUM_WIDTH] = {0};
    const double fmin = 60.0, fmax = 16000.0;

    while (g_running) {
        FILE *p = popen(cmd, "r");
        if (!p) {
            int quiet[SPECTRUM_WIDTH];
            for (int i = 0; i < SPECTRUM_WIDTH; i++) quiet[i] = 1;
            char buf[512] = {0};
            size_t u = 0;
            for (int i = 0; i < SPECTRUM_WIDTH; i++) u += (size_t)snprintf(buf + u, sizeof(buf) - u, "%d%s", quiet[i], i + 1 == SPECTRUM_WIDTH ? "\n" : " ");
            atomic_write(path, "%s", buf);
            sleep_ms(1000);
            continue;
        }

        while (g_running) {
            size_t got = fread(frame, sizeof(short), FFT_FRAME, p);
            if (got != FFT_FRAME) break;

            int levels[SPECTRUM_WIDTH];
            for (int i = 0; i < SPECTRUM_WIDTH; i++) {
                double t = (double)i / (SPECTRUM_WIDTH - 1);
                double freq = fmin * pow(fmax / fmin, t);
                double mag = goertzel(frame, FFT_FRAME, freq);
                double norm = log10(1.0 + mag) / 4.2;
                if (norm < 0.0) norm = 0.0;
                if (norm > 1.0) norm = 1.0;
                smoothed[i] = smoothed[i] * 0.58 + norm * 0.42;
                int lv = (int)lrint(smoothed[i] * 8.0);
                if (lv < 0) lv = 0;
                if (lv > 8) lv = 8;
                levels[i] = lv;
            }

            char buf[512] = {0};
            size_t u = 0;
            for (int i = 0; i < SPECTRUM_WIDTH; i++) {
                int n = snprintf(buf + u, sizeof(buf) - u, "%d%s", levels[i], i + 1 == SPECTRUM_WIDTH ? "\n" : " ");
                if (n < 0 || (size_t)n >= sizeof(buf) - u) break;
                u += (size_t)n;
            }
            atomic_write(path, "%s", buf);
            sleep_ms(1000 / SPECTRUM_FPS);
        }
        pclose(p);
        sleep_ms(250);
    }
}

static int spectrum_test(void) {
    int levels[SPECTRUM_WIDTH];
    for (int i = 0; i < SPECTRUM_WIDTH; i++) {
        int x = i <= SPECTRUM_WIDTH / 2 ? i : SPECTRUM_WIDTH - 1 - i;
        levels[i] = 1 + (x * 7) / (SPECTRUM_WIDTH / 2);
    }
    char markup[8192];
    spectrum_markup_from_levels(levels, markup, sizeof(markup));
    puts(markup);
    return 0;
}

static int panel_debug(void) {
    char meta[512], spec[512];
    path_join(meta, sizeof(meta), "ftyt-meta-state");
    path_join(spec, sizeof(spec), "ftyt-spectrum-state");
    printf("runtime=%s\n", runtime_dir());
    printf("meta=%s\n", meta);
    printf("spectrum=%s\n", spec);
    printf("title_width=%d\n", TITLE_WIDTH);
    printf("spectrum_width=%d\n", SPECTRUM_WIDTH);
    printf("spectrum_fps=%d\n", SPECTRUM_FPS);
    printf("playlist=%s\n", getenv("FTYT_PLAYLIST") && *getenv("FTYT_PLAYLIST") ? getenv("FTYT_PLAYLIST") : DEFAULT_PLAYLIST);
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr,
        "usage: %s [start|toggle|prev|next|stop|metadata-daemon|spectrum-daemon|panel-debug|spectrum-test]\n",
        argv0);
}

int main(int argc, char **argv) {
    if (argc == 1) return emit_panel();
    if (strcmp(argv[1], "start") == 0) return cmd_start();
    if (strcmp(argv[1], "toggle") == 0) {
        if (playerctl("play-pause") != 0) return cmd_start();
        return 0;
    }
    if (strcmp(argv[1], "prev") == 0) return playerctl("previous");
    if (strcmp(argv[1], "next") == 0) return playerctl("next");
    if (strcmp(argv[1], "stop") == 0) return playerctl("stop");
    if (strcmp(argv[1], "metadata-daemon") == 0) { metadata_daemon(); return 0; }
    if (strcmp(argv[1], "spectrum-daemon") == 0) { spectrum_daemon(); return 0; }
    if (strcmp(argv[1], "panel-debug") == 0) return panel_debug();
    if (strcmp(argv[1], "spectrum-test") == 0) return spectrum_test();
    usage(argv[0]);
    return 2;
}
