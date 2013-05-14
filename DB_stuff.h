#define VC_EXTRALEAN

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

struct DataBinding {
   SQLSMALLINT TargetType;
   SQLPOINTER TargetValuePtr;
   SQLINTEGER BufferLength;
   SQLINTEGER StrLen_or_Ind;
};
void printCatalog(const struct DataBinding* catalogResult) {
   if (catalogResult[0].StrLen_or_Ind != SQL_NULL_DATA) 
      printf("Catalog Name = %s\n", (char *)catalogResult[0].TargetValuePtr);
}
int MySQLSuccess(SQLRETURN rc) {
   return (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO);
}
int get_tables(DB_TREE *tree)
{
	HSTMT hStmt;
	char pcName[256];
	long lLen;
	int count=0;
	
	SQLAllocStmt(tree->hdbc, &hStmt);
	if (SQLTables (hStmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, (unsigned char*)"'TABLE'", SQL_NTS) != SQL_ERROR)
	{
		if (SQLFetch (hStmt) != SQL_NO_DATA_FOUND)
		{
			while (!SQLGetData (hStmt, 3, SQL_C_CHAR, pcName, 256, &lLen))
			{
				//printf("%s\n",pcName);
				insert_item(pcName,tree->hroot,IDC_TABLE_ITEM);
				SQLFetch(hStmt);
			}
		}
	}
	SQLFreeStmt (hStmt, SQL_CLOSE);
}

int extract_db_name(DB_TREE *tree)
{
	int i,len,index=0;
	char *s=0;
	s=strstr(tree->connect_str,"SourceDB=");
	if(s!=0){
		len=strlen(s);
		for(i=0;i<len;i++){
			if(s[i]==';')
				break;
			if(index>=sizeof(tree->name)-1)
				break;
			tree->name[index++]=s[i];
		}
	}
	s=strstr(tree->connect_str,"DSN=");
	if(s!=0){
		if(index>0 && index<sizeof(tree->name)-1)
			tree->name[index++]=';';
		len=strlen(s);
		for(i=0;i<len;i++){
			if(s[i]==';')
				break;
			if(index>=sizeof(tree->name)-1)
				break;
			tree->name[index++]=s[i];
		}
	}
	if(s!=0){
		tree->name[index]=0;
		if(index>=0)
			return TRUE;
	}
	return FALSE;
}
int open_db(DB_TREE *tree)
{
    SQLHENV     hEnv = NULL;
    SQLHDBC     hDbc = NULL;
	if(tree==0)
		return FALSE;
	if(tree->hdbc!=0 || tree->hdbenv!=0)
		return TRUE;
    if(SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv)==SQL_ERROR)
		return FALSE;
	if(SQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0)==SQL_SUCCESS){
		if(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)==SQL_SUCCESS){
			char str[1024]={0};
			char obdc_str[1024]={0};
			int result=0;
			_snprintf(obdc_str,sizeof(obdc_str),"%s",tree->name);
			result=SQLDriverConnect(hDbc,
							 GetDesktopWindow(),
							 (SQLCHAR*)obdc_str, //"ODBC;",
							 SQL_NTS,
							 (SQLCHAR*)str,
							 sizeof(str),
							 NULL,
							 SQL_DRIVER_COMPLETE);
			if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
				if(result==SQL_SUCCESS_WITH_INFO){
					SQLCHAR state[6],msg[SQL_MAX_MESSAGE_LENGTH];
					SQLINTEGER  error;
					SQLSMALLINT msglen;
					SQLGetDiagRec(SQL_HANDLE_DBC,hDbc,1,state,&error,msg,sizeof(msg),&msglen);
					printf("msg=%s\n",msg);
				}
				if(str[0]!=0){
					strncpy(tree->connect_str,str,sizeof(tree->connect_str));
					extract_db_name(tree);
					printf("connect str=%s\n",str);
					write_ini_str("DATABASES","1",str);
				}
				tree->hdbc=hDbc;
				tree->hdbenv=hEnv;
				return TRUE;
			}
		}
	}

    if (hDbc)
    {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }

    if (hEnv)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
	return FALSE;
}
int reassign_tables(DB_TREE *tree)
{
	int i;
	if(tree==0)
		return FALSE;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd!=0 && stricmp(table_windows[i].name,tree->name)==0){
			table_windows[i].hroot=tree->hroot;
			table_windows[i].hdbc=tree->hdbc;
			table_windows[i].hdbenv=tree->hdbenv;
		}
	}
	return TRUE;

}
int release_tables(HWND hroot)
{
	int i;
	if(hroot==0)
		return FALSE;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hroot==hroot){
			table_windows[i].hdbc=0;
			table_windows[i].hdbenv=0;
		}
	}
	return TRUE;
}
int release_db(DB_TREE *tree)
{
	if(tree!=0){
		if(tree->hdbc!=0){
			SQLDisconnect(tree->hdbc);
			SQLFreeHandle(SQL_HANDLE_DBC,tree->hdbc);
		}
		if(tree->hdbenv!=0)
			SQLFreeHandle(SQL_HANDLE_ENV,tree->hdbenv);
		tree->hdbc=0;
		tree->hdbenv=0;
		return TRUE;
	}
	return FALSE;
}
int close_db(DB_TREE *tree)
{
	if(tree!=0){
		release_db(tree);
		release_tables(tree->hroot,FALSE);
		return TRUE;
	}
	return FALSE;
}

