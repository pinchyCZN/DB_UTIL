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
			RECT rect={0};
			if(lparam==0)
				EndDialog(hwnd,-1);
			win=lparam;
			last_row=ListView_GetSelectionMark(win->hlistview);
			if(last_row<0)
				last_row=0;
			last_col=win->selected_column;
			GetWindowRect(win->hwnd,&rect);
			SetWindowPos(hwnd,NULL,rect.left,rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000))
				wparam=IDC_SEARCH_UP;
			else
				wparam=IDC_SEARCH_DOWN;

		case IDC_SEARCH_UP:
		case IDC_SEARCH_DOWN:
			{
				//int dir=1;
				int i,max,found=FALSE;
				char find[80]={0};
				GetWindowText(GetDlgItem(hwnd,IDC_EDIT1),find,sizeof(find));
				max=ListView_GetItemCount(win->hlistview);
				for(i=last_row;i<max;i++){
					int j=last_col+1;
					for( ;j<win->columns;j++){
						char str[80]={0};
						ListView_GetItemText(win->hlistview,i,j,str,sizeof(str));
						if(strstri(str,find)!=0){
							found=TRUE;
							break;
						}
					}
					if(found){
						RECT rect={0};
						last_col=j;
						last_row=i;
						win->selected_column=j;
						ListView_EnsureVisible(win->hlistview,i,TRUE);
						ListView_SetSelectionMark(win->hlistview,i);
						ListView_GetItemRect(win->hlistview,i,&rect,LVIR_BOUNDS);
						MapWindowPoints(win->hlistview,NULL,&rect,2);
						printf("found at %i %i\n",last_row,last_col);
						printf("rect %i %i\n",rect.left,rect.top);
						SetWindowPos(hwnd,NULL,rect.left,rect.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);
						break;
					}
					else
						last_col=0;
				}

			}
			break;
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}