/*
 * mx5000-lib-test - hardware test tool for libmx5000 functions which are
 * not exposed by mx5000-tool.
 *
 * This is intentionally kept separate from the normal command line tool:
 * some of the menu functions below are old reverse-engineering code with
 * incompletely understood payload fields.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libmx5000/mx5000.h"
#include "libmx5000/mx5000image.h"
#include "libmx5000/mx5000screencontent.h"

/*
 * These functions exist in mx5000screencontent.c, but are deliberately not
 * declared in the installed public header.  They are the historical menu
 * reverse-engineering code this test program is meant to exercise.
 */
int mx5000_sc_add_menuline(struct MX5000ScreenContent *sc,
                           char text[16], char quadruplet[4]);
int mx5000_sc_nextmenu(struct MX5000ScreenContent *sc);
int mx5000_sc_send_menus(struct MX5000ScreenContent *sc, int fd,
                         int pageid, int first);

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s [-d /dev/hidrawX] progress <0..100>\n"
        "  %s [-d /dev/hidrawX] menu <pageid> <first> <text> <8-hex-digits>\n"
        "  %s [-d /dev/hidrawX] two-menus <pageid> <first> \\\n"
        "       <text1> <8-hex-digits> <text2> <8-hex-digits>\n"
        "  %s pbm-dump <file.pbm>\n"
        "\n"
        "Commands:\n"
        "  progress   Test mx5000_sc_add_progress_bar_percentage().\n"
        "  menu       Test mx5000_sc_add_menuline() + mx5000_sc_send_menus().\n"
        "  two-menus  Also exercise mx5000_sc_nextmenu() between two entries.\n"
        "  pbm-dump   Test pbm_to_mx5000image() + print_mx5000image(); no keyboard needed.\n"
        "\n"
        "WARNING: menu/two-menus exercise experimental, incompletely understood\n"
        "firmware-menu code. Use controlled values and be prepared to reconnect\n"
        "the keyboard or run mx5000-tool --reset afterwards.\n",
        prog, prog, prog, prog);
}

static int parse_int(const char *s, int min, int max, const char *what)
{
    char *end = NULL;
    long v;

    errno = 0;
    v = strtol(s, &end, 0);
    if (errno || !s[0] || !end || *end || v < min || v > max) {
        fprintf(stderr, "Invalid %s: %s (expected %d..%d)\n",
                what, s, min, max);
        return -1;
    }

    return (int)v;
}

static int parse_quad(const char *s, char quad[4])
{
    unsigned int v[4];
    int i;

    if (strlen(s) != 8) {
        fprintf(stderr, "Quadruplet must be exactly 8 hexadecimal digits\n");
        return -1;
    }

    for (i = 0; i < 4; i++) {
        char byte[3];
        char *end = NULL;
        long n;

        byte[0] = s[i * 2];
        byte[1] = s[i * 2 + 1];
        byte[2] = '\0';

        errno = 0;
        n = strtol(byte, &end, 16);
        if (errno || !end || *end || n < 0 || n > 255) {
            fprintf(stderr, "Invalid hexadecimal quadruplet: %s\n", s);
            return -1;
        }
        v[i] = (unsigned int)n;
    }

    for (i = 0; i < 4; i++)
        quad[i] = (char)v[i];

    return 0;
}

static void make_menu_text(const char *src, char dst[16])
{
    size_t len = strlen(src);

    if (len > 16)
        len = 16;

    memset(dst, ' ', 16);
    memcpy(dst, src, len);
}

static int open_keyboard(const char *device)
{
    int fd = device ? mx5000_open_path(device) : mx5000_open();

    if (fd < 0)
        fprintf(stderr, "Could not open MX5000/MX5500 device: %s\n",
                strerror(errno));

    return fd;
}

