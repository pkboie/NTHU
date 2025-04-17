#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// for lex
#define MAXLEN 256

typedef enum{
    ENDFILE,        // EOF
    END,            // "\n"
    INT,            // interger
    ID,             // new variables
    INCDEC,         // ++  --
    ADDSUB,         // +  -
    MULDIV,         // *  /
    ASSIGN,         // =
    ADDSUB_ASSIGN,  // +=  -=
    AND,            // &
    OR,             // |
    XOR,            // ^
    LPAREN,         // (
    RPAREN,         // )
    UNKNOWN,
} TokenSet;

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);


// for parser
#define TBLSIZE 64
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 1

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

int sbcount = 0;
Symbol table[TBLSIZE];

int addsubflag = 0, mulflag = 0, andflag = 0, xorflag = 0, orflag = 0;

// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str, int val);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);

extern void statement();
extern BTNode *assign_expr();
extern BTNode *or_expr();
extern BTNode *or_expr_tail(BTNode *left);
extern BTNode *xor_expr();
extern BTNode *xor_expr_tail(BTNode *left);
extern BTNode *and_expr();
extern BTNode *and_expr_tail(BTNode *left);
extern BTNode *addsub_expr();
extern BTNode *addsub_expr_tail(BTNode *left);
extern BTNode *muldiv_expr();
extern BTNode *muldiv_expr_tail(BTNode *left);
extern BTNode *unary_expr();
extern BTNode *factor();

// Print error message and exit the program
void err(ErrorType errorNum);

// for codeGen
// Evaluate the syntax tree
int evaluateTree(BTNode *root);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);


/*============================================================================================
lex implementation
============================================================================================*/

TokenSet getToken() {
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t'); 

    if (isdigit(c)) {
        lexeme[i++] = c;
        c = fgetc(stdin);
        while (isdigit(c) && i < MAXLEN - 1) {
            lexeme[i++] = c;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    }
    else if (c == '+' || c == '-') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        c = fgetc(stdin);
        if(c == '='){
            lexeme[i++] = c;
            lexeme[i] = '\0';
            return ADDSUB_ASSIGN;
        }
        else if(c == '+' || c == '-'){
            lexeme[i++] = c;
            lexeme[i] = '\0';
            return INCDEC;
        }
        ungetc(c, stdin);
        return ADDSUB;
    }
    else if (c == '*' || c == '/') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return MULDIV;
    }
    else if (c == '=') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return ASSIGN;
    }
    else if (c == '(') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return LPAREN;
    }
    else if (c == ')') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return RPAREN;
    }
    else if (c == '&') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return AND;
    }
    else if (c == '|') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return OR;
    }
    else if (c == '^') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return XOR;
    }
    else if (isalpha(c) || c == '_') {
        lexeme[i++] = c;
        c = fgetc(stdin);
        while (isalnum(c) || c == '_' && i < MAXLEN-1) {
            lexeme[i++] = c;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    }
    else if (c == '\n') {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return END;
    }
    else if(c == EOF){
        return ENDFILE;
    }
    else {
        lexeme[i++] = c;
        lexeme[i] = '\0';
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}


/*============================================================================================
parser implementation
============================================================================================*/

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;
    int found = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0){
            found = 1;
            return table[i].val;
        }

    if (sbcount >= TBLSIZE){
        error(RUNOUT);
    }

    if(!found){
        error(NOTFOUND);
    }
    return 0;
}

int setval(char *str, int val) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            //printf("Before %s %d\n", table[i].name, table[i].val);
            table[i].val = val;
            //printf("After %s %d\n", table[i].name, table[i].val);
            return val;
        }
    }

    if (sbcount >= TBLSIZE){
        error(RUNOUT);
    }

    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// statement := ENDFILE | END | assign_expr END
void statement(){
    BTNode *retp = NULL;

    if(match(ENDFILE)){
        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        printf("EXIT 0\n");
        exit(0);
    }
    else if(match(END)){
        //printf(">> ");
        advance();
    }
    else{
        retp = assign_expr();
        if(match(END)) {
            // printf("%d\n", evaluateTree(retp));
            // printf("Prefix traversal: ");
            // printPrefix(retp);
            // printf("\n");
            // freeTree(retp);
            // printf(">> ");
            // advance();
            evaluateTree(retp);
            freeTree(retp);
            advance();
        }
        else{
            error(SYNTAXERR);
        }
    }
}

