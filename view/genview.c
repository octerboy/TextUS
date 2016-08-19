#include <unistd.h>
#include <tinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <errno.h> 
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/select.h>
#include <fcntl.h>
#include  <stdlib.h>
#include  <signal.h>
#include  <ctype.h>
#include "../include/window.h"
#include "../include/gendisp.h" 
#include "../include/jUtil.h"
#include "../include/udpcheck.h"
#include "../include/readdisp.h"
#define	MAXMAP 200
char   pathName[100];
char	Sleep_time[12],info[200];
int 	dtime,numwin=0,startflag=0,debugflag=0,iacceptflag=1,t=0,interrupt=0;
char	*pwin[100],sndwin[100],LogFile[20];
char    *dispResult; 
int 	act_key();
win_inf   wininf;
Tcl_Interp *interp; 
FILE	*fp;
struct stortkeymap {
	char	shortkey[20];
	char	keyvalue;    
	int	no;	     //所处的位置
	int	selected;    //使用标志
}shortkeymap[MAXMAP];
/**把小写字母转换成大写*/
LowerToUpper( char *string  )
{
	char * pcP;
	for(pcP=string;*pcP!=NULL;pcP++)
        {
        	if( *pcP >= 'a' && *pcP <= 'z' ) *pcP = *pcP - 32;
            	*string=*pcP;
            	string++;
        }
        *string=NULL;

}/**数据主处理
 输入参数：存放数据的结构指针、存放数据的结构指针、交易类型
	输出参数：无
	返回值  ：无		
 */
Data_Proc( J_showList* showline,gen_screen* shead,char *trantype)
{
	FILE *kk;
	gen_win      *whead,*wbody,*win,*wintmp;
        gen_win	*w1,*w2,*w3,*w4;
        gen_disp *gdisp;
        gen_screen* sheadtmp;
        J_showList* showlinetmp;
        int recnum=0,firstflag=0,i,wini,disp_flag,looptimes;
        char 	translate[50],transvalue[50],field_name[30],field_value[30];
        char    *objHandle;               /*数据特征判断句柄*/
	             /*数据特征判断结果*/
       	win_disp *wdisp;
        J_showLine *lhead,*lbody,*ltail,*freeline,*jhead,*jheadtmp;
        disp_line *disphead,*dispbody,*disptail,*disptmp,*dispstore;
        data_show *showhead,*showhead1;
        gen_field *fhead,*fheadtmp;//窗口中显示的列名安顺序
		
	int ishowhead=0,ishowline=0,j,k,j1,l;
	char	TranType[60];
	sheadtmp = shead;
	showlinetmp=showline;
	wini=0;
	memset(sndwin,0,100);
	while(shead){
		
		win=shead->multi_win;
		wintmp = win;
		while(win){
			wdisp=win->multi_disp;
			//gdisp=win->multi_disp->disp_addr;
			while(wdisp){//#####
			gdisp=wdisp->disp_addr;
			if(debugflag)
			{	
				fprintf(fp,"比较数据特征句柄dispname[%s]dispResult[%s]\n",gdisp->disp_name,dispResult);
				fflush(fp);
			}
			if(strstr(gdisp->disp_name,dispResult)!=NULL)
			{
			sndwin[wini]='1';
			showline=showlinetmp;
			while(showline){//  save show unit
			//to do ......add to window line
			ishowline++;
			ltail=malloc(sizeof(J_showLine));
			if(firstflag==0){//如果是第一个数据元
				
				lhead=ltail; //保存数据元链首地址
				lbody=ltail;
				
			}
			if(strlen(showline->field_name)==0||strlen(showline->value)==0)
			{
				showline=showline->next;
				continue;
			}
				
			
			ltail->field_name=malloc(strlen(showline->field_name)+10);
			ltail->value=malloc(strlen(showline->value)+10);
			
			showhead = gdisp->gen_data_show;
			LowerToUpper( showline->field_name );//字母转换成大写
			while(showhead){
				LowerToUpper( showhead->data_name );
				strcpy(field_name,showline->field_name);
				//找交易类型
				if(!strcmp(showhead->data_name,"TRANTYPE"))
					strcpy(TranType,showhead->d_field_name);
				if(!strcmp(showline->field_name,showhead->data_name))
				{
					
					strcpy(showline->field_name,showhead->d_field_name);
					if(showhead->translate ==1 )
					{//要翻译
					
						strcpy(translate,"translation,");
						strcat(translate,showhead->data_name);
						strcpy(transvalue,showline->value);
						ReadTrans(pathName,translate,transvalue);//取对应关系
						if(debugflag )
						{
							fprintf(fp,"recode[%s]\n",transvalue);
							fflush(fp);
						}
						strcpy(ltail->value,transvalue);
					}else
						strcpy(ltail->value,showline->value);	
					strcpy(ltail->field_name,showline->field_name);
				}			
					showhead=showhead->next;
				//取数据元值为下面SERIAL比较用
				if(!strcmp(showline->field_name,gdisp->serial_name))
					strcpy(field_value,showline->value);
				strcpy(showline->field_name,field_name);
					
				}//while showhead end
				if(firstflag==1)
				{///如果不是第一个数据元，则加入链表中
					lbody->next=ltail;
			    		lbody=ltail; 
					
				}
    				else
	    				firstflag=1;
	    			
		    		ltail->next=NULL;
			    	ltail=NULL;
			       
			    	showline=showline->next;
			    	
			  }//while showline end
			/***********加入交易类型********************/
			fheadtmp=fhead=win->multi_field;//要显示的内容
			ltail=malloc(sizeof(J_showLine));
		
			ltail->value=malloc(20);
			ltail->field_name=malloc(20);
			while(fhead){
			if(!strcmp(fhead->field_name,TranType))
			{
				strcpy(ltail->field_name,TranType);
				strcpy(transvalue,trantype);
					
				ReadTrans(pathName,"tranid_tab",transvalue);
				strcpy(ltail->value,transvalue);
				break;
			}
			fhead=fhead->next;
			}
			lbody->next=ltail;
			lbody=ltail; 
			ltail->next=NULL;
			ltail=NULL;
			/********************************/
			//填屏幕行与列
			
			disptail=malloc(sizeof(disp_line));
			disptail->disp_name=malloc(strlen(wdisp->disp_name+1));
			strcpy(disptail->disp_name,wdisp->disp_name);
			disptail->showLine=lhead;
			disptail->next=NULL;
			disphead = win->multi_line;
			disp_flag=0;//判断是否做常规处理
			if(win->numline==0)
			{
                                win->multi_line=disptail;
				disptail->next=NULL;
			}
                        else
                        {
				//处理显示的特除情况 
				switch(gdisp->lineoverlap)
				{
					case SERIAL:
						disptmp=disphead;
						jheadtmp=jhead;
						dispstore = disphead;
						looptimes=0;
               					while(disphead){
               					jheadtmp=jhead=disphead->showLine;//数据存放列
                			        while(jhead){
                       				 if(!memcmp(jhead->field_name,gdisp->serial_name,strlen(jhead->field_name)))//比较数据元
							if(!strcmp(field_value,jhead->value))//比较数据元值
							{
								if(looptimes==0)
								{
								win->multi_line=disptail;
								disptail->next=disphead->next;
								} else {
								dispstore->next=disptail;
								disptail->next=disphead->next;
								}
								disp_flag=1;
								break;
							}
                        				jhead=jhead->next;
                        				}
						if(disp_flag==1) break;
						dispstore = disphead;
        				        disphead=disphead->next;
						looptimes++;
                				}
						disphead=disptmp;
						jhead=jheadtmp;
						break;	
					case ONCE:
						break;	
					case TWICE:
						if(win->numline%2==1)
						{		
						disptmp=disphead;
						dispstore=disphead;
						looptimes=0;
               					while(disphead){
						if(disphead->next==NULL)
						{
							if(looptimes==0){
							win->multi_line=disptail;
							disptail->next=disphead->next;
							} else {
							dispstore->next=disptail;
							disptail->next=disphead->next;
							}
							disp_flag=1;
							break;
						}
						dispstore=disphead;
        				        disphead=disphead->next;
						looptimes++;
                				}
						disphead=disptmp;
						}
						break;	
					case STATIC:
						disptmp=disphead;
						dispstore=disphead;
						looptimes=0;
               					while(disphead){
						if(!strcmp(disphead->disp_name,wdisp->disp_name))
						{
							if(looptimes==0){
							win->multi_line=disptail;
							disptail->next=disphead->next;
							} else {
							dispstore->next=disptail;	
							disptail->next=disphead->next;
							}
							disp_flag=1;
							break;
						}
						dispstore=disphead;
        				        disphead=disphead->next;
						looptimes++;
                				}
						disphead=disptmp;
						break;	
				}  

				if(disp_flag == 0){//常规处理
                        	while(disphead)
                        	{	//覆盖方式显示
                        		if(win->win_roll == TRUE&&win->numline >= win->win_buffer_no)
                        		{	j=0;
                        			k=win->numline%win->win_buffer_no;
                        			while( j < k )
                        			{
                        				j1=j;
                        					if( j1++ != k )
                        						disptmp = disphead;
                        					disphead=disphead->next;
                        					
                        				j++;
                        			}
                        			if( k == 0 )
                        				win->multi_line=disptail;
                        			else
                        				disptmp->next=disptail;
                        				
                        			disptail->next=disphead->next;
                        			free( disphead);
                        			break;	
                        		}//翻屏方式显示
                        		if(disphead->next==NULL)
                        		{
                        		       
                        		        disphead->next=disptail;
                        		 	break;
                        		}
                        		disphead=disphead->next;		
                        	}
				}//end disp_flag
                               
                        }
                        win->numline=win->numline+1;
			if(disp_flag == 0)
                       	if( win->numline > win->win_buffer_no &&win->win_roll == FALSE ){//向前推移一个向上翻屏
				freeline=win->multi_line;
				win->multi_line=win->multi_line->next;
				free( freeline );
                        }
			}//if strstr end
			wdisp=wdisp->next;
			}//#####	
			win=win->next;
			if( win!=NULL )
				wintmp = wintmp->next;
			wini++;//判断发往那些屏目
			//showline=showlinetmp;
			}//wille win end
			win = wintmp;
			shead=shead->next;
			}//while shead end
			shead = sheadtmp;
			//showline=showlinetmp;
			
}
/*----------------------------------------------------------------------*/
int Write_Log(gen_screen* screen,int winflag)
{	
	FILE *pp;
	int x,y,i=0,len;
	gen_field* fhead,*fheadtmp;
	disp_line* lhead;
	J_showLine *jhead,*jheadtmp;
	data_show *showhead,*showtmp;
	gen_disp *gdisp;
	gen_win *genwin;
	
	char *p2,align,agBuf[30],logfile[20];
	if( debugflag != 1 )
		return( 0 );
	strcpy(logfile,LogFile);
	strcat(logfile,".");
	sprintf(logfile+strlen(logfile),"%d",winflag);//不同的窗口写到不同的文件
	pp=fopen(logfile,"a");
	if(genwin->multi_line!=NULL)
	{	
		fheadtmp=fhead=fheadtmp=genwin->multi_field;
		lhead=genwin->multi_line;//数据存放的行
		gdisp=genwin->multi_disp->disp_addr;
		while(lhead)
		{
			if( lhead->next == NULL )
				break;
			lhead=lhead->next;
		}
		jheadtmp=jhead=lhead->showLine;//数据存放列
		while(fhead)
		{
			while(jhead){
		
			if(strstr(jhead->field_name,fhead->field_name) != NULL)
			{	
				if((p2=strchr(jhead->value,'\01'))!=NULL)//去掉Ctrl-A
					*p2=0;
				break;
			}
			jhead=jhead->next;
			
			}
			jhead=jheadtmp;
			fhead=fhead->next;
			
		}
	}	
	fclose(pp);
}
/*-------------------------------------------------------*/
void    Drawline(int y, int x,int n,int sp)
{
    int     cunt;
    char    st[81],ch1[3],ch2[3],ch3[3];

    switch(sp)
    {
        case 0: strcpy(ch1,C_LXT_C);
            strcpy(ch2,"  ");
            strcpy(ch3,ch1);
            break;
        case 1: strcpy(ch1,C_ZSJ_C);
            strcpy(ch2,C_HXT_C);
            strcpy(ch3,C_YSJ_C);
            break;
        case 2: strcpy(ch1,C_ZZY_C_X);
            strcpy(ch2,C_HXT_X);
            strcpy(ch3,C_YZZ_C_X);
            break;
        case 3: strcpy(ch1,C_ZXJ_C);
            strcpy(ch2,C_HXT_C);
            strcpy(ch3,C_YXJ_C);
            break;
        case 4: strcpy(ch1,C_LXT_X);
            strcpy(ch2,"  ");
            strcpy(ch3,ch1);
            break;
        case 5: strcpy(ch1,C_ZSJ_X);
            strcpy(ch2,C_HXT_X);
            strcpy(ch3,C_YSJ_X);
            break;
        case 6: strcpy(ch1,C_ZZY_X);
            strcpy(ch2,C_HXT_X);
            strcpy(ch3,C_YZZ_X);
            break;
        case 7: strcpy(ch1,C_ZXJ_X);
            strcpy(ch2,C_HXT_X);
            strcpy(ch3,C_YXJ_X);
            break;
    }
    strcpy(st,ch1);
    for(cunt=1;cunt<=n;cunt++) 
        strcat(st,ch2);
    strcat(st,ch3);
    mvaddstr(y,x,st);
    refresh();
}
/*----------------------------------------------------------------------*/
void    Drawbox( int hs,int ls,int start_y,int start_x, int head)
{
    int num,count,y,flag;

    num=(ls-4)/2;
    y=start_y;
    flag = 0;
    if ( head > 3 )
    {
        head = head - 4;
        flag = 1;
    }
    Drawline(y++,start_x,num,1+flag*4);
    for(count=2;count<hs;count++)
    {
        if( (head==1 && count==3) || (head==2 && count==hs-2)
        || (head>=3 && (count==3 || count==hs-2)) )
            Drawline(y++,start_x,num,2+flag*4);
        else    
            Drawline(y++,start_x,num,0+flag*4);
    }
    Drawline(y++,start_x,num,3+flag*4);
}
/*----------------------------------------------------------------------*/
/*   thick--画线宽度  0-表示薄 1-表示厚 */
/*   direction--画线的方向，0-竖线  1-横线   */
void win_line(int x,int y,int length,int direction,int thick)
{
   char line[81],head[3],body[3],tail[3];
   int i;
   
   //从配置文件取得窗口最长的参数，并判断
   //要画的线是否超过此长度

   if(direction){
	   if(thick){//是粗横线
	       strcpy(head,C_ZZY_C);
    	   strcpy(body,C_HXT_C);
     	   strcpy(tail,C_YZZ_C);
	   }
	   else{//细横线
	       strcpy(head,C_ZZY_C_X);
    	   strcpy(body,C_HXT_X);
     	   strcpy(tail,C_YZZ_C_X);
	   }
   }
   else{
	   if(thick){//是粗竖线
		   mvaddstr(y,x,C_HZX_C);
		   for(i=0;i<length;i++)
			   mvaddstr(y,++x,C_LXT_C);
		   mvaddstr(y,x,C_HZS_C);

		   return;
	   }
	   else{//是细竖线
	   }
   }
   strcpy(line,head);
   for(i=0;i<length/2;i++)
	   strcat(line,body);
   strcat(line,tail);
 
   mvaddstr(y,x,line);  //画线
}
/**
交易数据显示屏幕 
输入参数：screen窗口指针.
	输出参数：无
	返回值  ：无
**/

