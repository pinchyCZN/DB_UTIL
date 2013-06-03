%include {   
	#include "sql1.h"
#define YYNOERRORRECOVERY 1

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
main ::= in. {printf("main\n");}
in ::= in cmd. {printf("in .\n");}
cmd ::= SELECT NEWLINE. {printf("newline\n");}


/*
state ::= SELECT FROM.   { printf("made it\n");

                         }  

*/
/*
expr(A) ::= expr(B) DIVIDE  expr(C).   { printf("divide\n");
					A.intval = B.intval - C.intval; 
                                      }  

expr(A) ::= SELECT FROM in. { printf("select from %i\n",A.intval);}

expr ::= SELECT in. { printf("select\n");}

*/