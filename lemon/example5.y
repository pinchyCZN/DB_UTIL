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

expr ::= expr.