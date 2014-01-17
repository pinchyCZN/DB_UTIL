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
			SendMessage(*hwndTT,TTM_SETMAXTIPWIDTH,0,640); //makes multiline tooltips
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

int do_search(TABLE_WINDOW *win,HWND hwnd,char *find,int dir,int col_only,int whole_word)
{
	extern char * strstri(char *s1,char *s2);
	int i,j,max,found=FALSE;
	int sizeof_str=0x4000;
	char *str=0;
	DWORD tick=0;
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

	str=malloc(sizeof_str);
	if(str==0){
		set_status_bar_text(ghstatusbar,0,"cant allocate memory!!!");
		return FALSE;
	}

	if(dir==DOWN){
		max=ListView_GetItemCount(win->hlistview);

		for(i=last_row;i<max;i++){
			if((GetTickCount()-tick)>250){
				tick=GetTickCount();
				if(GetAsyncKeyState(VK_ESCAPE)&0x8001){
					set_status_bar_text(ghstatusbar,0,"aborted search");
					break;
				}
			}
			if(last_dir==FIRST)
				j=0;
			else{
				j=last_col+1;
			}
			if(col_only){
					j=win->selected_column;
					ListView_GetItemText(win->hlistview,i,win->selected_column,str,sizeof_str);
					if(whole_word){
						if(strcmp(str,find)==0){
							found=TRUE;
							break;
						}
					}
					else if(strstri(str,find)!=0){
						found=TRUE;
						break;
					}
			}
			else{
				for( ;j<win->columns;j++){
					ListView_GetItemText(win->hlistview,i,j,str,sizeof_str);
					if(whole_word){
						if(strcmp(str,find)==0){
							found=TRUE;
							break;
						}
					}
					else if(strstri(str,find)!=0){
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
			if((GetTickCount()-tick)>250){
				tick=GetTickCount();
				if(GetAsyncKeyState(VK_ESCAPE)&0x8001){
					set_status_bar_text(ghstatusbar,0,"aborted search");
					break;
				}
			}
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
					j=win->selected_column;
					ListView_GetItemText(win->hlistview,i,win->selected_column,str,sizeof_str);
					if(whole_word){
						if(strcmp(str,find)==0){
							found=TRUE;
							break;
						}
					}
					else if(strstri(str,find)!=0){
						found=TRUE;
						break;
					}
			}
			else{
				for( ;j>=0;j--){
					ListView_GetItemText(win->hlistview,i,j,str,sizeof_str);
					if(whole_word){
						if(strcmp(str,find)==0){
							found=TRUE;
							break;
						}
					}
					else if(strstri(str,find)!=0){
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
	if(str!=0){
		free(str);
		str=0;
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
		if(hwnd!=0){
			HMONITOR hmon;
			POINT p;
			RECT srect={0};
			int mx,my;
			GetWindowRect(hwnd,&srect);
			mx=(srect.right-srect.left)/2;
			my=(srect.bottom-srect.top)/2;
			p.x=rect_col.left+mx;
			p.y=rect.bottom+my;
			hmon=MonitorFromPoint(p,MONITOR_DEFAULTTONULL);
			if(hmon==0){
				p.x=rect_col.left+mx;
				p.y=rect.top-my;
				hmon=MonitorFromPoint(p,MONITOR_DEFAULTTONULL);
				if(hmon!=0){
					//place on top
					p.x=rect_col.left;
					p.y=rect.top-(srect.bottom-srect.top);
				}else{
					p.x=rect_col.left-mx;
					p.y=rect.top-my;
					hmon=MonitorFromPoint(p,MONITOR_DEFAULTTONULL);
					if(hmon!=0){
						/*position at:
						 _________
						| search  |
						 ---------*--------
						          | match  |
						           --------
						*/
						p.x=rect_col.left-(srect.right-srect.left);
						p.y=rect.top-(srect.bottom-srect.top);
					}else{
						p.x=rect_col.left-mx;
						p.y=rect.bottom+my;
						hmon=MonitorFromPoint(p,MONITOR_DEFAULTTONULL);
						if(hmon!=0){
							/*position at:
							           --------
							          | match  |
							 ---------*--------
							| search  |
							 ---------
							*/
							p.x=rect_col.left-(srect.right-srect.left);
							p.y=rect.bottom+(srect.bottom-srect.top);
						}
					}

				}
			}
			else{
				//place below
				p.x=rect_col.left;
				p.y=rect.bottom;
			}
			if(hmon!=0){
				SetWindowPos(hwnd,NULL,p.x,p.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
			}
		}
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
	return TRUE;
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
	static int col_only=FALSE,whole_word=FALSE,timer=0;
	static WNDPROC wporigtedit=0;
	static HWND hwndTT=0,hedit=0;
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
	if(hwnd==hedit && wporigtedit!=0){
		if(msg==WM_KEYFIRST){
			if(wparam=='A'){
				if(GetKeyState(VK_CONTROL)&0x8000)
					SendMessage(hwnd,EM_SETSEL,0,-1);
			}
		}
		return CallWindowProc(wporigtedit,hwnd,msg,wparam,lparam);
	}
	switch(msg){
	case WM_INITDIALOG:
		{
			char *find=0;
			int find_len;
			int x,y;
			RECT rect={0};
			if(lparam==0)
				EndDialog(hwnd,-1);
			win=lparam;
			find_len=get_search_text(&find);
			if(find_len>0){
				SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETLIMITTEXT,find_len,0);
				SetWindowText(GetDlgItem(hwnd,IDC_EDIT1),find);
			}
			GetWindowRect(GetParent(win->hwnd),&rect);
			x=(rect.left+rect.right)/2;
			y=(rect.top+rect.bottom)/2;
			GetWindowRect(hwnd,&rect);
			x-=(rect.right-rect.left)/2;
			y-=(rect.bottom-rect.top)/2;
			SetWindowPos(hwnd,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETSEL,0,-1);
			SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
			search_fill_lb(hwnd,win->hlistview,win->selected_column);
			if(GetKeyState(VK_SHIFT)&0x8000)
				col_only=TRUE;
			else
				col_only=FALSE;

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
			if(win->table!=0 && win->table[0]!=0){
				char str[80]={0};
				_snprintf(str,sizeof(str),"search %s (shift+enter search up)",win->table);
				str[sizeof(str)-1]=0;
				SetWindowText(hwnd,str);
			}
			hedit=GetDlgItem(hwnd,IDC_EDIT1);
			if(hedit)
				wporigtedit=SetWindowLong(hedit,GWL_WNDPROC,(LONG)search_proc);
			else
				wporigtedit=0;
			break;

			hwndTT=0;
			timer=0;
			do_search(win,0,0,0,0,0);
		}
		break;
	case WM_HELP:
		{
			static int help_active=FALSE;
			if(!help_active){
				help_active=TRUE;
				MessageBox(hwnd,"shift+enter search up\r\nctrl+enter search whole word\r\n","HELP",MB_OK);
				help_active=FALSE;
			}
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
				do_search(win,0,0,0,0,0);
			break;
			break;
		case IDOK:
		case IDC_SEARCH_UP:
		case IDC_SEARCH_DOWN:
			if(GetKeyState(VK_SHIFT)&0x8000)
				search=UP;
			else
				search=DOWN;
			if(GetKeyState(VK_CONTROL)&0x8000)
				whole_word=TRUE;
			else
				whole_word=FALSE;
			if(IDC_SEARCH_UP==LOWORD(wparam))
				search=UP;
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
			RECT rect={0};
			int x,y;
			GetWindowRect(GetParent(win->hwnd),&rect);
			x=(rect.left+rect.right)/2;
			y=(rect.top+rect.bottom)/2;
			destroy_tooltip(hwndTT);
			hwndTT=0;
			create_tooltip(hwnd,"searching\r\npress escape to abort",x,y,&hwndTT);
			GetWindowText(GetDlgItem(hwnd,IDC_EDIT1),find,find_len);
			if(do_search(win,hwnd,find,search,col_only,whole_word)==0){
				x=rect.left;
				if(search==DOWN)
					y=rect.bottom-23;
				else
					y=rect.top;
				if(hwndTT!=0){
					destroy_tooltip(hwndTT);
					hwndTT=0;
				}
				create_tooltip(hwnd,"nothing more found",x,y,&hwndTT);
				if(timer!=0){
					KillTimer(hwnd,timer);
					timer=0;
				}
				timer=SetTimer(hwnd,0x1337,550,NULL);
			}else{
				destroy_tooltip(hwndTT);
				hwndTT=0;
			}

		}
	}

	return 0;
}