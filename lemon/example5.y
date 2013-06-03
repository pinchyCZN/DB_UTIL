%include {   
	#include "sql1.h"

	void token_destructor(){ printf("destructor\n"); }
}  
%token_type {YYSTYPE}
%default_type {YYSTYPE}
%token_destructor { token_destructor($$); 		}

%type expr {YYSTYPE}
   
%left PLUS MINUS.   
%left DIVIDE TIMES.  
   
%parse_accept {
  printf("parsing complete!\n\n\n");
}

   
%syntax_error {  
  printf("Syntax error!\n");  
}   
   
/*  This is to terminate with a new line */
main ::= in.
in ::= .
in ::= in state NEWLINE.



state ::= expr(A).   {  printf("Result.floatval=%f\n",A.floatval); 
                        printf("Result.n=%u",A.intval); 

                         }  



expr(A) ::= expr(B) DIVIDE  expr(C).   { 
					A.intval = B.intval - C.intval; 
                                      }  

expr(A) ::= SELECT(B). { printf("select\n"); A.intval = B.intval; A.intval = B.intval+1; }