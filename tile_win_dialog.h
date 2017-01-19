static int resize_win_list(HWND hwnd)
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
	return 0;
}
static int init_win_list(HWND hwnd)
{
	int i;
	SendMessage(hwnd,LB_RESETCONTENT,0,0);
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd!=0){
			char str[128];
			int index;
			_snprintf(str,sizeof(str),"%s - %s",table_windows[i].table,table_windows[i].name);
			index=SendMessage(hwnd,LB_ADDSTRING,0,str);
			if(index>=0)
				SendMessage(hwnd,LB_SETITEMDATA,index,i);
		}
	}
	return 0;
}
static int init_win_pos(HWND hwnd)
{
	int cx,cy,h;
	RECT rect={0};
	GetWindowRect(ghmdiclient,&rect);
	cx=(rect.right+rect.left)/2;
	cy=(rect.bottom+rect.top)/2;
	GetWindowRect(hwnd,&rect);
	h=rect.bottom-rect.top;
	SetWindowPos(hwnd,NULL,cx,cy-(h/2),0,0,SWP_NOSIZE|SWP_NOZORDER);
	SetFocus(GetDlgItem(hwnd,IDC_LIST1));
	return 0;
}
static int tile_windows(HWND hwnd)
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
	return 0;
}
static LRESULT CALLBACK tile_win_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hgrippy;
	switch(msg){
	case WM_INITDIALOG:
		init_win_list(GetDlgItem(hwnd,IDC_LIST1));
		init_win_pos(hwnd);
		hgrippy=create_grippy(hwnd);
		break;
	case WM_SIZE:
		grippy_move(hwnd,hgrippy);
		resize_win_list(hwnd);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			tile_windows(hwnd);
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
	return DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_WINDOW_LIST),hwnd,tile_win_proc,NULL);
}
static int move_items(HWND hwnd,int delta,int *ypos,int cy)
{
	int count;
	count=SendMessage(hwnd,LB_GETCOUNT,0,0);
	if(delta!=0 && count>0){
		int i,move=TRUE;
		for(i=0;i<count;i++){
			int state=SendMessage(hwnd,LB_GETSEL,i,0);
			if(state>0){
				if(delta<0 && (i+delta)<0){
					move=FALSE;
					break;
				}
				if(delta>0 && (i+delta)>=count){
					move=FALSE;
					break;
				}
			}
		}
		if(move && ypos){
			*ypos=cy;
		}
		if(move)
			for(i=delta>0?count-1:0;
				delta>0?i>=0:i<count;
				delta>0?i--:i++){
				int state=SendMessage(hwnd,LB_GETSEL,i,0);
				if(state>0){
					char str[512]={0};
					int data;
					data=SendMessage(hwnd,LB_GETITEMDATA,i,0);
					SendMessage(hwnd,LB_GETTEXT,i,str);
					SendMessage(hwnd,LB_DELETESTRING,i,0);
					
					SendMessage(hwnd,LB_INSERTSTRING,i+delta,str);
					SendMessage(hwnd,LB_SETITEMDATA,i+delta,data);
					SendMessage(hwnd,LB_SETSEL,TRUE,i+delta);
				}
			}
	}
	return 0;
}
static WNDPROC orig_reorder_listview=0;
LRESULT APIENTRY sc_reorder_listview(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int lmb_down=FALSE;
	static int ypos=0;
	if(FALSE)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_GETDLGCODE:
		{
			MSG *msg=lparam;
			if(msg && msg->message==WM_CHAR){
				int key=MapVirtualKey(HIWORD(msg->lParam),1);
				//diable character selection
				if(toupper(key)=='A' && (GetKeyState(VK_CONTROL)&0x8000))
					return 0;
			}
		}
		break;
	case WM_KEYDOWN:
		{
			int ctrl=GetKeyState(VK_CONTROL)&0x8000;
			int key=LOWORD(wparam);
			if(key=='A' && ctrl){
				int count;
				count=SendMessage(hwnd,LB_GETCOUNT,0,0);
				if(count>0){
					int i,selected=0;
					for(i=0;i<count;i++){
						if(0<SendMessage(hwnd,LB_GETSEL,i,0))
							selected++;
					}
					if(selected>=count){
						SendMessage(hwnd,LB_SELITEMRANGE,FALSE,MAKELPARAM(0,0xFFFF));
					}else{
						SendMessage(hwnd,LB_SELITEMRANGE,TRUE,MAKELPARAM(0,0xFFFF));
					}
				}
				return 0;
			}
			else if(key=='Z' && ctrl){
				init_win_list(hwnd);
			}
		}
		break;
	case WM_MOUSEWHEEL:
		{
			short delta=HIWORD(wparam);
			delta/=WHEEL_DELTA;
			move_items(hwnd,-delta,0,0);
		}
		break;
	case WM_MOUSEMOVE:
		{
			RECT rect={0};
			int height;
			if(lmb_down==FALSE)
				break;
			SendMessage(hwnd,LB_GETITEMRECT,0,&rect);
			height=rect.bottom-rect.top;
			if(height>0){
				int cy,delta;
				cy=HIWORD(lparam);
				delta=cy-ypos;
				if(delta>0 && delta<height)
					break;
				if(delta<0 && delta>(-height))
					break;
				delta/=height;
				move_items(hwnd,delta,&ypos,cy);
			}
		}
		break;
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
		{
			int index;
			index=SendMessage(hwnd,LB_ITEMFROMPOINT,0,lparam);
			if(index>=0 && HIWORD(index)==0){
				if(0==(wparam&(MK_CONTROL|MK_SHIFT))){
					int sel=SendMessage(hwnd,LB_GETSEL,LOWORD(index),0);
					if(sel>0){
						lmb_down=TRUE;
						ypos=HIWORD(lparam);
						return 0;
					}
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		lmb_down=FALSE;
		break;
	}
    return CallWindowProc(orig_reorder_listview,hwnd,msg,wparam,lparam);

}

int clear_memoization()
{
	int find_win_by_hedit(HWND hedit,TABLE_WINDOW **win);
	int find_win_by_hwnd(HWND hwnd,TABLE_WINDOW **win);
	typedef int (*find_win_ptr)(HWND,TABLE_WINDOW **);
	find_win_ptr fwin_list[]={
		find_win_by_hlistview,
		find_win_by_hwnd,
		find_win_by_hedit
	};
	int i;
	for(i=0;i<sizeof(fwin_list)/sizeof(find_win_ptr);i++){
		TABLE_WINDOW *w=0;
		fwin_list[i](0,&w);
	}
	return 0;
}

static LRESULT CALLBACK reorder_win_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hgrippy;
	if(FALSE)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_INITDIALOG:
		init_win_list(GetDlgItem(hwnd,IDC_LIST1));
		init_win_pos(hwnd);
		hgrippy=create_grippy(hwnd);
		orig_reorder_listview=SetWindowLong(GetDlgItem(hwnd,IDC_LIST1),GWL_WNDPROC,(LONG)sc_reorder_listview);
		SetWindowText(hwnd,"Re-order windows");
		SetDlgItemText(hwnd,IDOK,"Re-order");
		break;
	case WM_SIZE:
		grippy_move(hwnd,hgrippy);
		resize_win_list(hwnd);
		break;
	case WM_MOUSEWHEEL:
		SendDlgItemMessage(hwnd,IDC_LIST1,msg,wparam,lparam);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			{
				int count;
				count=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCOUNT,0,0);
				if(count>0){
					TABLE_WINDOW *old_tables;
					int i;
					old_tables=malloc(sizeof(table_windows));
					if(old_tables){
						int *buf;
						memcpy(old_tables,table_windows,sizeof(table_windows));
						memset(&table_windows,0,sizeof(table_windows));
						clear_memoization();
						for(i=0;i<count;i++){
							int data;
							data=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETITEMDATA,i,0);
							memcpy(&table_windows[i],&old_tables[data],sizeof(TABLE_WINDOW));
						}
						free(old_tables);
						buf=malloc(count*sizeof(int));
						if(buf){
							int ctrl,shift;
							ctrl=GetKeyState(VK_CONTROL)&0x8000;
							shift=GetKeyState(VK_SHIFT)&0x8000;
							for(i=0;i<count;i++)
								buf[i]=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETSEL,i,0);
							for(i=0;i<count;i++)
								SendDlgItemMessage(hwnd,IDC_LIST1,LB_SETSEL,TRUE,i);
							if(ctrl || shift)
								tile_windows(hwnd);
							else
								mdi_cascade_win_vert();
							init_win_list(GetDlgItem(hwnd,IDC_LIST1));
							for(i=0;i<count;i++)
								SendDlgItemMessage(hwnd,IDC_LIST1,LB_SETSEL,buf[i],i);
							free(buf);
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
int reorder_win_dialog(HWND hwnd)
{
	return DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_WINDOW_LIST),hwnd,reorder_win_proc,NULL);
}


static void refresh_thread(void *args[])
{
	int *thread_stop=0;
	int *thread_busy=0;
	int *list=0;
	HWND hwnd=0;
	int i;
	if(args){
		thread_stop=args[0];
		thread_busy=args[1];
		hwnd=args[2];
		list=args[3];
	}
	printf("thread started\n");
	if(hwnd)
		PostMessage(hwnd,WM_APP,1,0);
	if(thread_busy)
		*thread_busy=1;

	PostMessage(hwnd,WM_APP,2,0);
	wait_worker_idle(1,FALSE);

	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		int escape_pressed=FALSE;
		if(table_windows[i].hwnd!=0){
			int do_wait=FALSE;
			if(list){
				if(list[i]){
					task_execute_query(&table_windows[i]);
					do_wait=TRUE;
				}
			}
			else{
				task_execute_query(&table_windows[i]);
				do_wait=TRUE;
			}
			if(do_wait){
				set_status_bar_text(ghstatusbar,1,"refreshing table %s (press escape to abort)",table_windows[i].table);
			}
			while(do_wait && FALSE==wait_worker_idle(100,FALSE)){
				if(thread_stop && (*thread_stop!=0)){
					printf("exiting wait\n");
					break;
				}
				if(GetAsyncKeyState(VK_ESCAPE)&0x8000){
					escape_pressed=TRUE;
					break;
				}
			};
			if(do_wait){
				set_status_bar_text(ghstatusbar,1,"");
			}
		}
		if(thread_stop && (*thread_stop!=0))
			break;
		if(escape_pressed){
			set_status_bar_text(ghstatusbar,1,"refreshing aborted");
			break;
		}
	}
	if(thread_busy)
		*thread_busy=0;
	printf("thread exit\n");
	return;
}

