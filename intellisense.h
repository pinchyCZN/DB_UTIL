

char tab_word[20]={0};
int tab_continue=FALSE,tab_pos=0;

int create_intellisense(TABLE_WINDOW *win)
{
	int result=FALSE;
	if(win!=0 && win->hintel==0){
		win->hintel = CreateWindow("LISTBOX",
										 "",
										 WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LBS_HASSTRINGS|LBS_SORT|LBS_STANDARD,
										 0,0,
										 0,0,
										 win->hwnd,
										 IDC_INTELLISENSE,
										 ghinstance,
										 NULL);
		if(win->hintel!=0){
			int start=-1,end=-1;
			SendMessage(win->hedit,EM_GETSEL,&start,&end);
			if(end<start)
				start=end;
			if(start!=-1){
				POINT p={0};
				SendMessage(win->hedit,EM_POSFROMCHAR,&p,start);
				SetWindowPos(win->hintel,HWND_TOP,p.x,p.y+20,80,120,SWP_SHOWWINDOW);
				result=TRUE;
			}
		}
	}
	return result;
}
int destroy_intellisense(TABLE_WINDOW *win)
{
	if(win!=0 && win->hintel!=0){
		DestroyWindow(win->hintel);
		win->hintel=0;
		return TRUE;
	}
	return FALSE;
}
int get_intel_count(HWND hroot,char *src,int table)
{

}
int populate_intel(TABLE_WINDOW *win,char *src)
{
	if(win!=0 && src!=0){
		HTREEITEM h;
		h=TreeView_GetChild(ghtreeview,win->hroot);

		while(h!=0){
			int index;
			char str[80]={0};
			tree_get_info(h,str,sizeof(str),0);
			if(strnicmp(str,src,strlen(src))==0){
				index=SendMessage(win->hintel,LB_FINDSTRINGEXACT,-1,str);
				if(index==LB_ERR)
					SendMessage(win->hintel,LB_ADDSTRING,0,str);
			}
			else{
				index=SendMessage(win->hintel,LB_FINDSTRINGEXACT,-1,str);
				if(index!=LB_ERR)
					SendMessage(win->hintel,LB_DELETESTRING,index,0);
			}
			h=TreeView_GetNextSibling(ghtreeview,h);
		}
		if(SendMessage(win->hintel,LB_GETCOUNT,0,0)<=0)
			destroy_intellisense(win);
		else{
			SendMessage(win->hintel,LB_SETCURSEL,0,0);
			return TRUE;
		}
	}
	return FALSE;
}

int handle_intellisense(TABLE_WINDOW *win,int key)
{
	printf("key=%02X\n",0xFF&key);
	if(win!=0){
		int start=0,end=0,line,lindex;
		SendMessage(win->hedit,EM_GETSEL,&start,&end);
		if(end<start)
			start=end;
		line=SendMessage(win->hedit,EM_LINEFROMCHAR,start,0);
		lindex=SendMessage(win->hedit,EM_LINEINDEX,line,0);
		start-=lindex;
		if(start>=0){
			char str[1024]={0};
			str[sizeof(str)-1]=0;
			((WORD*)str)[0]=sizeof(str);
			SendMessage(win->hedit,EM_GETLINE,line,str);
			tab_word[0]=0;
			if(get_substr(str,start,tab_word,sizeof(tab_word),&tab_pos)){
				tab_continue=TRUE;
				printf("substr=%s\n",tab_word);
				create_intellisense(win);
				if(win->hintel!=0){
					populate_intel(win,tab_word);
				}
			}
			else{
				destroy_intellisense(win);
			}
		}
	}
	return tab_continue;
}



int get_substr(char *str,int start,char *substr,int size,int *pos)
{
	int i,index,len,found=FALSE;
	len=strlen(str);
	if(start<=len){
		if(start>0 && str[start-1]<=' ' && str[start]<=' ')
			return found;
		if(start==0 && str[0]<=' ')
			return found;

		while(start>0){
			start--;
			if(str[start]<=' '){
				start++;
				break;
			}
		}
		*pos=start;
		len=strlen(str+start);
		index=0;
		for(i=0;i<len;i++){
			if(str[start+i]<=' '){
				break;
			}
			else
				substr[index++]=str[start+i];
			if(index>=(size-1))
				break;
		}
		substr[index++]=0;
		if(substr[0]!=0)
			found=TRUE;
	}
	return found;
}

int find_win_by_hedit(HWND hedit,TABLE_WINDOW **win)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hedit==hedit){
			*win=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
int replace_current_word(TABLE_WINDOW *win,char *str)
{
	int start=0,end=0,line;
	if(win==0)
		return FALSE;
	if(win->hedit==0)
		return FALSE;
	SendMessage(win->hedit,EM_GETSEL,&start,&end);
	if(end<start)
		start=end;
	line=SendMessage(win->hedit,EM_LINEFROMCHAR,start,0);
	if(line>=0){
		char s[1024];
		int len,lindex;
		((WORD*)s)[0]=sizeof(s);
		len=SendMessage(win->hedit,EM_GETLINE,line,s);
		lindex=SendMessage(win->hedit,EM_LINEINDEX,line,0);
		start-=lindex;
		if(start>=0 && len>0){





			SendMessage(win->hedit,EM_REPLACESEL,TRUE,str);
			return TRUE;
		}


	}
	

}
int insert_selection(TABLE_WINDOW *win)
{
	if(win!=0 && win->hintel!=0){
		int sel=SendMessage(win->hintel,LB_GETCURSEL,0,0);
		if(sel>=0){
			int len;
			char str[256]={0};
			len=SendMessage(win->hintel,LB_GETTEXTLEN,sel,0);
			if(len<sizeof(str)){
				SendMessage(win->hintel,LB_GETTEXT,sel,str);
				if(str[0]!=0){
					return replace_current_word(win,str);
				}
			}
		}
	}
	return FALSE;
}


static WNDPROC wporigtedit=0;
LRESULT APIENTRY sc_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_ERASEBKGND)
		//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("e");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_CHAR:
		switch(wparam){
		case VK_ESCAPE:
			break;
		default:
			PostMessage(hwnd,WM_USER,wparam,lparam);
			if(wparam==VK_SPACE && (GetKeyState(VK_CONTROL)&0x8000))
				return 0;
			break;
		}
		break;
	case WM_KEYFIRST:
		{
		TABLE_WINDOW *win=0;
		find_win_by_hedit(hwnd,&win);
		if(win==0)
			break;
		switch(wparam){
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_UP:
		case VK_DOWN:
			if(win->hintel!=0)
				SendMessage(win->hintel,msg,wparam,lparam);
			return 0;
			break;
		case VK_RETURN:
			{
				int result=insert_selection(win);
				destroy_intellisense(win);
				if(result)
					return 0;
			}
			break;
		case VK_ESCAPE:
			destroy_intellisense(win);
			return 0;
			break;
		default:
			break;

		}
		}
		break;
	case WM_USER:
		{
		TABLE_WINDOW *win=0;
		find_win_by_hedit(hwnd,&win);
		if(win!=0)
			handle_intellisense(win,wparam);
		}
		break;
	}
	
    return CallWindowProc(wporigtedit,hwnd,msg,wparam,lparam);
}
int subclass_edit(HWND hedit)
{
	wporigtedit=SetWindowLong(hedit,GWL_WNDPROC,(LONG)sc_edit);
	printf("subclass=%08X\n",wporigtedit);
}