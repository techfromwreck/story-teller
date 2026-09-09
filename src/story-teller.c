/*
 * story-teller.c
 *
 * Reads a text file one line at a time and "types" each line to stdout,
 * one character at a time, clearing the terminal before each new line.
 *
 * Two independent timings are involved:
 *   - CHAR delay:    time between printing each character (typing speed).
 *   - REFRESH delay: time the finished line stays on screen before the
 *                     screen clears for the next line. This is calculated
 *                     as (characters in that line) * ms-per-char-refresh,
 *                     so longer lines linger longer before the wipe.
 *
 * Each character is drawn in two stages, like a moving highlight cursor:
 *   1. HIGHLIGHT - green background, terminal's own default text color
 *                  (via reverse video, so it never hard-codes a color).
 *   2. SETTLED   - transparent background, green text.
 * As soon as a character's char-delay elapses, it downgrades from
 * HIGHLIGHT to SETTLED and the next character begins its HIGHLIGHT stage,
 * so only the most-recently-typed character is ever highlighted.
 *
 * To render this without any manual cursor-column tracking (which breaks
 * once a line wraps past the terminal's width), every animation frame
 * clears the screen and redraws the settled prefix plus the current
 * highlighted character from scratch. The terminal's own line wrapping
 * then just works, at the cost of a full redraw per character.
 *
 * Before a line is animated, it is word-wrapped ourselves: the current
 * terminal width is queried and a space is turned into a real '\n'
 * wherever the next word would overflow the width, so the terminal never
 * has to auto-wrap mid-word. Width is re-checked once per line (so a
 * resize is only picked up by the next line, never mid-animation), and
 * assumes the terminal is at least 20 columns wide and no single word in
 * the file exceeds 20 characters - the one case that has no fix, since
 * there'd be nowhere left to wrap such a word to.
 *
 * Usage:
 *   ./story-teller [file.txt] [char_delay_ms] [refresh_ms_per_char]
 *
 *   file.txt             - path to the text file (default: story.txt)
 *   char_delay_ms        - ms between each printed character (default: 5)
 *   refresh_ms_per_char   - ms of post-line pause per character in that
 *                           line, before clearing for the next (default: 10)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEFAULT_CHAR_DELAY_MS 5
#define DEFAULT_REFRESH_MS_PER_CHAR 10
#define FALLBACK_TERMINAL_WIDTH 80  /* used only if the width can't be read */

#define ESC_HIGHLIGHT "\033[32;7m"  /* green bg (reverse video), default fg */
#define ESC_SETTLED   "\033[32m"    /* transparent bg, green fg */
#define ESC_RESET     "\033[0m"

static void clear_screen(void) {
    /* ANSI: move cursor to home position, then clear screen */
    fputs("\033[H\033[J", stdout);
}

static void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* Current terminal width in columns, or a fallback if it can't be read
 * (e.g. stdout isn't a tty). */
static int get_terminal_width(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
    return FALLBACK_TERMINAL_WIDTH;
}

/* Word-wrap a line in place for the given width: whenever the next word
 * would overflow the current row, the single space before it is turned
 * into '\n'. Same-length transformation, no reallocation needed.
 * Assumes single-space-separated words, none longer than `width`. */
static void word_wrap_line(char *line, ssize_t line_len, int width) {
    int col = 0;
    ssize_t word_start = 0;

    for (ssize_t i = 0; i <= line_len; i++) {
        int at_boundary = (i == line_len) || (line[i] == ' ') || (line[i] == '\n');
        if (!at_boundary) {
            continue;
        }

        ssize_t word_len = i - word_start;
        if (word_len > 0) {
            int needs_space = (col > 0);
            int prospective = col + (needs_space ? 1 : 0) + (int) word_len;

            if (prospective > width) {
                if (needs_space) {
                    line[word_start - 1] = '\n';  /* wrap: was a space */
                }
                col = (int) word_len;
            } else {
                col = prospective;
            }
        }
        word_start = i + 1;
    }
}

/* Print one character in either the highlighted or settled style.
 * Newlines are passed through with no styling. */
static void draw_char(char c, int highlighted) {
    if (c == '\n') {
        putchar('\n');
        return;
    }
    printf(highlighted ? ESC_HIGHLIGHT "%c" ESC_RESET
                        : ESC_SETTLED "%c" ESC_RESET, c);
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "story.txt";
    long char_delay_ms = (argc > 2) ? atol(argv[2]) : DEFAULT_CHAR_DELAY_MS;
    long refresh_ms_per_char = (argc > 3) ? atol(argv[3]) : DEFAULT_REFRESH_MS_PER_CHAR;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t buf_len = 0;
    ssize_t line_len;

    while ((line_len = getline(&line, &buf_len, fp)) != -1) {
        int width = get_terminal_width();
        word_wrap_line(line, line_len, width);

        for (ssize_t i = 0; i < line_len; i++) {
            clear_screen();
            for (ssize_t j = 0; j < i; j++) {
                draw_char(line[j], 0 /* settled */);
            }
            draw_char(line[i], 1 /* highlighted */);
            fflush(stdout);
            sleep_ms(char_delay_ms);
        }

        /* final frame: whole line settled, including the last character */
        clear_screen();
        for (ssize_t j = 0; j < line_len; j++) {
            draw_char(line[j], 0 /* settled */);
        }
        fflush(stdout);

        /* pause before the next clear, scaled by this line's length */
        sleep_ms(line_len * refresh_ms_per_char);
    }

    free(line);
    fclose(fp);
    return EXIT_SUCCESS;
}
