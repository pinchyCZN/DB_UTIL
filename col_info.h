
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
	switch(msg){
	case WM_INITDIALOG:
		win=0;
		if(find_win_by_hlistview(lparam,&win)){
			int i;
			for(i=0;i<win->columns;i++){
				char str[255]={0};
				_snprintf(str,sizeof(str),"col %i attr %i width%i",i,win->col_attr[i],win->col_width[i]);
				SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
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