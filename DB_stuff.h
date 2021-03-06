#define VC_EXTRALEAN

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

//#define	SQL_LONGVARCHAR		-1
//#define	SQL_BINARY			-2
//#define	SQL_VARBINARY		-3
//#define	SQL_LONGVARBINARY	-4
//#define	SQL_BIGINT			-5
//#define	SQL_TINYINT			-6
//#define	SQL_BIT				-7
#define	SQL_BLOB			-10
#define	SQL_CLOB			-11
#define	SQL_OTHER			100

int get_error_msg(SQLHANDLE handle,int handle_type,char *err,int len)
{
	SQLCHAR state[6]={0},msg[SQL_MAX_MESSAGE_LENGTH]={0};
	SQLINTEGER  error=0;
	SQLSMALLINT msglen;
	SQLGetDiagRec(handle_type,handle,1,state,&error,msg,sizeof(msg),&msglen);
	state[sizeof(state)-1]=0;
	msg[sizeof(msg)-1]=0;
	_snprintf(err,len,"%s\r\nSTATE=%s",msg,state);
	err[len-1]=0;
	return atoi(state);
}

int get_columns(DB_TREE *tree,char *table,HTREEITEM hitem)
{
	HSTMT hstmt;
	int count=0;
	if(SQLAllocStmt(tree->hdbc, &hstmt)!=SQL_SUCCESS)
		return count;
	if(SQLColumns(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,table,SQL_NTS,NULL,SQL_NTS)==SQL_SUCCESS){
		if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
			char name[256]={0};
			int len=0;
			while(!SQLGetData(hstmt,4,SQL_C_CHAR,name,sizeof(name),&len)){
				insert_item(name,hitem,IDC_COL_ITEM);
				SQLFetch(hstmt);
				count++;
				name[0]=0;
				len=0;
			}
		}
	}
	SQLFreeStmt(hstmt,SQL_CLOSE);
	return count;
}
int get_tables(DB_TREE *tree,int all)
{
	HSTMT hstmt;
	char table[256];
	long len;
	int count=0;
	int ctrl;
	const char *ttype="'TABLE'";
	ctrl=GetAsyncKeyState(VK_CONTROL)&0x8000;
	if(ctrl || all)
		ttype=0;

	if(SQLAllocStmt(tree->hdbc, &hstmt)!=SQL_SUCCESS)
		return count;
	if(SQLTables(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,NULL,SQL_NTS,ttype,SQL_NTS)!=SQL_ERROR){
		if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
			int print=TRUE;
			while(!SQLGetData(hstmt,3,SQL_C_CHAR,table,sizeof(table),&len))
			{
				HTREEITEM hitem;
				table[sizeof(table)-1]=0;
				if(all){
					char str[256]={0};
					//2=TABLE_SCHEM
					if(SQL_SUCCESS==SQLGetData(hstmt,2,SQL_C_CHAR,str,sizeof(str),&len)){
						if(str[0]!=0){
							char tmp[256]={0};
							_snprintf(tmp,sizeof(tmp),"%s.%s",str,table);
							tmp[sizeof(tmp)-1]=0;
							strncpy(table,tmp,sizeof(table));
							table[sizeof(table)-1]=0;
						}
					}
					//4=TABLE_TYPE
					else if(SQL_SUCCESS==SQLGetData(hstmt,4,SQL_C_CHAR,str,sizeof(str),&len)){
						if(str[0]!=0){
							char tmp[256]={0};
							str[sizeof(str)-1]=0;
							strlwr(str);
							if(strstr(str,"system")){
								_snprintf(tmp,sizeof(tmp),"sys.%s",table);
								tmp[sizeof(tmp)-1]=0;
								strncpy(table,tmp,sizeof(table));
								table[sizeof(table)-1]=0;
							}
						}
					}
					else if(print){
						char msg[SQL_MAX_MESSAGE_LENGTH]={0};
						get_error_msg(hstmt,SQL_HANDLE_STMT,msg,sizeof(msg));
						printf("SQLGetData err:%s\n",msg);
						print=FALSE;
					}
				}
				hitem=insert_item(table,tree->hroot,IDC_TABLE_ITEM);
				intelli_add_table(tree->name,table);
				//if(hitem!=0)
				//	get_columns(tree,table,hitem);

				SQLFetch(hstmt);
				count++;
				if(GetAsyncKeyState(VK_ESCAPE)&0x8000)
					break;
			}
		}
	}
	SQLFreeStmt(hstmt,SQL_CLOSE);
	return count;
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
			if(tree->connect_str[0]!=0)
				_snprintf(obdc_str,sizeof(obdc_str),"%s",tree->connect_str);
			else
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
					char msg[SQL_MAX_MESSAGE_LENGTH]={0};
					get_error_msg(hDbc,SQL_HANDLE_DBC,msg,sizeof(msg));
					printf("msg=%s\n",msg);
				}
				if(str[0]!=0){
					strncpy(tree->connect_str,str,sizeof(tree->connect_str));
					printf("connect str=%s\n",str);
					add_connect_str(str);
					if(tree->name[0]==0){
						strncpy(tree->name,str,sizeof(tree->name));
						tree->name[sizeof(tree->name)-1]=0;
					}
				}
				tree->hdbc=hDbc;
				tree->hdbenv=hEnv;
				return TRUE;
			}else if(result==SQL_ERROR){
				char msg[SQL_MAX_MESSAGE_LENGTH]={0};
				get_error_msg(hDbc,SQL_HANDLE_DBC,msg,sizeof(msg));
				printf("open_db error=%s\n",msg);
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
//used only for worker thread abort
int erase_sql_handles()
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		table_windows[i].hdbc=0;
		table_windows[i].hdbenv=0;
	}
	for(i=0;i<sizeof(db_tree)/sizeof(DB_TREE);i++){
		table_windows[i].hdbc=0;
		table_windows[i].hdbenv=0;
	}
	return 0;
}
int close_db(DB_TREE *tree)
{
	if(tree!=0){
		release_db(tree);
		release_tables(tree->hroot);
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
			if(str[0]==0)
				_snprintf(str,sizeof(str),"(No column name)");
			if(str[0]!=0){
				SQLINTEGER sqltype=0,sqllength=0;
				int *mem=0,width;
				win->columns++;
				width=lv_add_column(win->hlistview,str,i);
				SQLColAttribute(hstmt,i+1,SQL_DESC_TYPE,NULL,0,NULL,&sqltype);
				if(sqltype==0)
					SQLColAttribute(hstmt,i+1,2,NULL,0,NULL,&sqltype);

				SQLColAttribute(hstmt,i+1,SQL_DESC_LENGTH,NULL,0,NULL,&sqllength);
				if(sqllength==0)
					SQLColAttribute(hstmt,i+1,3,NULL,0,NULL,&sqllength);

				mem=realloc(win->col_attr,(sizeof(COL_ATTR))*win->columns);
				if(mem!=0){
					win->col_attr=mem;
					win->col_attr[win->columns-1].type=sqltype;
					win->col_attr[win->columns-1].length=sqllength;
					win->col_attr[win->columns-1].col_width=width;
				}
				intelli_add_field(win->name,win->table,str);
			}
		}
		return cols;
	}
	return cols;
}
int trim_trail_space(unsigned char *s,int slen)
{
	int i,len;
	len=strlen(s);
	if(len>slen)
		len=slen;
	for(i=len-1;i>=0;i--){
		if(s[i]<=' ')
			s[i]=0;
		else
			break;
	}
	return len;
}
int fetch_rows(SQLHSTMT hstmt,TABLE_WINDOW *win,int cols,int *aborted)
{
	SQLINTEGER rows=0;
	FILE *f=0;
	char *fname=0;
	if(fname!=0 && fname[0]!=0)
		f=fopen(fname,"wb");
	if(hstmt!=0 && win!=0){
		RECT rect={0};
		DWORD tick=GetTickCount();
		int max_width;
		int buf_size=0x20000;
		char *buf=malloc(buf_size);
		GetClientRect(win->hlistview,&rect);
		max_width=rect.right*.95;
		while(TRUE){
			int result=0;
			int i;
			if(win->abort || win->hwnd==0){
				*aborted=TRUE;
				break;
			}
			result=SQLFetch(hstmt);
			if(!(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO))
			{
				if(result==SQL_ERROR){
					char err[255]={0};
					int state;
					state=get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
					printf("fetch_rows error:%s\n",err);
					if(state!=24000) //invalid cursor state
						continue;
				}
				break;
			}
			for(i=0;i<cols;i++){
				char tmp[255]={0};
				char *str=tmp;
				int sizeof_str=sizeof(tmp);
				int len=0;
				if(win->abort || win->hwnd==0){
					*aborted=TRUE;
					break;
				}
				if(buf!=0){ //use buffer if its avail
					str=buf;
					sizeof_str=buf_size;
					str[0]=0;
				}
				result=SQLGetData(hstmt,i+1,SQL_C_CHAR,str,sizeof_str,&len);
				if(f!=0)
					fprintf(f,"%s,@@@@%s",str,i==cols-1?"\n----------------------------------------------------\n":"\n");
				if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
					extern int trim_trailing;
					char *s=str;
					int width;
					if(len==SQL_NULL_DATA)
						s="(NULL)";
					else if(trim_trailing)
						trim_trail_space(s,sizeof_str);

					if(i==0)
						lv_insert_data(win->hlistview,rows,i,s);
					else
						lv_update_data(win->hlistview,rows,i,s);
					if(len>255){
						int j;
						for(j=0;j<255;j++){
							if(str[j]=='\r' || str[j]=='\n'){
								str[j]=0;
								break;
							}
						}
					}
					width=get_str_width(win->hlistview,s)+4;
					if(max_width!=0 && width>max_width)
						width=max_width;
					if(win->col_attr!=0 && (width>win->col_attr[i].col_width))
						win->col_attr[i].col_width=width;
//Sleep(250);
				}
				else{
					char err[256]={0};
					int code;
					code=get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
					if(code==22012)
						lv_insert_data(win->hlistview,rows,i,"0");
					else
						lv_insert_data(win->hlistview,rows,i,"error retr value");
				}
			}
			rows++;
			if((GetTickCount()-tick) > 750){
				set_status_bar_text(ghstatusbar,0,"fetching results %i",rows);
				tick=GetTickCount();
			}
		}
		if(buf!=0)
			free(buf);
		{
			int i;
			if(win->col_attr!=0)
			for(i=0;i<win->columns;i++){
				ListView_SetColumnWidth(win->hlistview,i,win->col_attr[i].col_width);
			}
		}
	}
	if(f!=0)
		fclose(f);
	return rows;
}
int is_dbf(TABLE_WINDOW *win)
{
	int result=FALSE;
	if(win!=0){
		if(win->name!=0){
			if(strstri(win->name,"visual foxpro")!=0)
				result=TRUE;
			else if(strstri(win->name,"=DBF;")!=0)
				result=TRUE;
			else if(strstri(win->name,"*.DBF")!=0)
				result=TRUE;
		}
	}
	return result;
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
int get_column_type(TABLE_WINDOW *win,int col)
{
	int result=0;
	if(win!=0 && win->col_attr!=0){
		result=win->col_attr[col].type;
	}
	return result;
}
int get_col_brackets(TABLE_WINDOW *win,char *col_name,char **lbrack,char **rbrack)
{
	int DBF_TYPE=FALSE;
	if(win!=0)
		DBF_TYPE=is_dbf(win);
	if(col_name!=0 && (is_sql_reserved(col_name) || strchr(col_name,' '))){
		if(DBF_TYPE){
			*lbrack="`",*rbrack="`";
		}
		else{
			*lbrack="[",*rbrack="]";
		}
	}
	else{
		*lbrack="",*rbrack="";
	}
	return TRUE;
}
int sanitize_value(char *str,char *out,int size,int type)
{
	int result=FALSE;
	if(str!=0 && out!=0 && size>0){
		char tmp[255]={0};
		strncpy(tmp,str,sizeof(tmp));
		tmp[sizeof(tmp)-1]=0;
		if(stricmp(str,"NULL")==0 || stricmp(str,"(NULL)")==0){
			_snprintf(out,size,"%s","NULL");
		}
		else{
			switch(type){
			default:
			case SQL_UNKNOWN_TYPE:
			case SQL_NUMERIC:
			case SQL_DECIMAL:
			case SQL_INTEGER:
			case SQL_SMALLINT:
			case SQL_FLOAT:
			case SQL_REAL:
			case SQL_DOUBLE:
				_snprintf(out,size,"%s",tmp);
				break;
			case SQL_LONGVARCHAR: //-1
			case SQL_VARCHAR:
			case SQL_CHAR:
				{
					char *s=0;
					int ssize=size;
					if(ssize>0x4000)
						ssize=0x4000;
					s=malloc(ssize);
					if(s!=0 && ssize>0){
						if(strchr(str,'\'')!=0){
							int i,index,len;
							len=strlen(str);
							index=0;
							for(i=0;i<len;i++){
								char a=str[i];
								if(i==0) //surround with quote
									s[index++]='\'';

								if(a=='\''){
									s[index++]=a;
									s[index++]=a;
								}
								else
									s[index++]=a;
								if(i==len-1) //surround with quote
									s[index++]='\'';
								if(index>=ssize){
									index=ssize-1;
									break;
								}
							}
							s[index]=0;
							strncpy(out,s,size);
						}
						else{
							strncpy(s,str,ssize);
							_snprintf(out,size,"'%s'",s);
						}
						out[size-1]=0;
					}
					if(s!=0)
						free(s);
				}
				break;
			case SQL_DATETIME:
			case SQL_TYPE_TIME:
			case SQL_TYPE_DATE:
			case SQL_TYPE_TIMESTAMP:
				if(strchr(tmp,'-')){
					if(strchr(tmp,':'))
						_snprintf(out,size,"{ts'%s'}",tmp);
					else{
						if(strcmp(tmp,"1899-12-30")==0)
							_snprintf(out,size,"{dt'%s'}",tmp);
						else
							_snprintf(out,size,"{d'%s'}",tmp);
					}
				}
				else if(strchr(tmp,':')){
					_snprintf(out,size,"{t'%s'}",tmp);
				}
				else if(strstri(tmp,"now")){
					char *pre="";
					char datetime[40]={0};
					get_current_datetime(datetime,sizeof(datetime),type);
					switch(type){
					case SQL_DATETIME:
					case SQL_TYPE_TIMESTAMP:
						pre="ts";
						break;
					case SQL_TYPE_DATE:
						pre="d";
						break;
					case SQL_TYPE_TIME:
						pre="t";
					}
					_snprintf(out,size,"{%s'%s'}",pre,datetime);
				}
				else
					_snprintf(out,size,"'%s'",tmp);

				break;
			}
		}
		result=TRUE;
	}
	return result;
}
int delete_row(TABLE_WINDOW *win,int row)
{
	int result=FALSE;
	if(win!=0 && win->hlistview!=0 && win->table[0]!=0){
		char *sql=0,*tmp=0;
		int sql_size=0x10000;
		int tmp_size=0x10000;
		int col_count;
		sql=malloc(sql_size);
		tmp=malloc(tmp_size);
		col_count=lv_get_column_count(win->hlistview);
		if(sql!=0 && tmp!=0){
			int i;
			int DBF_TYPE=FALSE;
			char *lbrack="",*rbrack="";
			DBF_TYPE=is_dbf(win);

			if(is_sql_reserved(win->table) || strchr(win->table,' ')){
				if(DBF_TYPE){
					lbrack="`",rbrack="`";
				}else{
					lbrack="[",rbrack="]";
				}
			}
			sql[0]=0;
			_snprintf(sql,sql_size,"DELETE FROM %s%s%s WHERE \n",lbrack,win->table,rbrack);
			for(i=0;i<col_count;i++){
				char *v=0,*eq="=";
				char col_name[80]={0};
				lv_get_col_text(win->hlistview,i,col_name,sizeof(col_name));
				if(is_sql_reserved(col_name) || strchr(col_name,' ')){
					if(DBF_TYPE){
						lbrack="`",rbrack="`";
					}
					else{
						lbrack="[",rbrack="]";
					}
				}
				else{
					lbrack="",rbrack="";
				}
				tmp[0]=0;
				ListView_GetItemText(win->hlistview,row,i,tmp,tmp_size);
				if(stricmp(tmp,"(NULL)")==0){
					v="NULL";
					eq=" is ";
				}
				else if(tmp[0]==0)
					v="''";
				else
					v=tmp;
				sanitize_value(tmp,tmp,tmp_size,get_column_type(win,i));
				_snprintf(sql,sql_size,"%s%s%s%s%s%s%s",sql,lbrack,col_name,rbrack,eq,v,i>=col_count-1?"":" AND\r\n");
			}
			if(col_count>0){
				if(reopen_db(win)){
					mdi_create_abort(win);
					set_status_bar_text(ghstatusbar,0,"deleting row %i",row+1);
					result=execute_sql(win,sql,FALSE);
					if(result){
						ListView_DeleteItem(win->hlistview,row);
						set_status_bar_text(ghstatusbar,1,"rows affected=%i",result);
					}
					else{
						copy_str_clipboard(sql);
						set_status_bar_text(ghstatusbar,0,"FAILED to delete row %i",row+1);
					}
					mdi_destroy_abort(win);
					PostMessage(win->hwnd,WM_USER,win->hlistview,IDC_MDI_CLIENT);
				}
			}
		}
		if(sql!=0)
			free(sql);
		if(tmp!=0)
			free(tmp);

	}
	return result;
}
int get_current_datetime(char *str,int len,int sql_datetype)
{
	char time[20],date[20];
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	GetTimeFormat(LOCALE_USER_DEFAULT,NULL,&systime,"hh':'mm':'ss",time,sizeof(time));
	GetDateFormat(LOCALE_USER_DEFAULT,NULL,&systime,"yyyy'-'MM'-'dd",date,sizeof(date));
	if(str && len>0){
		switch(sql_datetype){
		case SQL_TYPE_TIMESTAMP:
		case SQL_DATETIME:
			_snprintf(str,len,"%s %s",date,time);
			break;
		case SQL_TYPE_TIME:
			_snprintf(str,len,"%s",time);
			break;
		case SQL_TYPE_DATE:
			_snprintf(str,len,"%s",date);
			break;
		}
		str[len-1]=0;
	}
	return TRUE;
}
int insert_row(TABLE_WINDOW *win,HWND hlistview)
{
	int result=FALSE;
	int FIELD_COL=0,VAL_COL=1,TYPE_COL=2;
	char *sql=0;
	char *tmp=0;
	int sql_size=0x10000,tmp_size=0x10000;
	if(win==0 || hlistview==0)
		return result;
	sql=malloc(sql_size);
	tmp=malloc(tmp_size);
	if(sql!=0 && tmp!=0){
		int i,count,inserted;
		char *lbrack="",*rbrack="";
		int DBF_TYPE=FALSE;
		tmp[0]=0;
		GetWindowText(GetParent(hlistview),tmp,tmp_size);
		if(strstri(tmp,"add row ")!=0){
			_snprintf(tmp,tmp_size,tmp+sizeof("add row ")-1);
		}
		_snprintf(sql,sql_size,"INSERT INTO %s (",tmp);
		if(tmp[0]=='`')
			DBF_TYPE=TRUE;

		count=ListView_GetItemCount(hlistview);
		inserted=0;
		for(i=0;i<count;i++){
			lbrack="",rbrack="";

			ListView_GetItemText(hlistview,i,VAL_COL,tmp,tmp_size);
			if(tmp[0]==0)
				continue;

			tmp[0]=0;
			ListView_GetItemText(hlistview,i,FIELD_COL,tmp,tmp_size);
			if(is_sql_reserved(tmp) || strchr(tmp,' ')){
				if(DBF_TYPE){
					lbrack="`",rbrack="`";
				}else{
					lbrack="[",rbrack="]";
				}
			}
			_snprintf(sql,sql_size,"%s%s%s%s%s",sql,(inserted>0)?",":"",lbrack,tmp,rbrack);
			inserted++;
		}
		
		_snprintf(sql,sql_size,"%s) VALUES (",sql);
		
		inserted=0;
		for(i=0;i<count;i++){
			char *d=0;
			char str[80]={0};
			char *lquote="",*rquote="";
			tmp[0]=0;
			ListView_GetItemText(hlistview,i,VAL_COL,tmp,tmp_size);
			if(tmp[0]==0)
				continue;
			d=tmp;
			if(stricmp(tmp,"(NULL)")==0){
				d="NULL";
			}
			else{
				ListView_GetItemText(hlistview,i,TYPE_COL,str,sizeof(str));
				if(strstri(str,"char")!=0){
					lquote=rquote="'";
				}else if(strstri(str,"datetime")!=0){
					lquote="{ts'";
					rquote="'}";
					if(strstri(tmp,"now")){
						get_current_datetime(tmp,tmp_size,SQL_DATETIME);
					}
				}else if(strstri(str,"date")!=0){
					lquote="{d'";
					rquote="'}";
					if(strstri(tmp,"now")){
						get_current_datetime(tmp,tmp_size,SQL_TYPE_DATE);
					}
				}else if(strstri(str,"time")!=0){
					lquote="{t'";
					rquote="'}";
					if(strstri(tmp,"now")){
						get_current_datetime(tmp,tmp_size,SQL_TYPE_TIME);
					}
				}
			}
			_snprintf(sql,sql_size,"%s%s%s%s%s",sql,(inserted>0)?",":"",lquote,d,rquote);
			inserted++;
		}
		if(count>0){
			_snprintf(sql,sql_size,"%s)",sql);
			sql[sql_size-1]=0;
			copy_str_clipboard(sql);
			if(reopen_db(win)){
				result=execute_sql(win,sql,FALSE);
			}
		}
	}
	if(sql!=0)
		free(sql);
	if(tmp!=0)
		free(tmp);
	return result;
}
int create_statement(TABLE_WINDOW *win,int row,char *data,char *sql,int sql_size,int create_delete)
{
	int result=FALSE;
	if(win!=0 && win->hlistview!=0 && data!=0 && sql!=0 && sql_size>0){
		char col_name[80]={0};
		char *tmp=0;
		int i,count,tmp_size=0x10000;
		count=lv_get_column_count(win->hlistview);
		lv_get_col_text(win->hlistview,win->selected_column,col_name,sizeof(col_name));
		tmp=malloc(tmp_size);
		if(count>0 && tmp!=0 && col_name[0]!=0){
			int DBF_TYPE=FALSE;
			char *lbrack="",*rbrack="";
			DBF_TYPE=is_dbf(win);
			if(is_sql_reserved(col_name) || strchr(col_name,' ')){
				if(DBF_TYPE){
					lbrack="`",rbrack="`";
				}else{
					lbrack="[",rbrack="]";
				}
			}
			sql[0]=0;
			tmp[0]=0;
			sanitize_value(data,tmp,tmp_size,get_column_type(win,win->selected_column));
			if(create_delete)
				_snprintf(sql,sql_size,"DELETE FROM [%s] WHERE\r\n",win->table);
			else
				_snprintf(sql,sql_size,"UPDATE [%s]\r\nSET %s%s%s=%s\r\nWHERE\r\n",win->table,lbrack,col_name,rbrack,tmp[0]==0?"''":tmp);
			for(i=0;i<count;i++){
				char *v=0,*eq="=";
				col_name[0]=0;
				lv_get_col_text(win->hlistview,i,col_name,sizeof(col_name));
				if(is_sql_reserved(col_name) || strchr(col_name,' ')){
					if(DBF_TYPE){
						lbrack="`",rbrack="`";
					}
					else{
						lbrack="[",rbrack="]";
					}
				}
				else{
					lbrack="",rbrack="";
				}
				tmp[0]=0;
				ListView_GetItemText(win->hlistview,row,i,tmp,tmp_size);
				if(stricmp(tmp,"(NULL)")==0){
					v="NULL";
					eq=" is ";
				}
				else if(tmp[0]==0)
					v="''";
				else
					v=tmp;
				sanitize_value(tmp,tmp,tmp_size,get_column_type(win,i));
				_snprintf(sql,sql_size,"%s%s%s%s%s%s%s",sql,lbrack,col_name,rbrack,eq,v,i>=count-1?"":" AND\r\n");
			}
			result=TRUE;
			printf("%s\n",sql);
		}
		if(tmp!=0)
			free(tmp);
	}
	return result;

}
int update_row(TABLE_WINDOW *win,int row,char *data)
{
	int result=FALSE;
	if(win!=0 && win->hlistview!=0 && win->table[0]!=0 && data!=0){
		char *sql=0;
		int sql_size=0x10000;
		sql=malloc(sql_size);
		if(sql!=0){
			sql[0]=0;
			if(create_statement(win,row,data,sql,sql_size,FALSE) && reopen_db(win)){
				mdi_create_abort(win);
				result=execute_sql(win,sql,FALSE);
				if(result)
					lv_update_data(win->hlistview,row,win->selected_column,data);
				else
					copy_str_clipboard(sql);
				mdi_destroy_abort(win);
				PostMessage(win->hwnd,WM_USER,win->hlistview,IDC_MDI_CLIENT);
			}
#ifdef _DEBUG
			copy_str_clipboard(sql);
#endif
		}
		if(sql!=0)
			free(sql);
	}
	return result;
}
int is_modify_sql(unsigned char *sql)
{
	int i,len;
	unsigned char *s=sql;
	len=strlen(sql);
	for(i=0;i<len;i++){
		if(sql[i]>' '){
			s=sql+i;
			break;
		}
	}
	if(strnicmp(s,"UPDATE",sizeof("UPDATE")-1)==0)
		return TRUE;
	else if(strnicmp(s,"INSERT",sizeof("INSERT")-1)==0)
		return TRUE;
	else if(strnicmp(s,"DELETE",sizeof("DELETE")-1)==0)
		return TRUE;
	else
		return FALSE;
}
int sql_remove_comments(unsigned char *str,int len)
{
	int i,char_found=FALSE,comment_found=FALSE;
	if(str==0 || len==0)
		return 0;
	for(i=0;i<len;i++){
		if(str[i]==0)
			break;
		else if(str[i]=='\r' || str[i]=='\n'){
			comment_found=char_found=FALSE;
			continue;
		}
		else if(comment_found)
			str[i]=' ';
		else if(!char_found){
			if(str[i]=='-' && str[i+1]=='-'){
				comment_found=TRUE;
				str[i]=' ';
			}
			else if(str[i]>' ')
				char_found=TRUE;
		}
	}
	return i;
}
int execute_sql(TABLE_WINDOW *win,char *sql,int display_results)
{
	int result=FALSE;
	if(win!=0 && win->hdbc!=0 && win->hdbenv!=0){
		int retcode;
		SQLHSTMT hstmt=0;
		SQLAllocHandle(SQL_HANDLE_STMT,win->hdbc,&hstmt);
		if(hstmt!=0){
			SetWindowText(ghstatusbar,"executing query");
			set_status_bar_text(ghstatusbar,1,"");
			retcode=SQLExecDirect(hstmt,sql,SQL_NTS);
			switch(retcode){
			case SQL_SUCCESS_WITH_INFO:
			case SQL_SUCCESS:
				{
				SQLINTEGER cols=0,total=0;
				SQLINTEGER row_count=0;
				int aborted=FALSE;
				int is_mod=is_modify_sql(sql);
				if(display_results && (!is_mod)){
					int mark;
					RECT rect={0};
					ListView_GetItemRect(win->hlistview,0,&rect,LVIR_BOUNDS);
					mark=ListView_GetSelectionMark(win->hlistview);
					SetWindowText(ghstatusbar,"clearing listview");
					mdi_clear_listview(win);
					cols=fetch_columns(hstmt,win);
					SetWindowText(ghstatusbar,"fetching results");
					total=fetch_rows(hstmt,win,cols,&aborted);
					if(mark>=0){
						RECT newrect={0};
						int dx,dy;
						ListView_SetItemState(win->hlistview,mark,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
						ListView_GetItemRect(win->hlistview,0,&newrect,LVIR_BOUNDS);
						dx=-rect.left+newrect.left;
						dy=-rect.top+newrect.top;
						ListView_Scroll(win->hlistview,dx,dy);
					}
					win->rows=total;
				}
				result=TRUE;
				SQLRowCount(hstmt,&row_count);
				if(row_count==-1)
					row_count=0;
				if(row_count>0){
					if(is_mod)
						set_status_bar_text(ghstatusbar,1,"rows affected %i",row_count);
					else
						set_status_bar_text(ghstatusbar,1,"row count %i",row_count);
				}
				set_status_bar_text(ghstatusbar,0,"returned %i rows %i cols%s",win->rows,win->columns,aborted?" (aborted)":"");
				printf("executed sql sucess\n");
				}
				break;
			case SQL_ERROR:
				{
					char msg[SQL_MAX_MESSAGE_LENGTH]={0};
					get_error_msg(hstmt,SQL_HANDLE_STMT,msg,sizeof(msg));
					printf("msg=%s\n",msg);
					SetWindowText(ghstatusbar,"error occured");
					MessageBox(win->hwnd,msg,"SQL Error",MB_OK);
				}
				break;
			case SQL_NO_DATA:
				set_status_bar_text(ghstatusbar,1,"no data returned");
				break;
			default:
				set_status_bar_text(ghstatusbar,1,"unhandled return code %i",retcode);
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
			if(!mdi_open_db(db)){
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
struct INFO{
	char *col_name;
	int col_number;
	int col_type;
};
int fetch_results_printf(HSTMT hstmt,struct INFO *info,int info_count,char *buf,int buf_size,int *col)
{
	int i;
	if(hstmt==0 || info==0 || buf==0 || col==0)
		return FALSE;
	while(SQLFetch(hstmt)==SQL_SUCCESS){
		int len;
		char str[256];
		short short_data;
		long long_data;
		for(i=0;i<info_count;i++){
			const char *delim;
			if(i==0)
				delim="";
			else
				delim="\t";
			switch(info[i].col_type){
			case SQL_C_CHAR:
				str[0]=0;
				len=0;
				SQLGetData(hstmt,info[i].col_number,info[i].col_type,str,sizeof(str),&len);
				str[sizeof(str)-1]=0;
				_snprintf(buf,buf_size,"%s%s%s",buf,delim,str);
				break;
			case SQL_C_SHORT:
				short_data=0;
				len=0;
				SQLGetData(hstmt,info[i].col_number,info[i].col_type,&short_data,sizeof(short_data),&len);
				_snprintf(buf,buf_size,"%s%s%i",buf,delim,short_data);
				break;
			case SQL_C_LONG:
				long_data=0;
				len=0;
				SQLGetData(hstmt,info[i].col_number,info[i].col_type,&long_data,sizeof(long_data),&len);
				_snprintf(buf,buf_size,"%s%s%i",buf,delim,long_data);
				break;
			default:
				_snprintf(buf,buf_size,"%s%s%i",buf,delim,*col);
				break;
			}
		}
		_snprintf(buf,buf_size,"%s\n",buf);
		*col++;
	}
	return TRUE;
}
int get_col_info(DB_TREE *tree,char *table)
{
	int result=FALSE;
	if(tree==0 || table==0 || table[0]==0)
		return result;
	if(open_db(tree)){
		HSTMT hstmt=0;
		if(SQLAllocStmt(tree->hdbc, &hstmt)==SQL_SUCCESS){
			if(SQLColumns(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,table,SQL_NTS,NULL,SQL_NTS)==SQL_SUCCESS){
				char *buf;
				int buf_size=0x10000;
				buf=malloc(buf_size);
				if(buf!=0){
					struct INFO info[]={ 
						{"field name",4,SQL_C_CHAR},
						{"type",6,SQL_C_CHAR},
						{"type #",5,SQL_C_SHORT},
						{"size",7,SQL_C_LONG},
						{"index",0,0},
						{"decimal digits",9,SQL_C_SHORT},
						{"num prec radix",10,SQL_C_SHORT},
						{"nullable",11,SQL_C_SHORT},
						{"remarks",12,SQL_C_CHAR},
						{"default val",13,SQL_C_CHAR},
						{"sql data type",14,SQL_C_SHORT},
						{"sql datetime sub",15,SQL_C_SHORT},
						{"char octect_len",16,SQL_C_LONG},
						{"ordinal position",17,SQL_C_LONG},
						{"is nullable",18,SQL_C_CHAR},
						{"catalog",1,SQL_C_CHAR},
						{"schema",2,SQL_C_CHAR},
					};
					int i,col=0;
					memset(buf,0,buf_size);
					_snprintf(buf,buf_size,"%s - column info\n",table);
					for(i=0;i<sizeof(info)/sizeof(struct INFO);i++){
						_snprintf(buf,buf_size,"%s%s%s",buf,i>0?"\t":"",info[i].col_name);
					}
					_snprintf(buf,buf_size,"%s\n",buf);
					fetch_results_printf(hstmt,&info,sizeof(info)/sizeof(struct INFO),buf,buf_size,&col);
					buf[buf_size-1]=0;
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),tree->htree,col_info_proc,buf);
					free(buf);
				}
			}else{
				char err[256]={0};
				get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
				set_status_bar_text(ghstatusbar,0,"error:SQLColumns:%s",err);
			}
		}
		if(hstmt!=0)
			SQLFreeStmt(hstmt,SQL_CLOSE);	
	}else{
		set_status_bar_text(ghstatusbar,0,"failed to open table %s",table);
	}
	return result;
}
int get_index_info(DB_TREE *tree,char *table)
{
	int result=FALSE;
	if(tree==0 || table==0 || table[0]==0)
		return result;
	if(open_db(tree)){
		HSTMT hstmt=0;
		if(SQLAllocStmt(tree->hdbc, &hstmt)==SQL_SUCCESS){
			if(SQLStatistics(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,table,SQL_NTS,SQL_INDEX_ALL,SQL_QUICK)==SQL_SUCCESS){
				char *buf;
				int buf_size=0x10000;
				buf=malloc(buf_size);
				if(buf!=0){
					struct INFO info[]={ 
						{"non unique",4,SQL_C_SHORT},
						{"index qualifier",5,SQL_C_CHAR},
						{"INDEX NAME",6,SQL_C_CHAR},
						{"type #",7,SQL_C_SHORT},
						{"position",8,SQL_C_SHORT},
						{"column name",9,SQL_C_CHAR},
						{"asc or desc",10,SQL_C_CHAR},
						{"cardinality",11,SQL_C_LONG},
						{"pages",12,SQL_C_LONG},
						{"filter condition",13,SQL_C_CHAR},
						{"catalog",1,SQL_C_CHAR},
						{"schema",2,SQL_C_CHAR},
					};
					int i,col=0;
					memset(buf,0,buf_size);
					_snprintf(buf,buf_size,"%s - index info\n",table);
					for(i=0;i<sizeof(info)/sizeof(struct INFO);i++){
						_snprintf(buf,buf_size,"%s%s%s",buf,i>0?"\t":"",info[i].col_name);
					}
					_snprintf(buf,buf_size,"%s\n",buf);
					fetch_results_printf(hstmt,&info,sizeof(info)/sizeof(struct INFO),buf,buf_size,&col);
					buf[buf_size-1]=0;
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),tree->htree,col_info_proc,buf);
					free(buf);
					result=TRUE;
				}
			}
			else{
				char err[256]={0};
				get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
				set_status_bar_text(ghstatusbar,0,"error:SQLStatistics:%s",err);
			}
		}
		if(hstmt!=0)
			SQLFreeStmt(hstmt,SQL_CLOSE);	
	}
	else{
		set_status_bar_text(ghstatusbar,0,"failed to open table %s",table);
	}
	return result;
}
int get_foreign_keys(DB_TREE *tree,char *table)
{
	int result=FALSE;
	if(tree==0 || table==0 || table[0]==0)
		return result;
	if(open_db(tree)){
		HSTMT hstmt=0;
		if(SQLAllocStmt(tree->hdbc, &hstmt)==SQL_SUCCESS){
			if(SQLForeignKeys(hstmt,
				NULL,0,
				NULL,0,
				table,SQL_NTS,
				NULL,0,
				NULL,SQL_NTS,
				NULL,SQL_NTS
				)
				==SQL_SUCCESS){
				char *buf;
				int buf_size=0x10000;
				buf=malloc(buf_size);
				if(buf!=0){
					struct INFO info[]={ 
						{"primary table",3,SQL_C_CHAR},
						{"primary col",4,SQL_C_CHAR},
						{"foreign table",7,SQL_C_CHAR},
						{"foreign col",8,SQL_C_CHAR},
						{"foreign key",12,SQL_C_CHAR},
						{"primary key",13,SQL_C_CHAR},
						{"key seq",9,SQL_C_SHORT},
						{"update rule",10,SQL_C_SHORT},
						{"delete rule",11,SQL_C_SHORT},
						{"deferrability",14,SQL_C_SHORT},
					};
					int i,col=0;
					memset(buf,0,buf_size);
					_snprintf(buf,buf_size,"%s - foreign keys info\n",table);
					for(i=0;i<sizeof(info)/sizeof(struct INFO);i++){
						_snprintf(buf,buf_size,"%s%s%s",buf,i>0?"\t":"",info[i].col_name);
					}
					_snprintf(buf,buf_size,"%s\n",buf);
					fetch_results_printf(hstmt,&info,sizeof(info)/sizeof(struct INFO),buf,buf_size,&col);
					buf[buf_size-1]=0;
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),tree->htree,col_info_proc,buf);
					free(buf);
					result=TRUE;
				}
			}
			else{
				char err[256]={0};
				get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
				set_status_bar_text(ghstatusbar,0,"error:SQLForeignKeys:%s",err);
			}
		}
		if(hstmt!=0)
			SQLFreeStmt(hstmt,SQL_CLOSE);	
	}
	else{
		set_status_bar_text(ghstatusbar,0,"failed to open table %s",table);
	}
	return result;
}
int get_table_list(DB_TREE *tree)
{
	int result=FALSE;
	if(tree==0)
		return result;
	if(open_db(tree)){
		HSTMT hstmt=0;
		if(SQLAllocStmt(tree->hdbc, &hstmt)==SQL_SUCCESS){
			char *buf=0;
			int buf_len=0x80000;
			int str_offset=0;
			buf=malloc(buf_len);
			if(buf!=0){
				{
					int count;
					count=_snprintf(buf,buf_len,"Table list:%s\n%s",tree->name,"NAME\tTYPE\tPrivileges\n");
					if(count>0){
						str_offset+=count;
					}
				}
				if(SQLTables(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,NULL,SQL_NTS,0,SQL_NTS)!=SQL_ERROR){
					if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
						HSTMT hpriv=0;
						char table[256]={0};
						int print_once=TRUE,len=0;
						SQLAllocStmt(tree->hdbc,&hpriv);
						GetAsyncKeyState(VK_ESCAPE);
						while(!SQLGetData(hstmt,3,SQL_C_CHAR,table,sizeof(table),&len))
						{
							char *priv=0;
							char ttype[80]={0};
							table[sizeof(table)-1]=0;
							SQLGetData(hstmt,4,SQL_C_CHAR,ttype,sizeof(ttype),&len);
							ttype[sizeof(ttype)-1]=0;
							if(hpriv){
								if(SQLTablePrivileges(hpriv,"",SQL_NTS,"",SQL_NTS,table,SQL_NTS)==SQL_SUCCESS)
								{
									int priv_len=0x8000;
									priv=malloc(priv_len);
									SQLFetch(hpriv);
									if(priv){
										int count=0;
										int priv_offset=0;
										char grantee[80]={0};
										char pv[80]={0};
										priv[0]=0;
										while(!SQLGetData(hpriv,5,SQL_C_CHAR,grantee,sizeof(grantee),&len)){
											int write=0;
											SQLGetData(hpriv,6,SQL_C_CHAR,pv,sizeof(pv),&len);
											if(priv_len>priv_offset)
												write=_snprintf(priv+priv_offset,priv_len-priv_offset,"%s%s=%s",count==0?"":",",grantee,pv);
											if(write>0)
												priv_offset+=write;
											count++;
											grantee[0]=0;
											pv[0]=0;
											SQLFetch(hpriv);
											if(count>100)
												break;
										}
										priv[priv_len-1]=0;
										//printf("name=%s | %s\n",table,priv);
									}
									SQLCloseCursor(hpriv);
								}
								else if(print_once){
									char msg[SQL_MAX_MESSAGE_LENGTH]={0};
									get_error_msg(hpriv,SQL_HANDLE_STMT,msg,sizeof(msg));
									printf("SQLTablePrivileges err:%s\n",msg);
									print_once=FALSE;
								}
							}
							if((buf_len-str_offset)>0){
								int count;
								count=_snprintf(buf+str_offset,buf_len-str_offset,"%s\t%s\t%s\n",table,ttype,priv==0?"":priv);
								if(count>0)
									str_offset+=count;
							}
							if(priv)
								free(priv);
							SQLFetch(hstmt);
							if(GetAsyncKeyState(VK_ESCAPE)&0x8001)
								break;
						}
						if(hpriv)
							SQLFreeStmt(hpriv,SQL_CLOSE);
					}


				}
				else{
					char err[256]={0};
					get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
					set_status_bar_text(ghstatusbar,0,"error:SQLForeignKeys:%s",err);
				}
				buf[buf_len-1]=0;
				DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),tree->htree,col_info_proc,buf);
				result=TRUE;
			}
			if(buf)
				free(buf);
		}
		if(hstmt!=0)
			SQLFreeStmt(hstmt,SQL_CLOSE);	
	}
	else{
		set_status_bar_text(ghstatusbar,0,"failed to open tree:%s",tree->name);
	}
	return result;
}

