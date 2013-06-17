
int find_win_by_hlistview(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hlistview==hwnd){
			*win=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CALLBACK col_info_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static TABLE_WINDOW *win=0;
	static HWND grippy=0;
	switch(msg){
	case WM_INITDIALOG:
		win=0;
		if(find_win_by_hlistview(lparam,&win)){
			int i;
			for(i=0;i<win->columns;i++){
				char str[255]={0};
				char name[80]={0};
				lv_get_col_text(win->hlistview,i,name,sizeof(name));
				_snprintf(str,sizeof(str),"%2i-%-10s attr=%2i length=%3i col_width=%3i",
					i,name,win->col_attr[i].type,win->col_attr[i].length,win->col_attr[i].col_width);
				SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
			}
		}
		SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
		grippy=create_grippy(hwnd);
		resize_col_info(hwnd);
		break;
	case WM_SIZE:
		resize_col_info(hwnd);
		grippy_move(hwnd,grippy);
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