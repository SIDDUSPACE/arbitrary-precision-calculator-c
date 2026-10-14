#include "apc.h"
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if INT_MAX >= 2147483647
#define CHUNK_DIGITS 4
#define BASE 10000
#elif INT_MAX >= 32767
#define CHUNK_DIGITS 2
#define BASE 100
#else
#define  CHUNK_DIGITS 1
#define BASE 10
#endif
static char *dupstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (copy) memcpy(copy, s, n);
    return copy;
}
static void list_clear(Dlist **h, Dlist **t) {
    Dlist *p = *h;
    while (p) {
        Dlist *next = p->next;
        free(p);
        p = next;
    }
    *h = *t = NULL;
}
static int push_back(Dlist **h, Dlist **t, int v) {
    Dlist *n = malloc(sizeof(*n));
    if (!n) return 0;
    n->value = v; n->prev = *t; n->next = NULL;
    if (*t) (*t)->next = n; else *h = n;
    *t = n;
    return 1;
}
static void trim(Dlist **t) {
    while (*t && (*t)->value == 0 && (*t)->prev) {
        Dlist *old = *t;
        *t = old->prev; (*t)->next = NULL; free(old);
    }
}
static int is_zero(const Decimal *n) {
    return !n->tail || (n->head == n->tail && n->head->value == 0);
}
static int mag_cmp(const Decimal *a, const Decimal *b) {
    Dlist *x, *y;
    size_t nx = 0, ny = 0;
    for (x=a->head; x; x=x->next) nx++;
    for (y=b->head; y; y=y->next) ny++;
    if (nx != ny) return nx < ny ? -1 : 1;
    x=a->tail; y=b->tail;
    while (x && y) {
        if (x->value != y->value) return x->value < y->value ? -1 : 1;
        x=x->prev; y=y->prev;
    }
    return 0;
}
static int from_digits(const char *digits, Decimal *out) {
    out->head = out->tail = NULL;
    size_t len = strlen(digits);
    if (!len) return push_back(&out->head, &out->tail, 0);
    size_t end = len;
    while (end > 0) {
        size_t start = end >= CHUNK_DIGITS ? end - CHUNK_DIGITS : 0;
        int v = 0;
        for (size_t i=start; i<end; ++i) v = v*10 + (digits[i]-'0');
        if (!push_back(&out->head, &out->tail, v)) {
            list_clear(&out->head, &out->tail); return 0;
        }
        end = start;
    }
    trim(&out->tail);
    return 1;
}
static char *mag_to_digits(const Decimal *n) {
    if (!n->tail) return NULL;
    size_t chunks=0;
    for (Dlist *p=n->head; p; p=p->next) chunks++;
    size_t cap = chunks * CHUNK_DIGITS + 2;
    char *s = malloc(cap);
    if (!s) return NULL;
    int k = snprintf(s, cap, "%d", n->tail->value);
    if (k < 0) { free(s); return NULL; }
    size_t pos = (size_t)k;
    for (Dlist *p=n->tail->prev; p; p=p->prev) {
        snprintf(s+pos, cap-pos, "%0*d", CHUNK_DIGITS, p->value);
        pos += CHUNK_DIGITS;
    }
    return s;
}
static char *strip_zeros(const char *s) {
    while (*s == '0' && s[1]) s++;
    return dupstr(s);
}
static char *addstr(const char *a, const char *b) {
    size_t la=strlen(a), lb=strlen(b), n=(la>lb?la:lb);
    char *r=malloc(n+2); if(!r) return NULL;
    r[n+1]='\0'; int carry=0;
    for(size_t i=0;i<n;i++) {
        int x=la>i?a[la-1-i]-'0':0, y=lb>i?b[lb-1-i]-'0':0;
        int v=x+y+carry; r[n-i]=(char)('0'+v%10); carry=v/10;
    }
    r[0]=(char)('0'+carry);
    char *out = strip_zeros(r); free(r); return out;
}
static char *substr(const char *a, const char *b) {
    size_t la=strlen(a), lb=strlen(b);
    char *r=malloc(la+1); if(!r) return NULL;
    r[la]='\0'; int borrow=0;
    for(size_t i=0;i<la;i++) {
        int x=a[la-1-i]-'0'-borrow, y=lb>i?b[lb-1-i]-'0':0;
        if(x<y){x+=10;borrow=1;}else borrow=0;
        r[la-1-i]=(char)('0'+x-y);
    }
    char *out=strip_zeros(r); free(r); return out;
}
static char *mulstr(const char *a, const char *b) {
    size_t la=strlen(a), lb=strlen(b);
    if ((la==1&&a[0]=='0') || (lb==1&&b[0]=='0')) return dupstr("0");
    int *v=calloc(la+lb, sizeof(int)); if(!v) return NULL;
    for (size_t ii=la; ii>0; --ii) {
        int carry=0;
        size_t i=ii-1;
        for (size_t jj=lb; jj>0; --jj) {
            size_t j=jj-1;
            int sum=(a[i]-'0')*(b[j]-'0')+v[i+j+1]+carry;
            v[i+j+1]=sum%10;
            carry=sum/10;
        }
        v[i]+=carry;
    }
    char *r=malloc(la+lb+1); if(!r){free(v);return NULL;}
    for(size_t i=0;i<la+lb;i++)r[i]=(char)('0'+v[i]);
    r[la+lb]='\0'; free(v);
    char *out=strip_zeros(r);free(r);return out;
}
static char *mul_small(const char *a, unsigned m) {
    size_t la=strlen(a); char *r=malloc(la+16); if(!r)return NULL;
    unsigned long long carry=0; size_t pos=la+15; r[pos]='\0';
    for(size_t i=0;i<la;i++){
        unsigned long long v=(unsigned)(a[la-1-i]-'0')*(unsigned long long)m+carry;
        r[--pos]=(char)('0'+v%10); carry=v/10;
    }
    while(carry){r[--pos]=(char)('0'+carry%10);carry/=10;}
    char *out=strip_zeros(r+pos);free(r);return out;
}
static char *divmodstr(const char *a, const char *b, char **remout) {
    if (strcmp(b,"0")==0) return NULL;
    size_t la=strlen(a); char *q=malloc(la+1); if(!q)return NULL;
    char *rem=dupstr("0"); if(!rem){free(q);return NULL;}
    for(size_t i=0;i<la;i++){
        size_t lr=strlen(rem); char *next=malloc(lr+2);
        if(!next){free(q);free(rem);return NULL;}
        memcpy(next,rem,lr); next[lr]=a[i];next[lr+1]='\0';
        free(rem); rem=strip_zeros(next);free(next);
        int digit=0;
        for(int d=9;d>=1;d--){
            char *prod=mul_small(b,(unsigned)d);
            if(!prod){free(q);free(rem);return NULL;}
            int c=(strlen(rem)!=strlen(prod))?(strlen(rem)<strlen(prod)?-1:1):strcmp(rem,prod);
            free(prod);
            if(c>=0){digit=d;break;}
        }
        if(digit){
            char *prod=mul_small(b,(unsigned)digit);
            char *nr=substr(rem,prod);free(prod);free(rem);rem=nr;
            if(!rem){free(q);return NULL;}
        }
        q[i]=(char)('0'+digit);
    }
    q[la]='\0';
    char *qq=strip_zeros(q);free(q);
    if(remout)*remout=rem;else free(rem);
    return qq;
}
static char *shift10(const char *s, size_t n) {
    size_t l=strlen(s); char *r=malloc(l+n+1); if(!r)return NULL;
    memcpy(r,s,l); memset(r+l,'0',n);r[l+n]='\0';return r;
}
static int set_from_coeff(const char *digits, size_t scale, int sign, Decimal *out) {
    char *d=strip_zeros(digits); if(!d)return 0;
    if(!from_digits(d,out)){free(d);return 0;}
    out->scale=scale;out->sign=(strcmp(d,"0")==0)?0:(sign<0?-1:1);
    free(d);return 1;
}
static int normalize(Decimal *n) {
    char *d=mag_to_digits(n); if(!d)return 0;
    size_t len=strlen(d);
    while(n->scale>0 && len>1 && d[len-1]=='0'){d[--len]='\0';n->scale--;}
    if(strcmp(d,"0")==0){n->sign=0;n->scale=0;}
    Decimal tmp={0};
    int ok=from_digits(d,&tmp);
    free(d);
    if(!ok)return 0;
    list_clear(&n->head,&n->tail);
    n->head=tmp.head;n->tail=tmp.tail;
    return 1;
}
void decimal_free(Decimal *n) {
    if(!n)return;
    list_clear(&n->head,&n->tail);n->sign=0;n->scale=0;
}
int decimal_parse(const char *text, Decimal *out) {
    if(!text||!out)return 0;
    out->head=out->tail=NULL;out->sign=1;out->scale=0;
    while(isspace((unsigned char)*text))text++;
    int sign=1;if(*text=='+'||*text=='-'){if(*text=='-')sign=-1;text++;}
    size_t cap=strlen(text)+2;char *digits=malloc(cap);if(!digits)return 0;
    size_t n=0;int dot=0,seen=0;
    for(;*text&&!isspace((unsigned char)*text);text++){
        if(*text=='.'&&!dot){dot=1;continue;}
        if(!isdigit((unsigned char)*text)){free(digits);return 0;}
        digits[n++]=*text;seen=1;if(dot)out->scale++;
    }
    if(!seen){free(digits);return 0;}
    digits[n]='\0';
    char *d=strip_zeros(digits);free(digits);if(!d)return 0;
    if(!set_from_coeff(d,out->scale,sign,out)){free(d);return 0;}
    free(d);return normalize(out);
}
static int scaled_digits(const Decimal *n,size_t scale,char **out) {
    char *d=mag_to_digits(n);if(!d)return 0;
    if(scale<n->scale){free(d);return 0;}
    *out=shift10(d,scale-n->scale);free(d);return *out!=NULL;
}
static int result_from_str(const char *s,size_t scale,int sign,Decimal *out) {
    out->head=out->tail=NULL;
    return set_from_coeff(s,scale,sign,out)&&normalize(out);
}
int decimal_calculate(const Decimal *a,const Decimal *b,char op,Decimal *out) {
    if(!a||!b||!out)return 0;
    out->head=out->tail=NULL;out->sign=0;out->scale=0;
    char *ad=NULL,*bd=NULL,*r=NULL,*rem=NULL;int sign=1;size_t sc=0;
    if(op=='+'||op=='-'){
        sc=a->scale>b->scale?a->scale:b->scale;
        if(!scaled_digits(a,sc,&ad)||!scaled_digits(b,sc,&bd))goto fail;
        int sb=b->sign*(op=='-'?-1:1);
        if(a->sign==0){r=dupstr(bd);sign=sb;}
        else if(sb==0){r=dupstr(ad);sign=a->sign;}
        else if(a->sign==sb){r=addstr(ad,bd);sign=a->sign;}
        else {
            Decimal aa={0},bb={0};from_digits(ad,&aa);from_digits(bd,&bb);
            int c=mag_cmp(&aa,&bb);decimal_free(&aa);decimal_free(&bb);
            if(c==0){r=dupstr("0");sign=0;}
            else if(c>0){r=substr(ad,bd);sign=a->sign;}
            else {r=substr(bd,ad);sign=sb;}
        }
    } else if(op=='*'){
        ad=mag_to_digits(a);bd=mag_to_digits(b);if(!ad||!bd)goto fail;
        r=mulstr(ad,bd);sign=a->sign*b->sign;sc=a->scale+b->scale;
    } else if(op=='/'){
        if(is_zero(b))goto fail;
        const size_t precision=20;
        ad=mag_to_digits(a);bd=mag_to_digits(b);if(!ad||!bd)goto fail;
        size_t lhszeros=b->scale+precision, rhszeros=a->scale;
        char *lhs=shift10(ad,lhszeros),*rhs=shift10(bd,rhszeros);
        if(!lhs||!rhs){free(lhs);free(rhs);goto fail;}
        r=divmodstr(lhs,rhs,NULL);free(lhs);free(rhs);
        sign=a->sign*b->sign;sc=precision;
    } else if(op=='%'){
        sc=a->scale>b->scale?a->scale:b->scale;
        if(!scaled_digits(a,sc,&ad)||!scaled_digits(b,sc,&bd)||is_zero(b))goto fail;
        Decimal aa={0},bb={0};from_digits(ad,&aa);from_digits(bd,&bb);
        if(!aa.head||!bb.head){decimal_free(&aa);decimal_free(&bb);goto fail;}
        rem=NULL;char *q=divmodstr(ad,bd,&rem);free(q);decimal_free(&aa);decimal_free(&bb);
        r=rem;rem=NULL;sign=a->sign;
    } else if(op=='^'){
        ad=mag_to_digits(a);bd=mag_to_digits(b);if(!ad||!bd)goto fail;
        if(b->scale!=0)goto fail;
        if(strlen(bd)>6)goto fail;
        unsigned long exp=strtoul(bd,NULL,10);
        if(exp>10000)goto fail;
        if(b->sign<0){
            Decimal one={0},pow={0};
            if(!decimal_parse("1",&one)){goto fail;}
            if(!result_from_str("1",0,1,&pow)){decimal_free(&one);goto fail;}
            char *acc=dupstr("1"),*base=dupstr(ad);
            size_t accum_scale=0,base_scale=a->scale;
            if(!acc||!base){free(acc);free(base);decimal_free(&one);decimal_free(&pow);goto fail;}
            unsigned long e=exp;
            while(e){if(e&1){char *t=mulstr(acc,base);free(acc);acc=t;accum_scale+=base_scale;if(!acc)break;}
                e>>=1;if(e){char *t=mulstr(base,base);free(base);base=t;base_scale*=2;if(!base)break;}}
            free(base);
            if(!acc){free(acc);decimal_free(&one);decimal_free(&pow);goto fail;}
            char *numer=shift10("1",accum_scale+20);
            char *quot=numer?divmodstr(numer,acc,NULL):NULL;
            free(numer);free(acc);decimal_free(&one);decimal_free(&pow);
            if(!quot)goto fail;
            r=quot;sc=20;sign=(a->sign<0 && (exp%2))?-1:1;
        } else {
            r=dupstr("1");char *base=dupstr(ad);size_t rs=0,bs=a->scale;
            if(!r||!base){free(r);free(base);r=NULL;goto fail;}
            unsigned long e=exp;
            while(e){
                if(e&1){char *t=mulstr(r,base);free(r);r=t;rs+=bs;if(!r)break;}
                e>>=1;
                if(e){char *t=mulstr(base,base);free(base);base=t;bs*=2;if(!base)break;}
            }
            free(base);sc=rs;sign=(a->sign<0 && (exp%2))?-1:1;
        }
    } else goto fail;
    if(!r)goto fail;
    if(!result_from_str(r,sc,sign,out))goto fail;
    free(ad);free(bd);free(r);free(rem);return 1;
fail:
    free(ad);free(bd);free(r);free(rem);decimal_free(out);return 0;
}
char *decimal_to_string(const Decimal *n) {
    char *d=mag_to_digits(n);if(!d)return NULL;
    size_t len=strlen(d),scale=n->scale;
    size_t extra=(scale>=len)?(scale-len+2):0;
    char *r=malloc(len+extra+4);if(!r){free(d);return NULL;}
    size_t p=0;if(n->sign<0)r[p++]='-';
    if(scale==0){memcpy(r+p,d,len+1);}
    else if(len>scale){memcpy(r+p,d,len-scale);p+=len-scale;r[p++]='.';memcpy(r+p,d+len-scale,scale);p+=scale;r[p]='\0';}
    else {r[p++]='0';r[p++]='.';for(size_t i=0;i<scale-len;i++)r[p++]='0';memcpy(r+p,d,len);p+=len;r[p]='\0';}
    free(d);return r;
}
