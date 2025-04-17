#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//2 6�O���~ 1 5 8�O��
//�u����?

/// for lex
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN,
    LPAREN, RPAREN,
    INCDEC, //++, --
    AND, OR, XOR,
    ADDSUB_ASSIGN //+=, -=
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

/// for parser
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
int paren_flag = 0, mulflag = 0, andflag = 0, xorflag = 0, orflag = 0;

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
BTNode *factor(void);
BTNode *term(void);
BTNode *term_tail(BTNode *left);
BTNode *expr(void);
BTNode *expr_tail(BTNode *left);
//=============
BTNode *assign_expr(void);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
BTNode *unary_expr(void);
//=============
void statement(void);
// Print error message and exit the program
void err(ErrorType errorNum);


/// for codeGen
// Evaluate the syntax tree
int evaluateTree(BTNode *root);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);
int findVar(BTNode *root); //�s��!!
int r_cnt;

/*============================================================================================
lex implementation
============================================================================================*/
//lex.c/.h
//recognizing which strings of symbols from the source program represent a single entity called token
//identifying whether they are numeric values, words, arithmetic operators, and so on.


//extracts the next token from the input string;
//stores the token in "char lexeme[MAXLEN]";
//identifies the token's type
TokenSet getToken(void) //�o��n��token�ɤW�h      //return�ȬO���A
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t'); //�p�G�O�Ů�N����Ū������ �]��stdin���r�|���_���e �ҥH�j��|���� �a

    if (isdigit(c)) { //�p�G�O12345 �NŪ�J"12345\0"
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);

        }
        //printf("-->>>>%c\n", c);
        ungetc(c, stdin); //��U��c�h�^��stdin�� �]���e��while�j��w�g��LŪ�X�� ���o�̨S�Ψ� �ҥH��L�h�^�h���U��ifŪ
        //printf("-->>>>%c\n", c);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') { //�U�ӬO=�N... ���O�N�h�^stdin
        char tmp = c;
        //printf("%c\n", c);
        c = fgetc(stdin);
        if (c == '='){
            lexeme[0] = tmp;
            lexeme[1] = c;
            lexeme[2] = '\0';
            return ADDSUB_ASSIGN;
        }
        else if (c == tmp){
            lexeme[0] = tmp;
            lexeme[1] = c;
            lexeme[2] = '\0';
            return INCDEC;
        }
        else {
            ungetc(c, stdin); //c���|�ܦ^�h
        }
        lexeme[0] = tmp;
        lexeme[1] = '\0';
        return ADDSUB;
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "="); //���ƻ�U���M�e�����@��?
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    }
    //else if (isalpha(c)) {
    //    lexeme[0] = c;
    //    lexeme[1] = '\0';
    //    return ID;
    //}

    //������h�R�W 1�}�Y���n�����? �ثe�Osyntax error
    else if (isalpha(c) || c == '_') {
        lexeme[0] = c;
        i = 1;
        c = fgetc(stdin);
        while ((isalnum(c) || c == '_') && i < MAXLEN) {
            lexeme[i++] = c;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    }




    else if (c == EOF) {
        return ENDFILE;
    }
    //====================
    else if (c == '&') {
        lexeme[0] = '&';
        lexeme[1] = '\0';
        return AND;
    }
    else if (c == '|') {
        lexeme[0] = '|';
        lexeme[1] = '\0';
        return OR;
    }
    else if (c == '^'){
        lexeme[0] = '^';
        lexeme[1] = '\0';
        return XOR;
    }
    //=====================
    else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken(); //current token? //��o�U��token
    //printf("\\\%d///\n", curToken);
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
//group tokens into statements based on a set of rules, collectively called a grammar.

//�o��n��� built-in variablies
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
    //printf("getval called for: '%s'\n", str);

    int i = 0;
    int found = 0;



    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0){
            found = 1;
            printf("MOV r%d [%d]\n", r_cnt++, i*4);
            return table[i].val;
        }
    }



    if (sbcount >= TBLSIZE)
        error(RUNOUT);

    if (!found) error(NOTFOUND);

    //�W�����N�U�� ����
    //strcpy(table[sbcount].name, str);
    //table[sbcount].val = 0;
    //sbcount++;
    return 0;
}

