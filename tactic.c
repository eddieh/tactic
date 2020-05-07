/*
 * Tactic is a small Scheme like langauge in one file.
 *
 * Instead of parenthesis Tactic use brackets. Characters and strings
 * are delimited by the usual single and double quotes. Quoted
 * (literal) list are lists that begin with a leading l, quasiquoted
 * list begin with a leading q, and unquoted expressions begin with a
 * leading $. Lambdas begin with a leading ^. The idenity function has
 * the following form:
 *
 *     ^[x x]
 *
 * A function is a list that's car is the formal parameter list and
 * the cdr is the body, when the formal parameters is a list the
 * function takes multiple arguments:
 *
 *     ^[[a b] [fn a b]]
 *
 * Here are some valid Tactic definitions:
 *
 *     [def a 1]
 *     [def b 2]
 *     [def c ^[[a b] [cat a b]]]
 *     [def d l[1 2 3]]
 *     [def e q[$a $b 3]]
 *     [def f ^[x [pow x 5]]
 *     [def g 'c']
 *     [def h "hello"]
 *     [def i 3.14]
 */

#include <histedit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


#define PROMPT "tt> "
#define PROMPTLEN 4
#define BUFLEN 1024

#define EOF (-1)

enum Type {
    StringType,
    IntegerType,
    FloatType,
    PairType,
};


struct Pair {
    unsigned flags;
    union {
        struct {
            char *sval;
            unsigned len;
        };
        long ival;
        double fval;
        struct {
            struct Pair *car;
            struct Pair *cdr;
        };
    };
};

enum TokenType {
    TOK_STRM_START,
    TOK_STRM_END,
    TOK_ERR,
    TOK_IDENT,
    TOK_NUM,
    TOK_CHAR = '\'',
    TOK_STR = '"',
    TOK_LST = '[',
    TOK_LLST = 'l',
    TOK_QLST = 'q',
    TOK_FN = '^',
    TOK_END = ']',
};

struct Position {
    size_t line;
    size_t col;
};

struct String {
    char *sval;
    size_t len;
};

struct Token {
    enum TokenType type;
    struct Position pos;
    struct String *value;
    struct Token *next;
};

struct Token EOS;
#define END_OF_STREAM(x) ((x) == &EOS)

struct Stream {
    int fd, newline, whitespace;
    size_t size, offset;
    struct Position pos;
    struct Token **tokenlist;
    struct Token *start;
    struct Token *token;
    struct Token *end;
    char *buffer;
};

struct Interpreter {
    struct Stream *stream;
};

struct String *String_alloc()
{
    struct String *str = malloc(sizeof(struct String));
    return str;
}

struct String *String_from_cstr(char *cstr)
{
    return NULL;
}

struct String *String_from_buffer(char *buf, size_t len)
{
    struct String *str = malloc(sizeof(struct String));
    str->sval = malloc(len);
    str->len = len;
    strcpy(str->sval, buf);
    str->sval[len] = 0;
    return str;
}

void String_free(struct String *str)
{
    free(str->sval);
    free(str);
}

struct Token *Token_alloc()
{
    struct Token *tok = malloc(sizeof(struct Token));
    return tok;
}

void Token_free(struct Token *tok)
{
    String_free(tok->value);
    free(tok);
}

void Token_free_all(struct Token *start_tok)
{
    // takes a start token
}

void Token_dump_tokens(struct Token *tok)
{
    // takes a start token
    while (!END_OF_STREAM(tok)) {
        switch (tok->type) {
        case TOK_STRM_START:
            printf("<start> ");
            break;
        case TOK_STRM_END:
            printf("<end>");
            break;
        case TOK_LST:
            printf("[ ");
            break;
        case TOK_END:
            printf("] ");
            break;
        case TOK_IDENT:
            printf("<ident %s> ", tok->value->sval);
            break;
        case TOK_NUM:
            printf("<num %s> ", tok->value->sval);
            break;
        case TOK_CHAR:
            printf("'%s' ", tok->value->sval);
            break;
        case TOK_STR:
            printf("\"%s\" ", tok->value->sval);
            break;
        case TOK_LLST:
        case TOK_QLST:
        case TOK_FN:
            printf("%c[ ", tok->type);
            break;
        default:
            break;
        }
        tok = tok->next;
    }
    printf("\n");
}

struct Stream *Stream_alloc()
{
    struct Stream *strm = malloc(sizeof(struct Stream));
    return strm;
}

void Stream_free(struct Stream *strm)
{
}

struct Token *Stream_end(struct Stream *strm)
{
    struct Token *end = Token_alloc();
    end->type = TOK_STRM_END;
    EOS.next = &EOS;
    end->next = &EOS;
    strm->end = end;
    *strm->tokenlist = end;
    strm->tokenlist = NULL;
    return end;
}

