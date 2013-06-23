
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
static int find_sql_type_str(int type,const char **str)
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
int populate_col_info(HWND hwnd,HWND hlistview,LPARAM lparam)
{
	int i;
	static TABLE_WINDOW *win=0;
	char *cols[]={"field name","type","type #","size"};
	for(i=0;i<sizeof(cols)/sizeof(char *);i++)
		lv_add_column(hlistview,cols[i],i);

	win=0;
	if(find_win_by_hlistview(lparam,&win)){
		for(i=0;i<win->columns;i++){
			char str[10]={0};
			char name[80]={0};
			char *sql_name="";
			lv_get_col_text(win->hlistview,i,name,sizeof(name));
			lv_insert_data(hlistview,i,0,name);
			find_sql_type_str(win->col_attr[i].type,&sql_name);
			lv_insert_data(hlistview,i,1,sql_name);
			_snprintf(str,sizeof(str),"%i",win->col_attr[i].type);
			lv_insert_data(hlistview,i,2,str);
			_snprintf(str,sizeof(str),"%i",win->col_attr[i].length);
			lv_insert_data(hlistview,i,3,str);
		}
		if(win->table[0]!=0)
			SetWindowText(hwnd,win->table);
	}
	for(i=0;i<sizeof(cols)/sizeof(char *);i++){
		int method=LVSCW_AUTOSIZE;
		if(i>=2)
			method=LVSCW_AUTOSIZE_USEHEADER;
		ListView_SetColumnWidth(hlistview,i,method);
	}

}
LRESULT CALLBACK col_info_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0,hlistview=0;
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
		hlistview=CreateWindow(WC_LISTVIEW,"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_LIST1,
                                     ghinstance,
                                     NULL);
		ListView_SetExtendedListViewStyle(hlistview,LVS_EX_FULLROWSELECT);
		SendMessage(hlistview,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
		populate_col_info(hwnd,hlistview,lparam);
		SetFocus(hlistview);
		grippy=create_grippy(hwnd);
		resize_col_info(hwnd);
		break;
	case WM_SIZE:
		resize_col_info(hwnd);
		grippy_move(hwnd,grippy);
		break;
	case WM_NOTIFY:
		if(LOWORD(wparam)==IDC_LIST1){
			NMHDR *nmhdr=lparam;
			if(lparam==0)
				break;
			if(nmhdr->code==LVN_KEYDOWN){
				LV_KEYDOWN *lvk=lparam;
				switch(lvk->wVKey){
				case 'A':
					if(GetKeyState(VK_CONTROL)&0x8000){
						int i,count;
						count=ListView_GetItemCount(hlistview);
						for(i=0;i<count;i++)
							ListView_SetItemState(hlistview,i,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					}
					break;
				case 'C':
					{
						int count,buf_size=0x10000;
						char *buf;
						if(!(GetKeyState(VK_CONTROL)&0x8000))
							break;
						count=ListView_GetItemCount(hlistview);
						buf=malloc(buf_size);
						if(buf!=0){
							int i;
							memset(buf,0,buf_size);
							for(i=0;i<count;i++){
								int cols,j;
								cols=lv_get_column_count(hlistview);
								for(j=0;j<cols;j++){
									int len=0;
									len=strlen(buf);
									if(len<(buf_size-2)){
										if(ListView_GetItemState(hlistview,i,LVIS_SELECTED)==LVIS_SELECTED){
											ListView_GetItemText(hlistview,i,j,buf+len,buf_size-len-2);
											if(j==(cols-1))
												strcat(buf,"\n");
											else
												strcat(buf,",");
										}
									}
									else
										break;
								}
							}
							copy_str_clipboard(buf);
							free(buf);
						}
					}
					break;
				}
			}
		}
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