void disp_Data(gen_screen* screen)
{	
	FILE *pp;
	int x,y,i=0,len;
	gen_field* fhead,*fheadtmp;
	disp_line* lhead;
	J_showLine *jhead,*jheadtmp;
	data_show *showhead,*showtmp;
	gen_disp *gdisp;
	gen_win* genwin;
	char *p2,align,agBuf[30],LogFile[20];
	genwin=screen->multi_win;
	while(genwin)
	{	
		
		fheadtmp=fhead=fheadtmp=genwin->multi_field;
		lhead=genwin->multi_line;//数据存放的行
		gdisp=genwin->multi_disp->disp_addr;
		x=genwin->win_startx+2;
		y=genwin->win_starty+2;
		while(lhead){
		jheadtmp=jhead=lhead->showLine;//数据存放列
		while(fhead)
		{
			
			while(jhead){
			if(!memcmp(jhead->field_name,fhead->field_name,strlen(fhead->field_name)))
			{	
				if((p2=strchr(jhead->value,'\01'))!=NULL)//去掉Ctrl-A
					*p2=0;
				/*=========显示方式（R-M-L）==========*/
				showtmp=showhead=gdisp->gen_data_show;
				
				while(showhead){
					if(strstr(jhead->field_name,showhead->d_field_name)!=NULL)
					{
						align = showhead->align;
						break;
					}				
				showhead=showhead->next;
				}//while showhead end
				memset(agBuf,0,30);
				if(strlen(jhead->value)>fhead->field_length)
					jhead->value[fhead->field_length]=0;
				if( align == 'M' )
				{
					len=(fhead->field_length-strlen(jhead->value))/2;
					memcpy(agBuf,"\x20\x20\x20\x20\x20\x20\x20\x20",len);
					memcpy( agBuf+len,jhead->value,strlen(jhead->value));
					memcpy( agBuf+len+strlen(jhead->value),"\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20",len);
					agBuf[len+strlen(jhead->value)+len]=0;
				}
				else if( align == 'R' )
				{
					len=fhead->field_length-strlen(jhead->value);
					strncpy(agBuf,"\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20",len);
					strncpy(agBuf+len,jhead->value,strlen(jhead->value));
				}
				else strcpy(agBuf,jhead->value);
				
				/*=================================*/
				mvaddstr(y+1,x,agBuf);
				x=x+fhead->field_length+fhead->field_space;
				
				break;
			}
			if( jhead->next == NULL )
				x=x+fhead->field_length+fhead->field_space;
			jhead=jhead->next;
			
			}
			jhead=jheadtmp;
			fhead=fhead->next;
		}
		y=y+1;
		x=genwin->win_startx+2;
		lhead=lhead->next;
		fhead=fheadtmp;
		}
		genwin=genwin->next;
	}
	
}	
/*-------------------------------------------------------*/
/*  根据窗口结构画窗口
    输入参数：窗口结构、窗口类型
	输出参数：无
	返回值  ：无										 */
	
