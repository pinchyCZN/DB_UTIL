LRESULT CALLBACK search_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static TABLE_WINDOW *win=0;
	static int last_row=0,last_col=0;

	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_USER&&msg!=WM_GETFONT)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("l");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_INITDIALOG:
		{
			if(lparam==0)
				EndDialog(hwnd,-1);
			win=lparam;
			last_row=last_col=0;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
		case IDC_SEARCH_UP:
		case IDC_SEARCH_DOWN:

		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}