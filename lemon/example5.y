%token_type {int}  
   
%left PLUS MINUS.   
%left DIVIDE TIMES.  
   
%include {   
#include "stdio.h" 
#include "example5.h"
}  
   
%syntax_error {  
  printf("Syntax error!\n");  
}   
   
test ::= in. {printf("main\n");}
in ::= . {printf("in .\n");}
in ::= in state NEWLINE. {printf("newline\n");}



state ::= expr(A).   {  printf("Result=%i\n",A); }  
   
expr(A) ::= expr(B) MINUS  expr(C).   { A = B - C; }  
expr(A) ::= expr(B) PLUS  expr(C).   { printf("plus\n"); A = B + C; }  
expr(A) ::= expr(B) TIMES  expr(C).   { A = B * C; }  
expr(A) ::= expr(B) DIVIDE expr(C).  { 

         if(C != 0){
           A = B / C;
          }else{
           printf("divide by zero\n");
           }
}  /* end of DIVIDE */

expr(A) ::= INTEGER(B). { printf("integer\n"); A = B; } 
 