int fetch_columns(SQLHSTMT hstmt,TABLE_WINDOW *win)
{
	SQLSMALLINT i,cols=0; 
	if(hstmt!=0 && win!=0){
		SQLNumResultCols(hstmt,&cols);
		win->columns=0;
		for(i=0;i<cols;i++){
			char str[255]={0};
			SQLColAttribute(hstmt,i+1,SQL_DESC_NAME,str,sizeof(str),NULL,NULL);
			if(str[0]!=0){
				win->columns++;
				lv_add_column(win->hlistview,str,i);
			}
		}
		return cols;
	}
	return cols;
}
int fetch_rows(SQLHSTMT hstmt,TABLE_WINDOW *win,int cols)
{
	SQLINTEGER rows=0;
	if(hstmt!=0){
		while(TRUE){
			int result=0;
			int i,len;
			result=SQLFetch(hstmt);
			if(!(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO))
				break;
			for(i=0;i<cols;i++){
				char str[1024]={0};
				int len=0;
				result=SQLGetData(hstmt,i+1,SQL_C_CHAR,str,sizeof(str),&len);
				if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
					lv_insert_data(win->hlistview,rows,i,str);
//Sleep(250);
				}
				else
					break;
				if(win->abort || win->hwnd==0)
					break;

			}
			rows++;
			if(win->abort || win->hwnd==0)
				break;
		}
	}
	return rows;
}
int execute_sql(TABLE_WINDOW *win,char *sql)
{
	int result=FALSE;
	if(win!=0 && win->hdbc!=0 && win->hdbenv!=0){
		int retcode;
		SQLHSTMT hstmt=0;
		SQLAllocHandle(SQL_HANDLE_STMT,win->hdbc,&hstmt);
		retcode=SQLExecDirect(hstmt,sql,SQL_NTS);
		switch(retcode){
        case SQL_SUCCESS_WITH_INFO:
        case SQL_SUCCESS:
			{
			SQLINTEGER rows=0,cols=0;
			SQLRowCount(hstmt,&rows);
			mdi_clear_listview(win);
			cols=fetch_columns(hstmt,win);
			fetch_rows(hstmt,win,cols);
			result=TRUE;
			}
			break;
        case SQL_ERROR:
			break;
		}
		SQLFreeStmt(hstmt,SQL_CLOSE);
	}
	return result;
}
int reopen_db(TABLE_WINDOW *win)
{
	if(win!=0){
		if(win->hdbc==0 && win->hdbenv==0){
			DB_TREE *db=0;
			acquire_db_tree(win->name,&db);
			if(!mdi_open_db(db,FALSE)){
				char str[80];
				mdi_remove_db(db);
				_snprintf(str,sizeof(str),"Cant open %s",win->name);
				MessageBox(ghmainframe,str,"OPEN DB FAIL",MB_OK);
				return FALSE;
			}
			else{
				reassign_tables(db);
				return TRUE;
			}

		}
		else
			return TRUE;
	}
	return FALSE;
}
int get_db_info(void *db,void **info)
{

//SQLTables

	return 0;
}
int assign_db_to_table(DB_TREE *db,TABLE_WINDOW *win)
{
	if(db!=0 && win!=0){
		win->hdbc=db->hdbc;
		win->hdbenv=db->hdbenv;
		win->hroot=db->hroot;
		strncpy(win->name,db->name,sizeof(win->name));
		SetWindowText(win->hwnd,db->name);
		return TRUE;
	}
	return FALSE;
}
