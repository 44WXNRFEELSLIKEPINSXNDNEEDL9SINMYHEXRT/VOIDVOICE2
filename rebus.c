#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_WORDS 8
#define MAX_LEN 32

static char words[MAX_WORDS][MAX_LEN];
static int word_count;

static char letters[10];
static int letter_count;

static int digit_of[26];
static int used[10];

static int parse(const char *s)
{
    word_count = 0;
    int len = 0;

    for (; *s; s++) {
        if (isupper((unsigned char)*s)) {
            if (word_count >= MAX_WORDS || len >= MAX_LEN - 1)
                return 0;
            words[word_count][len++] = *s;
            words[word_count][len] = '\0';
        } else if (*s == '+' || *s == '=') {
            if (len == 0)
                return 0;
            word_count++;
            len = 0;
        } else if (!isspace((unsigned char)*s)) {
            return 0;
        }
    }
    if (len == 0)
        return 0;
    word_count++;
    return word_count >= 3;
}

static int collect_letters(void)
{
    int seen[26] = {0};
    letter_count = 0;

    for (int w = 0; w < word_count; w++) {
        for (int i = 0; words[w][i]; i++) {
            int c = words[w][i] - 'A';
            if (!seen[c]) {
                seen[c] = 1;
                if (letter_count >= 10)
                    return 0;
                letters[letter_count++] = words[w][i];
            }
        }
    }
    return 1;
}

static long long word_value(const char *w)
{
    long long v = 0;
    for (int i = 0; w[i]; i++)
        v = v * 10 + digit_of[w[i] - 'A'];
    return v;
}

static int check(void)
{
    for (int w = 0; w < word_count; w++) {
        if (strlen(words[w]) > 1 && digit_of[words[w][0] - 'A'] == 0)
            return 0;
    }

    long long sum = 0;
    for (int w = 0; w < word_count - 1; w++)
        sum += word_value(words[w]);

    return sum == word_value(words[word_count - 1]);
}

static int search(int pos)
{
    if (pos == letter_count)
        return check();

    for (int d = 0; d < 10; d++) {
        if (used[d])
            continue;
        used[d] = 1;
        digit_of[letters[pos] - 'A'] = d;
        if (search(pos + 1))
            return 1;
        used[d] = 0;
    }
    return 0;
}

char *solve_rebus(const char *rebus)
{
    if (!parse(rebus) || !collect_letters())
        return NULL;

    memset(used, 0, sizeof(used));
    if (!search(0))
        return NULL;

    char *out = malloc(word_count * (MAX_LEN + 4) + 1);
    out[0] = '\0';
    for (int w = 0; w < word_count; w++) {
        char buf[MAX_LEN + 4];
        for (int i = 0; words[w][i]; i++)
            buf[i] = (char)('0' + digit_of[words[w][i] - 'A']);
        buf[strlen(words[w])] = '\0';

        strcat(out, buf);
        if (w < word_count - 2)
            strcat(out, " + ");
        else if (w == word_count - 2)
            strcat(out, " = ");
    }
    return out;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, "Использование: %s [входной_файл]\n", argv[0]);
        return 1;
    }

    FILE *in = stdin;
    if (argc == 2) {
        in = fopen(argv[1], "r");
        if (!in) {
            perror(argv[1]);
            return 1;
        }
    }

    char line[512];
    int line_no = 0;
    double total = 0.0;

    while (fgets(line, sizeof(line), in)) {
        line_no++;

        if (!strchr(line, '\n') && !feof(in)) {
            fprintf(stderr, "Строка %d: слишком длинная\n", line_no);
            int c;
            while ((c = fgetc(in)) != '\n' && c != EOF)
                ;
            continue;
        }
        line[strcspn(line, "\r\n")] = '\0';

        const char *p = line;
        while (isspace((unsigned char)*p))
            p++;
        if (!*p)
            continue;

        double t0 = now_sec();
        char *res = solve_rebus(line);
        double dt = now_sec() - t0;
        total += dt;
        fprintf(stderr, "Строка %d: %.6f с\n", line_no, dt);

        if (res) {
            printf("%s\n", res);
            free(res);
        } else {
            printf("Строка %d: решение не найдено\n", line_no);
        }
    }

    if (ferror(in)) {
        perror("Ошибка чтения");
        if (in != stdin)
            fclose(in);
        return 1;
    }

    fprintf(stderr, "Всего: %.6f с\n", total);

    if (in != stdin)
        fclose(in);
    return 0;
}