int setval(char *str, int val) { // set the value and return?
    //printf("%s = %d\n", str, val);

    int i = 0;
    //printf("%d\n", sbcount);
    for (i = 0; i < sbcount; i++) {

        if (strcmp(str, table[i].name) == 0) {
            //printf("%s\n", table[i].name);
            table[i].val = val;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);

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
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

//INT | ID | INCDEC ID | LPAREN assign_expr RPAREN assign��b�o�̼g: ID ASSIGN or_expr | ID ADDSUB_ASSIGN or_expr
BTNode *factor(void) {
    BTNode *retp = NULL, *left = NULL;

    if(match(INT))
    {
        retp = makeNode(INT, getLexeme());
        advance();
    }
    else if(match(ID))
    {
        left = makeNode(ID, getLexeme());
        advance();
        if(match(ASSIGN))
        {
            retp = makeNode(ASSIGN, getLexeme());

            retp->left = left;
            advance();
            retp->right = or_expr();
        }
        else if(match(ADDSUB_ASSIGN))
        {
            retp = makeNode(ADDSUB_ASSIGN, getLexeme());
            //printf("%s\n", retp->lexeme);
            //printf("left %s\n",left->left->lexeme);
            retp->left = left;
            advance();
            retp->right = or_expr();
            //printf("%s\n", retp->right->lexeme);
        }
        else
        {
            retp = left;
        }
    }
    else if(match(INCDEC))
    {
        retp = makeNode(INCDEC, getLexeme());
        advance();
        if(match(ID))
        {
            left = makeNode(ID, getLexeme());
            retp->left = left;
            advance();
        }
        else
        {
            error(NOTNUMID);
            //error(SYNTAXERR);
        }
    }
    else if(match(LPAREN))
    {
        paren_flag = mulflag = andflag = xorflag = orflag = 1;
        advance();
        retp = or_expr();
        if(match(RPAREN))
            advance();
        else
            error(MISPAREN);
    }
    else
    {
        //printf("%d<<\n", retp->data);
        error(NOTNUMID);
    }
    return retp;

}

//================================================================below is new

//ADDSUB unary_expr | factor //chatGPT
BTNode *unary_expr(void) {
    BTNode *retp = NULL;

    if (match(ADDSUB)) {
        retp = makeNode(ADDSUB, getLexeme());
        advance();
        retp->left = makeNode(INT, "0"); // �o�O�`���ޥ��A�� -x ���@ 0 - x
        retp->right = unary_expr();
    } else {
        retp = factor();
    }

    return retp;
}

//unary_expr muldiv_expr_tail
BTNode *muldiv_expr(void){
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}

//MULDIV unary_expr muldiv_expr_tail | NiL
BTNode *muldiv_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(MULDIV)){
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        mulflag = 0;
        node->right = unary_expr();
        //rintf("%d\n", paren_flag);
        if ((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !mulflag) error(NOTLVAL);
        mulflag = 0;
        return muldiv_expr_tail(node);
    }
    else {
        return left;
    }
}

//muldiv_expr addsub_expr_tail
BTNode *addsub_expr(void){
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}

//ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode *addsub_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(ADDSUB)){
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        paren_flag = 0;
        node->right = muldiv_expr();
        //printf("%s\n", node->right->lexeme);
        //if (node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) error(NOTLVAL);
        if ((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !paren_flag) error(NOTLVAL);
        paren_flag = 0;
        return addsub_expr_tail(node);
    }
    else {
        return left;
    }
}

//addsub_expr and_expr_tail
BTNode *and_expr(void){
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}

//AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(AND)){
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        andflag = 0;
        node->right = addsub_expr();
        if ((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !andflag) error(NOTLVAL);
        andflag = 0;
        return and_expr_tail(node);
    }
    else {
        return left;
    }
}

//and_expr xor_expr_tail
BTNode *xor_expr(void){
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}

//XOR and_expr xor_expr_tail | NiL
BTNode *xor_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(XOR)){
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        xorflag = 0;
        node->right = and_expr();
        if ((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !xorflag) error(NOTLVAL);
        xorflag = 0;
        return xor_expr_tail(node);
    }
    else {
        return left;
    }
}

//xor_expr or_expr_tail
BTNode *or_expr(void){
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

//OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(OR)){
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        orflag = 0;
        node->right = xor_expr();
        if ((node->right->data == ASSIGN || node->right->data == ADDSUB_ASSIGN) && !orflag) error(NOTLVAL);
        orflag = 0;
        return or_expr_tail(node);
    }
    else {
        return left;
    }
}

//ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr ���O��error?
//�ܥi��O�o���
BTNode *assign_expr(void) {
    BTNode *node = NULL, *left = NULL, *retp = NULL;
    if (match(ID)){
        left = makeNode(ID, getLexeme());
        advance();
        //printf("/////%s\n", left->lexeme);
        if (match(ASSIGN)){
            retp = makeNode(ASSIGN, getLexeme());
            advance();
            retp->left = left;
            retp->right = assign_expr();
        }
        else if (match(ADDSUB_ASSIGN)) {
            retp = makeNode(ADDSUB_ASSIGN, getLexeme());
            advance();
            retp->left = left;
            retp->right = assign_expr();
        }
        else err(SYNTAXERR);
    }
    else {
        retp = or_expr();
        //return or_expr_tail(node);
    }
    return retp;
}

//=================================================================

// statement := ENDFILE | END | assign_expr END           //new
void statement(void) {
    BTNode *retp = NULL;

    if (match(ENDFILE)) {

        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        printf("EXIT 0\n");
        //printf("%d %d %d", getval("x"), getval("y"), getval("z"));
        exit(0);
    } else if (match(END)) {
        //printf(">> ");
        advance();
    } else {
        retp = or_expr(); //�o�̪����ior_expr �]���p�G����getToken �����D�᭱�O���Oassign ��get�S��k��h �ҥHassign�d��factor�A��
        if (match(END)) {
            //printf("%d\n", evaluateTree(retp));
            //printf("Prefix traversal: ");
            //printPrefix(retp);
            //printf("\n");
            evaluateTree(retp);
            freeTree(retp);
            //printf(">> ");
            advance();
        } else {
            error(SYNTAXERR);
        }
    }
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

//number or variable: MOV r0 100 / MOV r1 [0]...
//assign: MOV [4] r2
//MUL�BADD�BSUB�BDIV: �s�b����
int findVar(BTNode *root)
{
    if (root == NULL) return 0;//���n �_�h�|runtime error
    else if (root->data == ID) return 1;
    else if (findVar(root->left)) return 1;
    else if (findVar(root->right)) return 1;
    else return 0;
}

//1/(0+0*(0&(x-0)))
int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;
    //static int r_cnt = 0; //�o�ˬO�諸��??
    int pos_cnt = 0;
    //printf("EVAL %s\n", root->lexeme);
    if (root != NULL) {
        switch (root->data) {
            //����ر��p �b=����M�b=�k�� �{�b�o�����Ӥ��� ���n�������\???
            case ID:
                retval = getval(root->lexeme);

                //printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                break;
            case INT:
                retval = atoi(root->lexeme);
                printf("MOV r%d %d\n", r_cnt++, retval);
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme, rv);
                //if (root->left->data != ID) error(SYNTAXERR);
                //printf("%s = %d\n", root->left->lexeme, rv);
                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->right->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                pos_cnt = 0;
                for (int i=0;i<sbcount;i++){
                    if (strcmp (root->left->lexeme, table[i].name) == 0) break;
                    pos_cnt++;
                }
                //r_cnt--;
                printf("MOV [%d] r%d\n", pos_cnt*4, r_cnt-1);
                r_cnt--;
                //printf("MOV ");
                //if (root->left->data)
                break;
            case ADDSUB_ASSIGN: //�s�g��
                lv = getval(root->left->lexeme); // �����o���䪺�ܼƭ�
                rv = evaluateTree(root->right);

                //if (root->left->data != ID) error(SYNTAXERR);

                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->right->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }
                pos_cnt = 0;
                for (int i=0;i<sbcount;i++){
                    if (strcmp (root->left->lexeme, table[i].name) == 0) break;
                    pos_cnt++;
                }
                //printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                if (strcmp(root->lexeme, "+=") == 0) {
                    retval = setval(root->left->lexeme, lv + rv); //�쥻�O�gretval + rv ������retval�٬ƻ򳣨S��
                    printf("ADD r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                    printf("MOV [%d] r%d\n", pos_cnt*4, r_cnt-1);
                    r_cnt--;
                }
                else if (strcmp(root->lexeme, "-=") == 0) {
                    retval = setval(root->left->lexeme, lv - rv);
                    printf("SUB r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                    printf("MOV [%d] r%d\n", pos_cnt*4, r_cnt-1);
                    r_cnt--;
                }
                break;
            case INCDEC:
                retval = getval(root->left->lexeme); // ���l��O�ܼ�
                pos_cnt = 0;
                for (int i=0;i<sbcount;i++){
                    if (strcmp (root->left->lexeme, table[i].name) == 0) break;
                    pos_cnt++;
                }
                //printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                printf("MOV r%d 1\n", r_cnt);


                if (strcmp(root->lexeme, "++") == 0) {
                    retval++;
                    printf("ADD r%d r%d\n", r_cnt-1, r_cnt);

                    //r_cnt--;
                }
                else if (strcmp(root->lexeme, "--") == 0) {
                    retval--;
                    printf("SUB r%d r%d\n", r_cnt-1, r_cnt);
                    //r_cnt--;
                }
                printf("MOV [%d] r%d\n", pos_cnt*4, r_cnt-1);
                r_cnt--;
                setval(root->left->lexeme, retval); // ���L���Ȧs�^�ܼ�
                //printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                break;
            case ADDSUB: //�N��O��ӨS�t �ΦP�@��
            case MULDIV:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                //int minus_cnt = 1;

                if (root->left->data == ASSIGN || root->left->data == ADDSUB_ASSIGN || root->left->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->left->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }


                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->right->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }

                //printf("%d %d<<<\n", root->left->data, root->right->data);
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                    printf("ADD r%d r%d\n", r_cnt-2, r_cnt-1); //ADD�i�H�����B��`�� ���o�ˤ���·� �����D�o�˦椣��
                    r_cnt--;
                    //if (root->left->data == INT) printf("%d ", lv);
                    //else {
                        //printf("r%d ", r_cnt-minus_cnt);
                        //minus_cnt++;
                    //}
                    //if (root->right->data == INT) printf("%d\n", rv);
                    //else {
                        //printf("r%d\n", r_cnt-minus_cnt);
                    //}
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                    printf("SUB r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--; //�᭱���ӥΤ���F �i�H�^��?
                    //printf("MOV r%d -1\n", r_cnt++);
                    //printf("MUL r%d r%d\n", r_cnt-2, r_cnt-1);
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                    printf("MUL r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if (rv == 0 && findVar(root->right) == 0)
                        error(DIVZERO); //��error�令err?
                    //retval = lv / rv; //������Ӥ��έp��?
                    retval = 0;
                    printf("DIV r%d r%d\n", r_cnt-2, r_cnt-1);
                    r_cnt--;
                }
                break;
            //==========================================
            case AND:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (root->left->data == ASSIGN || root->left->data == ADDSUB_ASSIGN || root->left->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->left->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }


                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->right->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
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
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->left->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }


                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->right->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
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
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->left->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }


                if (root->right->data == ASSIGN || root->right->data == ADDSUB_ASSIGN || root->right->data == INCDEC){
                    pos_cnt = 0;
                    for (int i=0;i<sbcount;i++){
                        if (strcmp (root->right->left->lexeme, table[i].name) == 0) break;
                        pos_cnt++;
                    }
                    printf("MOV r%d [%d]\n", r_cnt++, pos_cnt*4);
                }
                retval = lv ^ rv;
                printf("XOR r%d r%d\n", r_cnt-2, r_cnt-1);
                r_cnt--;
                break;
            //===========================================
            default:
                retval = 0;
        }
    }
    //printf("r0 = %d, r1 = %d, r2 = %d\n", table[0].val, table[1].val, table[2].val);
    //retval = 1;
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

// This is the grammar used in this package
// You can modify it according to the spec and the slide
// statement  :=  ENDFILE | END | expr END
// expr    	  :=  term expr_tail
// expr_tail  :=  ADDSUB term expr_tail | NiL
// term 	  :=  factor term_tail
// term_tail  :=  MULDIV factor term_tail| NiL
// factor	  :=  INT | ADDSUB INT |
//		   	      ID  | ADDSUB ID  |
//		   	      ID ASSIGN expr |
//		   	      LPAREN expr RPAREN |
//		   	      ADDSUB LPAREN expr RPAREN

int main() {
    initTable();
    //printf(">> ");
    while (1) {
        statement();
        //printf("r0 = %d, r1 = %d, r2 = %d\n", table[0].val, table[1].val, table[2].val);
    }
    //printf("EXIT 0\n");

    return 0;
}
