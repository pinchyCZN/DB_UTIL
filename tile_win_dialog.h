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
	return DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_WINDOW_LIST),hwnd,tile_win_proc,NULL);
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
			};
			if(do_wait){
				set_status_bar_text(ghstatusbar,1,"");
			}
		}
		if(thread_stop && (*thread_stop!=0))
			break;
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
		{
			int i,cx,cy,h;
			RECT rect={0};
			for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
				if(table_windows[i].hwnd!=0){
					char str[128];
					int index;
					_snprintf(str,sizeof(str),"%s - %s",table_windows[i].table,table_windows[i].name);
					index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
					if(index>=0)
						SendDlgItemMessage(hwnd,IDC_LIST1,LB_SETITEMDATA,index,i);
				}
			}
			SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGE,TRUE,MAKELPARAM(0,-1));
			SetWindowText(hwnd,"Refresh windows");
			SetDlgItemText(hwnd,IDOK,"Refresh");
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