static LRESULT CALLBACK refresh_all_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hgrippy;
	static int thread_stop=0;
	static int thread_busy=0;
	if(FALSE)
	if(msg!=WM_SETCURSOR)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}

	switch(msg){
	case WM_INITDIALOG:
		init_win_list(GetDlgItem(hwnd,IDC_LIST1));
		init_win_pos(hwnd);
		hgrippy=create_grippy(hwnd);
		SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGE,TRUE,MAKELPARAM(0,-1));
		SetWindowText(hwnd,"Refresh windows");
		SetDlgItemText(hwnd,IDOK,"Refresh");
		break;
	case WM_SIZE:
		grippy_move(hwnd,hgrippy);
		resize_win_list(hwnd);
		break;
	case WM_APP:
		switch(wparam){
		case 0:
			SetDlgItemText(hwnd,IDOK,"Refresh");
			break;
		case 1:
			SetDlgItemText(hwnd,IDOK,"Stop");
			break;
		case 2:
			EndDialog(hwnd,0);
			break;
		}
		break;
	case WM_CLOSE:
		thread_stop=1;
		EndDialog(hwnd,0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if(thread_busy==0){
				int i,count,*thread_handle;
				static void *args[4]={&thread_stop,&thread_busy};
				static int list[sizeof(table_windows)/sizeof(TABLE_WINDOW)];
				args[2]=hwnd;
				args[3]=list;
				memset(list,0,sizeof(list));
				count=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCOUNT,0,0);
				for(i=0;i<count;i++){
					int sel;
					sel=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETSEL,i,0);
					if(sel>0){
						int data;
						data=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETITEMDATA,i,0);
						if(data>=0){
							if(data<(sizeof(list)/sizeof(int)))
								list[data]=1;
						}
					}
				}
				thread_stop=0;
				thread_handle=_beginthread(refresh_thread,0,args);
				if(thread_handle==-1)
					MessageBox(hwnd,"Failed to create worker thread","Error",MB_OK|MB_SYSTEMMODAL);
			}else
				thread_stop=1;
			break;
		case IDCANCEL:
			thread_stop=1;
			EndDialog(hwnd,0);
			break;
		}
	}
	return 0;
}
int refresh_all_dialog(HWND hwnd)
{
	return DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_WINDOW_LIST),hwnd,refresh_all_proc,NULL);
}