int Stream_next(struct Stream *strm)
{
    int c, offset = strm->offset;

repeat:
    if (offset < strm->size) {
        if ((c = strm->buffer[offset++]) != '\n') {
            strm->offset = offset;
            strm->pos.col++;
            return c;
        }
        strm->pos.line++;
        strm->pos.col = 0;
        strm->offset = offset;
        strm->newline = 1;
        return c;
    }

    if (strm->fd < 0)
        return EOF;

    strm->size = read(strm->fd, strm->buffer, 1024);
    if (strm->size <= 0)
        return EOF;

    strm->offset = offset = 0;

    goto repeat;
}

int Stream_get_char_token(struct Stream *strm, int c, int next)
{
    char buf[5];
    // read bytes until '
    return next;
}

int Stream_get_str_token(struct Stream *strm, int c, int next)
{
    char buf[1028];
    // read bytes until "
    return next;
}

int Stream_get_num_token(struct Stream *strm, int c, int next)
{
    char buf[1028];
    // reead bytes until <ws> or ]
    return next;
}


int Stream_get_ident_token(struct Stream *strm, int c, int next)
{
    char buf[1028];
    int len = 1;

    // read bytes until <ws> or ]
    buf[0] = c;
    for (;;) {
        if (isspace(next) || next == TOK_END)
            break;
        if (len >= sizeof(buf))
            break;
        buf[len] = next;
        len++;
        next = Stream_next(strm);
    }
    buf[len] = 0;

    // handle ident exceeds maximum length

    strm->token->type = TOK_IDENT;
    strm->token->value = String_from_buffer(buf, len);

    return next;
}

void Stream_add_token(struct Stream *strm)
{
    struct Token *tok = strm->token;
    strm->token = NULL;
    tok->next = NULL;
    *strm->tokenlist = tok;
    strm->tokenlist = &tok->next;
}

int Stream_get_token(struct Stream *strm, int c)
{
    int next = Stream_next(strm);
    if (c == TOK_LLST && next == TOK_LST) {
        strm->token->type = TOK_LLST;
        next = Stream_next(strm);
    } else if (c == TOK_QLST && next == TOK_LST) {
        strm->token->type = TOK_QLST;
        next = Stream_next(strm);
    } else if (c == TOK_FN && next == TOK_LST) {
        strm->token->type = TOK_FN;
        next = Stream_next(strm);
    } else if (c == TOK_LST)
        strm->token->type = TOK_LST;
    else if (c == TOK_CHAR)
        next = Stream_get_char_token(strm, c, next);
    else if (c == TOK_STR)
        next = Stream_get_str_token(strm, c, next);
    else if (c == TOK_END)
        strm->token->type = TOK_END;
    else if (c >= '0' && c <= '9')
        next = Stream_get_num_token(strm, c, next);
    else
        next = Stream_get_ident_token(strm, c, next);
    Stream_add_token(strm);
    return next;
}

struct Token *Stream_tokenize(struct Stream *strm)
{
    int c = Stream_next(strm);
    while (c != EOF) {
        if (!isspace(c)) {
            struct Token *tok = Token_alloc();
            strm->token = tok;
            strm->newline = 0;
            strm->whitespace = 0;
            c = Stream_get_token(strm, c);
            continue;
        }
        strm->whitespace = 1;
        c = Stream_next(strm);
    }
    return Stream_end(strm);
}

struct Token *Stream_setup(struct Stream *strm, int fd, char *buf, size_t size)
{
    struct Token *start;

	strm->newline = 1;
	strm->whitespace = 0;
	strm->pos.line = 1;
    strm->pos.col = 0;

	strm->token = NULL;
	strm->fd = fd;
	strm->offset = 0;
	strm->size = size;
	strm->buffer = buf;

	start = Token_alloc();
	start->type = TOK_STRM_START;
	strm->tokenlist = &start->next;

	return start;
}

struct Interpreter *Interpreter_alloc()
{
    struct Interpreter *interp = malloc(sizeof(struct Interpreter));
    return interp;
}


char *prompt(EditLine *el)
{
    return "tt> ";
}

int interactive(struct Interpreter *interp, char *progn)
{
    EditLine *el;
    History *hist;
    HistEvent ev;
    const char *line;
    int count;

    interp->stream = Stream_alloc();

    el = el_init(progn, stdin, stdout, stderr);
    if (!el) {
        fprintf(stderr, "editline could not be initialized\n");
        goto cleanup;
    }
    el_set(el, EL_PROMPT, &prompt);
    el_set(el, EL_EDITOR, "emacs");

    hist = history_init();
    if (!hist) {
        fprintf(stderr, "history could not be initialized\n");
        goto cleanup;
    }
    history(hist, &ev, H_SETSIZE, 800);
    el_set(el, EL_HIST, history, hist);

    for (;;) {
        line = el_gets(el, &count);

        if (strcmp(line, "quit\n") == 0)
            break;

        struct Token *start = Stream_setup(interp->stream, -1, line, count);
        struct Token *end = Stream_tokenize(interp->stream);
        Token_dump_tokens(start);

        if (count > 0)
            history(hist, &ev, H_ENTER, line);
    }

cleanup:
    if (hist) history_end(hist);
    if (el) el_end(el);

    return 1;
}


int main(int argc, char **argv)
{
    struct Interpreter *interp = Interpreter_alloc();
    interactive(interp, argv[0]);

    return 0;
}
