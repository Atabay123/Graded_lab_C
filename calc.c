// <<Atabay Atabaev 251ADB244>>
// <<https://github.com/your-username/your-repo>>
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <<only what you really used, plus your conversation share link>>

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIRST_NAME "<<atabay>>"
#define LAST_NAME  "<<atabaev>>"
#define STUDENT_ID "<<251adb244>>"

enum { T_INT, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_POW, T_LP, T_RP, T_END, T_BAD };


static char *buf;
static long len;
static long p;        
static long line_end; 
static int tok;       
static long tok_pos;   
static long long tok_int;
static int failed;     
static long fail_pos;

typedef struct {
    long long i;
    long start; 
} Value;


static void fail(long pos) {
    if (!failed) {
        failed = 1;
        fail_pos = pos;
    }
}


static void next_token(void) {
    while (p < line_end && (buf[p] == ' ' || buf[p] == '\t')) p++;

    
    if (p >= line_end) {
        tok = T_END;
        tok_pos = line_end + 1;
        return;
    }

    tok_pos = p + 1;
    char c = buf[p];
    if (c == '+') { tok = T_PLUS; p++; }
    else if (c == '-') { tok = T_MINUS; p++; }
    else if (c == '*') {
        
        if (p + 1 < line_end && buf[p + 1] == '*') { tok = T_POW; p += 2; }
        else { tok = T_STAR; p++; }
    }
    else if (c == '/') { tok = T_SLASH; p++; }
        else if (c == '(') { tok = T_LP; p++; }
    else if (c == ')') { tok = T_RP; p++; }
    else if (isdigit((unsigned char)c)) {
        tok_int = 0;
        while (p < line_end && isdigit((unsigned char)buf[p])) {
            tok_int = tok_int * 10 + (buf[p] - '0');
            p++;
        }
        tok = T_INT;
    }
    else { tok = T_BAD; p++; }  
}


static Value parse_expr(void);


static Value parse_primary(void) {
    Value v = {0};
    long start = tok_pos;
    v.start = start;
    if (tok == T_INT) {
        v.i = tok_int;
        next_token();
    } else if (tok == T_LP) {
        next_token();
        v = parse_expr();
        if (failed) return v;
        
        v.start = start;
        if (tok == T_RP) {
            next_token();
        } else {
            
            fail(tok_pos);
        }
    } else {

        fail(tok_pos);
    }
    return v;
}


static long long int_pow(long long base, long long exp) {
    long long result = 1;
    while (exp > 0) {
        if (exp & 1) result = result * base;
        exp = exp >> 1;
        if (exp > 0) base = base * base;
    }
    return result;
}


static Value parse_power(void) {
    Value left = parse_primary();
    if (failed) return left;
    if (tok == T_POW) {
        next_token();
        Value right = parse_power();
        if (failed) return left;
        if (right.i < 0) {
           
            fail(right.start);
            return left;
        }
        left.i = int_pow(left.i, right.i);
    }
    return left;
}


static Value parse_term(void) {
    Value left = parse_power();
    if (failed) return left;
    while (tok == T_STAR || tok == T_SLASH) {
        int op = tok;
        next_token();
        Value right = parse_power();
        if (failed) return left;
        if (op == T_STAR) {
            left.i = left.i * right.i;
        } else {
            if (right.i == 0) {
                
                fail(right.start);
                return left;
            }
            left.i = left.i / right.i;  
        }
    }
    return left;
}

static Value parse_expr(void) {
    Value left = parse_term();
    if (failed) return left;
    while (tok == T_PLUS || tok == T_MINUS) {
        int op = tok;
        next_token();
        Value right = parse_term();
        if (failed) return left;
        if (op == T_PLUS) left.i = left.i + right.i;
        else left.i = left.i - right.i;
    }
    return left;
}


static char *out_text;
static long out_len;
static long out_cap;

static void add_text(const char *s) {
    long n = strlen(s);
    if (out_len + n + 1 > out_cap) {
        while (out_len + n + 1 > out_cap) out_cap = out_cap * 2;
        char *bigger = realloc(out_text, out_cap);
        if (bigger == NULL) {
            fprintf(stderr, "Error: out of memory\n");
            exit(1);
        }
        out_text = bigger;
    }
    memcpy(out_text + out_len, s, n + 1);
    out_len = out_len + n;
}


static void base_name(const char *path, char *out, int strip_txt) {
    long end = strlen(path);
    while (end > 1 && path[end - 1] == '/') end--;
    long start = end;
    while (start > 0 && path[start - 1] != '/') start--;
    long n = end - start;
    if (n > 255) n = 255;
    memcpy(out, path + start, n);
    out[n] = '\0';
    if (strip_txt && n > 4 && strcmp(out + n - 4, ".txt") == 0) out[n - 4] = '\0';
}

static int process_file(const char *in_path, const char *out_dir) {
    FILE *f = fopen(in_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: cannot open '%s': %s\n", in_path, strerror(errno));
        return 1;
    }
   
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);
    buf = malloc(len + 1);
    if (buf == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        return 1;
    }
    len = fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    out_cap = 1024;
    out_len = 0;
    out_text = malloc(out_cap);
    if (out_text == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        free(buf);
        return 1;
    }
    out_text[0] = '\0';

    failed = 0;
    long cursor = 0;
    while (cursor < len && !failed) {
        long start = cursor;
        while (cursor < len && buf[cursor] != '\n') cursor++;
        line_end = cursor;
        if (cursor < len) cursor++;  
         long k = start;
        while (k < line_end && (buf[k] == ' ' || buf[k] == '\t')) k++;
        if (k == line_end || buf[k] == '#') continue;  
        p = start;
        next_token();
        Value v = parse_expr();

        if (!failed && tok != T_END) fail(tok_pos);
        if (failed) break;

        char text[64];
        snprintf(text, sizeof text, "%lld\n", v.i);
        add_text(text);
    }

    char base[256];
    base_name(in_path, base, 1);
    char out_path[4096];
    snprintf(out_path, sizeof out_path, "%s/%s_%s_%s_%s.txt",
             out_dir, base, FIRST_NAME, LAST_NAME, STUDENT_ID);

    int status = 0;
    FILE *out = fopen(out_path, "w");
    if (out == NULL) {
        fprintf(stderr, "Error: cannot write '%s': %s\n", out_path, strerror(errno));
        status = 1;
    } else {
       
        if (failed) fprintf(out, "ERROR:%ld\n", fail_pos);
        else fwrite(out_text, 1, out_len, out);
        if (ferror(out) || fclose(out) != 0) {
            fprintf(stderr, "Error: write failed for '%s'\n", out_path);
            status = 1;
        }
    }
    free(buf);
    free(out_text);
    return status;
}

// 
int main(int argc, char **argv) {
    const char *out_dir = NULL;
    const char *in_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out_dir = argv[++i];
        else in_path = argv[i];
    }
    if (out_dir == NULL || in_path == NULL) {
        fprintf(stderr, "Usage: %s -o OUTDIR input.txt\n", argv[0]);
        return 1;
    }
    if (mkdir(out_dir, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: cannot create '%s': %s\n", out_dir, strerror(errno));
        return 1;
    }
    return process_file(in_path, out_dir);
}