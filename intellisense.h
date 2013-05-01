

char tab_word[20]={0};
int tab_continue=FALSE,tab_pos=0;

int create_intellisense(HWND hwnd)
{
	TABLE_WINDOW *win=0;
	find_win_by_hwnd(hwnd,&win);
	if(win!=0 && win->hintel==0){
		win->hintel = CreateWindow("LISTBOX",
										 "",
										 WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LBS_HASSTRINGS|LBS_SORT|LBS_STANDARD,
										 0,0,
										 80,20,
										 win->hwnd,
										 IDC_INTELLISENSE,
										 ghinstance,
										 NULL);
		if(win->hintel!=0){
			SetWindowPos(win->hintel,HWND_TOP,0,30,80,120,SWP_SHOWWINDOW);
			return TRUE;
		}
	}
	return FALSE;
}
int destroy_intellisense(HWND hwnd)
{
	TABLE_WINDOW *win=0;
	find_win_by_hwnd(hwnd,&win);
	if(win!=0 && win->hintel!=0){
		DestroyWindow(win->hintel);
		win->hintel=0;
		return TRUE;
	}
	return FALSE;
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
	}
}
int handle_intellisense(HWND hwnd)
{
	TABLE_WINDOW *win=0;
	find_win_by_hwnd(hwnd,&win);
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
				create_intellisense(hwnd);
				if(win->hintel!=0){
					populate_intel(win,tab_word);
				}
			}
			else{
				destroy_intellisense(hwnd);
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