void draw_win(gen_screen* screen,int style,char *info)
{
	FILE *pp;
	int x,y,i,len;
	char agBuf[30];
	gen_field* fhead,*fheadtmp;
	disp_line* lhead;
	J_showLine *jhead,*jheadtmp;
	gen_win *genwin;
	if(genwin==NULL)
		exit(0);
	//画窗口
	memset(agBuf,0,30);
	genwin=screen->multi_win;
	while(genwin){
	if(genwin->win_border)
		Drawbox(genwin->win_height,genwin->win_width,genwin->win_starty,
		        genwin->win_startx,style);

	if(genwin->win_title!=NULL){//显示窗口标题
    	x=(genwin->win_width-strlen(genwin->win_title))/2+genwin->win_startx; 
    	mvaddstr(genwin->win_starty-1,x,genwin->win_title);
	}
	//窗口脚注
	if(!strcmp(info,"noupdateinfo")){
	if(genwin->win_field_line){
		x=(genwin->win_width-strlen(screen->scr_bottom))/2+genwin->win_startx; 
		mvaddstr(genwin->win_starty+genwin->win_height,x,screen->scr_bottom);
	}
	}else if(genwin->win_field_line && strcmp(info,"nowriteinfo")){
		x=(genwin->win_width-strlen(screen->scr_bottom))/2+genwin->win_startx; 
		mvaddstr(genwin->win_starty+genwin->win_height,2,info);
	//	mvaddstr(genwin->win_starty+genwin->win_height,x,info);
	}
	if(genwin->multi_field!=NULL){
		fheadtmp=fhead=genwin->multi_field;
		x=genwin->win_startx+2;
		y=genwin->win_starty+1;
		while(fhead){//显示所有字段
			
            		/*****/
            		if(strlen(fhead->field_name)>fhead->field_length)
					fhead->field_name[fhead->field_length]=0;
			len=(fhead->field_length-strlen(fhead->field_name))/2;
			memcpy(agBuf,"\x20\x20\x20\x20\x20\x20\x20\x20",len);
			memcpy( agBuf+len,fhead->field_name,strlen(fhead->field_name));
			memcpy( agBuf+len+strlen(fhead->field_name),"\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20",len);
			agBuf[len+strlen(fhead->field_name)+len]=0;
			/*****/
			mvaddstr(y,x,agBuf);
			x=x+fhead->field_length+fhead->field_space;
            		genwin->win_starty; 
			fhead=fhead->next;
		}
	}
	genwin=genwin->next;
	}
	
	
	disp_Data( screen ); 
	refresh();
}

/*----------------------------------------------------------------------*/

/*-------------------------------------------------------*/
/*   判断一个字符是否包括在一个串中   */
int chin_str(char* aid,char ch)
{
	register char *p,*end;

	//if(aid=NULL) return FALSE;
	end=aid+strlen(aid);
	p=aid;
	while(p<end){
		if(*p==ch)
			return TRUE;
		p++;
	}
	return FALSE;
}
/*-------------------------------------------------------*/
/*   将字符串中的所有空格去掉   */
char* erase_blank(char* s)
{
	char *p,*end,temp[MAXLINE*4];
	int i=0;

	if(s==NULL) return NULL;
	end=s+strlen(s);
	for(p=s;p<end;p++){
		if(!IsTRIM(*p)){
			temp[i++]=*p;
		}
	}
	temp[i]='\0';
	return temp;
}
/*-------------------------------------------------------*/
/*   判断s2是否在s1中，都不包括空格   */
/*   mode 0--不区分大小写，1--区分大小写   */
int sin_str(char* s1,char* s2,int mode)
{
	char temp[MAXLINE]; //保存去掉空格后的s1
	char *p,*end;
	int i=0;

	if(s2==NULL) return TRUE;
	if(s1==NULL) return FALSE;

	//将s1中的空格全部去掉保存在temp中
	strcpy(temp,erase_blank(s1));
	//转化
	if(!mode){ //如果不区分大小写,将两个字符串转化为大写比较(也可转化为小写)
		end=temp+strlen(temp);
		for(p=temp;p<end;p++)
			*p=toupper(*p);
		end=s2+strlen(s2);
		for(p=s2;p<end;p++)
			*p=toupper(*p);
	}
	//开始比较
    if(strstr(temp,s2)!=NULL)
       	return TRUE;
   	else
   		return FALSE;
}
/*-------------------------------------------------------*/
/*   将一个字符串中的指定字符替换为给定字符，替换后的字串
     仍然是原来的字串
	 输入参数：要处理的字串、被替换字符、替换字符
	 输出参数：该字串
	 返回值  ：无										 */
void rep_str(char* s,char aid, char des)
{
	char *p,*end;

	end=s+strlen(s);
	for(p=s;p<end;p++){
		if(*p==aid)
			*p=des;
	}
}
/*-------------------------------------------------------*/
/*   处理得到的屏幕描述字符串，将其写到一个屏幕描述结构中，
     同时生成该屏幕中窗口的指针链，其地址包括在屏幕结构中。
     输入参数：屏幕描述信息的字串
	 输出参数：无
	 返回值  ：屏幕结构地址                              */

gen_screen* make_aScreen(char* state)
{
	gen_screen* ascreen;               //生成的屏幕结构
	gen_win     *whead,*wbody,*wtail;  //窗口结构链
	char        *p,*end;
	char        word[MAXLINE],wkey[MAXLINE];   //一个关键词、存放得到的小写关键词
	int         i=0,j,firstWin=1;      //是否第一个窗口
	int         iwin=-1;               //window定义的累加记数(值0、1、2、3)-1表示读到没有窗口
	 
	end=state+strlen(state);
	ascreen=malloc(sizeof(gen_screen));
	//逐个遍历描述字串，找出关键词填到屏幕及窗口结构中
	for(p=state;p<end;p++){
		if(*p=='='){//忽略第一个等号左边的字符，因为是屏幕名称
			break;
		}
	}
	while(p<end+1){
		if(p==end)//最后一个关键词的处理
			*p='=';
		switch(*p){
    		case ',':{//已经取到一个描述关键词
    			word[i++]='\0';
				if(strcmp(wkey,"window")==0){//如果一个window定义开始
					wtail=malloc(sizeof(gen_win));
					if(firstWin){//如果是第一个窗口
						whead=wtail; //保存窗口链首地址
						wbody=wtail;
					}
					iwin=0;
				}
				if(iwin>=0){//如果有窗口开始在读
					switch(iwin){
    					case 0:{//窗口名称
							wtail->win_title=malloc(strlen(word)+1);
							strcpy(wtail->win_title,word);
							break;
							   }
    					case 1:{//窗口坐标X
							wtail->win_startx=atoi(word);
							break;
							   }					
    					case 2:{//窗口坐标Y
							wtail->win_starty=atoi(word);
							break;
							   }
    					case 3:{//窗口宽度
							wtail->win_width=atoi(word);
							break;
							   }
					}
					iwin++;
				}
				//将word转化为全小写
				for(j=0;j<i;j++)
					word[j]=tolower(word[j]);
				strcpy(wkey,word);
				i=0;
				break;
					 }
	    	case '=':{//已经取到一个该描述关键词的值
     			word[i++]='\0';
				
				i=0; 
				if(strcmp(wkey,"shortkey")==0){//如果是屏幕快捷键
					ascreen->scr_shortkey=malloc(strlen(word)+1);
					strcpy(ascreen->scr_shortkey,word);
					break;
				}
				if(strcmp(wkey,"title")==0){//如果是屏幕标题
					ascreen->scr_title=malloc(strlen(word)+1);
					strcpy(ascreen->scr_title,word);
					break;
				}
				if(strcmp(wkey,"bottom")==0){//如果是屏幕底部标题
					ascreen->scr_bottom=malloc(strlen(word)+1);
					strcpy(ascreen->scr_bottom,word);
					break;
				}
				if(strcmp(wkey,"frame")==0){//如果是屏幕边框信息
					if(sin_str(word,"yes",0))
						ascreen->scr_border=TRUE;
					else
						ascreen->scr_border=FALSE;
					break;
				}
				//否则，一定是某个窗口的最后一个参数——(窗口高度)被读进来
				if(iwin>=0){
    				wtail->win_height=atoi(word);
	    			if(firstWin==0){//如果不是第一个窗口，则加入链表中
		    			wbody->next=wtail;
			    		wbody=wtail; 
					}
    				else
	    				firstWin=0;
		    		wtail->next=NULL;
			    	wtail=NULL;
				    iwin=-1;//该窗口读完
				}
				break;
					 }
			default:{word[i++]=*p;break;}
		}
		p++;
	}
	ascreen->multi_win=whead;
	return ascreen;
}
/*-------------------------------------------------------*/
/*   从配置文件中读取屏幕信息并返回一个链表首地址        
     输入参数：配置文件句柄
	 输出参数：无
	 返回值  ：屏幕链表首地址                            */

