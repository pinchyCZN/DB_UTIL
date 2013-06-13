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
					SQLCHAR state[6],msg[SQL_MAX_MESSAGE_LENGTH]={0};
					SQLINTEGER  error;
					SQLSMALLINT msglen;
					SQLGetDiagRec(SQL_HANDLE_DBC,hDbc,1,state,&error,msg,sizeof(msg),&msglen);
					printf("msg=%s\n",msg);
				}
				if(str[0]!=0){
					strncpy(tree->connect_str,str,sizeof(tree->connect_str));
					extract_db_name(tree);
					printf("connect str=%s\n",str);
					save_connect_str(str);
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
			if(win->abort || win->hwnd==0)
				break;
			result=SQLFetch(hstmt);
			if(!(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO))
				break;
			for(i=0;i<cols;i++){
				char str[1024]={0};
				int len=0;
				if(win->abort || win->hwnd==0)
					break;
				result=SQLGetData(hstmt,i+1,SQL_C_CHAR,str,sizeof(str),&len);
				if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
					char *s=str;
					if(len==SQL_NULL_DATA)
						s="(NULL)";
					if(i==0)
						lv_insert_data(win->hlistview,rows,i,s);
					else
						lv_update_data(win->hlistview,rows,i,s);
//Sleep(250);
				}
				else
					break;
			}
			rows++;
		}
	}
	return rows;
}
int is_sql_reserved(const char *sql)
{
	extern const char *sql_reserved_words[];
	int i,result=FALSE;
	for(i=0;i<1000;i++){
		if(sql_reserved_words[i]==0)
			break;
		if(stricmp(sql_reserved_words[i],sql)==0)
			return TRUE;
	}
	return result;
}
int sanitize_value(char *str,char *out,int size)
{
	int result=FALSE;
	if(str!=0 && out!=0 && size>0){
		char tmp[255]={0};
		int i,len,quote=FALSE;
		strncpy(tmp,str,sizeof(tmp));
		tmp[sizeof(tmp)-1]=0;
		len=strlen(str);
		if(stricmp(str,"NULL")==0 || stricmp(str,"(NULL)")==0){
			_snprintf(out,size,"%s","NULL");
		}
		else{
			if(strchr(str,'-') || strchr(str,':')){
				enum{none,d,t,ts};
				int found=none;
				for(i=0;i<len;i++){
					if(i>=0 && i<=3)
						if(!isdigit(str[i]))
							break;
					if(i==4 && str[i]!='-')
							break;
					if(i>=5 && i<=6)
						if(!isdigit(str[i]))
							break;
					if(i==7 && str[i]!='-')
							break;
					if(i>=8 && i<=9)
						if(!isdigit(str[i]))
							break;
					if(i==9)
						if(i==len-1){
							found=d;
							break;
						}
					if(i==10)
						if(isspace(str[i]))
							continue;
					if(i>=11 && i<=12)
						if(!isdigit(str[i]))
							break;
					if(i==13 && str[i]!=':')
							break;
					if(i>=14 && i<=15)
						if(!isdigit(str[i]))
							break;
					if(i==16 && str[i]!=':')
							break;
					if(i>=17 && i<=18)
						if(!isdigit(str[i]))
							break;
					if(i>=18){
						found=ts;
						break;
					}
				}
				if(found==none){
					for(i=0;i<len;i++){
						if(i>=0 && i<=1)
							if(!isdigit(str[i]))
								break;
						if(i==2 && str[i]!=':')
								break;
						if(i>=3 && i<=4)
							if(!isdigit(str[i]))
								break;
						if(i==5 && str[i]!=':')
								break;
						if(i>=6 && i<=7)
							if(!isdigit(str[i]))
								break;
						if(i==7 && (i==len-1)){
							found=t;
							break;
						}
						if(i==8)
							if(str[i]=='.'){
								found=t;
								break;
							}
							else
								break;
					}
				}
				if(found!=none){
					_snprintf(out,size,"{%s'%s'}",
							found==ts?"ts":
							found==t?"t":"d",
							tmp);
					return TRUE;
				}

			}
			for(i=0;i<len;i++){
				if(isalpha(str[i]))
					quote=TRUE;
				else if(ispunct(str[i]))
					quote=TRUE;
				else if(str[i]==' ')
					quote=TRUE;
			}
			if(quote){
				_snprintf(out,size,"'%s'",tmp);
			}
			else
				_snprintf(out,size,"%s",tmp);
		}
		result=TRUE;
	}
	return result;
}
int update_row(TABLE_WINDOW *win,int row,char *data)
{
	if(win!=0 && win->hlistview!=0 && win->table[0]!=0 && data!=0){
		char *sql=0;
		char col_name[80]={0};
		int i,count,sql_size=0x10000;
		count=lv_get_column_count(win->hlistview);
		lv_get_col_text(win->hlistview,win->selected_column,col_name,sizeof(col_name));
		sql=malloc(sql_size);
		if(count>0 && sql!=0 && col_name[0]!=0){
			char cdata[80]={0};
			char *lbrack="",*rbrack="";
			if(is_sql_reserved(col_name) || strchr(col_name,' ')){
				lbrack="[",rbrack="]";
			}
			sql[0]=0;
			sanitize_value(data,cdata,sizeof(cdata));
			_snprintf(sql,sql_size,"UPDATE [%s] SET %s%s%s=%s WHERE ",win->table,lbrack,col_name,rbrack,cdata[0]==0?"''":cdata);
			for(i=0;i<count;i++){
				char tmp[128]={0};
				char *v=0,*eq="=";
				col_name[0]=0;
				lv_get_col_text(win->hlistview,i,col_name,sizeof(col_name));
				if(is_sql_reserved(col_name) || strchr(col_name,' ')){
					lbrack="[",rbrack="]";
				}
				else{
					lbrack="",rbrack="";
				}
				ListView_GetItemText(win->hlistview,row,i,tmp,sizeof(tmp));
				if(stricmp(tmp,"(NULL)")==0){
					v="NULL";
					eq=" is ";
				}
				else if(tmp[0]==0)
					v="''";
				else
					v=tmp;
				sanitize_value(tmp,tmp,sizeof(tmp));
				_snprintf(sql,sql_size,"%s%s%s%s%s%s%s",sql,lbrack,col_name,rbrack,eq,v,i>=count-1?"":" AND\r\n");
			}
			printf("%s\n",sql);
			if(reopen_db(win)){
				mdi_create_abort(win);
				if(execute_sql(win,sql,FALSE)){
					lv_update_data(win->hlistview,row,win->selected_column,data);
				}
				mdi_destroy_abort(win);
			}
			//SetWindowText(win->hedit,sql);
		}
		if(sql!=0)
			free(sql);
	}
}
int execute_sql(TABLE_WINDOW *win,char *sql,int display_results)
{
	int result=FALSE;
	if(win!=0 && win->hdbc!=0 && win->hdbenv!=0){
		int retcode;
		SQLHSTMT hstmt=0;
		SQLAllocHandle(SQL_HANDLE_STMT,win->hdbc,&hstmt);
		if(hstmt!=0){
			char msg[80];
			SetWindowText(ghstatusbar,"executing query");
			retcode=SQLExecDirect(hstmt,sql,SQL_NTS);
			switch(retcode){
			case SQL_SUCCESS_WITH_INFO:
			case SQL_SUCCESS:
				{
				SQLINTEGER rows=0,cols=0,total=0;
				SQLRowCount(hstmt,&rows);
				if(display_results){
					int mark;
					mark=ListView_GetSelectionMark(win->hlistview);
					SetWindowText(ghstatusbar,"clearing listview");
					mdi_clear_listview(win);
					cols=fetch_columns(hstmt,win);
					SetWindowText(ghstatusbar,"fetching results");
					total=fetch_rows(hstmt,win,cols);
					if(mark>=0){
						ListView_SetItemState(win->hlistview,mark,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
						ListView_EnsureVisible(win->hlistview,mark,FALSE);
					}
					win->rows=total;
				}
				result=TRUE;
				if(total==rows)
					_snprintf(msg,sizeof(msg),"returned %i rows",rows);
				else
					_snprintf(msg,sizeof(msg),"returned %i of %i rows",total,rows);
				SetWindowText(ghstatusbar,msg);
				printf("executed sql sucess\n");
				}
				break;
			case SQL_ERROR:
				{
					SQLCHAR state[6],msg[SQL_MAX_MESSAGE_LENGTH]={0};
					SQLINTEGER  error;
					SQLSMALLINT msglen;
					SQLGetDiagRec(SQL_HANDLE_STMT,hstmt,1,state,&error,msg,sizeof(msg),&msglen);
					printf("msg=%s\n",msg);
					SetWindowText(ghstatusbar,"error occured");
					MessageBox(win->hwnd,msg,"SQL Error",MB_OK);
				}
				break;
			case SQL_NO_DATA:
				SetWindowText(ghstatusbar,"no data returned");
				break;
			default:
				_snprintf(msg,sizeof(msg),"unhandled return code %i",retcode);
				SetWindowText(ghstatusbar,msg);
				break;
			}
			SQLFreeStmt(hstmt,SQL_CLOSE);
		}
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

int get_ini_entry(char *section,int num,char *str,int len)
{
	char key[20];
	_snprintf(key,sizeof(key),"ENTRY%i",num);
	return get_ini_str(section,key,str,len);
}
int set_ini_entry(char *section,int num,char *str)
{
	char key[20];
	_snprintf(key,sizeof(key),"ENTRY%i",num);
	if(str==NULL)
		return delete_ini_key(section,key);
	return write_ini_str(section,key,str);
}
int save_connect_str(char *connect_str)
{
	int i;
	int found=-1;
	const char *section="DATABASES";
	if(connect_str==0 || connect_str[0]==0)
		return FALSE;
	for(i=0;i<100;i++){
		char str[1024]={0};
		get_ini_entry(section,i,str,sizeof(str));
		if(str[0]!=0){
			if(stricmp(connect_str,str)==0){
				found=i;
				if(i>0){
					set_ini_entry(section,i,NULL);
				}
				break;
			}
		}
	}
	if(found!=0){
		for(i=100-1;i>=0;i--){
			char str[1024]={0};
			get_ini_entry(section,i,str,sizeof(str));
			set_ini_entry(section,i+1,str);
		}
	}
	if(found!=0)
		set_ini_entry(section,0,connect_str);
	return TRUE;
}