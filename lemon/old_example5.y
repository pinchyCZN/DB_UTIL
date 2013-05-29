/* Copyright (GPL) 2004 mchirico@users.sourceforge.net or mchirico@comcast.net
  Simple lemon parser  example.

  
    $ ./lemon example2.y                          

  

*/

%include {   
	#include "ex5def.h"
	#include "example5.h"
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <stdlib.h>
	#include "lexglobal.h"
	#define BUFS 1024



	void token_destructor(Token t)
	{
	  printf("In token_destructor t.value=%f\n",t.value);
	  printf("In token_destructor t.n=%u\n",t.n);
	}


}  


%token_type {Token}
%default_type {Token}
%token_destructor { token_destructor($$); }

%type expr {Token}
%type id {Token}



   
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



state ::= expr(A).   { 
                        printf("Result.value=%f\n",A.value); 
                        printf("Result.n=%u",A.n); 

                         }  



expr(A) ::= expr(B) MINUS  expr(C).   { A.value = B.value - C.value; 
                                       A.n = B.n+1  + C.n+1;
                                      }  

expr(A) ::= expr(B) PLUS  expr(C).   { A.value = B.value + C.value; 
                                       A.n = B.n+1  + C.n+1;
                                      }  

expr(A) ::= expr(B) TIMES  expr(C).   { A.value = B.value * C.value;
                                        A.n = B.n+1  + C.n+1;

                                         }  
expr(A) ::= expr(B) DIVIDE expr(C).  { 

         if(C.value != 0){
           A.value = B.value / C.value;
           A.n = B.n+1 + C.n+1;
          }else{
           printf("divide by zero\n");
           }
}  /* end of DIVIDE */
expr(A) ::= NUM(B). { A.value = B.value; A.n = B.n+1; }
