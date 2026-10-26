// <<Atabay Atabaev 251ADB244
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
#include <dirent.h>
#include <math.h>

#define FIRST_NAME "atabay"
#define LAST_NAME  "atabaev"
#define STUDENT_ID "251adb244"

enum { T_INT, T_FLOAT, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_POW, T_LP, T_RP, T_END, T_BAD };

static char *buf;
static long len;
static long p;        
static long line_end; 
static int tok;       
static long tok_pos;   
static long long tok_int;
static double tok_float;
static int failed;     
static long fail_pos;

typedef struct {
    int is_float;
    long long i;
    double f;
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
        long start = p;
        while (p < line_end && isdigit((unsigned char)buf[p])) p++;

        if (p + 1 < line_end && buf[p] == '.' && isdigit((unsigned char)buf[p + 1])) {
            p++;
            while (p < line_end && isdigit((unsigned char)buf[p])) p++;
            
            char saved = buf[p];
            buf[p] = '\0';
            tok_float = strtod(buf + start, NULL);
            buf[p] = saved;
            tok = T_FLOAT;
        } else {
            tok_int = 0;
            for (long k = start; k < p; k++) tok_int = tok_int * 10 + (buf[k] - '0');
            tok = T_INT;
        }
    }
    else { tok = T_BAD; p++; }  
}


static Value parse_expr(void);





static long long int_pow(long long base, long long exp) {
    long long result = 1;
    while (exp > 0) {
        if (exp & 1) result = result * base;
        exp = exp >> 1;
        if (exp > 0) base = base * base;
    }
    return result;
}



static double to_double(Value v) {
    if (v.is_float) return v.f;
    return (double)v.i;
}




static Value parse_primary(void) {
    Value v = {0};
    long start = tok_pos;
    v.start = start;
    if (tok == T_INT) {
        v.i = tok_int;
        next_token();
    } else if (tok == T_FLOAT) {
        v.is_float = 1;
        v.f = tok_float;
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


static Value parse_power(void) {
    Value left = parse_primary();
    if (failed) return left;
    if (tok == T_POW) {
        long op_pos = tok_pos;
        next_token();
        Value right = parse_power();
        if (failed) return left;
        if (!left.is_float && !right.is_float) {
   
            if (right.i < 0) {
                fail(right.start); 
                return left;
            }
            left.i = int_pow(left.i, right.i);
        } else {
           
            double r = pow(to_double(left), to_double(right));
            if (isnan(r) || isinf(r)) {
                fail(op_pos);  
                return left;
            }
            left.is_float = 1;
            left.f = r;
        }
    }
    return left;
}


static Value parse_term(void) {
    Value left = parse_power();
    if (failed) return left;
    while (tok == T_STAR || tok == T_SLASH) {
        int op = tok;
        long op_pos = tok_pos;
        next_token();
        Value right = parse_power();
        if (failed) return left;
        if (op == T_SLASH && ((!right.is_float && right.i == 0) ||
                              (right.is_float && right.f == 0.0))) {
            fail(right.start);  
            return left;
        }
        if (!left.is_float && !right.is_float) {
            if (op == T_STAR) left.i = left.i * right.i;
            else left.i = left.i / right.i;
        } else {
            double a = to_double(left);
            double b = to_double(right);
            double r;
            if (op == T_STAR) r = a * b;
            else r = a / b;
            if (isnan(r) || isinf(r)) {
                fail(op_pos);
                return left;
            }
            left.is_float = 1;
            left.f = r;
        }
    }
    return left;
}


static Value parse_expr(void) {
    Value left = parse_term();
    if (failed) return left;
    while (tok == T_PLUS || tok == T_MINUS) {
        int op = tok;
        long op_pos = tok_pos;
        next_token();
        Value right = parse_term();
        if (failed) return left;
        if (!left.is_float && !right.is_float) {
            if (op == T_PLUS) left.i = left.i + right.i;
            else left.i = left.i - right.i;
        } else {
            double a = to_double(left);
            double b = to_double(right);
            double r;
            if (op == T_PLUS) r = a + b;
            else r = a - b;
            if (isnan(r) || isinf(r)) {
                fail(op_pos);
                return left;
            }
            left.is_float = 1;
            left.f = r;
        }
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
        if (v.is_float) snprintf(text, sizeof text, "%.15g\n", v.f);
        else snprintf(text, sizeof text, "%lld\n", v.i);
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


int main(int argc, char **argv) {
    const char *dir_arg = NULL;
    const char *out_arg = NULL;
    const char *in_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value after %s\n", argv[i]);
                return 1;
            }
            dir_arg = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: missing value after %s\n", argv[i]);
                return 1;
            }
            out_arg = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            if (in_arg != NULL) {
                fprintf(stderr, "Error: more than one input file\n");
                return 1;
            }
            in_arg = argv[i];
        }
    }
    if ((dir_arg == NULL) == (in_arg == NULL)) {  // neither or both
        fprintf(stderr, "Usage: %s [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n", argv[0]);
        return 1;
    }

   
    DIR *dir = NULL;
    if (in_arg != NULL) {
        struct stat st;
        if (stat(in_arg, &st) != 0 || !S_ISREG(st.st_mode)) {
            fprintf(stderr, "Error: cannot read input file '%s'\n", in_arg);
            return 1;
        }
    } else {
        dir = opendir(dir_arg);
        if (dir == NULL) {
            fprintf(stderr, "Error: cannot open folder '%s': %s\n", dir_arg, strerror(errno));
            return 1;
        }
    }

    
    char out_dir[4096];
    if (out_arg != NULL) {
        snprintf(out_dir, sizeof out_dir, "%s", out_arg);
    } else {
        char base[256];
        if (dir_arg != NULL) base_name(dir_arg, base, 0);
        else base_name(in_arg, base, 1);
        snprintf(out_dir, sizeof out_dir, "%s_%s_%s", base, FIRST_NAME, STUDENT_ID);
    }
    if (mkdir(out_dir, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: cannot create '%s': %s\n", out_dir, strerror(errno));
        if (dir != NULL) closedir(dir);
        return 1;
    }

    if (in_arg != NULL) return process_file(in_arg, out_dir);

    
    int status = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t n = strlen(entry->d_name);
        if (n <= 4 || strcmp(entry->d_name + n - 4, ".txt") != 0) continue;
        char path[4096];
        snprintf(path, sizeof path, "%s/%s", dir_arg, entry->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (process_file(path, out_dir) != 0) status = 1;
    }
    closedir(dir);
    return status;
}