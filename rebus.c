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

static int idx[MAX_WORDS][MAX_LEN];
static int wlen[MAX_WORDS];
static int lead[MAX_WORDS];
static int lead_count;

static int digit[10];
static long long coef[10];

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

static void precompute(void)
{
    int index_of[26];
    for (int k = 0; k < letter_count; k++)
        index_of[letters[k] - 'A'] = k;

    lead_count = 0;
    for (int k = 0; k < letter_count; k++)
        coef[k] = 0;
    for (int w = 0; w < word_count; w++) {
        wlen[w] = (int)strlen(words[w]);
        for (int i = 0; i < wlen[w]; i++)
            idx[w][i] = index_of[words[w][i] - 'A'];
        long long weight = 1;
        for (int i = wlen[w] - 1; i >= 0; i--) {
            if (w < word_count - 1)
                coef[idx[w][i]] += weight;
            else
                coef[idx[w][i]] -= weight;
            weight *= 10;
        }
        if (wlen[w] > 1)
            lead[lead_count++] = idx[w][0];
    }
}

static int check(void)
{
    for (int k = 0; k < lead_count; k++) {
        if (digit[lead[k]] == 0)
            return 0;
    }

    long long sum = 0;
    for (int k = 0; k < letter_count; k++)
        sum += coef[k] * digit[k];

    return sum == 0;
}

static int search(int pos, int used)
{
    if (pos == letter_count)
        return check();

    for (int d = 0; d < 10; d++) {
        if (used & (1 << d))
            continue;
        digit[pos] = d;
        if (search(pos + 1, used | (1 << d)))
            return 1;
    }
    return 0;
}

char *solve_rebus(const char *rebus)
{
    if (!parse(rebus) || !collect_letters())
        return NULL;

    precompute();
    if (!search(0, 0))
        return NULL;

    char *out = malloc(word_count * (MAX_LEN + 4) + 1);
    out[0] = '\0';
    for (int w = 0; w < word_count; w++) {
        char buf[MAX_LEN + 4];
        for (int i = 0; i < wlen[w]; i++)
            buf[i] = (char)('0' + digit[idx[w][i]]);
        buf[wlen[w]] = '\0';

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
