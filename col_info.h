
int find_win_by_hlistview(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	static TABLE_WINDOW *old_win=0;
	if(old_win!=0 && old_win->hlistview==hwnd){
		*win=old_win;
		return TRUE;
	}
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hlistview==hwnd){
			*win=&table_windows[i];
			old_win=*win;
			return TRUE;
		}
	}
	old_win=0;
	return FALSE;
}
static const char *SQL_TYPES[]={
	"SQL_UNKNOWN_TYPE",0,
	"SQL_CHAR",1,
	"SQL_NUMERIC",2,
	"SQL_DECIMAL",3,
	"SQL_INTEGER",4,
	"SQL_SMALLINT",5,
	"SQL_FLOAT",6,
	"SQL_REAL",7,
	"SQL_DOUBLE",8,
	"SQL_DATETIME",9,
	"SQL_VARCHAR",12,
	"SQL_TYPE_DATE",91,
	"SQL_TYPE_TIME",92,
	"SQL_TYPE_TIMESTAMP",93
};
static int find_sql_type_str(int type,char **str)
{
	int i;
	for(i=0;i<sizeof(SQL_TYPES)/sizeof(char *);i+=2){
		if(SQL_TYPES[i+1]==type){
			*str=SQL_TYPES[i];
			return TRUE;
		}
	}
	return FALSE;
}
LRESULT CALLBACK col_info_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("ci");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_INITDIALOG:
		{
			static TABLE_WINDOW *win=0;
			win=0;
			if(find_win_by_hlistview(lparam,&win)){
				int i;
				for(i=0;i<win->columns;i++){
					char str[255]={0};
					char name[80]={0};
					char *sql_name="";
					lv_get_col_text(win->hlistview,i,name,sizeof(name));
					find_sql_type_str(win->col_attr[i].type,&sql_name);
					_snprintf(str,sizeof(str),"%2i-%-10s attr=%2i (%s) length=%3i col_width=%3i",
						i,name,win->col_attr[i].type,sql_name,win->col_attr[i].length,win->col_attr[i].col_width);
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
				}
				if(win->table[0]!=0)
					SetWindowText(hwnd,win->table);
			}
		}
		SetFocus(GetDlgItem(hwnd,IDC_LIST1));
		SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
		grippy=create_grippy(hwnd);
		resize_col_info(hwnd);
		break;
	case WM_SIZE:
		resize_col_info(hwnd);
		grippy_move(hwnd,grippy);
		break;
	case WM_VKEYTOITEM:
		switch(LOWORD(wparam)){
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000){
				SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGE,TRUE,MAKELPARAM(0,-1));
			}
			break;
		case 'C':
			{
				int i,count,buf_size=0x10000;
				char *buf;
				if(!(GetKeyState(VK_CONTROL)&0x8000))
					break;
				count=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCOUNT,0,0);
				buf=malloc(buf_size);
				if(buf!=0){
					int len=0;
					char *s=buf;
					memset(buf,0,buf_size);
					for(i=0;i<count;i++){
						if(SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETSEL,i,0)>0){
							int read;
							read=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXTLEN,i,0);
							if((len+read)>=(buf_size-1))
								break;
							read=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,i,s);
							if(read>0){
								len+=read;
								if(len>=(buf_size-1))
									break;
								buf[len++]='\n';
								s=buf+len;
							}
						}
					}
					copy_str_clipboard(buf);
					free(buf);
				}
			}
			break;
		}
		return -2;
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}