gen_screen* read_screen(FILE* fp)
{
	//生成屏幕链的几个指针
	gen_screen *shead,*sbody,*stail;
	//依此保存读到的一行数据、没有空格的一行数据、没有空格的所有一屏幕定义数据
	char aline[MAXLINE],noBlank[MAXLINE],totalLine[MAXLINE*4];
	char ch[2],num[10];  //临时变量，用来计算屏幕个数
	int sNum,firstScr=1,i;   //屏幕的个数、是否第一个屏幕
	
	//将文件指针返回文件头
	rewind(fp);
	//如果文件句柄为空，则退出
	if(fp==NULL) return NULL;
	//找到屏幕定义部分
	while(fgets(aline,MAXLINE,fp)){ 
		//不区分大小写比较是否为屏幕定义开始
		if(sin_str(aline,"[screens]",0)){
			break;
		}
	}
	//处理屏幕定义的个数和快捷键
    while(fgets(aline,MAXLINE,fp)){
		//如果找到屏幕个数定义
		if(sin_str(aline,"total",0)){ 
			strcpy(noBlank,erase_blank(aline)); 
			strcpy(aline,strstr(noBlank,"=")); 
			ch[0]=aline[1];
			ch[1]='\0';
			strcpy(num,strstr(aline,ch)); 
			sNum=atoi(num); 
			break;
		}
	}
NEXTSCR:
	//将屏幕描述串清空
	strcpy(totalLine,"\0");
	//找到第一个具体屏幕定义开始
	while(fgets(aline,MAXLINE,fp)){
		//如果是某个具体屏幕定义开始,这里假设屏幕定义第一行一定有'\'字符
		if(chin_str(aline,'\\')){
			break;
		}
		//如果已经到了窗口的定义部分，则表示屏幕定义结束（假设屏幕定义完一定是窗口定义）
		if(sin_str(aline,"[windows]",0) ||feof(fp)){
			return shead;
		}
	}
    //获取一个屏幕定义信息
	strcpy(noBlank,erase_blank(aline));
	//加入屏幕定义第一行
	strcat(totalLine,aline);
	while(fgets(aline,MAXLINE,fp)){
		strcpy(noBlank,erase_blank(aline)); 
		//如果该行空，不处理
		if(IsBlank(noBlank))
			continue;
		//如果该行含'\'字符，则不是该屏幕定义结束处
		if(chin_str(noBlank,'\\'))
   			strcat(totalLine,noBlank);
		else
			break;
	}
	//加入屏幕定义最后一行
	strcat(totalLine,noBlank);
	/* 由于得到的字串包含'\'及回车和换行符号，全部用空格来替换 */
	rep_str(totalLine,'\\',' ');
	rep_str(totalLine,0x0a,' ');
	rep_str(totalLine,0x0d,' ');
    strcpy(totalLine,erase_blank(totalLine));
	/*   处理得到的屏幕描述字符串，将其写到一个屏幕描述结构中，
	     同时生成该屏幕中窗口的指针链，其地址包括在屏幕结构中。*/
	if(firstScr){//如果是第一个屏幕的定义
		shead=make_aScreen(totalLine); //保存链表头
		sbody=shead;
		firstScr=0; 
	}
	else{//将该屏幕定义加入到链表中
		stail=make_aScreen(totalLine);
		sbody->next=stail;
		sbody=stail;
		sbody->next=NULL;
		stail=NULL;
	}
	
	goto NEXTSCR;
}
/*-------------------------------------------------------*/
/*   根据窗口描述串填充窗口结构
     输入参数：窗口描述串、屏幕链首地址
	 输出参数：无
	 返回值  ：无                                        */
void make_aWindow(char* state,gen_screen* shead)
{
	gen_win*  wbody;
	gen_field *fhead,*fbody,*ftail;
	win_disp  *dhead,*dbody,*dtail;
	char      word[MAXLINE],wkey[MAXLINE];  //窗口关键词
	char      *p,*end;
	int       i=0,j,iFlag=0,firstField=1,firstDisp=1;
	int       iField=-1;  //窗口字段的属性（-1，0，1）-1表示没有读到字段


	end=state+strlen(state);
	//读出描述串中窗口名称--第一个关键词
	for(p=state;p<end;p++){
		if(*p=='='){
			word[i]='\0'; 
			break;
		}
		word[i++]=*p;
	}
	i=0;
	/*在屏幕链中检索和该窗口名称相同的窗口结构
	  找到要修改的窗口结构地址                  */
	while(shead){
		wbody=shead->multi_win;
		while(wbody){	
			if(strcmp(wbody->win_title,word)==0){
				iFlag=1;
				break;
			}
			wbody=wbody->next;
		}
		shead=shead->next;
		if(iFlag)
			break;
	}

	strcpy(wkey,"\0");
    strcpy(word,"\0");
	//处理窗口的描述到结构中
	while(p<end+1){
		if(p==end)//最后一个关键词的处理
			*p='=';
		switch(*p){
    		case ',':{//已经取到一个描述关键词
    			word[i++]='\0';
				if(strcmp(wkey,"field")==0){//如果一个field定义开始
					ftail=malloc(sizeof(gen_field));
					if(firstField){//如果是第一个字段
						fhead=ftail; //保存字段链首地址
						fbody=ftail;
					}
					iField=0;
				}
				if(iField>=0){//如果有字段开始在读
					switch(iField){
    					case 0:{//字段名称
							ftail->field_name=malloc(strlen(word)+1);
							strcpy(ftail->field_name,word);
							break;
							   }
    					case 1:{//显示宽度
							ftail->field_length=atoi(word);
							break;
							   }					
					}
					iField++;
				}

				//将word转化为全小写
				for(j=0;j<i;j++)
					word[j]=tolower(word[j]);
				strcpy(wkey,word);
				i=0;
				break;
					 }
	    	case '=':{//已经取到一个该描述关键词的值
     			word[i++]='\0';
        			i=0; 
				if(strcmp(wkey,"title")==0){//标题
					free(wbody->win_title);  //因为该出在读屏幕时已经分配来保存窗口名称，所以要先释放
					wbody->win_title=(char *)malloc(strlen(word)+1);
					strcpy(wbody->win_title,word);
					break;
				}
				if(strcmp(wkey,"buffer")==0){//缓冲记录数
					wbody->win_buffer_no=atoi(word);
					break;
				}
				if(strcmp(wkey,"frame")==0){//是否显示边框
					if(sin_str(word,"yes",0))
    					wbody->win_border=TRUE;
					else
						wbody->win_border=FALSE;
					break;
				}

				if(strcmp(wkey,"fieldline")==0){//显示字段否
					if(sin_str(word,"yes",0))
						wbody->win_field_line=TRUE;
					else
						wbody->win_field_line=FALSE;
					break;
				}
				if(strcmp(wkey,"roll")==0){//满屏后显示方式
					if(!strcmp(word,"fresh"))
						wbody->win_roll=TRUE;
					else
						wbody->win_roll=FALSE;
					break;
				}
				if(strcmp(wkey,"display")==0){//显示特征
					//生成显示特征结构链
					dtail=malloc(sizeof(win_disp));
					dtail->disp_name=malloc(strlen(word)+1);
					strcpy(dtail->disp_name,word);
					if(firstDisp){//如果是第一个显示特征
						dhead=dtail;
						firstDisp=0;
					}
					else{
						dbody->next=dtail;
					}
					dtail->disp_addr=NULL;
					dbody=dtail;
					dtail->next=NULL;
					dtail=NULL;
					break;
				}
				if(iField>=0){
    				//如果上一个是在读字段，那就一定是字段描述的最后一个词--"间隔"
	    			ftail->field_space=atoi(word);
		    		if(firstField==0){//如果不是第一个字段，则加入链表中
			    		fbody->next=ftail;
				    	fbody=ftail; 
					}
    				else
	    				firstField=0;
		    		ftail->next=NULL;
			    	ftail=NULL;
				    iField=-1;//该字段读完
				}
				break;
					 }
			default:{word[i++]=*p;break;}
		}
		p++;
		//numwin++;
	}
    //将字段连上窗口中
	wbody->multi_field=fhead; 
	wbody->multi_disp=dhead;
}
/*-------------------------------------------------------*/
/*   读取窗口配置
     输入参数：配置文件句柄、屏幕链表首地址
	 输出参数：无,只改写屏幕链中窗口链的值
	 返回值  ：无                                        */
