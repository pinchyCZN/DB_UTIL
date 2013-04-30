

char tab_word[20]={0};
int tab_continue=FALSE,tab_pos=0;

int handle_intellisense(HWND hwnd)
{
	TABLE_WINDOW *win=0;
	find_win_by_hwnd(hwnd,&win);
	if(win!=0){
		char str[1024]={0};
		str[sizeof(str)-1]=0;
		if(GetWindowText(win->hedit,str,sizeof(str)-1)>0){
			int start=0,end=0;
			SendMessage(win->hedit,EM_GETSEL,&start,&end);
			if(end<start)
				start=end;

			tab_word[0]=0;
			if(get_substr(str,start,tab_word,sizeof(tab_word),&tab_pos)){
				tab_continue=TRUE;
				printf("substr=%s\n",tab_word);
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

