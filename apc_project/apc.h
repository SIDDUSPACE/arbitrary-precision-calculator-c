#ifndef APC_H
#define APC_H
#include <stddef.h>
typedef struct node {
    struct node *prev;
    int value;
    struct node *next;
} Dlist;
typedef struct {
    Dlist *head;
    Dlist *tail;
    int sign; 
    size_t scale;   
} Decimal;
int decimal_parse(const char *text, Decimal *out);
void decimal_free(Decimal *n);
int decimal_calculate(const Decimal *a, const Decimal *b, char op, Decimal *out);
char *decimal_to_string(const Decimal *n);
#endif