void read_window(FILE* fp,gen_screen* scrPtr)
{
	gen_win   * wbody;
	char aline[MAXLINE],noBlank[MAXLINE],totalLine[MAXLINE*4];

	rewind(fp);
	//如果文件句柄为空，则退出
	if(fp==NULL) return;
	//找到窗口定义部分
	while(fgets(aline,MAXLINE,fp)){ 
		//不区分大小写比较是否为窗口定义开始
		if(sin_str(aline,"[windows]",0))
			break;
	}
NEXTWIN:
	//将窗口描述串清空
	strcpy(totalLine,"\0");
	//找到第一行具体窗口定义开始
	while(fgets(aline,MAXLINE,fp)){
		//如果是某个具体屏幕定义开始,这里假设屏幕定义第一行一定有'\'字符
		if(chin_str(aline,'\\')){
			break;
		}
		//如果已经到了显示特征的定义部分，则表示窗口定义结束（假设屏幕定义完一定是显示定义）
		if(sin_str(aline,"[display]",0) ||feof(fp)){
			return;
		}
	}
    //获取一个窗口定义信息
	strcpy(noBlank,erase_blank(aline));
	//加入窗口定义第一行
	strcat(totalLine,aline);
	while(fgets(aline,MAXLINE,fp)){
		strcpy(noBlank,erase_blank(aline)); 
		//如果该行空，不处理
		if(IsBlank(noBlank))
			continue;
		//如果该行含'\'字符，则不是该窗口定义结束处
		if(chin_str(noBlank,'\\'))
   			strcat(totalLine,noBlank);
		else
			break;
	}
	//加入该窗口定义最后一行
	strcat(totalLine,noBlank);
	/* 由于得到的字串包含'\'及回车和换行符号，全部用空格来替换 */
	rep_str(totalLine,'\\',' ');
	rep_str(totalLine,0x0a,' ');
	rep_str(totalLine,0x0d,' ');
    
    strcpy(totalLine,erase_blank(totalLine));

	make_aWindow(totalLine,scrPtr); 
	goto NEXTWIN;
}
/*-------------------------------------------------------*/
/*   将一个显示特征配置填充到显示特征结构中
     输入参数：显示特征描述字串、屏幕链首地址
	 输出参数：无
	 返回值  ：无										 */
void make_aDisplay(char* state,gen_screen* scrPtr)
{
	gen_disp*   dbody;                 //该显示特征的地址结构
	gen_win*    wbody;                 //
	win_disp*   pbody;                 //
	data_show   *ahead,*abody,*atail;  //数据元结构链
	char        *p,*end;
	char        dataMark[MAXLINE];     //存放一个显示特征里的所有数据特征
	char        word[100],wkey[100],dispName[MAXLINE];   //一个关键词、存放得到的小写关键词、该显示特征名
	int         i=0,j,firstData=1;     //是否第一个数据元
	int         iData=-1;              //数据元定义的累加记数(值-1、0、1、2)-1表示读到没有数据元
	 
	dbody=malloc(sizeof(gen_disp));
	end=state+strlen(state);
	//找出显示特征名--第一个关键词
	for(p=state;p<end;p++){
		if(*p=='=')
			break;
		dispName[i++]=*p;
		
	}
	dataMark[0]=0;
	dispName[i]='\0';//保存该显示特征的名称
	i=0;
	while(p<end+1){
		if(p==end)//最后一个关键词的处理
			*p='=';
		switch(*p){
    		case ',':{//已经取到一个描述关键词
    			word[i++]='\0';
				if(strcmp(wkey,"show")==0){//如果一个数据元定义开始
					atail=malloc(sizeof(data_show));
					if(firstData){//如果是第一个数据元
						ahead=atail; //保存数据元链首地址
						abody=atail;
					}
					iData=0;
				}
				if(iData>=0){//如果有数据元开始在读
					switch(iData){
    					case 0:{//数据员显示名称
							atail->d_field_name=malloc(strlen(word)+1);
							strcpy(atail->d_field_name,word);
														
							break;
							   }
    					case 1:{//数据员名称
							atail->data_name=malloc(strlen(word)+1);
							strcpy(atail->data_name,word);
							break;
							   }
    					case 2:{//是否翻译
							if(sin_str(word,"yes",0))
							    atail->translate=TRUE;
							else
								atail->translate=FALSE;
							break;
							   }
					}
					iData++;
				}
				//将word转化为全小写
				for(j=0;j<i;j++)
					word[j]=tolower(word[j]);
				strcpy(wkey,word);
				i=0;
				break;
					 }
	    	case '=':{//已经取到一个该描述关键词的值
     			word[i++]='\0';
				i=0; 
				if(strcmp(wkey,"lineoverlap")==0){//数据显示方式
					if(strcmp(word,"serial")==0)
     				    dbody->lineoverlap=SERIAL;
					if(strcmp(word,"static")==0)
					    dbody->lineoverlap=STATIC;
					if(strcmp(word,"once")==0)
					    dbody->lineoverlap=ONCE;
					if(strcmp(word,"twice")==0)
					    dbody->lineoverlap=TWICE;					
					break;
				}
				if(strcmp(wkey,"datacharacter")==0){//数据特征名
					strcat( dataMark,word );
					strcat( dataMark,",");
					break;
				}
				if(strcmp(wkey,"serial_name")==0){//序列号数据元名
					dbody->serial_name=(char*)malloc(strlen(word)+1);
					strcpy(dbody->serial_name,word);
					break;
				}
				//否则，一定是某个窗口的最后一个参数——(窗口高度)被读进来
				if(iData>=0){
    				atail->align=word[0];
	    			if(firstData==0){//如果不是第一个数据元，则加入链表中
		    			abody->next=atail;
			    		abody=atail; 
					}
    				else
	    				firstData=0;
		    		atail->next=NULL;
			    	atail=NULL;
				    iData=-1;//该数据元读完
				}
				break;
					 }
			default:{word[i++]=*p;break;}
		}
		p++;
	}
	//to do
	//赋值数据特征
        dbody->disp_name=(char*)malloc(strlen(dataMark)+1);
	strcpy(dbody->disp_name,dataMark);
	
	
	dbody->gen_data_show=ahead;
	//遍历屏幕链，找出有该显示特征的窗口，将该显示特征的地址赋给相应窗口
	//找出有该显示特征名的显示特征在屏幕链-》窗口链-》显示特征链中的位置
	while(scrPtr){
		wbody=scrPtr->multi_win;
		while(wbody){
			pbody=wbody->multi_disp;
			while(pbody){
				if(strcmp(pbody->disp_name,dispName)==0){
					pbody->disp_addr=dbody;
			 }
				abody=wbody->multi_disp->disp_addr->gen_data_show;
				pbody=pbody->next;
			}
			wbody=wbody->next;
		}
		scrPtr=scrPtr->next;
	}
	
}
/*-------------------------------------------------------*/
/*   读取显示特征配置
     输入参数：配置文件句柄、屏幕链表首地址
	 输出参数：无,只改写屏幕链中显示特征链下面显示特征链的值
	 返回值  ：无                                        */
void read_display(FILE* fp,gen_screen* scrPtr)
{
	gen_win   * wbody;
	char aline[MAXLINE],noBlank[MAXLINE],totalLine[MAXLINE*4];

	rewind(fp);
	//如果文件句柄为空，则退出
	if(fp==NULL) return;
	//找到显示特征定义部分
	while(fgets(aline,MAXLINE,fp)){ 
		//不区分大小写比较是否为显示特征定义开始
		if(sin_str(aline,"[display]",0))
			break;
	}
NEXTDISP: 
	//将显示特征描述串清空
	strcpy(totalLine,"\0");
	//找到第一行具体显示特征定义开始
	while(fgets(aline,MAXLINE,fp)){
		//如果是某个具体显示特征定义开始,这里假设显示特征定义第一行一定有'\'字符
		if(chin_str(aline,'\\')){
			break;
		}
		//如果已经到了数据元名称的定义部分，则表示显示特征定义结束（假设屏幕定义完一定是显示定义）
		if(sin_str(aline,"[end]",0) ||feof(fp)){
			return;
		}
	}
    //获取一个显示特征定义信息
	strcpy(noBlank,erase_blank(aline));
	//加入显示特征定义第一行
	strcat(totalLine,aline);
	while(fgets(aline,MAXLINE,fp)){
		strcpy(noBlank,erase_blank(aline)); 
		//如果该行空，不处理
		if(IsBlank(noBlank))
			continue;
		//如果该行含'\'字符，则不是该显示特征定义结束处
		if(chin_str(noBlank,'\\'))
   			strcat(totalLine,noBlank);
		else
			break;
	}
	//加入该显示特征定义最后一行
	strcat(totalLine,noBlank);
	/* 由于得到的字串包含'\'及回车和换行符号，全部用空格来替换 */
	rep_str(totalLine,'\\',' ');
	rep_str(totalLine,0x0a,' ');
	rep_str(totalLine,0x0d,' ');
    strcpy(totalLine,erase_blank(totalLine));

    //填充显示特征结构
	make_aDisplay(totalLine,scrPtr); 
	goto NEXTDISP;
}
/*-------------------------------------------------------*/
/*  处理一个显示配置文件，处理结果包括屏幕、窗口、显示特征
    及数据元等的结构信息
	输入参数：配置文件名
	输出参数：无
	返回值  ：屏幕的结构链								 */
gen_screen* read_disp_ini(char* pathName)
{
	FILE*       hfile;
	gen_screen* shead;
	gen_win* whead;

	hfile=fopen(pathName,"r");
	if(!hfile){
		printf("open display ini file error!\n");
		return NULL;
	}   
	shead=read_screen(hfile);
	read_window(hfile,shead);
	read_display(hfile,shead);

	fclose(hfile);
	
	return shead;
}
/*-------------------------------------------------------*/
//2001-1-2增加释放函数 by gyb  没有测试

