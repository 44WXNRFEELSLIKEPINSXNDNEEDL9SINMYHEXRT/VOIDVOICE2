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
static int is_lead[10];
static int max_digit[10];
static int forced_col[10];

static int digit[10];

static int ncols;
static int ready[MAX_LEN];
static int col_add[MAX_LEN][MAX_WORDS];
static int col_add_n[MAX_LEN];
static int col_res[MAX_LEN];
static int carry[MAX_LEN + 1];

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
    int ord_of[26];
    for (int c = 0; c < 26; c++)
        ord_of[c] = -1;

    ncols = 0;
    for (int w = 0; w < word_count; w++) {
        wlen[w] = (int)strlen(words[w]);
        if (wlen[w] > ncols)
            ncols = wlen[w];
    }

    int intro_col[10];
    int cnt = 0;
    for (int c = 0; c < ncols; c++) {
        for (int w = 0; w < word_count; w++) {
            if (wlen[w] <= c)
                continue;
            char ch = words[w][wlen[w] - 1 - c];
            if (ord_of[ch - 'A'] < 0) {
                ord_of[ch - 'A'] = cnt;
                intro_col[cnt] = c;
                letters[cnt++] = ch;
            }
        }
    }

    for (int k = 0; k < letter_count; k++) {
        is_lead[k] = 0;
        max_digit[k] = 9;
        forced_col[k] = -1;
    }
    for (int w = 0; w < word_count; w++) {
        for (int i = 0; i < wlen[w]; i++)
            idx[w][i] = ord_of[words[w][i] - 'A'];
        if (wlen[w] > 1)
            is_lead[idx[w][0]] = 1;
    }

    int max_add = 0;
    for (int w = 0; w < word_count - 1; w++) {
        if (wlen[w] > max_add)
            max_add = wlen[w];
    }
    if (wlen[word_count - 1] == max_add + 1)
        max_digit[idx[word_count - 1][0]] = word_count - 2;

    int prev_ready = 0;
    for (int c = 0; c < ncols; c++) {
        int last = 0;
        col_add_n[c] = 0;
        col_res[c] = -1;
        for (int w = 0; w < word_count; w++) {
            if (wlen[w] <= c)
                continue;
            int k = idx[w][wlen[w] - 1 - c];
            if (k > last)
                last = k;
            if (w < word_count - 1)
                col_add[c][col_add_n[c]++] = k;
            else
                col_res[c] = k;
        }
        ready[c] = last > prev_ready ? last : prev_ready;
        prev_ready = ready[c];
        if (col_res[c] >= 0 && intro_col[col_res[c]] == c) {
            int in_add = 0;
            for (int j = 0; j < col_add_n[c]; j++) {
                if (col_add[c][j] == col_res[c])
                    in_add = 1;
            }
            if (!in_add)
                forced_col[col_res[c]] = c;
        }
    }
    carry[0] = 0;
}

static int check_column(int c)
{
    int sum = carry[c];
    for (int j = 0; j < col_add_n[c]; j++)
        sum += digit[col_add[c][j]];

    int expected = col_res[c] >= 0 ? digit[col_res[c]] : 0;
    if (sum % 10 != expected)
        return 0;

    carry[c + 1] = sum / 10;
    if (c == ncols - 1 && carry[c + 1] != 0)
        return 0;
    return 1;
}

static int search(int pos, int used, int col)
{
    if (pos == letter_count)
        return 1;

    int lo = is_lead[pos];
    int hi = max_digit[pos];
    if (forced_col[pos] >= 0) {
        int c = forced_col[pos];
        int sum = carry[c];
        for (int j = 0; j < col_add_n[c]; j++)
            sum += digit[col_add[c][j]];
        lo = hi = sum % 10;
        if (lo < is_lead[pos] || lo > max_digit[pos])
            return 0;
    }

    for (int d = lo; d <= hi; d++) {
        if (used & (1 << d))
            continue;
        digit[pos] = d;

        int c = col;
        int ok = 1;
        while (c < ncols && ready[c] == pos) {
            if (!check_column(c)) {
                ok = 0;
                break;
            }
            c++;
        }
        if (ok && search(pos + 1, used | (1 << d), c))
            return 1;
    }
    return 0;
}

char *solve_rebus(const char *rebus)
{
    if (!parse(rebus) || !collect_letters())
        return NULL;

    precompute();
    if (!search(0, 0, 0))
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