int update_tmp_string(char *buf,int buf_len,int *_str_offset,char *str,int last_col)
{
	int count,str_offset;
	str_offset=*_str_offset;
	count=_snprintf(buf+str_offset,buf_len-str_offset,"%s%c",str,last_col?'\n':'\t');
	if(count>0)
		str_offset+=count;
	*_str_offset=str_offset;
	return count;
}
int get_proc_list(DB_TREE *tree)
{
	int result=FALSE;
	if(tree==0)
		return result;
	if(open_db(tree)){
		HSTMT hstmt=0;
		if(SQLAllocStmt(tree->hdbc, &hstmt)==SQL_SUCCESS){
			char *buf=0;
			int buf_len=0x80000;
			int str_offset=0;
			buf=malloc(buf_len);
			if(buf!=0){
				{
					int count;
					count=_snprintf(buf,buf_len,"Stored Procedures list:%s\n%s",tree->name,"NAME\tNUM_INPUT_PARAMS\tNUM_OUTPUT_PARAMS"
						"\tNUM_RESULT_SETS\tREMARKS\tPROCEDURE_TYPE\tPROCEDURE_SCHEM\n");
					if(count>0){
						str_offset+=count;
					}
				}
				if(SQLProcedures(hstmt,NULL,0,NULL,0,NULL,0)!=SQL_ERROR){
					if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
						char str[256]={0};
						int len=0;
						GetAsyncKeyState(VK_ESCAPE);
						while(!SQLGetData(hstmt,3,SQL_C_CHAR,str,sizeof(str),&len))
						{
							str[sizeof(str)-1]=0;
							update_tmp_string(buf,buf_len,&str_offset,str,0);
							str[0]=0;
							SQLGetData(hstmt,4,SQL_C_CHAR,str,sizeof(str),&len);
							str[sizeof(str)-1]=0;
							update_tmp_string(buf,buf_len,&str_offset,str,0);
							str[0]=0;
							SQLGetData(hstmt,5,SQL_C_CHAR,str,sizeof(str),&len);
							str[sizeof(str)-1]=0;
							update_tmp_string(buf,buf_len,&str_offset,str,0);
							str[0]=0;
							SQLGetData(hstmt,6,SQL_C_CHAR,str,sizeof(str),&len);
							str[sizeof(str)-1]=0;
							update_tmp_string(buf,buf_len,&str_offset,str,0);
							str[0]=0;
							SQLGetData(hstmt,7,SQL_C_CHAR,str,sizeof(str),&len);
							str[sizeof(str)-1]=0;
							update_tmp_string(buf,buf_len,&str_offset,str,0);
							str[0]=0;
							SQLGetData(hstmt,8,SQL_C_CHAR,str,sizeof(str),&len);
							str[sizeof(str)-1]=0;
							update_tmp_string(buf,buf_len,&str_offset,str,0);
							str[0]=0;
							SQLGetData(hstmt,2,SQL_C_CHAR,str,sizeof(str),&len);
							str[sizeof(str)-1]=0;
							update_tmp_string(buf,buf_len,&str_offset,str,1);

							SQLFetch(hstmt);
							if(GetAsyncKeyState(VK_ESCAPE)&0x8001)
								break;
						}
					}
				}
				else{
					char err[256]={0};
					get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
					set_status_bar_text(ghstatusbar,0,"error:SQLProcedures:%s",err);
				}
				buf[buf_len-1]=0;
				DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),tree->htree,col_info_proc,buf);
				result=TRUE;
			}
			if(buf)
				free(buf);
		}
		if(hstmt!=0)
			SQLFreeStmt(hstmt,SQL_CLOSE);	
	}
	else{
		set_status_bar_text(ghstatusbar,0,"failed to open tree:%s",tree->name);
	}
	return result;
}
int intellisense_add_all(TABLE_WINDOW *win)
{
	int result=FALSE;
	if(win!=0 && win->hdbc!=0 && win->hdbenv!=0){
		int retcode;
		SQLHSTMT hstmt=0;
		SQLAllocHandle(SQL_HANDLE_STMT,win->hdbc,&hstmt);
		if(hstmt!=0){
			const char *sql="SELECT ";
			SetWindowText(ghstatusbar,"fetching all table info");
			set_status_bar_text(ghstatusbar,1,"");
			retcode=SQLExecDirect(hstmt,sql,SQL_NTS);
			switch(retcode){
			case SQL_SUCCESS_WITH_INFO:
			case SQL_SUCCESS:
				{

				SQLINTEGER row_count=0;
				result=TRUE;
				SQLRowCount(hstmt,&row_count);
				printf("executed sql sucess\n");
				}
				break;
			case SQL_ERROR:
				{
					char msg[SQL_MAX_MESSAGE_LENGTH]={0};
					get_error_msg(hstmt,SQL_HANDLE_STMT,msg,sizeof(msg));
					printf("msg=%s\n",msg);
					SetWindowText(ghstatusbar,"error occured");
					MessageBox(win->hwnd,msg,"SQL Error",MB_OK);
				}
				break;
			case SQL_NO_DATA:
				set_status_bar_text(ghstatusbar,1,"no data returned");
				break;
			default:
				set_status_bar_text(ghstatusbar,1,"unhandled return code %i",retcode);
				break;
			}
			SQLFreeStmt(hstmt,SQL_CLOSE);
		}
	}
	return result;
}
int assign_db_to_table(DB_TREE *db,TABLE_WINDOW *win,char *table)
{
	if(db!=0 && win!=0){
		win->hdbc=db->hdbc;
		win->hdbenv=db->hdbenv;
		win->hroot=db->hroot;
		strncpy(win->name,db->name,sizeof(win->name));
		set_window_text(win->hwnd,"%s - %s",table,db->name);
		return TRUE;
	}
	return FALSE;
}