/*释放整个窗口链的指针，包括窗口、字段、窗口中的显示特征、
  显示特征、数据元等链表的释放

  输入参数：窗口链的首地址
  输出参数：无
  返回值  ：成功返回TRUE，失败返回FALSE					 */
int free_win_stru(gen_win* wHead)
{
	gen_win   *wBody;         //窗口
	gen_field *fHead,*fBody;  //字段
	win_disp  *pHead,*pBody;  //窗口中的显示特征
	gen_disp  *dHead,*dBody;  //显示特征
	data_show *aHead,*aBody;  //数据元

	while(wHead){

		//释放该窗口
		if(wHead->win_title!=NULL)
			free(wHead->win_title);
		wBody=wHead;
		wHead=wHead->next;
		free(wBody);
	}
	return TRUE;
}
/*-------------------------------------------------------*/
/*释放整个屏幕链的指针，包括屏幕、窗口、字段、
  窗口中的显示特征、显示特征、数据元等链表的释放

  输入参数：屏幕链的首地址
  输出参数：无
  返回值  ：成功返回TRUE，失败返回FALSE					 */
int free_disp_stru(gen_screen* sHead)
{
	gen_screen* sBody;        //屏幕
	gen_win*    wHead;        //窗口

	while(sHead){
		wHead=sHead->multi_win;
		free_win_stru(wHead);
		if(sHead->scr_shortkey!=NULL)
			free(sHead->scr_shortkey);
		if(sHead->scr_title!=NULL)
			free(sHead->scr_title);
		if(sHead->scr_bottom!=NULL)
			free(sHead->scr_bottom);
		sBody=sHead;
		sHead=sHead->next;
		free(sBody);
	}

	return TRUE;
}
/*----------------------------------------------------------------------*/
/*从一个J_showList结构中获取显示的数据信息，并返回一个显示链地址，本函数
  只处理1个list多个element及每个element下有1个cell的情况                */  

J_showList* make_show_list(J_tranobj* tranobj)
{
	J_showList    *lHead,*lTemp,*lNext;  //构造一行显示链的三个结构指针
	J_dataelement *eleScan;              //遍历所有字段（element）的指针
	int           i,count;

	if(tranobj==NULL)
		return NULL;

	count=tranobj->lists->num_eles;
	if(count>0){
		/*     建立第一个字段   */
		eleScan=tranobj->lists->elements;   //取得第一个element的首地址
	    lHead=malloc(sizeof(J_showList));
    	lNext=lHead;
		lHead->field_name=(char *)malloc(strlen(eleScan->name));    //取得第一个字段名字
		strcpy(lHead->field_name,eleScan->name); 
		if(eleScan->num_cells<1){  //如果没有字段值
			lHead->value=NULL;
		}
		else{// 否则取得第一个字段值
    		lHead->value=(char *)malloc(strlen(eleScan->cells->value)+1); 
		    strcpy(lHead->value,eleScan->cells->value);
		}
		//lHead->row=
		//lHead->col=
		lHead->next=NULL;
		lNext=lHead;

		/*  建立剩下的字段   */
		for(i=0;i<count-1;i++){
			eleScan++;
			lTemp=malloc(sizeof(J_showList));
    	    lTemp->field_name=(char *)malloc(strlen(eleScan->name)+1);    //取得第一个字段名字
		    strcpy(lTemp->field_name,eleScan->name);  
		    if(eleScan->num_cells<1){  //如果没有字段值
		     	lTemp->value=NULL;
			}
       		else{// 否则取得下一个字段值
    	    	lTemp->value=(char *)malloc(strlen(eleScan->cells->value)+1); 
		        strcpy(lTemp->value,eleScan->cells->value);
			}
    		//lHead->row=
    		//lHead->col=
			/*  将新的结构加入链表中   */
     		lNext->next=lTemp;
	    	lNext=lTemp;
    		lTemp=NULL;
		}
        lNext->next=NULL;
		return lHead;
	}
	else
		return NULL;
}
/*----------------------------------------------------------------------*/
void free_cell(J_showList* showlist)
{
        J_showList* snext;

        if(showlist==NULL)
                return;
        while(showlist){
        if(showlist->field_name!=NULL)
                free(showlist->field_name);
        if(showlist->value!=NULL)
                free(showlist->value);
        snext=showlist->next;
        free(showlist);
        showlist=snext;
        }
}

/*   释放一行显示数据的空间   */
int free_showline(J_showLine * showlist)
{
	J_showLine* snext;

	if(showlist==NULL)
		return;
	while(showlist){
    	if(showlist->field_name!=NULL)
    		free(showlist->field_name);
    	if(showlist->value!=NULL)
	    	free(showlist->value);
    	snext=showlist->next;
    	free(showlist);
    	showlist=snext;
	}
}
/*----------------------------------------------------------------------*/
/*  UDP端口侦听并处理   */
void udps_respon(int sockfd,gen_screen* shead)
{
	
        int     addrlen,n,k,row=0,firstflag=0,recnum=0,winseq=0,i,time,j,ll,ikey,icleartimes=1,curscr=0;
        char    msg[MAX_MSG_SIZE],*scanPtr,*headPtr,buffer[MAX_MSG_SIZE];
	    char    ch,str[3];                       /*从键盘读入的字符*/
		char    blankLine[78],delspac[30],routFile[30],binfo[200];            /*窗口中空白的一行*/   
		char 	translate[50],transvalue[50],QuitKey[20];
		struct sockaddr_in addr;
		struct timeval     timeout;       /*select语句循环时间*/
		fd_set  my_readfd;                /*select文件描述符集*/ 
        J_tranobj   tranobj;
        gen_win      *whead,*wbody,*win,*wintmp;
        gen_win	*w1,*w2,*w3,*w4;
        gen_screen * sheadtmp;
       
        J_showLine *lhead,*lbody,*ltail;
        disp_line *disphead,*dispbody,*disptail;
        data_show *showhead,*showhead1;
        gen_field *fhead;//窗口中显示的列名安顺序
	J_showList* showline,*showlinetmp;
	char    *objHandle;               /*数据特征判断句柄*/
	if(debugflag)
		fp=fopen("data","w");
    	FD_ZERO(&my_readfd);
		FD_SET(STDIN,&my_readfd);        /*加入标准输入到select中*/
        FD_SET(sockfd,&my_readfd);       /*加入UDP监听到select中*/

		timeout.tv_sec=0;       /*   等待多少秒      */
		timeout.tv_usec=10000;  /*   等待多少微秒   */
		strcpy(routFile,"filename");
		ReadTrans(pathName,"dataCharacter",routFile);//取对应关系
	    /*  执行初始化脚本 */
        interp = Tcl_CreateInterp();
        
		objHandle=getRamidef (interp,routFile);/*取得一个数据特征判断的句柄*/
		
		if(objHandle==NULL){
			printf("judge display handle error!\n");
			exit(0);
		}
	sheadtmp=shead;
	if( startflag == 0 )
	{
		//填充热键对应关系
		ShortKeyMap();
    		strcpy(QuitKey,"exitKey");
   		ReadTrans(pathName,"general",QuitKey);//获得退出键
		UpdateQuitFlag(QuitKey);
		//给每个窗口一个指针
		i=0;
		while(shead){
		pwin[i] = shead;
		//找热键
		for(ikey=0;ikey<MAXMAP;ikey++)
		{
			if(!strcmp(shortkeymap[ikey].shortkey,shead->scr_shortkey))
			{
				shortkeymap[ikey].no=i;
				shortkeymap[ikey].selected=1;
				break;
			}
		}		
		if( ikey == MAXMAP )
			WarnQuit(shead->scr_shortkey);
		shead=shead->next;
		i++;	
		numwin++;
		}
		startflag = 1;
		ch = '0';
	}
	shead=sheadtmp;
	strcpy(binfo,"noupdateinfo");//低部信息为原窗口中定义信息 
	draw_win(pwin[0],5,binfo);
			
        while(1)
        {     
        	
		FD_ZERO(&my_readfd);
	    	    FD_SET(STDIN,&my_readfd);        /*加入标准输入到select中*/
                FD_SET(sockfd,&my_readfd);       /*加入UDP监听到select中*/

			/*     并发处理   */
                if ((n=select(sockfd+1,&my_readfd,NULL,NULL,&timeout))<0)
					printf("select error!\n"); 
               /*        有数据包可以读了       */
              
		t++;
		//超时控制
		if( t/50 == atoi(Sleep_time) && t%50 == 0 && iacceptflag == 1 )
		{
			clear();
			draw_win(pwin[curscr],5,info);
			iacceptflag=0;
		}
		if( t/50 == atoi(Sleep_time) && t%50 == 0 && iacceptflag == 0 )
		{
			signal( SIGINT,(void(*)()) act_key);
			continue;
		}
                if(FD_ISSET(sockfd,&my_readfd))
			{
		if(interrupt == 0 )	
		{
		    /* 从网络上读数据   */
                    n=recvfrom(sockfd,msg,MAX_MSG_SIZE,0,
                           (struct sockaddr*)&addr,&addrlen);
		}
		else 
		{
			interrupt = 0;
			continue;
		}
		
                    if( n == 0  ){
                    	if(debugflag){
                    	       	fprintf(fp,"接收数据小于零!\n");
                    		fflush(fp);
                    	}
                    	continue;
                    }
                           if(debugflag)
                           {
                           	msg[n]=0;
                           	fprintf(fp,"[%s]\n",msg);
				fflush(fp);
                           }
			erase_blank( msg );
                    msg[n]=0; 

                	scanPtr= (char *)ckalloc(2*n+1);
					/*   保留字串首地址   */
                    headPtr=scanPtr;
					/*   字串赋值    */
					for (k=0 ; k < n; k++)  {
                    	scanPtr += Tcl_UniCharToUtf((unsigned char) msg[k],scanPtr);
					}
					*scanPtr = 0;
			        /*   单双字节转换   */
				   ll= TranObj_tcltoJ (interp,headPtr,&tranobj);
					free(headPtr);
				   if( ll != 0 )
				   {
				   	if(debugflag){
				   		fprintf(fp,"TranObj_tcltoj error!\n");
				   		fflush(fp);
				   	}
				   	continue;
				   }
            	    /*   从得到的结构生成可显示的字段链，即一行数据   */
    	            if((showline=make_show_list(&tranobj)) == NULL )
    	            	 continue;  
					
					resultRami(interp,&dispResult,objHandle,&tranobj);
					//将符合dispResult特征的该数据放到有该特征的窗口中
					//screen
				if(Data_Proc( showline, shead,tranobj.tranid )==-1)
					continue;
				strcpy(binfo,"nowriteinfo"); //低部信息不变且不刷新
				draw_win(pwin[curscr],5,binfo);
				for(j=0;j<numwin;j++){
					if(sndwin[j]=='1'){
						//Write_Log(pwin[j],j);
					}
				}
				/*   回收内存   */
				TranObj_Destroy(&tranobj);
				free_cell(showline);
			}
			
				if(FD_ISSET(STDIN,&my_readfd))
				{
					if(iacceptflag == 1)
						ch=getch();
					else 
						continue;
					for(ikey=0;ikey<MAXMAP;ikey++)
					{
						if(shortkeymap[ikey].keyvalue==ch)
						{
							if(shortkeymap[ikey].selected==1&&shortkeymap[ikey].no!=MAXMAP+1)
							{
								clear();
								strcpy(binfo,"noupdateinfo");//低部信息为原窗口中定义信息 
							draw_win(pwin[shortkeymap[ikey].no],5,binfo);//显示屏幕
								dtime=curscr=shortkeymap[ikey].no;
								t = 0;//时间设为零
							}
							if(shortkeymap[ikey].no==MAXMAP+1)
								Quit();
						}
					}
				}
                }
		/*删除一个数据特征判断的句柄*/
		rmRamidef (interp,objHandle);
}

