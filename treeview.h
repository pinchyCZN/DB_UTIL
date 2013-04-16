int draw_tree(HWND hwnd,DRAWITEMSTRUCT *di)
{
	FillRect(di->hDC,&di->rcItem,(HBRUSH)(COLOR_BTNHIGHLIGHT+1));
	DrawEdge(di->hDC,&di->rcItem,EDGE_SUNKEN,BF_RECT);
	return 0;
}

LRESULT CALLBACK treeview_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg){
	case WM_DRAWITEM:
		draw_tree(hwnd,(LPDRAWITEMSTRUCT)lparam);
		return TRUE;
	}
	return DefWindowProc(hwnd,msg,wparam,lparam);
}
