%{
#include "lexglobal.h" 
#include "example5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int line = 1, col = 1;

%}

%%


[0-9]+|[0-9]*\.[0-9]+    {                      col += (int) strlen(yytext); 
                                                yylval.dval = atof(yytext); 
                                                return NUMBER; }
[ \t]   { col += (int) strlen(yytext); }               /* ignore but count white space */

"SELECT"	{return SELECT;}
"FROM"		{return FROM;}
"WHERE"		{return WHERE;}




[A-Za-z\[\*][A-Za-z0-9\]]*   {return VARIABLE;}

\n      { col = 0; ++line; return NEWLINE; }

[.]+       { col += (int) strlen(yytext); return JUNK;
			return yytext[0]; }


%%
/**
 * reset the line and column count
 *
 *
 */
void reset_lexer(void)
{

  line = 1;
  col  = 1;

}


void yyerror(char *s)
{
  printf("error: %s at line: %d col: %d\n",s,line,col);

}

int yywrap(void)
{
  return 1;
}