static int test_progress(const char *device, const char *arg)
{
    struct MX5000ScreenContent *sc;
    int fd, perc, ret;

    perc = parse_int(arg, 0, 100, "percentage");
    if (perc < 0)
        return 2;

    fd = open_keyboard(device);
    if (fd < 0)
        return 1;

    sc = mx5000_sc_new_static();
    if (!sc) {
        fprintf(stderr, "Could not allocate screen content\n");
        close(fd);
        return 1;
    }

    ret = mx5000_sc_add_progress_bar_percentage(sc, (unsigned char)perc);
    if (ret >= 0)
        ret = mx5000_sc_send(sc, fd);

    mx5000_sc_free(sc);
    close(fd);
    return ret < 0 ? 1 : 0;
}

static int add_menu_line(struct MX5000ScreenContent *sc,
                         const char *text_arg, const char *quad_arg)
{
    char text[16];
    char quad[4];

    make_menu_text(text_arg, text);
    if (parse_quad(quad_arg, quad) < 0)
        return -1;

    fprintf(stderr,
            "menu line: text=\"%.*s\" quad=%02x %02x %02x %02x\n",
            16, text,
            (unsigned char)quad[0], (unsigned char)quad[1],
            (unsigned char)quad[2], (unsigned char)quad[3]);

    return mx5000_sc_add_menuline(sc, text, quad);
}

static int test_menu(const char *device, int argc, char **argv, int two)
{
    struct MX5000ScreenContent *sc;
    int fd, pageid, first, ret;

    if (argc != (two ? 6 : 4))
        return 2;

    pageid = parse_int(argv[0], 0, 255, "pageid");
    first = parse_int(argv[1], 0, 255, "first");
    if (pageid < 0 || first < 0)
        return 2;

    sc = mx5000_sc_new_static();
    if (!sc) {
        fprintf(stderr, "Could not allocate screen content\n");
        return 1;
    }

    ret = add_menu_line(sc, argv[2], argv[3]);
    if (ret < 0)
        goto out_sc;

    if (two) {
        ret = mx5000_sc_nextmenu(sc);
        if (ret < 0)
            goto out_sc;

        ret = add_menu_line(sc, argv[4], argv[5]);
        if (ret < 0)
            goto out_sc;
    }

    fprintf(stderr, "sending experimental menu data: pageid=%d first=%d\n",
            pageid, first);

    fd = open_keyboard(device);
    if (fd < 0) {
        ret = -1;
        goto out_sc;
    }

    ret = mx5000_sc_send_menus(sc, fd, pageid, first);
    close(fd);

out_sc:
    mx5000_sc_free(sc);
    return ret < 0 ? 1 : 0;
}

static int test_pbm_dump(const char *filename)
{
    unsigned char *image;
    int width = 0, height = 0;
    int len;

    image = pbm_to_mx5000image((char *)filename, &width, &height);
    if (!image) {
        fprintf(stderr, "Could not convert PBM file: %s\n", filename);
        return 1;
    }

    len = (width * height) / 8;
    printf("PBM %s -> %dx%d, %d bytes\n", filename, width, height, len);
    print_mx5000image(image, len, width, height);
    free(image);
    return 0;
}

int main(int argc, char **argv)
{
    const char *device = NULL;
    const char *cmd;
    int argi = 1;
    int ret;

    if (argc > 2 && !strcmp(argv[argi], "-d")) {
        device = argv[argi + 1];
        argi += 2;
    }

    if (argi >= argc) {
        usage(argv[0]);
        return 2;
    }

    cmd = argv[argi++];

    if (!strcmp(cmd, "progress")) {
        if (argc - argi != 1) {
            usage(argv[0]);
            return 2;
        }
        return test_progress(device, argv[argi]);
    }

    if (!strcmp(cmd, "menu")) {
        ret = test_menu(device, argc - argi, argv + argi, 0);
        if (ret == 2)
            usage(argv[0]);
        return ret;
    }

    if (!strcmp(cmd, "two-menus")) {
        ret = test_menu(device, argc - argi, argv + argi, 1);
        if (ret == 2)
            usage(argv[0]);
        return ret;
    }

    if (!strcmp(cmd, "pbm-dump")) {
        if (argc - argi != 1 || device) {
            usage(argv[0]);
            return 2;
        }
        return test_pbm_dump(argv[argi]);
    }

    usage(argv[0]);
    return 2;
}