// assign_expr := ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr
BTNode *assign_expr(){
    BTNode *retp = NULL, *left = or_expr();
    if(left->data == ID){
        if(match(ASSIGN)){
            retp = makeNode(ASSIGN, getLexeme());
            advance();
            retp->left = left;
            retp->right = assign_expr();
            return retp;
        }
        else if(match(ADDSUB_ASSIGN)){
            retp = makeNode(ADDSUB_ASSIGN, getLexeme());
            advance();
            retp->left =left;
            retp->right = assign_expr();
            return retp;
        }
    }
    return left;
}

// or_expr := xor_expr or_expr_tail
BTNode *or_expr(){
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if(match(OR)){
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        orflag = 0;
        node->right = xor_expr();
        if((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !orflag) error(NOTLVAL);
        orflag = 0;
        return or_expr_tail(node);
    }
    else{
        return left;
    }
}

// xor_expr := and_expr xor_expr_tail
BTNode *xor_expr(){
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}

// xor_expr_tail := XOR and_expr xor_expr_tail | NiL
BTNode *xor_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if(match(XOR)){
        node = makeNode(XOR, getLexeme());
        node->left = left;
        xorflag = 0;
        node->right = and_expr();
        if((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !xorflag) error(NOTLVAL);
        xorflag = 0;
    }
    else{
        return left;
    }
}

// and_expr := addsub_expr and_expr_tail
BTNode *and_expr(){
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}

// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if(match(AND)){
        node = makeNode(AND, getLexeme());
        node->left = left;
        andflag = 0;
        node->right = addsub_expr();
        if((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !andflag) error(NOTLVAL);
        andflag = 0;
        return and_expr_tail(node);
    }
    return left;
}

// addsub_expr := muldiv_expr addsub_expr_tail
BTNode *addsub_expr(){
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}

// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode *addsub_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(ADDSUB)){
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        addsubflag = 0;
        node->right = muldiv_expr();
        if ((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !addsubflag) error(NOTLVAL);
        addsubflag = 0;
        return addsub_expr_tail(node);
    }
    else {
        return left;
    }
}

// muldiv_expr := unary_expr muldiv_expr_tail
BTNode *muldiv_expr(){
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}

// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode *muldiv_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(MULDIV)){
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        mulflag = 0;
        node->right = unary_expr();
        if ((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !mulflag) error(NOTLVAL);
        mulflag = 0;
        return muldiv_expr_tail(node);
    }
    else {
        return left;
    }
}

// unary_expr := ADDSUB unary_expr | factor
BTNode *unary_expr(){
    BTNode *retp = NULL;
    if(match(ADDSUB)){
        retp = makeNode(ADDSUB, getLexeme());
        advance();
        retp->left = makeNode(INT, "0");
        retp->right = unary_expr();
    }
    else{
        retp = factor();
    }

    return retp;
}

// factor := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(){
    BTNode *retp = NULL, *left = NULL;
    if(match(INT)){
        retp = makeNode(INT, getLexeme());
        advance();
    }
    else if(match(ID)){
        retp = makeNode(ID, getLexeme());
        advance();
    }
    else if(match(INCDEC)){
        retp = makeNode(INCDEC, getLexeme());
        advance();
        if(match(ID)){
            left = makeNode(ID, getLexeme());
            retp->left = left;
            advance();
        }
        else{
            error(NOTNUMID);
        }
    }
    else if(match(LPAREN)){
        addsubflag = mulflag = andflag = xorflag = orflag = 1;
        advance();
        retp = assign_expr();
        if(match(RPAREN)){
            advance();
        }
        else{
            error(MISPAREN);
        }
    }
    else{
        error(NOTNUMID);
    }
    return retp;
}

void err(ErrorType errorNum) {
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
        printf("EXIT 1\n");
    }
    exit(0);
}

/*============================================================================================
codeGen implementation
============================================================================================*/

int r_cnt;
int pos_cnt;

