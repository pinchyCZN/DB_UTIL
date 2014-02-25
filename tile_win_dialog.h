static LRESULT CALLBACK tile_win_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hgrippy;
	switch(msg){
	case WM_INITDIALOG:
		{
			int i,cx,cy,h;
			RECT rect={0};
			for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
				if(table_windows[i].hwnd!=0){
					char str[128];
					_snprintf(str,sizeof(str),"%s - %s",table_windows[i].table,table_windows[i].name);
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
				}
			}
			GetWindowRect(ghmdiclient,&rect);
			cx=(rect.right+rect.left)/2;
			cy=(rect.bottom+rect.top)/2;
			GetWindowRect(hwnd,&rect);
			h=rect.bottom-rect.top;
			SetWindowPos(hwnd,NULL,cx,cy-(h/2),0,0,SWP_NOSIZE|SWP_NOZORDER);
			hgrippy=create_grippy(hwnd);
			SetFocus(GetDlgItem(hwnd,IDC_LIST1));
		}
		break;
	case WM_SIZE:
		grippy_move(hwnd,hgrippy);
		{
			RECT rect={0},brect={0};
			int w,h,x,y;
			GetClientRect(hwnd,&rect);
			GetWindowRect(GetDlgItem(hwnd,IDOK),&brect);
			w=rect.right;
			h=rect.bottom-(brect.bottom-brect.top)+2;
			SetWindowPos(GetDlgItem(hwnd,IDC_LIST1),NULL,0,0,w,h,SWP_NOZORDER);
			x=0;
			y=rect.bottom-(brect.bottom-brect.top);
			SetWindowPos(GetDlgItem(hwnd,IDOK),NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
			x=rect.right/2;
			y=rect.bottom-(brect.bottom-brect.top);
			SetWindowPos(GetDlgItem(hwnd,IDCANCEL),NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			{
				int i,count=0;
				int caption_height,y,width,height;
				RECT rect={0};
				caption_height=GetSystemMetrics(SM_CYCAPTION);
				caption_height+=GetSystemMetrics(SM_CXEDGE)*2;
				GetClientRect(ghmdiclient,&rect);
				count=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETSELCOUNT,0,0);
				if(count<=0)
					return FALSE;
				height=rect.bottom/count;
				if(height<caption_height)
					height=caption_height;
				width=rect.right;
				y=0;
				for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
					int sel;
					sel=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETSEL,i,0);
					if(sel>0){
						TABLE_WINDOW *win=&table_windows[i];
						if(win->hwnd!=0){
							int flags=0;
							if(width<=0 || height<=0)
								flags=SWP_NOSIZE;
							SetWindowPos(win->hwnd,NULL,0,y,width,height,flags);
							y+=height;
						}
					}
				}

			}
			break;
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
	}
	return 0;
}

int tile_select_dialog(HWND hwnd)
{
	return DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_TILE_WINDOWS),hwnd,tile_win_proc,NULL);
}