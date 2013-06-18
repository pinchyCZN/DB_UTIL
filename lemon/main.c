#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "ex5def.h"
#include "lexglobal.h"

#define BUFS 1024


/**
 * We have to declare these here - they're not  in any header files
 * we can inclde.  yyparse() is declared with an empty argument list
 * so that it is compatible with the generated C code from bison.
 *
 */

extern FILE *yyin;
typedef struct yy_buffer_state *YY_BUFFER_STATE;

#ifdef __cplusplus
extern "C" {
  int             yylex( void );
  YY_BUFFER_STATE yy_scan_string( const char * );
  void            yy_delete_buffer( YY_BUFFER_STATE );
}
#endif
int assert()
{
}

int main(int argc,char** argv)
{
  int n;
  int yv;
  char buf[BUFS+1];
  void* pParser = ParseAlloc (malloc);

  struct Token t0,t1;
  struct Token mToken;

  t0.n=0;
  t0.value=0;

  printf("%s %s\n",__TIME__,__DATE__);
  //printf("Enter an expression like 3+5 <return>\n");
  printf("  Terminate with ^D\n");

  while ( fgets(buf, BUFS,stdin )!= 0)
    {
	  char *s;
	  printf("\n");
	  strupr(buf);
	  s=strchr(buf,'\n');
	  //if(s!=0)
		//  s[0]=0;
      yy_scan_string(buf);
	  Parse (pParser, 0, 0);
      // on EOF yylex will return 0
      while( (yv=yylex()) != 0)
        { 
          printf(" yylex() %i yylval.dval %i\n",yv,yylval.dval);
          //t0.value=yylval.dval;
          Parse (pParser, yv, 0);
        }
        Parse (pParser, yv, 0);


    }

  Parse (pParser, 0, t0);
  ParseFree(pParser, free );

}
