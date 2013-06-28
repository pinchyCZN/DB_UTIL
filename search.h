int create_tooltip(HWND hwnd,char *msg,int x, int y,HWND *hwndTT)
{
	int result=FALSE;
	if(hwndTT!=0 && *hwndTT==0 &&  msg!=0 && msg[0]!=0){
		*hwndTT=CreateWindowEx(WS_EX_TOPMOST,
			TOOLTIPS_CLASS,NULL,
			WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,        
			CW_USEDEFAULT,CW_USEDEFAULT,
			CW_USEDEFAULT,CW_USEDEFAULT,
			hwnd,NULL,NULL,NULL);
		if(*hwndTT!=0){
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_IDISHWND|TTF_TRACK|TTF_ABSOLUTE;
			ti.hwnd = *hwndTT;
			ti.uId = *hwndTT;
			ti.lpszText = msg;
			SendMessage(*hwndTT,TTM_ADDTOOL,0,&ti);
			SendMessage(*hwndTT,TTM_UPDATETIPTEXTA,0,&ti);
			SendMessage(*hwndTT,TTM_TRACKPOSITION,0,MAKELONG(x,y)); 
			SendMessage(*hwndTT,TTM_TRACKACTIVATE,TRUE,&ti);
			result=TRUE;
		}
	}
	return result;
}
int destroy_tooltip(HWND hwndTT)
{
	if(hwndTT!=0){
		DestroyWindow(hwndTT);
		return TRUE;
	}
	return FALSE;
}

#define DOWN IDC_SEARCH_DOWN
#define UP IDC_SEARCH_UP
#define FIRST 0