/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
void    Draw_Box( int hs,int ls,int start_y,int start_x, int head)
{
    int num,count,y,flag;

    num=(ls-4)/2;
    y=start_y;
    flag = 0;
    if ( head > 3 )
    {
        head = head - 4;
        flag = 1;
    }
    Drawline(y++,start_x,num,1+flag*4);
    for(count=2;count<hs;count++)
    {
        if( (head==1 && count==3) || (head==2 && count==hs-2)
        || (head>=3 && (count==3 || count==hs-2)) )
            Drawline(y++,start_x,num,2+flag*4);
        else    
            Drawline(y++,start_x,num,0+flag*4);
    }
    Drawline(y++,start_x,num,3+flag*4);
} 

/*----------------------------------------------------------------------*/

int main(int argc,char *argv[])
{
        int    sockfd;
        char ajPort[20];
        struct sockaddr_in      addr;
        gen_screen   *shead,*sbody;
	gen_win      *whead,*wbody;
	gen_field*   fhead;
	data_show* abody;
	win_disp*   pbody;  
    	WINDOW *win;
    	gen_win* genwin;
  
		/*初始化全局窗口位置结构*/
    	
	char   ch,ServerPort[10],tmp[20],errmsg[200]; 
	int i;
	
	if( argc != 2 ){
		system("clear");
		printf("\n\n\n\n\n\n\n\t\t\t启动要带配置文件名！！\n\n\n\n\n\n\n\n\n\n",ajPort);
		getchar();
		system("clear");
		exit( 0 );
	}
	if( CheckIniFile(argv[1],errmsg) != SUCC )	
	{
		system("clear");
		printf("\n\n\n\n\n\n\n\t配置文件有误%s\n\n\n\n\n\n\n\n\n\n",errmsg);
		getchar();
		system("clear");
		exit( 0 );
	}
	strcpy(pathName,argv[1]);
   	shead=read_disp_ini(pathName);
   	
    	strcpy(Sleep_time,"dozzyTime");
   	ReadTrans(pathName,"general",Sleep_time);//取屏幕超时限制 
   	strcpy(ajPort,"recv" );
	if( ReadTrans(pathName,"PORT",ajPort) == -1 )//取端口号
	{	
		system("clear");
		printf("\n\n\n\n\n\n\n\t\t\t取端口号出错[%s]！！\n\n\n\n\n\n\n\n\n\n",ajPort);
		getchar();
		system("clear");
		exit( 0 );
	}
	GetPort( ajPort );
	strcpy(LogFile,"logfile");
	ReadTrans(pathName,"dataCharacter",LogFile);
	strcpy(tmp,"switch");
	ReadTrans(pathName,"dataCharacter",tmp);
	debugflag=atoi(tmp);
	InitScreen();
	//获得超时信息
	GetTimeOutMsg( argv[1],info );
	signal( SIGINT,SIG_IGN);
	//显示第一屏第一个窗口
	win= newwin(25,80,0,0);
 	/*  socket 初始化  */
        sockfd=socket(AF_INET,SOCK_DGRAM,0);
        if(sockfd<0)
        {
                fprintf(stderr,"Socket Error:%s\n",strerror(errno));
                exit(1);
        }
        bzero(&addr,sizeof(struct sockaddr_in));
        addr.sin_family=AF_INET;
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
        addr.sin_port=htons(atoi(ajPort));
        /*   绑定    */
		if(bind(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))<0)
        {
                fprintf(stderr,"Bind Error:%s\n",strerror(errno));
                exit(1);
        }

	startflag=0;	
        udps_respon(sockfd,shead);
        close(sockfd);
	    
		endwin();		
		system("clear");
		exit(0);
}

