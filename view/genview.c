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
	int	no;	     //������λ��
	int	selected;    //ʹ�ñ�־
}shortkeymap[MAXMAP];
/**��Сд��ĸת���ɴ�д*/
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

}/**����������
 ���������������ݵĽṹָ�롢������ݵĽṹָ�롢��������
	�����������
	����ֵ  ����		
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
        char    *objHandle;               /*���������жϾ��*/
	             /*���������жϽ��*/
       	win_disp *wdisp;
        J_showLine *lhead,*lbody,*ltail,*freeline,*jhead,*jheadtmp;
        disp_line *disphead,*dispbody,*disptail,*disptmp,*dispstore;
        data_show *showhead,*showhead1;
        gen_field *fhead,*fheadtmp;//��������ʾ��������˳��
		
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
				fprintf(fp,"�Ƚ������������dispname[%s]dispResult[%s]\n",gdisp->disp_name,dispResult);
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
			if(firstflag==0){//����ǵ�һ������Ԫ
				
				lhead=ltail; //��������Ԫ���׵�ַ
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
			LowerToUpper( showline->field_name );//��ĸת���ɴ�д
			while(showhead){
				LowerToUpper( showhead->data_name );
				strcpy(field_name,showline->field_name);
				//�ҽ�������
				if(!strcmp(showhead->data_name,"TRANTYPE"))
					strcpy(TranType,showhead->d_field_name);
				if(!strcmp(showline->field_name,showhead->data_name))
				{
					
					strcpy(showline->field_name,showhead->d_field_name);
					if(showhead->translate ==1 )
					{//Ҫ����
					
						strcpy(translate,"translation,");
						strcat(translate,showhead->data_name);
						strcpy(transvalue,showline->value);
						ReadTrans(pathName,translate,transvalue);//ȡ��Ӧ��ϵ
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
				//ȡ����ԪֵΪ����SERIAL�Ƚ���
				if(!strcmp(showline->field_name,gdisp->serial_name))
					strcpy(field_value,showline->value);
				strcpy(showline->field_name,field_name);
					
				}//while showhead end
				if(firstflag==1)
				{///������ǵ�һ������Ԫ�������������
					lbody->next=ltail;
			    		lbody=ltail; 
					
				}
    				else
	    				firstflag=1;
	    			
		    		ltail->next=NULL;
			    	ltail=NULL;
			       
			    	showline=showline->next;
			    	
			  }//while showline end
			/***********���뽻������********************/
			fheadtmp=fhead=win->multi_field;//Ҫ��ʾ������
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
			//����Ļ������
			
			disptail=malloc(sizeof(disp_line));
			disptail->disp_name=malloc(strlen(wdisp->disp_name+1));
			strcpy(disptail->disp_name,wdisp->disp_name);
			disptail->showLine=lhead;
			disptail->next=NULL;
			disphead = win->multi_line;
			disp_flag=0;//�ж��Ƿ������洦��
			if(win->numline==0)
			{
                                win->multi_line=disptail;
				disptail->next=NULL;
			}
                        else
                        {
				//������ʾ���س���� 
				switch(gdisp->lineoverlap)
				{
					case SERIAL:
						disptmp=disphead;
						jheadtmp=jhead;
						dispstore = disphead;
						looptimes=0;
               					while(disphead){
               					jheadtmp=jhead=disphead->showLine;//���ݴ����
                			        while(jhead){
                       				 if(!memcmp(jhead->field_name,gdisp->serial_name,strlen(jhead->field_name)))//�Ƚ�����Ԫ
							if(!strcmp(field_value,jhead->value))//�Ƚ�����Ԫֵ
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

				if(disp_flag == 0){//���洦��
                        	while(disphead)
                        	{	//���Ƿ�ʽ��ʾ
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
                        		}//������ʽ��ʾ
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
                       	if( win->numline > win->win_buffer_no &&win->win_roll == FALSE ){//��ǰ����һ�����Ϸ���
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
			wini++;//�жϷ�����Щ��Ŀ
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
	sprintf(logfile+strlen(logfile),"%d",winflag);//��ͬ�Ĵ���д����ͬ���ļ�
	pp=fopen(logfile,"a");
	if(genwin->multi_line!=NULL)
	{	
		fheadtmp=fhead=fheadtmp=genwin->multi_field;
		lhead=genwin->multi_line;//���ݴ�ŵ���
		gdisp=genwin->multi_disp->disp_addr;
		while(lhead)
		{
			if( lhead->next == NULL )
				break;
			lhead=lhead->next;
		}
		jheadtmp=jhead=lhead->showLine;//���ݴ����
		while(fhead)
		{
			while(jhead){
		
			if(strstr(jhead->field_name,fhead->field_name) != NULL)
			{	
				if((p2=strchr(jhead->value,'\01'))!=NULL)//ȥ��Ctrl-A
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
/*   thick--���߿��  0-��ʾ�� 1-��ʾ�� */
/*   direction--���ߵķ���0-����  1-����   */
void win_line(int x,int y,int length,int direction,int thick)
{
   char line[81],head[3],body[3],tail[3];
   int i;
   
   //�������ļ�ȡ�ô�����Ĳ��������ж�
   //Ҫ�������Ƿ񳬹��˳���

   if(direction){
	   if(thick){//�Ǵֺ���
	       strcpy(head,C_ZZY_C);
    	   strcpy(body,C_HXT_C);
     	   strcpy(tail,C_YZZ_C);
	   }
	   else{//ϸ����
	       strcpy(head,C_ZZY_C_X);
    	   strcpy(body,C_HXT_X);
     	   strcpy(tail,C_YZZ_C_X);
	   }
   }
   else{
	   if(thick){//�Ǵ�����
		   mvaddstr(y,x,C_HZX_C);
		   for(i=0;i<length;i++)
			   mvaddstr(y,++x,C_LXT_C);
		   mvaddstr(y,x,C_HZS_C);

		   return;
	   }
	   else{//��ϸ����
	   }
   }
   strcpy(line,head);
   for(i=0;i<length/2;i++)
	   strcat(line,body);
   strcat(line,tail);
 
   mvaddstr(y,x,line);  //����
}
/**
����������ʾ��Ļ 
���������screen����ָ��.
	�����������
	����ֵ  ����
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
		lhead=genwin->multi_line;//���ݴ�ŵ���
		gdisp=genwin->multi_disp->disp_addr;
		x=genwin->win_startx+2;
		y=genwin->win_starty+2;
		while(lhead){
		jheadtmp=jhead=lhead->showLine;//���ݴ����
		while(fhead)
		{
			
			while(jhead){
			if(!memcmp(jhead->field_name,fhead->field_name,strlen(fhead->field_name)))
			{	
				if((p2=strchr(jhead->value,'\01'))!=NULL)//ȥ��Ctrl-A
					*p2=0;
				/*=========��ʾ��ʽ��R-M-L��==========*/
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
/*  ���ݴ��ڽṹ������
    ������������ڽṹ����������
	�����������
	����ֵ  ����										 */
	
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
	//������
	memset(agBuf,0,30);
	genwin=screen->multi_win;
	while(genwin){
	if(genwin->win_border)
		Drawbox(genwin->win_height,genwin->win_width,genwin->win_starty,
		        genwin->win_startx,style);

	if(genwin->win_title!=NULL){//��ʾ���ڱ���
    	x=(genwin->win_width-strlen(genwin->win_title))/2+genwin->win_startx; 
    	mvaddstr(genwin->win_starty-1,x,genwin->win_title);
	}
	//���ڽ�ע
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
		while(fhead){//��ʾ�����ֶ�
			
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
/*   �ж�һ���ַ��Ƿ������һ������   */
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
/*   ���ַ����е����пո�ȥ��   */
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
/*   �ж�s2�Ƿ���s1�У����������ո�   */
/*   mode 0--�����ִ�Сд��1--���ִ�Сд   */
int sin_str(char* s1,char* s2,int mode)
{
	char temp[MAXLINE]; //����ȥ���ո���s1
	char *p,*end;
	int i=0;

	if(s2==NULL) return TRUE;
	if(s1==NULL) return FALSE;

	//��s1�еĿո�ȫ��ȥ��������temp��
	strcpy(temp,erase_blank(s1));
	//ת��
	if(!mode){ //��������ִ�Сд,�������ַ���ת��Ϊ��д�Ƚ�(Ҳ��ת��ΪСд)
		end=temp+strlen(temp);
		for(p=temp;p<end;p++)
			*p=toupper(*p);
		end=s2+strlen(s2);
		for(p=s2;p<end;p++)
			*p=toupper(*p);
	}
	//��ʼ�Ƚ�
    if(strstr(temp,s2)!=NULL)
       	return TRUE;
   	else
   		return FALSE;
}
/*-------------------------------------------------------*/
/*   ��һ���ַ����е�ָ���ַ��滻Ϊ�����ַ����滻����ִ�
     ��Ȼ��ԭ�����ִ�
	 ���������Ҫ������ִ������滻�ַ����滻�ַ�
	 ������������ִ�
	 ����ֵ  ����										 */
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
/*   ����õ�����Ļ�����ַ���������д��һ����Ļ�����ṹ�У�
     ͬʱ���ɸ���Ļ�д��ڵ�ָ���������ַ��������Ļ�ṹ�С�
     �����������Ļ������Ϣ���ִ�
	 �����������
	 ����ֵ  ����Ļ�ṹ��ַ                              */

gen_screen* make_aScreen(char* state)
{
	gen_screen* ascreen;               //���ɵ���Ļ�ṹ
	gen_win     *whead,*wbody,*wtail;  //���ڽṹ��
	char        *p,*end;
	char        word[MAXLINE],wkey[MAXLINE];   //һ���ؼ��ʡ���ŵõ���Сд�ؼ���
	int         i=0,j,firstWin=1;      //�Ƿ��һ������
	int         iwin=-1;               //window������ۼӼ���(ֵ0��1��2��3)-1��ʾ����û�д���
	 
	end=state+strlen(state);
	ascreen=malloc(sizeof(gen_screen));
	//������������ִ����ҳ��ؼ������Ļ�����ڽṹ��
	for(p=state;p<end;p++){
		if(*p=='='){//���Ե�һ���Ⱥ���ߵ��ַ�����Ϊ����Ļ����
			break;
		}
	}
	while(p<end+1){
		if(p==end)//���һ���ؼ��ʵĴ���
			*p='=';
		switch(*p){
    		case ',':{//�Ѿ�ȡ��һ�������ؼ���
    			word[i++]='\0';
				if(strcmp(wkey,"window")==0){//���һ��window���忪ʼ
					wtail=malloc(sizeof(gen_win));
					if(firstWin){//����ǵ�һ������
						whead=wtail; //���洰�����׵�ַ
						wbody=wtail;
					}
					iwin=0;
				}
				if(iwin>=0){//����д��ڿ�ʼ�ڶ�
					switch(iwin){
    					case 0:{//��������
							wtail->win_title=malloc(strlen(word)+1);
							strcpy(wtail->win_title,word);
							break;
							   }
    					case 1:{//��������X
							wtail->win_startx=atoi(word);
							break;
							   }					
    					case 2:{//��������Y
							wtail->win_starty=atoi(word);
							break;
							   }
    					case 3:{//���ڿ��
							wtail->win_width=atoi(word);
							break;
							   }
					}
					iwin++;
				}
				//��wordת��ΪȫСд
				for(j=0;j<i;j++)
					word[j]=tolower(word[j]);
				strcpy(wkey,word);
				i=0;
				break;
					 }
	    	case '=':{//�Ѿ�ȡ��һ���������ؼ��ʵ�ֵ
     			word[i++]='\0';
				
				i=0; 
				if(strcmp(wkey,"shortkey")==0){//�������Ļ��ݼ�
					ascreen->scr_shortkey=malloc(strlen(word)+1);
					strcpy(ascreen->scr_shortkey,word);
					break;
				}
				if(strcmp(wkey,"title")==0){//�������Ļ����
					ascreen->scr_title=malloc(strlen(word)+1);
					strcpy(ascreen->scr_title,word);
					break;
				}
				if(strcmp(wkey,"bottom")==0){//�������Ļ�ײ�����
					ascreen->scr_bottom=malloc(strlen(word)+1);
					strcpy(ascreen->scr_bottom,word);
					break;
				}
				if(strcmp(wkey,"frame")==0){//�������Ļ�߿���Ϣ
					if(sin_str(word,"yes",0))
						ascreen->scr_border=TRUE;
					else
						ascreen->scr_border=FALSE;
					break;
				}
				//����һ����ĳ�����ڵ����һ����������(���ڸ߶�)��������
				if(iwin>=0){
    				wtail->win_height=atoi(word);
	    			if(firstWin==0){//������ǵ�һ�����ڣ������������
		    			wbody->next=wtail;
			    		wbody=wtail; 
					}
    				else
	    				firstWin=0;
		    		wtail->next=NULL;
			    	wtail=NULL;
				    iwin=-1;//�ô��ڶ���
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
/*   �������ļ��ж�ȡ��Ļ��Ϣ������һ�������׵�ַ        
     ��������������ļ����
	 �����������
	 ����ֵ  ����Ļ�����׵�ַ                            */

gen_screen* read_screen(FILE* fp)
{
	//������Ļ���ļ���ָ��
	gen_screen *shead,*sbody,*stail;
	//���˱��������һ�����ݡ�û�пո��һ�����ݡ�û�пո������һ��Ļ��������
	char aline[MAXLINE],noBlank[MAXLINE],totalLine[MAXLINE*4];
	char ch[2],num[10];  //��ʱ����������������Ļ����
	int sNum,firstScr=1,i;   //��Ļ�ĸ������Ƿ��һ����Ļ
	
	//���ļ�ָ�뷵���ļ�ͷ
	rewind(fp);
	//����ļ����Ϊ�գ����˳�
	if(fp==NULL) return NULL;
	//�ҵ���Ļ���岿��
	while(fgets(aline,MAXLINE,fp)){ 
		//�����ִ�Сд�Ƚ��Ƿ�Ϊ��Ļ���忪ʼ
		if(sin_str(aline,"[screens]",0)){
			break;
		}
	}
	//������Ļ����ĸ����Ϳ�ݼ�
    while(fgets(aline,MAXLINE,fp)){
		//����ҵ���Ļ��������
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
	//����Ļ���������
	strcpy(totalLine,"\0");
	//�ҵ���һ��������Ļ���忪ʼ
	while(fgets(aline,MAXLINE,fp)){
		//�����ĳ��������Ļ���忪ʼ,���������Ļ�����һ��һ����'\'�ַ�
		if(chin_str(aline,'\\')){
			break;
		}
		//����Ѿ����˴��ڵĶ��岿�֣����ʾ��Ļ���������������Ļ������һ���Ǵ��ڶ��壩
		if(sin_str(aline,"[windows]",0) ||feof(fp)){
			return shead;
		}
	}
    //��ȡһ����Ļ������Ϣ
	strcpy(noBlank,erase_blank(aline));
	//������Ļ�����һ��
	strcat(totalLine,aline);
	while(fgets(aline,MAXLINE,fp)){
		strcpy(noBlank,erase_blank(aline)); 
		//������пգ�������
		if(IsBlank(noBlank))
			continue;
		//������к�'\'�ַ������Ǹ���Ļ���������
		if(chin_str(noBlank,'\\'))
   			strcat(totalLine,noBlank);
		else
			break;
	}
	//������Ļ�������һ��
	strcat(totalLine,noBlank);
	/* ���ڵõ����ִ�����'\'���س��ͻ��з��ţ�ȫ���ÿո����滻 */
	rep_str(totalLine,'\\',' ');
	rep_str(totalLine,0x0a,' ');
	rep_str(totalLine,0x0d,' ');
    strcpy(totalLine,erase_blank(totalLine));
	/*   ����õ�����Ļ�����ַ���������д��һ����Ļ�����ṹ�У�
	     ͬʱ���ɸ���Ļ�д��ڵ�ָ���������ַ��������Ļ�ṹ�С�*/
	if(firstScr){//����ǵ�һ����Ļ�Ķ���
		shead=make_aScreen(totalLine); //��������ͷ
		sbody=shead;
		firstScr=0; 
	}
	else{//������Ļ������뵽������
		stail=make_aScreen(totalLine);
		sbody->next=stail;
		sbody=stail;
		sbody->next=NULL;
		stail=NULL;
	}
	
	goto NEXTSCR;
}
/*-------------------------------------------------------*/
/*   ���ݴ�����������䴰�ڽṹ
     �����������������������Ļ���׵�ַ
	 �����������
	 ����ֵ  ����                                        */
void make_aWindow(char* state,gen_screen* shead)
{
	gen_win*  wbody;
	gen_field *fhead,*fbody,*ftail;
	win_disp  *dhead,*dbody,*dtail;
	char      word[MAXLINE],wkey[MAXLINE];  //���ڹؼ���
	char      *p,*end;
	int       i=0,j,iFlag=0,firstField=1,firstDisp=1;
	int       iField=-1;  //�����ֶε����ԣ�-1��0��1��-1��ʾû�ж����ֶ�


	end=state+strlen(state);
	//�����������д�������--��һ���ؼ���
	for(p=state;p<end;p++){
		if(*p=='='){
			word[i]='\0'; 
			break;
		}
		word[i++]=*p;
	}
	i=0;
	/*����Ļ���м����͸ô���������ͬ�Ĵ��ڽṹ
	  �ҵ�Ҫ�޸ĵĴ��ڽṹ��ַ                  */
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
	//�����ڵ��������ṹ��
	while(p<end+1){
		if(p==end)//���һ���ؼ��ʵĴ���
			*p='=';
		switch(*p){
    		case ',':{//�Ѿ�ȡ��һ�������ؼ���
    			word[i++]='\0';
				if(strcmp(wkey,"field")==0){//���һ��field���忪ʼ
					ftail=malloc(sizeof(gen_field));
					if(firstField){//����ǵ�һ���ֶ�
						fhead=ftail; //�����ֶ����׵�ַ
						fbody=ftail;
					}
					iField=0;
				}
				if(iField>=0){//������ֶο�ʼ�ڶ�
					switch(iField){
    					case 0:{//�ֶ�����
							ftail->field_name=malloc(strlen(word)+1);
							strcpy(ftail->field_name,word);
							break;
							   }
    					case 1:{//��ʾ���
							ftail->field_length=atoi(word);
							break;
							   }					
					}
					iField++;
				}

				//��wordת��ΪȫСд
				for(j=0;j<i;j++)
					word[j]=tolower(word[j]);
				strcpy(wkey,word);
				i=0;
				break;
					 }
	    	case '=':{//�Ѿ�ȡ��һ���������ؼ��ʵ�ֵ
     			word[i++]='\0';
        			i=0; 
				if(strcmp(wkey,"title")==0){//����
					free(wbody->win_title);  //��Ϊ�ó��ڶ���Ļʱ�Ѿ����������洰�����ƣ�����Ҫ���ͷ�
					wbody->win_title=(char *)malloc(strlen(word)+1);
					strcpy(wbody->win_title,word);
					break;
				}
				if(strcmp(wkey,"buffer")==0){//�����¼��
					wbody->win_buffer_no=atoi(word);
					break;
				}
				if(strcmp(wkey,"frame")==0){//�Ƿ���ʾ�߿�
					if(sin_str(word,"yes",0))
    					wbody->win_border=TRUE;
					else
						wbody->win_border=FALSE;
					break;
				}

				if(strcmp(wkey,"fieldline")==0){//��ʾ�ֶη�
					if(sin_str(word,"yes",0))
						wbody->win_field_line=TRUE;
					else
						wbody->win_field_line=FALSE;
					break;
				}
				if(strcmp(wkey,"roll")==0){//��������ʾ��ʽ
					if(!strcmp(word,"fresh"))
						wbody->win_roll=TRUE;
					else
						wbody->win_roll=FALSE;
					break;
				}
				if(strcmp(wkey,"display")==0){//��ʾ����
					//������ʾ�����ṹ��
					dtail=malloc(sizeof(win_disp));
					dtail->disp_name=malloc(strlen(word)+1);
					strcpy(dtail->disp_name,word);
					if(firstDisp){//����ǵ�һ����ʾ����
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
    				//�����һ�����ڶ��ֶΣ��Ǿ�һ�����ֶ����������һ����--"���"
	    			ftail->field_space=atoi(word);
		    		if(firstField==0){//������ǵ�һ���ֶΣ������������
			    		fbody->next=ftail;
				    	fbody=ftail; 
					}
    				else
	    				firstField=0;
		    		ftail->next=NULL;
			    	ftail=NULL;
				    iField=-1;//���ֶζ���
				}
				break;
					 }
			default:{word[i++]=*p;break;}
		}
		p++;
		//numwin++;
	}
    //���ֶ����ϴ�����
	wbody->multi_field=fhead; 
	wbody->multi_disp=dhead;
}
/*-------------------------------------------------------*/
/*   ��ȡ��������
     ��������������ļ��������Ļ�����׵�ַ
	 �����������,ֻ��д��Ļ���д�������ֵ
	 ����ֵ  ����                                        */
void read_window(FILE* fp,gen_screen* scrPtr)
{
	gen_win   * wbody;
	char aline[MAXLINE],noBlank[MAXLINE],totalLine[MAXLINE*4];

	rewind(fp);
	//����ļ����Ϊ�գ����˳�
	if(fp==NULL) return;
	//�ҵ����ڶ��岿��
	while(fgets(aline,MAXLINE,fp)){ 
		//�����ִ�Сд�Ƚ��Ƿ�Ϊ���ڶ��忪ʼ
		if(sin_str(aline,"[windows]",0))
			break;
	}
NEXTWIN:
	//���������������
	strcpy(totalLine,"\0");
	//�ҵ���һ�о��崰�ڶ��忪ʼ
	while(fgets(aline,MAXLINE,fp)){
		//�����ĳ��������Ļ���忪ʼ,���������Ļ�����һ��һ����'\'�ַ�
		if(chin_str(aline,'\\')){
			break;
		}
		//����Ѿ�������ʾ�����Ķ��岿�֣����ʾ���ڶ��������������Ļ������һ������ʾ���壩
		if(sin_str(aline,"[display]",0) ||feof(fp)){
			return;
		}
	}
    //��ȡһ�����ڶ�����Ϣ
	strcpy(noBlank,erase_blank(aline));
	//���봰�ڶ����һ��
	strcat(totalLine,aline);
	while(fgets(aline,MAXLINE,fp)){
		strcpy(noBlank,erase_blank(aline)); 
		//������пգ�������
		if(IsBlank(noBlank))
			continue;
		//������к�'\'�ַ������Ǹô��ڶ��������
		if(chin_str(noBlank,'\\'))
   			strcat(totalLine,noBlank);
		else
			break;
	}
	//����ô��ڶ������һ��
	strcat(totalLine,noBlank);
	/* ���ڵõ����ִ�����'\'���س��ͻ��з��ţ�ȫ���ÿո����滻 */
	rep_str(totalLine,'\\',' ');
	rep_str(totalLine,0x0a,' ');
	rep_str(totalLine,0x0d,' ');
    
    strcpy(totalLine,erase_blank(totalLine));

	make_aWindow(totalLine,scrPtr); 
	goto NEXTWIN;
}
/*-------------------------------------------------------*/
/*   ��һ����ʾ����������䵽��ʾ�����ṹ��
     �����������ʾ���������ִ�����Ļ���׵�ַ
	 �����������
	 ����ֵ  ����										 */
void make_aDisplay(char* state,gen_screen* scrPtr)
{
	gen_disp*   dbody;                 //����ʾ�����ĵ�ַ�ṹ
	gen_win*    wbody;                 //
	win_disp*   pbody;                 //
	data_show   *ahead,*abody,*atail;  //����Ԫ�ṹ��
	char        *p,*end;
	char        dataMark[MAXLINE];     //���һ����ʾ�������������������
	char        word[100],wkey[100],dispName[MAXLINE];   //һ���ؼ��ʡ���ŵõ���Сд�ؼ��ʡ�����ʾ������
	int         i=0,j,firstData=1;     //�Ƿ��һ������Ԫ
	int         iData=-1;              //����Ԫ������ۼӼ���(ֵ-1��0��1��2)-1��ʾ����û������Ԫ
	 
	dbody=malloc(sizeof(gen_disp));
	end=state+strlen(state);
	//�ҳ���ʾ������--��һ���ؼ���
	for(p=state;p<end;p++){
		if(*p=='=')
			break;
		dispName[i++]=*p;
		
	}
	dataMark[0]=0;
	dispName[i]='\0';//�������ʾ����������
	i=0;
	while(p<end+1){
		if(p==end)//���һ���ؼ��ʵĴ���
			*p='=';
		switch(*p){
    		case ',':{//�Ѿ�ȡ��һ�������ؼ���
    			word[i++]='\0';
				if(strcmp(wkey,"show")==0){//���һ������Ԫ���忪ʼ
					atail=malloc(sizeof(data_show));
					if(firstData){//����ǵ�һ������Ԫ
						ahead=atail; //��������Ԫ���׵�ַ
						abody=atail;
					}
					iData=0;
				}
				if(iData>=0){//���������Ԫ��ʼ�ڶ�
					switch(iData){
    					case 0:{//����Ա��ʾ����
							atail->d_field_name=malloc(strlen(word)+1);
							strcpy(atail->d_field_name,word);
														
							break;
							   }
    					case 1:{//����Ա����
							atail->data_name=malloc(strlen(word)+1);
							strcpy(atail->data_name,word);
							break;
							   }
    					case 2:{//�Ƿ���
							if(sin_str(word,"yes",0))
							    atail->translate=TRUE;
							else
								atail->translate=FALSE;
							break;
							   }
					}
					iData++;
				}
				//��wordת��ΪȫСд
				for(j=0;j<i;j++)
					word[j]=tolower(word[j]);
				strcpy(wkey,word);
				i=0;
				break;
					 }
	    	case '=':{//�Ѿ�ȡ��һ���������ؼ��ʵ�ֵ
     			word[i++]='\0';
				i=0; 
				if(strcmp(wkey,"lineoverlap")==0){//������ʾ��ʽ
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
				if(strcmp(wkey,"datacharacter")==0){//����������
					strcat( dataMark,word );
					strcat( dataMark,",");
					break;
				}
				if(strcmp(wkey,"serial_name")==0){//���к�����Ԫ��
					dbody->serial_name=(char*)malloc(strlen(word)+1);
					strcpy(dbody->serial_name,word);
					break;
				}
				//����һ����ĳ�����ڵ����һ����������(���ڸ߶�)��������
				if(iData>=0){
    				atail->align=word[0];
	    			if(firstData==0){//������ǵ�һ������Ԫ�������������
		    			abody->next=atail;
			    		abody=atail; 
					}
    				else
	    				firstData=0;
		    		atail->next=NULL;
			    	atail=NULL;
				    iData=-1;//������Ԫ����
				}
				break;
					 }
			default:{word[i++]=*p;break;}
		}
		p++;
	}
	//to do
	//��ֵ��������
        dbody->disp_name=(char*)malloc(strlen(dataMark)+1);
	strcpy(dbody->disp_name,dataMark);
	
	
	dbody->gen_data_show=ahead;
	//������Ļ�����ҳ��и���ʾ�����Ĵ��ڣ�������ʾ�����ĵ�ַ������Ӧ����
	//�ҳ��и���ʾ����������ʾ��������Ļ��-��������-����ʾ�������е�λ��
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
/*   ��ȡ��ʾ��������
     ��������������ļ��������Ļ�����׵�ַ
	 �����������,ֻ��д��Ļ������ʾ������������ʾ��������ֵ
	 ����ֵ  ����                                        */
void read_display(FILE* fp,gen_screen* scrPtr)
{
	gen_win   * wbody;
	char aline[MAXLINE],noBlank[MAXLINE],totalLine[MAXLINE*4];

	rewind(fp);
	//����ļ����Ϊ�գ����˳�
	if(fp==NULL) return;
	//�ҵ���ʾ�������岿��
	while(fgets(aline,MAXLINE,fp)){ 
		//�����ִ�Сд�Ƚ��Ƿ�Ϊ��ʾ�������忪ʼ
		if(sin_str(aline,"[display]",0))
			break;
	}
NEXTDISP: 
	//����ʾ�������������
	strcpy(totalLine,"\0");
	//�ҵ���һ�о�����ʾ�������忪ʼ
	while(fgets(aline,MAXLINE,fp)){
		//�����ĳ��������ʾ�������忪ʼ,���������ʾ���������һ��һ����'\'�ַ�
		if(chin_str(aline,'\\')){
			break;
		}
		//����Ѿ���������Ԫ���ƵĶ��岿�֣����ʾ��ʾ�������������������Ļ������һ������ʾ���壩
		if(sin_str(aline,"[end]",0) ||feof(fp)){
			return;
		}
	}
    //��ȡһ����ʾ����������Ϣ
	strcpy(noBlank,erase_blank(aline));
	//������ʾ���������һ��
	strcat(totalLine,aline);
	while(fgets(aline,MAXLINE,fp)){
		strcpy(noBlank,erase_blank(aline)); 
		//������пգ�������
		if(IsBlank(noBlank))
			continue;
		//������к�'\'�ַ������Ǹ���ʾ�������������
		if(chin_str(noBlank,'\\'))
   			strcat(totalLine,noBlank);
		else
			break;
	}
	//�������ʾ�����������һ��
	strcat(totalLine,noBlank);
	/* ���ڵõ����ִ�����'\'���س��ͻ��з��ţ�ȫ���ÿո����滻 */
	rep_str(totalLine,'\\',' ');
	rep_str(totalLine,0x0a,' ');
	rep_str(totalLine,0x0d,' ');
    strcpy(totalLine,erase_blank(totalLine));

    //�����ʾ�����ṹ
	make_aDisplay(totalLine,scrPtr); 
	goto NEXTDISP;
}
/*-------------------------------------------------------*/
/*  ����һ����ʾ�����ļ���������������Ļ�����ڡ���ʾ����
    ������Ԫ�ȵĽṹ��Ϣ
	��������������ļ���
	�����������
	����ֵ  ����Ļ�Ľṹ��								 */
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
//2001-1-2�����ͷź��� by gyb  û�в���

/*�ͷ�������������ָ�룬�������ڡ��ֶΡ������е���ʾ������
  ��ʾ����������Ԫ��������ͷ�

  ������������������׵�ַ
  �����������
  ����ֵ  ���ɹ�����TRUE��ʧ�ܷ���FALSE					 */
int free_win_stru(gen_win* wHead)
{
	gen_win   *wBody;         //����
	gen_field *fHead,*fBody;  //�ֶ�
	win_disp  *pHead,*pBody;  //�����е���ʾ����
	gen_disp  *dHead,*dBody;  //��ʾ����
	data_show *aHead,*aBody;  //����Ԫ

	while(wHead){

		//�ͷŸô���
		if(wHead->win_title!=NULL)
			free(wHead->win_title);
		wBody=wHead;
		wHead=wHead->next;
		free(wBody);
	}
	return TRUE;
}
/*-------------------------------------------------------*/
/*�ͷ�������Ļ����ָ�룬������Ļ�����ڡ��ֶΡ�
  �����е���ʾ��������ʾ����������Ԫ��������ͷ�

  �����������Ļ�����׵�ַ
  �����������
  ����ֵ  ���ɹ�����TRUE��ʧ�ܷ���FALSE					 */
int free_disp_stru(gen_screen* sHead)
{
	gen_screen* sBody;        //��Ļ
	gen_win*    wHead;        //����

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
/*��һ��J_showList�ṹ�л�ȡ��ʾ��������Ϣ��������һ����ʾ����ַ��������
  ֻ����1��list���element��ÿ��element����1��cell�����                */  

J_showList* make_show_list(J_tranobj* tranobj)
{
	J_showList    *lHead,*lTemp,*lNext;  //����һ����ʾ���������ṹָ��
	J_dataelement *eleScan;              //���������ֶΣ�element����ָ��
	int           i,count;

	if(tranobj==NULL)
		return NULL;

	count=tranobj->lists->num_eles;
	if(count>0){
		/*     ������һ���ֶ�   */
		eleScan=tranobj->lists->elements;   //ȡ�õ�һ��element���׵�ַ
	    lHead=malloc(sizeof(J_showList));
    	lNext=lHead;
		lHead->field_name=(char *)malloc(strlen(eleScan->name));    //ȡ�õ�һ���ֶ�����
		strcpy(lHead->field_name,eleScan->name); 
		if(eleScan->num_cells<1){  //���û���ֶ�ֵ
			lHead->value=NULL;
		}
		else{// ����ȡ�õ�һ���ֶ�ֵ
    		lHead->value=(char *)malloc(strlen(eleScan->cells->value)+1); 
		    strcpy(lHead->value,eleScan->cells->value);
		}
		//lHead->row=
		//lHead->col=
		lHead->next=NULL;
		lNext=lHead;

		/*  ����ʣ�µ��ֶ�   */
		for(i=0;i<count-1;i++){
			eleScan++;
			lTemp=malloc(sizeof(J_showList));
    	    lTemp->field_name=(char *)malloc(strlen(eleScan->name)+1);    //ȡ�õ�һ���ֶ�����
		    strcpy(lTemp->field_name,eleScan->name);  
		    if(eleScan->num_cells<1){  //���û���ֶ�ֵ
		     	lTemp->value=NULL;
			}
       		else{// ����ȡ����һ���ֶ�ֵ
    	    	lTemp->value=(char *)malloc(strlen(eleScan->cells->value)+1); 
		        strcpy(lTemp->value,eleScan->cells->value);
			}
    		//lHead->row=
    		//lHead->col=
			/*  ���µĽṹ����������   */
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

/*   �ͷ�һ����ʾ���ݵĿռ�   */
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
/*  UDP�˿�����������   */
void udps_respon(int sockfd,gen_screen* shead)
{
	
        int     addrlen,n,k,row=0,firstflag=0,recnum=0,winseq=0,i,time,j,ll,ikey,icleartimes=1,curscr=0;
        char    msg[MAX_MSG_SIZE],*scanPtr,*headPtr,buffer[MAX_MSG_SIZE];
	    char    ch,str[3];                       /*�Ӽ��̶�����ַ�*/
		char    blankLine[78],delspac[30],routFile[30],binfo[200];            /*�����пհ׵�һ��*/   
		char 	translate[50],transvalue[50],QuitKey[20];
		struct sockaddr_in addr;
		struct timeval     timeout;       /*select���ѭ��ʱ��*/
		fd_set  my_readfd;                /*select�ļ���������*/ 
        J_tranobj   tranobj;
        gen_win      *whead,*wbody,*win,*wintmp;
        gen_win	*w1,*w2,*w3,*w4;
        gen_screen * sheadtmp;
       
        J_showLine *lhead,*lbody,*ltail;
        disp_line *disphead,*dispbody,*disptail;
        data_show *showhead,*showhead1;
        gen_field *fhead;//��������ʾ��������˳��
	J_showList* showline,*showlinetmp;
	char    *objHandle;               /*���������жϾ��*/
	if(debugflag)
		fp=fopen("data","w");
    	FD_ZERO(&my_readfd);
		FD_SET(STDIN,&my_readfd);        /*�����׼���뵽select��*/
        FD_SET(sockfd,&my_readfd);       /*����UDP������select��*/

		timeout.tv_sec=0;       /*   �ȴ�������      */
		timeout.tv_usec=10000;  /*   �ȴ�����΢��   */
		strcpy(routFile,"filename");
		ReadTrans(pathName,"dataCharacter",routFile);//ȡ��Ӧ��ϵ
	    /*  ִ�г�ʼ���ű� */
        interp = Tcl_CreateInterp();
        
		objHandle=getRamidef (interp,routFile);/*ȡ��һ�����������жϵľ��*/
		
		if(objHandle==NULL){
			printf("judge display handle error!\n");
			exit(0);
		}
	sheadtmp=shead;
	if( startflag == 0 )
	{
		//����ȼ���Ӧ��ϵ
		ShortKeyMap();
    		strcpy(QuitKey,"exitKey");
   		ReadTrans(pathName,"general",QuitKey);//����˳���
		UpdateQuitFlag(QuitKey);
		//��ÿ������һ��ָ��
		i=0;
		while(shead){
		pwin[i] = shead;
		//���ȼ�
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
	strcpy(binfo,"noupdateinfo");//�Ͳ���ϢΪԭ�����ж�����Ϣ 
	draw_win(pwin[0],5,binfo);
			
        while(1)
        {     
        	
		FD_ZERO(&my_readfd);
	    	    FD_SET(STDIN,&my_readfd);        /*�����׼���뵽select��*/
                FD_SET(sockfd,&my_readfd);       /*����UDP������select��*/

			/*     ��������   */
                if ((n=select(sockfd+1,&my_readfd,NULL,NULL,&timeout))<0)
					printf("select error!\n"); 
               /*        �����ݰ����Զ���       */
              
		t++;
		//��ʱ����
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
		    /* �������϶�����   */
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
                    	       	fprintf(fp,"��������С����!\n");
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
					/*   �����ִ��׵�ַ   */
                    headPtr=scanPtr;
					/*   �ִ���ֵ    */
					for (k=0 ; k < n; k++)  {
                    	scanPtr += Tcl_UniCharToUtf((unsigned char) msg[k],scanPtr);
					}
					*scanPtr = 0;
			        /*   ��˫�ֽ�ת��   */
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
            	    /*   �ӵõ��Ľṹ���ɿ���ʾ���ֶ�������һ������   */
    	            if((showline=make_show_list(&tranobj)) == NULL )
    	            	 continue;  
					
					resultRami(interp,&dispResult,objHandle,&tranobj);
					//������dispResult�����ĸ����ݷŵ��и������Ĵ�����
					//screen
				if(Data_Proc( showline, shead,tranobj.tranid )==-1)
					continue;
				strcpy(binfo,"nowriteinfo"); //�Ͳ���Ϣ�����Ҳ�ˢ��
				draw_win(pwin[curscr],5,binfo);
				for(j=0;j<numwin;j++){
					if(sndwin[j]=='1'){
						//Write_Log(pwin[j],j);
					}
				}
				/*   �����ڴ�   */
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
								strcpy(binfo,"noupdateinfo");//�Ͳ���ϢΪԭ�����ж�����Ϣ 
							draw_win(pwin[shortkeymap[ikey].no],5,binfo);//��ʾ��Ļ
								dtime=curscr=shortkeymap[ikey].no;
								t = 0;//ʱ����Ϊ��
							}
							if(shortkeymap[ikey].no==MAXMAP+1)
								Quit();
						}
					}
				}
                }
		/*ɾ��һ�����������жϵľ��*/
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
  
		/*��ʼ��ȫ�ִ���λ�ýṹ*/
    	
	char   ch,ServerPort[10],tmp[20],errmsg[200]; 
	int i;
	
	if( argc != 2 ){
		system("clear");
		printf("\n\n\n\n\n\n\n\t\t\t����Ҫ�������ļ�������\n\n\n\n\n\n\n\n\n\n",ajPort);
		getchar();
		system("clear");
		exit( 0 );
	}
	if( CheckIniFile(argv[1],errmsg) != SUCC )	
	{
		system("clear");
		printf("\n\n\n\n\n\n\n\t�����ļ�����%s\n\n\n\n\n\n\n\n\n\n",errmsg);
		getchar();
		system("clear");
		exit( 0 );
	}
	strcpy(pathName,argv[1]);
   	shead=read_disp_ini(pathName);
   	
    	strcpy(Sleep_time,"dozzyTime");
   	ReadTrans(pathName,"general",Sleep_time);//ȡ��Ļ��ʱ���� 
   	strcpy(ajPort,"recv" );
	if( ReadTrans(pathName,"PORT",ajPort) == -1 )//ȡ�˿ں�
	{	
		system("clear");
		printf("\n\n\n\n\n\n\n\t\t\tȡ�˿ںų���[%s]����\n\n\n\n\n\n\n\n\n\n",ajPort);
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
	//��ó�ʱ��Ϣ
	GetTimeOutMsg( argv[1],info );
	signal( SIGINT,SIG_IGN);
	//��ʾ��һ����һ������
	win= newwin(25,80,0,0);
 	/*  socket ��ʼ��  */
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
        /*   ��    */
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
//�����ȼ��������Ĺ�ϵ
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
//�Ӷ�Ӧ��ϵ�����ҵ��˳��������ı�־Ϊ�����ż�һ
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
		fprintf(stderr,"\n\n\n\n\n\n\n\t\t    �Բ���,�㶨���[%s]��ʱ����֧��\n",errormsg);
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
	draw_win(pwin[dtime],5,"noupdateinfo");//��ʾԭ����
	return( 0 );
}