int do_search(TABLE_WINDOW *win,HWND hwnd,char *find,int dir,int col_only)
{
	int i,j,max,found=FALSE;
	static int last_row=0,last_col=0,last_dir=FIRST;
	if(hwnd==0 && find==0){
		if(win!=0){
			last_row=ListView_GetSelectionMark(win->hlistview);
			if(last_row<0)
				last_row=0;
			last_col=win->selected_column;
		}
		else{
			last_col=0;
			last_row=last_col=0;
		}
		last_dir=FIRST;
		return TRUE;
	}
	if(win==0 || find==0 || find[0]==0)
		return FALSE;

	if(last_dir!=FIRST && col_only){
		if(last_dir!=dir){
			if(dir==DOWN)
				last_row++;
			else
				last_row--;
		}
	}
	if(dir==DOWN){
		max=ListView_GetItemCount(win->hlistview);

		for(i=last_row;i<max;i++){
			if(last_dir==FIRST)
				j=0;
			else{
				j=last_col+1;
			}
			if(col_only){
					char str[80]={0};
					j=win->selected_column;
					ListView_GetItemText(win->hlistview,i,win->selected_column,str,sizeof(str));
					if(strstri(str,find)!=0){
						found=TRUE;
						break;
					}
			}
			else{
				for( ;j<win->columns;j++){
					char str[80]={0};
					ListView_GetItemText(win->hlistview,i,j,str,sizeof(str));
					if(strstri(str,find)!=0){
						found=TRUE;
						break;
					}
				}
			}
			if(found)
				break;
			else{
				last_col=0;
				last_dir=FIRST;
			}
		}
	}
	//UP ------------------------------
	else{
		for(i=last_row;i>=0;i--){
			if(last_dir==FIRST)
				j=win->columns-1;
			else{
				j=last_col-1;
			}
			if(j<0){
				j=win->columns-1;
				i--;
				if(i<0)
					break;
			}
			if(col_only){
					char str[80]={0};
					j=win->selected_column;
					ListView_GetItemText(win->hlistview,i,win->selected_column,str,sizeof(str));
					if(strstri(str,find)!=0){
						found=TRUE;
						break;
					}
			}
			else{
				for( ;j>=0;j--){
					char str[80]={0};
					ListView_GetItemText(win->hlistview,i,j,str,sizeof(str));
					if(strstri(str,find)!=0){
						found=TRUE;
						break;
					}
				}
			}
			if(found)
				break;
			else{
				last_col=win->columns-1;
				last_dir=FIRST;
			}
		}
	}
	if(found){
		RECT rect={0},rect_col={0};
		int mark;
		last_col=j;
		last_row=i;
		win->selected_column=j;
		ListView_EnsureVisible(win->hlistview,i,TRUE);
		
		mark=ListView_GetSelectionMark(win->hlistview);
		ListView_SetItemState(win->hlistview,mark,0,LVIS_SELECTED|LVIS_FOCUSED);
		
		ListView_SetItemState(win->hlistview,i,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
		ListView_SetSelectionMark(win->hlistview,i);
		
		ListView_GetItemRect(win->hlistview,i,&rect,LVIR_BOUNDS);
		lv_scroll_column(win->hlistview,win->selected_column);
		if(hwnd!=0)
			InvalidateRect(hwnd,NULL,TRUE);
		MapWindowPoints(win->hlistview,NULL,&rect,2);
		printf("found at %i %i\n",last_row,last_col);
		printf("rect %i %i\n",rect.left,rect.top);
		
		lv_get_col_rect(win->hlistview,j,&rect_col);
		MapWindowPoints(win->hlistview,NULL,&rect_col,2);
		{
			SCROLLINFO si;
			si.cbSize=sizeof(si);
			si.fMask=SIF_POS;
			if(GetScrollInfo(win->hlistview,SB_HORZ,&si)!=0){
				rect_col.left-=si.nPos;
			}


		}
		set_status_bar_text(ghstatusbar,1,"row=%3i col=%2i",last_row+1,win->selected_column+1);
		if(hwnd!=0)
			SetWindowPos(hwnd,NULL,rect_col.left,rect.bottom,0,0,SWP_NOSIZE|SWP_NOZORDER);
		if(dir==DOWN){
			last_dir=DOWN;
			if(col_only)
				last_row++;

		}
		else{
			last_dir=UP;
			if(col_only)
				last_row--;
		}
	}
	return found;
}

int search_fill_lb(HWND hwnd,HWND hlistview,int index)
{
	int i,max;
	SendDlgItemMessage(hwnd,IDC_COMBO1,CB_RESETCONTENT,0,0);
	max=lv_get_column_count(hlistview);
	for(i=0;i<max;i++){
		char str[100]={0};
		lv_get_col_text(hlistview,i,str,sizeof(str));
		if(str[0]!=0)
			SendDlgItemMessage(hwnd,IDC_COMBO1,CB_ADDSTRING,0,str);
	}
	SendDlgItemMessage(hwnd,IDC_COMBO1,CB_SETCURSEL,index,0);
}
int get_search_text(char **str)
{
	static char find[80]={0};
	if(str!=0){
		*str=find;
		return sizeof(find);
	}
	else
		return 0;
}
LRESULT CALLBACK search_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{

	static TABLE_WINDOW *win=0;
	static int col_only=FALSE,timer=0;
	//static char find[80]={0};
	static HWND hwndTT=0;
	int search=0;

	if(FALSE)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_USER&&msg!=WM_GETFONT)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("src");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_INITDIALOG:
		{
			char *find=0;
			int find_len;
			RECT rect={0};
			if(lparam==0)
				EndDialog(hwnd,-1);
			win=lparam;
			find_len=get_search_text(&find);
			if(find_len>0){
				SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETLIMITTEXT,find_len,0);
				SetWindowText(GetDlgItem(hwnd,IDC_EDIT1),find);
			}
			GetWindowRect(win->hwnd,&rect);
			SetWindowPos(hwnd,NULL,rect.left,rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETSEL,0,-1);
			SetFocus(GetDlgItem(hwnd,IDC_EDIT1));

			search_fill_lb(hwnd,win->hlistview,win->selected_column);
			if(col_only){
				ShowWindow(GetDlgItem(hwnd,IDC_COMBO1),SW_SHOW);
				CheckDlgButton(hwnd,IDC_SEARCH_COL,BST_CHECKED);
			}
			else
				ShowWindow(GetDlgItem(hwnd,IDC_COMBO1),SW_HIDE);

			if(ListView_GetSelectionMark(win->hlistview)<0){
				ListView_SetSelectionMark(win->hlistview,0);
				ListView_SetItemState(win->hlistview,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
				InvalidateRect(win->hlistview,NULL,TRUE);
			}
			hwndTT=0;
			timer=0;
			do_search(win,0,0,0,0);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_COMBO1:
			if(HIWORD(wparam)==CBN_SELCHANGE){
				int sel=SendDlgItemMessage(hwnd,IDC_COMBO1,CB_GETCURSEL,0,0);
				if(sel>=0){
					int index;
					win->selected_column=sel;
					index=ListView_GetSelectionMark(win->hlistview);
					if(index>=0){
						ListView_RedrawItems(win->hlistview,index,index);
						set_status_bar_text(ghstatusbar,1,"row=%3i col=%2i",index+1,win->selected_column+1);
					}
					lv_scroll_column(win->hlistview,sel);
				}
			}
			break;
		case IDC_EDIT1:
			if(HIWORD(wparam)==EN_CHANGE)
				do_search(win,0,0,0,0);
			break;
		case IDOK:
			if((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000))
				search=UP;
			else
				search=DOWN;
			break;
		case IDC_SEARCH_UP:
			search=UP;
			break;
		case IDC_SEARCH_DOWN:
			if((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000))
				search=UP;
			else
				search=DOWN;
			break;
		case IDCANCEL:
			if(timer!=0)
				KillTimer(hwnd,timer);
			if(hwndTT!=0)
				destroy_tooltip(hwndTT);
			EndDialog(hwnd,0);
			break;
		case IDC_SEARCH_COL:
			if(HIWORD(wparam)==BN_CLICKED){
				if(IsDlgButtonChecked(hwnd,IDC_SEARCH_COL)==BST_CHECKED)
					col_only=TRUE;
				else
					col_only=FALSE;
				if(col_only){
					ShowWindow(GetDlgItem(hwnd,IDC_COMBO1),SW_SHOW);
					SendDlgItemMessage(hwnd,IDC_COMBO1,CB_SETCURSEL,win->selected_column,0);
				}
				else
					ShowWindow(GetDlgItem(hwnd,IDC_COMBO1),SW_HIDE);
			}
			break;
		}
		break;
	case WM_TIMER:
		if(timer!=0){
			KillTimer(hwnd,timer);
			timer=0;
		}
		destroy_tooltip(hwndTT);
		hwndTT=0;
		break;
	}
	if(search){
		char *find=0;
		int find_len;
		find_len=get_search_text(&find);
		if(find_len>0){
			GetWindowText(GetDlgItem(hwnd,IDC_EDIT1),find,find_len);
			if(do_search(win,hwnd,find,search,col_only)==0){
				RECT rect={0};
				int y;
				GetWindowRect(win->hlistview,&rect);
				if(search==DOWN)
					y=rect.bottom;
				else
					y=rect.top;
				create_tooltip(hwnd,"nothing more found",rect.left,y,&hwndTT);
				timer=SetTimer(hwnd,0x1337,550,NULL);
			}
		}
	}

	return 0;
}