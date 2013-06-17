%token_type {int}  
   
%left PLUS MINUS.   
%left DIVIDE TIMES.  
   
%include {   
#include <stdio.h>
#include "example5.h"
#define TABLE_MODE 1
#define FIELD_MODE 0
#define NO_MODE -1

int intel_mode=0;

	int get_sql_mode()
	{
		return intel_mode;
	}
	int set_sql_mode(int mode)
	{
		intel_mode=mode;
		switch(mode){
		case TABLE_MODE:
			printf("table mode\n");
			break;
		case FIELD_MODE:
			printf("field mode\n");
			break;
		case NO_MODE:
		default:
			intel_mode=-1;
			printf("NO mode\n");
			break;
		}
		return mode;
	}
}  
   
%syntax_error {  
	printf("Syntax error!\n");
	set_sql_mode(NO_MODE);
}   
   
main ::= in. {printf("main\n");}
in ::= . {printf("step 2\n");}
in ::= in state. {printf("step 3\n");}



state ::= SELECT VARIABLE. {printf("--select\n"); set_sql_mode(FIELD_MODE);}

state ::= VARIABLE. {printf("--variable\n");}

state ::= NUMBER. {printf("--number\n");}

state ::= JUNK. {printf("--junk\n");}


state ::= FROM VARIABLE . {printf("--from\n"); set_sql_mode(TABLE_MODE);}
state ::= WHERE VARIABLE. {printf("--where\n"); set_sql_mode(FIELD_MODE);}

state ::= NEWLINE. {printf("--newline\n");}