int GetPort( char *buf )
{
	char *p;
	int i;
	p=buf;
	for( i=0;i<3;i++)
        {
                if((p=strchr( p,','))!=NULL)
                        p++;
 	}
 	strcpy( buf,p );
}
int	InitScreen()
{
	initscr();
	clear();
	refresh();
	keypad(stdscr,TRUE);
	noecho();
}
int	Quit()
{
	sleep(1);
	endwin();
	system("clear");
	exit(0);
}
//增加热键与机器码的关系
int ShortKeyMap()
{
	int i;
	for(i=0;i<MAXMAP;i++)
	{
		shortkeymap[i].shortkey[0]=0;
	}
	strcpy(shortkeymap[0].shortkey,"0");
	shortkeymap[0].keyvalue='0';
	strcpy(shortkeymap[1].shortkey,"1");
	shortkeymap[1].keyvalue='1';
	strcpy(shortkeymap[2].shortkey,"2");
	shortkeymap[2].keyvalue='2';
	strcpy(shortkeymap[3].shortkey,"3");
	shortkeymap[3].keyvalue='3';
	strcpy(shortkeymap[4].shortkey,"4");
	shortkeymap[4].keyvalue='4';
	strcpy(shortkeymap[5].shortkey,"5");
	shortkeymap[5].keyvalue='5';
	strcpy(shortkeymap[6].shortkey,"6");
	shortkeymap[6].keyvalue='6';
	strcpy(shortkeymap[7].shortkey,"7");
	shortkeymap[7].keyvalue='7';
	strcpy(shortkeymap[8].shortkey,"8");
	shortkeymap[8].keyvalue='8';
	strcpy(shortkeymap[9].shortkey,"9");
	shortkeymap[9].keyvalue='9';
	strcpy(shortkeymap[10].shortkey,"a");
	shortkeymap[10].keyvalue='a';
	strcpy(shortkeymap[11].shortkey,"b");
	shortkeymap[11].keyvalue='b';
	strcpy(shortkeymap[12].shortkey,"c");
	shortkeymap[12].keyvalue='c';
	strcpy(shortkeymap[13].shortkey,"d");
	shortkeymap[13].keyvalue='d';
	strcpy(shortkeymap[14].shortkey,"e");
	shortkeymap[14].keyvalue='e';
	strcpy(shortkeymap[15].shortkey,"f");
	shortkeymap[15].keyvalue='f';
	strcpy(shortkeymap[16].shortkey,"g");
	shortkeymap[16].keyvalue='g';
	strcpy(shortkeymap[17].shortkey,"h");
	shortkeymap[17].keyvalue='h';
	strcpy(shortkeymap[18].shortkey,"i");
	shortkeymap[18].keyvalue='i';
	strcpy(shortkeymap[19].shortkey,"j");
	shortkeymap[19].keyvalue='j';
	strcpy(shortkeymap[20].shortkey,"k");
	shortkeymap[20].keyvalue='k';
	strcpy(shortkeymap[21].shortkey,"l");
	shortkeymap[21].keyvalue='l';
	strcpy(shortkeymap[22].shortkey,"m");
	shortkeymap[22].keyvalue='m';
	strcpy(shortkeymap[23].shortkey,"n");
	shortkeymap[23].keyvalue='n';
	strcpy(shortkeymap[24].shortkey,"o");
	shortkeymap[24].keyvalue='o';
	strcpy(shortkeymap[25].shortkey,"p");
	shortkeymap[25].keyvalue='p';
	strcpy(shortkeymap[26].shortkey,"q");
	shortkeymap[26].keyvalue='q';
	strcpy(shortkeymap[27].shortkey,"r");
	shortkeymap[27].keyvalue='r';
	strcpy(shortkeymap[28].shortkey,"s");
	shortkeymap[28].keyvalue='s';
	strcpy(shortkeymap[29].shortkey,"t");
	shortkeymap[29].keyvalue='t';
	strcpy(shortkeymap[30].shortkey,"u");
	shortkeymap[30].keyvalue='u';
	strcpy(shortkeymap[31].shortkey,"v");
	shortkeymap[31].keyvalue='v';
	strcpy(shortkeymap[32].shortkey,"w");
	shortkeymap[32].keyvalue='w';
	strcpy(shortkeymap[33].shortkey,"x");
	shortkeymap[33].keyvalue='x';
	strcpy(shortkeymap[34].shortkey,"y");
	shortkeymap[34].keyvalue='y';
	strcpy(shortkeymap[35].shortkey,"z");
	shortkeymap[35].keyvalue='z';
	strcpy(shortkeymap[36].shortkey,"F1");
	shortkeymap[36].keyvalue=KEY_F(1);
	strcpy(shortkeymap[37].shortkey,"F2");
	shortkeymap[37].keyvalue=KEY_F(2);
	strcpy(shortkeymap[38].shortkey,"F3");
	shortkeymap[38].keyvalue=KEY_F(3);
	strcpy(shortkeymap[39].shortkey,"F4");
	shortkeymap[39].keyvalue=KEY_F(4);
	strcpy(shortkeymap[40].shortkey,"F5");
	shortkeymap[40].keyvalue=KEY_F(5);
	strcpy(shortkeymap[41].shortkey,"F6");
	shortkeymap[41].keyvalue=KEY_F(6);
	strcpy(shortkeymap[42].shortkey,"F7");
	shortkeymap[42].keyvalue=KEY_F(7);
	strcpy(shortkeymap[43].shortkey,"F8");
	shortkeymap[43].keyvalue=KEY_F(8);
	strcpy(shortkeymap[44].shortkey,"F9");
	shortkeymap[44].keyvalue=KEY_F(9);
	strcpy(shortkeymap[45].shortkey,"F10");
	shortkeymap[45].keyvalue=KEY_F(10);
	strcpy(shortkeymap[46].shortkey,"F11");
	shortkeymap[46].keyvalue=KEY_F(11);
	strcpy(shortkeymap[47].shortkey,"F12");
	shortkeymap[47].keyvalue=KEY_F(12);
	strcpy(shortkeymap[48].shortkey,"Q");
	shortkeymap[48].keyvalue='Q';
	strcpy(shortkeymap[49].shortkey,"Ctrl+A");
	shortkeymap[49].keyvalue=1;
	strcpy(shortkeymap[50].shortkey,"Ctrl+B");
	shortkeymap[50].keyvalue=2;
	strcpy(shortkeymap[51].shortkey,"Ctrl+C");
	shortkeymap[51].keyvalue=3;
	strcpy(shortkeymap[52].shortkey,"Ctrl+D");
	shortkeymap[52].keyvalue=4;
	strcpy(shortkeymap[53].shortkey,"Ctrl+E");
	shortkeymap[53].keyvalue=5;
	strcpy(shortkeymap[54].shortkey,"Ctrl+F");
	shortkeymap[54].keyvalue=6;
	strcpy(shortkeymap[55].shortkey,"Ctrl+G");
	shortkeymap[55].keyvalue=7;
	strcpy(shortkeymap[56].shortkey,"Ctrl+H");
	shortkeymap[56].keyvalue=8;
	strcpy(shortkeymap[57].shortkey,"Ctrl+I");
	shortkeymap[57].keyvalue=9;
	strcpy(shortkeymap[58].shortkey,"Ctrl+J");
	shortkeymap[58].keyvalue=10;
	strcpy(shortkeymap[59].shortkey,"Ctrl+K");
	shortkeymap[59].keyvalue=11;
	strcpy(shortkeymap[60].shortkey,"Ctrl+L");
	shortkeymap[60].keyvalue=12;
	strcpy(shortkeymap[61].shortkey,"Ctrl+M");
	shortkeymap[61].keyvalue=13;
	strcpy(shortkeymap[62].shortkey,"Ctrl+N");
	shortkeymap[62].keyvalue=14;
	strcpy(shortkeymap[63].shortkey,"Ctrl+O");
	shortkeymap[63].keyvalue=15;
	strcpy(shortkeymap[64].shortkey,"Ctrl+P");
	shortkeymap[64].keyvalue=16;
	strcpy(shortkeymap[65].shortkey,"Ctrl+Q");
	shortkeymap[65].keyvalue=17;
	strcpy(shortkeymap[66].shortkey,"Ctrl+R");
	shortkeymap[66].keyvalue=18;
	strcpy(shortkeymap[67].shortkey,"Ctrl+S");
	shortkeymap[67].keyvalue=19;
	strcpy(shortkeymap[68].shortkey,"Ctrl+T");
	shortkeymap[68].keyvalue=20;
	strcpy(shortkeymap[69].shortkey,"Ctrl+U");
	shortkeymap[69].keyvalue=21;
	strcpy(shortkeymap[70].shortkey,"Ctrl+V");
	shortkeymap[70].keyvalue=22;
	strcpy(shortkeymap[71].shortkey,"Ctrl+W");
	shortkeymap[71].keyvalue=23;
	strcpy(shortkeymap[72].shortkey,"Ctrl+X");
	shortkeymap[72].keyvalue=24;
	strcpy(shortkeymap[73].shortkey,"Ctrl+Y");
	shortkeymap[73].keyvalue=25;
	strcpy(shortkeymap[74].shortkey,"Ctrl+Z");
	shortkeymap[74].keyvalue=26;
	strcpy(shortkeymap[75].shortkey,"Esc");
	shortkeymap[75].keyvalue=27;
}
//从对应关系表中找到退出键并更改标志为最大序号加一
int	UpdateQuitFlag(char *QuitKey)
{
	int i;
	for(i=0;i<MAXMAP;i++)
	{
		if(!strcmp(shortkeymap[i].shortkey,QuitKey))
		{
			shortkeymap[i].no=MAXMAP+1;
			shortkeymap[i].selected=1;			
			break;
		}
	}
	if( i == MAXMAP )
		WarnQuit(QuitKey);
}
int WarnQuit(char *errormsg)
{
		clear();
		fprintf(stderr,"\n\n\n\n\n\n\n\t\t    对不起,你定义的[%s]暂时还不支持\n",errormsg);
		fflush(stderr);
		getchar();
		Quit();
}
int CheckIniFile( char *file,char *tmp )
{
	FILE *pfp,*p;
	char buf[200];
	int ifind = 0;
	pfp = fopen(file,"r");
	while(fgets(buf,120,pfp))
	{
		strcpy(buf,erase_blank(buf));
		if( strlen(buf) ==1 )
			continue;
		else break;
	}
	while(p)
	{
		strcpy(tmp,buf);
		if(strchr(buf,'\\') != NULL)
			ifind = 1;
		else	ifind = 0;
		while((p=fgets(buf,120,pfp)))
		{
			strcpy(buf,erase_blank(buf));
			if( strlen(buf) ==1 )
				continue;
			else	break;
		}
		switch(ifind)
		{
			case 0:
				if(buf[0] == '=')
				{
					fclose( pfp );
					return( FAIL );
				}
				break;
			case 1:
				if(buf[0] != '=')
				{
					fclose( pfp );
					return( FAIL );
				}
				break;
		}
	}
	fclose( pfp );
	return( SUCC );
}
int GetTimeOutMsg(char *file,char *msg)
{
	FILE	*pfp;
	char 	buf[120],*p;
	pfp = fopen(file,"r");
	p=buf;
	while(fgets( buf, 120, pfp))	
	{
		if(!memcmp(buf,"timeoutinf",10))
		{
			if((p=strchr(buf,'='))==NULL)
			{
				fclose( pfp );
				WarnQuit( buf );	
			}
			strcpy( msg,p+1 );
			break;
		}
	}
	fclose( pfp );
}
int act_key( )
{
	iacceptflag = 1;
	t=0;
	interrupt = 1;
	clear();
	signal( SIGINT,SIG_IGN);
	draw_win(pwin[dtime],5,"noupdateinfo");//显示原窗口
	return( 0 );
}