int find_pos_cnt(char *name){
    pos_cnt = 0;
    for(int i=0; i<sbcount; i++){
        if(strcmp(name, table[i].name) == 0) break;
        pos_cnt++;
    }
    return pos_cnt;
}

int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                retval = getval(root->lexeme);
                pos_cnt = find_pos_cnt(root->lexeme);
                printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                break;
            case INT:
                retval = atoi(root->lexeme);
                printf("MOV r%d %d\n", r_cnt++, retval);
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme, rv);
                
                if(root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->right->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                pos_cnt = find_pos_cnt(root->left->lexeme);
                printf("MOV [%d] r%d\n", pos_cnt*4, r_cnt-1);
                r_cnt--; // 運算完 r_cnt 會直接丟掉
                break;
            case ADDSUB_ASSIGN:
                lv = getval(root->left->lexeme);
                rv = evaluateTree(root->right);
                
                if(root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->right->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                pos_cnt = find_pos_cnt(root->left->lexeme);

                if(strcmp(root->lexeme, "+=") == 0){
                    retval = setval(root->left->lexeme, lv + rv);
                    printf("ADD r%d r%d", r_cnt-2, r_cnt-1);
                    r_cnt--;
                    printf("MOV [%d] r%d\n", pos_cnt*4, r_cnt-1);
                    r_cnt--;
                }
                else if(strcmp(root->lexeme, "-=") == 0){
                    retval = setval(root->left->lexeme, lv - rv);
                    printf("SUB r%d r%d", r_cnt-2, r_cnt-1);
                    r_cnt--;
                    printf("MOV [%d] r%d", pos_cnt*4, r_cnt-1);
                    r_cnt--;
                }
                break;
            case ADDSUB:
            case MULDIV:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);

                if (root->left->data == ASSIGN || root->left->data == ADDSUB_ASSIGN || root->left->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->left->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->right->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                    printf("ADD r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                } 
                else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                    printf("SUB r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                } 
                else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                    printf("MUL r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                } 
                else if (strcmp(root->lexeme, "/") == 0) {
                    if (rv == 0){
                        error(DIVZERO);
                    }
                    retval = lv / rv;
                    printf("DIV r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                }
                break;
            case INCDEC:
                retval = getval(root->left->lexeme);
                pos_cnt = 0;

                pos_cnt = find_pos_cnt(root->left->lexeme);
                printf("MOV r%d 1\n", r_cnt++);

                if(strcmp(root->lexeme, "++") == 0){
                    retval++;
                    setval(root->right->lexeme, retval);
                    printf("ADD r%d r%d\n", r_cnt-2, r_cnt-1);
                }
                else if(strcmp(root->lexeme, "--") == 0){
                    retval--;
                    setval(root->right->lexeme, retval);
                    printf("SUB r%d r%d\n", r_cnt-2, r_cnt-1);
                }
                printf("MOV [%d] r%d\n", pos_cnt*4, r_cnt-2);
                r_cnt--;
                break;
            case AND:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (root->left->data == ASSIGN || root->left->data == ADDSUB_ASSIGN || root->left->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->left->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->right->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }
                retval = lv & rv;
                printf("AND r%d r%d\n", r_cnt-2, r_cnt-1);
                r_cnt--;
                break;
            case OR:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (root->left->data == ASSIGN || root->left->data == ADDSUB_ASSIGN || root->left->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->left->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->right->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }
                retval = lv | rv;
                printf("OR r%d r%d\n", r_cnt-2, r_cnt-1);
                r_cnt--;
                break;
            case XOR:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (root->left->data == ASSIGN || root->left->data == ADDSUB_ASSIGN || root->left->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->left->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = find_pos_cnt(root->right->left->lexeme);
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }
                retval = lv ^ rv;
                printf("XOR r%d r%d\n", r_cnt-2, r_cnt-1);
                r_cnt--;
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}

/*============================================================================================
main
============================================================================================*/

// This package is a calculator
// It works like a Python interpretor
// Example:
// >> y = 2
// >> z = 2
// >> x = 3 * y + 4 / (2 * z)
// It will print the answer of every line
// You should turn it into an expression compiler
// And print the assembly code according to the input

int main() {
    initTable();
    // printf(">> ");
    while (1) {
        statement();
    }
    return 0;
}
