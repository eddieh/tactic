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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


#define PROMPT "tt> "
#define BUFLEN 1024

#define EOF (-1)

enum Type {
    IdentType,
    StringType,
    CharType,
    IntegerType,
    FloatType,
    PairType,
    LListType,
    QListType,
    FNType,
};


struct Pair {
    enum Type type;
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

struct Pair EmptyList;
#define isNil(x) ((x) == &EmptyList)
#define Nil (&EmptyList)

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
    struct Token *currtoken;
};

struct Pair *parse(struct Interpreter *interp);

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

int read_escape_seq(int c)
{
    switch (c) {
    case 'a':
        c = '\a';
        break;
    case 'b':
        c = '\b';
        break;
    case 'f':
        c = '\f';
        break;
    case 'n':
        c = '\n';
        break;
    case 'r':
        c = '\r';
        break;
    case 't':
        c = '\t';
        break;
    case 'v':
        c = '\v';
        break;
    case '\\':
    case '\'':
    case '\"':
        break;
    }
    return c;
}

int Stream_get_char_token(struct Stream *strm, int c, int next)
{
    char buf[5];
    int len = 0;

    // read bytes until ', but handle \' and \\ as well as other
    // escape sequences
    buf[len] = next;
    for (;;) {
        if (next == '\'') {
            next = Stream_next(strm);
            break;
        }
        if (len >= 4)
            break;
        if (next == '\\')
            next = read_escape_seq(Stream_next(strm));
        buf[len] = next;
        len++;
        next = Stream_next(strm);
    }
    buf[len] = 0;

    // TODO: handle character exceeds 4 bytes

    strm->token->type = TOK_CHAR;
    strm->token->value = String_from_buffer(buf, len);

    return next;
}

int Stream_get_str_token(struct Stream *strm, int c, int next)
{
    char buf[1028];
    int len = 0;

    // read bytes until ", but handle \' and \\ as well as other
    // escape sequences
    buf[len] = next;
    for (;;) {
        if (next == '\"') {
            next = Stream_next(strm);
            break;
        }
        if (len >= sizeof(buf))
            break;
        if (next == '\\')
            next = read_escape_seq(Stream_next(strm));
        buf[len] = next;
        len++;
        next = Stream_next(strm);
    }
    buf[len] = 0;

    // TODO: handle string exceeds maximum length

    strm->token->type = TOK_STR;
    strm->token->value = String_from_buffer(buf, len);

    return next;
}

// valid numbers:
//   12    100.1    1/3    56%
//   0x1a1c    0b01010    10^3
//   34e13    3+6i    3i+4j+4k
//   102,000
int Stream_get_num_token(struct Stream *strm, int c, int next)
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

    // TODO: handle number exceeds maximum lenght

    strm->token->type = TOK_NUM;
    strm->token->value = String_from_buffer(buf, len);

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

    // TODO: handle ident exceeds maximum length

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
    strm->start = start;

	return start;
}

struct Interpreter *Interpreter_alloc()
{
    struct Interpreter *interp = malloc(sizeof(struct Interpreter));
    interp->stream = NULL;
    interp->currtoken = NULL;
    return interp;
}

struct Pair *pair_alloc(struct Pair *car, struct Pair *cdr)
{
    struct Pair *pr = malloc(sizeof(struct Pair));
    pr->type = PairType;
    pr->car = car;
    pr->cdr = cdr;
    return pr;
}

struct Pair *ident_alloc(struct Token *tok)
{
    struct Pair *pr = malloc(sizeof(struct Pair));
    pr->type = IdentType;
    pr->sval = tok->value->sval;
    pr->len = tok->value->len;
    return pr;
}

struct Token *next(struct Interpreter *interp)
{
    if (!interp->currtoken)
        interp->currtoken = interp->stream->start;
    else
        interp->currtoken = interp->currtoken->next;
    return interp->currtoken;
}

struct Token *peek(struct Interpreter *interp)
{
    /* if (!interp->currtoken) */
    /*     interp->currtoken = interp->stream->start; */
    return interp->currtoken->next;
}

struct Pair *parse_num(struct Interpreter *interp)
{
    return Nil;
}

struct Pair *parse_char(struct Interpreter *interp)
{
    return Nil;
}


struct Pair *parse_str(struct Interpreter *interp)
{
    return Nil;
}

struct Pair *parse_ident(struct Interpreter *interp)
{
    return Nil;
}

struct Pair *parse_fn(struct Interpreter *interp)
{
    return Nil;
}

struct Pair *parse_qlist(struct Interpreter *interp)
{
    return Nil;
}

struct Pair *parse_llist(struct Interpreter *interp)
{
    return Nil;
}

struct Pair *parse_list(struct Interpreter *interp)
{
    struct Pair *head = pair_alloc(Nil, Nil);
    struct Pair *pr = head;
    struct Token *tok;
    for (;;) {
        tok = peek(interp);
        if (tok->type == TOK_END) {
            pr->cdr = Nil;
            break;
        }
        pr->car = parse(interp);
        pr->cdr = pair_alloc(Nil, Nil);
        pr = pr->cdr;
    }
    return head;
}

struct Pair *parse(struct Interpreter *interp)
{
    struct Token *tok = next(interp);
    struct Pair *pr;

    switch (tok->type) {
    case TOK_STRM_START:
        pr = pair_alloc(parse(interp), Nil);//parse(interp));
        break;
    case TOK_LST:
        pr = parse_list(interp);
        break;
    case TOK_LLST:
        pr = parse_llist(interp);
        break;
    case TOK_QLST:
        pr = parse_qlist(interp);
        break;
    case TOK_FN:
        pr = parse_fn(interp);
        break;
    case TOK_IDENT:
        pr = ident_alloc(tok);
        break;
    case TOK_STR:
        pr = parse_str(interp);
        break;
    case TOK_CHAR:
        pr = parse_char(interp);
        break;
    case TOK_NUM:
        pr = parse_num(interp);
        break;
    case TOK_END:
    case TOK_STRM_END:
    default:
        pr = Nil;
        break;
    }

    return pr;
}

int interactive(struct Interpreter *interp)
{
    char line[BUFLEN];
    ssize_t count, out;

    interp->stream = Stream_alloc();

    for (;;) {
        out = write(STDOUT_FILENO, PROMPT, sizeof(PROMPT) - 1);
        if (out == -1) {
            perror("write fail");
            exit(1);
        }

        count = read(STDIN_FILENO, line, BUFLEN);
        if (count == -1) {
            perror("read fail");
            exit(1);
        }
        line[count] = 0;

        if (strcmp(line, "quit\n") == 0)
            break;

        struct Token *start = Stream_setup(interp->stream, -1, line, count);
        struct Token *end = Stream_tokenize(interp->stream);
        Token_dump_tokens(start);

        struct Pair *lst = parse(interp);
    }

    return 1;
}


int main(int argc, char **argv)
{
    struct Interpreter *interp = Interpreter_alloc();
    interactive(interp);

    return 0;
}
