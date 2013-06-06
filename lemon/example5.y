%token_type {int}  
   
%left PLUS MINUS.   
%left DIVIDE TIMES.  
   
%include {   
#include <stdio.h>
#include "example5.h"
#define TABLE_MODE 1
#define FIELD_MODE 0

	int select_mode(int table)
	{
		if(table)
			printf("table mode\n");
		else
			printf("field mode\n");
		return table;
	}
}  
   
%syntax_error {  
  printf("Syntax error!\n");  
}   
   
main ::= in. {printf("main\n");}
in ::= . {printf("step 2\n");}
in ::= in state. {printf("step 3\n");}



state ::= SELECT VARIABLE. {printf("select var\n"); select_mode(FIELD_MODE);}

state ::= VARIABLE. {printf("variable state\n");}



state ::= FROM VARIABLE . {printf("from\n"); select_mode(TABLE_MODE);}
state ::= WHERE VARIABLE. {printf("where\n"); select_mode(FIELD_MODE);}

state ::= NEWLINE. {printf("newline\n");}


   
state(A) ::= state(B) MINUS  state(C).   { A = B - C; }  
state(A) ::= state(B) PLUS  state(C).   { printf("plus\n"); A = B + C; }  
state(A) ::= state(B) TIMES  state(C).   { A = B * C; }  
state ::= INTEGER. {printf("integer\n");}
state(A) ::= state(B) DIVIDE state(C).  { 

         if(C != 0){
           A = B / C;
          }else{
           printf("divide by zero\n");
           }
		}

 