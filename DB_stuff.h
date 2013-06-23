#define VC_EXTRALEAN

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

int get_error_msg(SQLHANDLE handle,int handle_type,char *err,int len)
{
	SQLCHAR state[6]={0},msg[SQL_MAX_MESSAGE_LENGTH]={0};
	SQLINTEGER  error;
	SQLSMALLINT msglen;
	SQLGetDiagRec(handle_type,handle,1,state,&error,msg,sizeof(msg),&msglen);
	_snprintf(err,len,"%s %s",msg,state);
	return TRUE;
}

int get_columns(DB_TREE *tree,char *table,HTREEITEM hitem)
{
	HSTMT hstmt;
	int count=0;
	if(SQLAllocStmt(tree->hdbc, &hstmt)!=SQL_SUCCESS)
		return count;
//	printf("table=%s\n",table);
	if(SQLColumns(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,table,SQL_NTS,NULL,SQL_NTS)==SQL_SUCCESS){
		if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
			char name[256]={0};
			int len=0;
			while(!SQLGetData(hstmt,4,SQL_C_CHAR,name,sizeof(name),&len)){
//				printf("cole %i = %s\n",count,name);
				
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
int get_tables(DB_TREE *tree)
{
	HSTMT hstmt;
	char table[256];
	long len;
	int count=0;
	
	if(SQLAllocStmt(tree->hdbc, &hstmt)!=SQL_SUCCESS)
		return count;
	if(SQLTables(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,NULL,SQL_NTS,"'TABLE'",SQL_NTS)!=SQL_ERROR){
		if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
			HSTMT hpriv=0;
			SQLAllocStmt(tree->hdbc,&hpriv);
			while(!SQLGetData(hstmt,3,SQL_C_CHAR,table,sizeof(table),&len))
			{
				HTREEITEM hitem;
				table[sizeof(table)-1]=0;
				/*
				if(SQLTablePrivileges(hpriv,"",SQL_NTS,"",SQL_NTS,table,SQL_NTS)==SQL_SUCCESS)
				{
					char str[256]={0};
					SQLFetch(hpriv);
					SQLGetData(hpriv,6,SQL_C_CHAR,str,sizeof(str),&len);
					SQLCloseCursor(hpriv);
					printf("name=%s | %s\n",table,str);
				}
				*/
				hitem=insert_item(table,tree->hroot,IDC_TABLE_ITEM);
				if(hitem!=0)
					get_columns(tree,table,hitem);

				SQLFetch(hstmt);
				count++;
			}
			SQLFreeStmt(hpriv,SQL_CLOSE);
		}
	}
	SQLFreeStmt(hstmt,SQL_CLOSE);
	return count;
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
		release_tables(tree->hroot);
		return TRUE;
	}
	return FALSE;
}
int select_table(DB_TREE *tree,char *table)
{
	int result=FALSE;
	if(tree!=0 && tree->htree!=0 && tree->hroot!=0){
		HTREEITEM hchild=TreeView_GetChild(tree->htree,tree->hroot);
		while(hchild!=0){
			char str[80]={0};
			tree_get_item_text(hchild,str,sizeof(str));
			if(stricmp(str,table)==0){
				result=TreeView_SelectItem(tree->htree,hchild);
				break;
			}
			hchild=TreeView_GetNextSibling(tree->htree,hchild);
		}
	}
	return result;
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
				SQLINTEGER sqltype=0,sqllength=0;
				int *mem=0,width;
				win->columns++;
				width=lv_add_column(win->hlistview,str,i);
				SQLColAttribute(hstmt,i+1,SQL_DESC_TYPE,NULL,0,NULL,&sqltype);

				{
					char err[256]={0};
					char str[256]={0};
				SQLColAttribute(hstmt,i+1,2,str,sizeof(str),NULL,&sqltype);
				get_error_msg(hstmt,SQL_HANDLE_STMT,err,sizeof(err));
				}
				SQLColAttribute(hstmt,i+1,SQL_DESC_LENGTH,NULL,0,NULL,&sqllength);
				mem=realloc(win->col_attr,(sizeof(COL_ATTR))*win->columns);
				if(mem!=0){
					win->col_attr=mem;
					win->col_attr[win->columns-1].type=sqltype;
					win->col_attr[win->columns-1].length=sqllength;
					win->col_attr[win->columns-1].col_width=width;
				}
			}
		}
		return cols;
	}
	return cols;
}
int fetch_rows(SQLHSTMT hstmt,TABLE_WINDOW *win,int cols)
{
	SQLINTEGER rows=0;
	FILE *f=0;
	//f=fopen("c:\\temp\\sql_data.txt","wb");
	if(hstmt!=0 && win!=0){
		while(TRUE){
			int result=0;
			int i;
			if(win->abort || win->hwnd==0)
				break;
			result=SQLFetch(hstmt);
			if(!(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO))
				break;
			for(i=0;i<cols;i++){
				static char str[0x10000]={0};
				int len=0;
				if(win->abort || win->hwnd==0)
					break;
				result=SQLGetData(hstmt,i+1,SQL_C_CHAR,str,sizeof(str),&len);
				if(f!=0)
					fprintf(f,"%s%s",str,i==cols-1?"\n----------------------------------------------------\n":",@@@@\n");
				if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
					char *s=str;
					int width;
					if(len==SQL_NULL_DATA)
						s="(NULL)";
					if(i==0)
						lv_insert_data(win->hlistview,rows,i,s);
					else
						lv_update_data(win->hlistview,rows,i,s);
					width=get_str_width(win->hlistview,s)+4;
					if(win->col_attr!=0 && (width>win->col_attr[i].col_width))
						win->col_attr[i].col_width=width;
//Sleep(250);
				}
				else
					break;
			}
			rows++;
		}
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
			case SQL_VARCHAR:
			case SQL_CHAR:
				_snprintf(out,size,"'%s'",tmp);
				break;
			case SQL_DATETIME:
			case SQL_TYPE_DATE:
				if(strchr(tmp,'-')){
					if(strchr(tmp,':'))
						_snprintf(out,size,"{ts'%s'}",tmp);
					else
						_snprintf(out,size,"{d'%s'}",tmp);
				}
				else
					_snprintf(out,size,"'%s'",tmp);

				break;
			case SQL_TYPE_TIME:
				_snprintf(out,size,"{t'%s'}",tmp);
				break;
			}
		}
		result=TRUE;
	}
	return result;
}
int update_row(TABLE_WINDOW *win,int row,char *data)
{
	int result=FALSE;
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
			sanitize_value(data,cdata,sizeof(cdata),get_column_type(win,win->selected_column));
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
				sanitize_value(tmp,tmp,sizeof(tmp),get_column_type(win,i));
				_snprintf(sql,sql_size,"%s%s%s%s%s%s%s",sql,lbrack,col_name,rbrack,eq,v,i>=count-1?"":" AND\r\n");
			}
			printf("%s\n",sql);
			if(reopen_db(win)){
				mdi_create_abort(win);
				if(execute_sql(win,sql,FALSE)){
					lv_update_data(win->hlistview,row,win->selected_column,data);
					result=TRUE;
				}
				mdi_destroy_abort(win);
			}
			//SetWindowText(win->hedit,sql);
		}
		if(sql!=0)
			free(sql);
	}
	return result;
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
			retcode=SQLExecDirect(hstmt,sql,SQL_NTS);
			switch(retcode){
			case SQL_SUCCESS_WITH_INFO:
			case SQL_SUCCESS:
				{
				SQLINTEGER rows=0,cols=0,total=0;
				SQLRowCount(hstmt,&rows);
				if(display_results){
					int mark;
					int horz;
					horz=GetScrollPos(win->hlistview,SB_HORZ);
					mark=ListView_GetSelectionMark(win->hlistview);
					SetWindowText(ghstatusbar,"clearing listview");
					mdi_clear_listview(win);
					cols=fetch_columns(hstmt,win);
					SetWindowText(ghstatusbar,"fetching results");
					total=fetch_rows(hstmt,win,cols);
					if(mark>=0){
						ListView_SetItemState(win->hlistview,mark,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
						ListView_EnsureVisible(win->hlistview,mark,FALSE);
						ListView_Scroll(win->hlistview,horz,0);
					}
					win->rows=total;
				}
				result=TRUE;
				if(total==rows)
					set_status_bar_text(ghstatusbar,0,"returned %i rows %i cols",win->rows,win->columns);
				else
					set_status_bar_text(ghstatusbar,0,"returned %i of %i rows, %i cols",total,rows,win->columns);
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
				SetWindowText(ghstatusbar,"no data returned");
				break;
			default:
				set_status_bar_text(ghstatusbar,0,"unhandled return code %i",retcode);
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
int get_col_info(DB_TREE *tree,char *table)
{
	if(tree==0)
		return FALSE;
	if(open_db(tree)){
		HSTMT hstmt;
		int count=0;
		if(SQLAllocStmt(tree->hdbc, &hstmt)==SQL_SUCCESS){
			if(SQLColumns(hstmt,NULL,SQL_NTS,NULL,SQL_NTS,table,SQL_NTS,NULL,SQL_NTS)==SQL_SUCCESS){
				if(SQLFetch(hstmt)!=SQL_NO_DATA_FOUND){
					char name[256]={0};
					int len=0;
					while(!SQLGetData(hstmt,4,SQL_C_CHAR,name,sizeof(name),&len)){
						printf("cole %i = %s\n",count,name);
						
						SQLFetch(hstmt);
						count++;
						name[0]=0;
						len=0;
					}
				}
			}
		}
		SQLFreeStmt(hstmt,SQL_CLOSE);	
	}
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
	char *section="DATABASES";
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