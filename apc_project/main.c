#include "apc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void usage(const char *name) {
    printf("Usage: %s <number1> <operator> <number2>\n", name);
    puts("Operators: +  -  *  /  %  ^");
    puts("Quote numbers if they contain shell-special characters.");
    puts("Division returns up to 20 fractional digits. Power requires an integer exponent.");
}
int main(int argc, char **argv) {
    char left[4096], right[4096], op;
    if (argc == 4) {
        if (strlen(argv[2]) != 1) { usage(argv[0]); return 2; }
        snprintf(left,sizeof left,"%s",argv[1]);
        snprintf(right,sizeof right,"%s",argv[3]);op=argv[2][0];
    } else {
        printf("Arbitrary Precision Calculator (Doubly Linked List)\n");
        printf("Enter expression as: number operator number\n> ");
        if (scanf("%4095s %c %4095s",left,&op,right)!=3) {
            fprintf(stderr,"Invalid input.\n");return 2;
        }
    }
    Decimal a={0},b={0},result={0};
    if(!decimal_parse(left,&a)||!decimal_parse(right,&b)) {
        fprintf(stderr,"Invalid number. Use digits and at most one decimal point.\n");
        decimal_free(&a);decimal_free(&b);return 2;
    }
    if(op!='+'&&op!='-'&&op!='*'&&op!='/'&&op!='%'&&op!='^') {
        fprintf(stderr,"Unsupported operator: %c\n",op);
        decimal_free(&a);decimal_free(&b);return 2;
    }
    if(op=='^'&&b.scale!=0) {
        fprintf(stderr,"Power requires an integer exponent.\n");
        decimal_free(&a);decimal_free(&b);return 2;
    }
    if(op=='/' && b.sign==0) {
        fprintf(stderr,"Error: division by zero.\n");
        decimal_free(&a);decimal_free(&b);return 2;
    }
    if(op=='%' && b.sign==0) {
        fprintf(stderr,"Error: modulus by zero.\n");
        decimal_free(&a);decimal_free(&b);return 2;
    }
    if(!decimal_calculate(&a,&b,op,&result)) {
        fprintf(stderr,"Calculation failed (unsupported range or memory allocation failure).\n");
        decimal_free(&a);decimal_free(&b);decimal_free(&result);return 1;
    }
    char *s=decimal_to_string(&result);
    if(!s){fprintf(stderr,"Out of memory.\n");decimal_free(&a);decimal_free(&b);decimal_free(&result);return 1;}
    printf("%s %c %s = %s\n",left,op,right,s);
    free(s);decimal_free(&a);decimal_free(&b);decimal_free(&result);
    return 0;
}
