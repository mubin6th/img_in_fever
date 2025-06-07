#include <stdio.h>
#include <stdint.h>
#include <string.h>

const uint8_t EXIT_ERR = 255;

uint32_t getColorHexFromChar(char c);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "expected: ./hex_to_arr <filepath>\n");
        return EXIT_ERR;
    }

    FILE *file_ptr;

    if ((file_ptr = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr,
                "error: file not found or permission not met!\n");
        return EXIT_ERR;
    }

    size_t crnt_line = 0;
    char line_buf[1 << 7];
    const char *line_not_color_msg = "warning: skipping line %d as "
                                     "it is not a color.\n";

    uint32_t crnt_color;
    uint32_t colors[256];
    size_t colors_len = 0;

    while (fgets(line_buf, sizeof(line_buf), file_ptr) != NULL) {
        crnt_line++;

        size_t line_len = strlen(line_buf);
        if (line_len != 8 || line_buf[0] != '#') {
            fprintf(stderr, line_not_color_msg, crnt_line);
            continue;
        }

        char flag = 0;
        crnt_color = 0;
        for (size_t i = 6; i >= 1; i--) {
            if ((line_buf[i] < '0' || line_buf[i] > '9') &&
                (line_buf[i] < 'a' || line_buf[i] > 'f') &&
                (line_buf[i] < 'A' || line_buf[i] > 'F'))
            {
                fprintf(stderr, line_not_color_msg, crnt_line);
                flag = 1;
                break;
            }

            crnt_color |= getColorHexFromChar(line_buf[i]) <<
                          ((8 - i) * 4);
        }

        if (!flag) {
            colors[colors_len] = crnt_color;
            colors_len++;
        }
    }

    fclose(file_ptr);

    fprintf(stdout,
            "#include \"%s.h\"\n\nsize_t colors_len = %lu;\n"
            "unsigned char colors[%lu] = {\n   ",
            argv[1], colors_len * 3, colors_len * 3);

    for (size_t i = 0; i < colors_len; i++) {
        if (i % 3 == 0 && i != 0) {
            fprintf(stdout, "\n   ");
        }

        crnt_color = colors[i];

        fprintf(stdout, " 0x%.2x, 0x%.2x, 0x%.2x,",
                crnt_color >> 24 & 0xff,
                crnt_color >> 16 & 0xff,
                crnt_color >>  8 & 0xff);
    }

    fprintf(stdout, "\n};\n");

    return 0;
}

uint32_t getColorHexFromChar(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c >= 'a' || c <= 'f') {
        return c - 'a' + 10;
    }
    else if (c >= 'A' || c <= 'F') {
        return c - 'A' + 10;
    }

    return UINT32_MAX;
}
