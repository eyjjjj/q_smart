//===========================================================================================================================
#define _CRT_SECURE_NO_DEPRECATE
//===========================================================================================================================

#include <windows.h>

#include "INF_Scheduler.h"
#include "INF_CimSeqnc.h"
#include "INF_Sch_prm_Cluster.h"

#include "lotpre.h"

//=======================================================================================================================


int LPLAST_STATUS[MAX_CHAMBER];	// enum { LPLAST_READY , LPLAST_PRODUCT , LPLAST_DUMMY };
int LPLAST_MODE[MAX_CHAMBER];
int LPLAST_TYPE[MAX_CHAMBER];
int LPLAST_TYPE[MAX_CHAMBER];
char LPLAST_NAME[MAX_CHAMBER][256];
char CUR_TYPE_RCPNAME[MAX_CASSETTE_SIDE][256];
char CUR_TYPE_RCPNAME2[MAX_CASSETTE_SIDE][MAX_CHAMBER][256];

int _pre_side; 

//=======================================================================================================================
void GET_CUR_RECIPE(int side , char *recipename )
{
	sprintf(recipename,"%s",CUR_TYPE_RCPNAME[side]);
	//_SCH_LOG_LOT_PRINTF( side  ,"GET_CUR_RECIPE  - [%s][%s]\n",CUR_TYPE_RCPNAME[side],recipename);
}


void SET_CUR_RECIPE(int side , char *recipename )
{
	sprintf(CUR_TYPE_RCPNAME[side],"%s",recipename);
	//_SCH_LOG_LOT_PRINTF( side  ,"SET_CUR_RECIPE  - [%s][%s]\n",CUR_TYPE_RCPNAME[side],recipename);
}
void GET_CUR_RECIPE2(int side , int ch, char *recipename )
{
	sprintf(recipename,"%s",CUR_TYPE_RCPNAME2[side][ch]);
	//_SCH_LOG_LOT_PRINTF( side  ,"GET_CUR_RECIPE  - [%s][%s]\n",CUR_TYPE_RCPNAME[side],recipename);
}


void SET_CUR_RECIPE2(int side , int ch, char *recipename )
{
	sprintf(CUR_TYPE_RCPNAME2[side][ch],"%s",recipename);
	//_SCH_LOG_LOT_PRINTF( side  ,"SET_CUR_RECIPE  - [%s][%s]\n",CUR_TYPE_RCPNAME[side],recipename);
}

void LotPre_Init() {
	int i;

	for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
		LPLAST_STATUS[i] = LPLAST_READY;
		LPLAST_MODE[i] = 0;
		LPLAST_TYPE[i] = 0;
		strcpy( LPLAST_NAME[i] , "" );
	}

	for ( i = 0 ; i < MAX_CASSETTE_SIDE ; i++ ) {
		strcpy( CUR_TYPE_RCPNAME[i] , "" );
	}

	_pre_side= -1;
}
void Set_LPLAST_Data (int ch, int data)
{
	LPLAST_TYPE[ch] = data;
}

int Get_LPLAST_Data (int ch)
{
	return LPLAST_TYPE[ch];
}

void RECIPE_CHANGE_DATA(int side , int pt ,int ch,int CUR_TYPE) 
{ //해당 Chamber의 Recipe 가 변경 되어야 하는지 확인 후 Recipe를 이전의 Lottype  Recipe로 변경.
	int clnControl; 
	char changefile[512];

	char sub_changefile[256];
	char sub_changefile2[256];
	
	char  Buffer[1025];
	char  Buffer2[1025];
	
	clnControl= IO_DIFF_CLN_CONTROL_GET(ch); //"PM?.DiffLotPre_Ctrl_Type" IO 읽어 오기 
	//printf("Change Recipe type[ch]   : %d [%d][%d]\n",clnControl,LPLAST_TYPE[ch],CUR_TYPE);
	_IO_CIM_PRINTF(" RECIPE_CHANGE_DATA - Change Recipe type[%d] : %d [%d][%d]\n",ch, clnControl,LPLAST_TYPE[ch],CUR_TYPE);
	if(clnControl == 1 ) //MLT_1
	{
		if(LPLAST_TYPE[ch]>0)
		{
			//Recipe Change
			IO_DIFF_CLN_RECIPE_GET(ch, LPLAST_TYPE[ch] ,changefile); // "PM?.LOT_PRE_RECIPE?" IO에서 Recipe 읽어 오기 
		}
		else
		{
			//Recipe Change - LPLAST_TYPE이 0이거나  -1이면 현재 Type 전달.
			IO_DIFF_CLN_RECIPE_GET(ch, CUR_TYPE ,changefile); // "PM?.LOT_PRE_RECIPE?" IO에서 Recipe 읽어 오기 
		}

		GET_CUR_RECIPE2(side ,ch, Buffer2 );

		printf("Change Recipe2  : %s \n",Buffer2);
		
		if(strcmp(changefile,Buffer2)!=0)
		{	
			sprintf ( Buffer , "DEFAULT_RECIPE_CHANGE2 READY0|PM%d|%s", ch - PM1 + 1 , changefile );
			//sprintf ( Buffer , "DEFAULT_RECIPE_CHANGE READY0|PM%d|%s", ch - PM1 + 1 , changefile );
			_dWRITE_FUNCTION_EVENT	(-1, Buffer );
			SET_CUR_RECIPE(side , changefile );
			SET_CUR_RECIPE2(side , ch, changefile );
		}
		//printf("Change Recipe  : %s \n",changefile);
		//printf("Change Recipe2  : %s \n",Buffer);

		//_SCH_LOG_LOT_PRINTF( side , "[%d]RECIPE_CHANGE_DATA [%s][CUR_TYPE=%d][LPLAST_TYPE =%d]\n" , clnControl,Buffer,CUR_TYPE ,LPLAST_TYPE[ch]);
		//Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , ch , _SCH_CLUSTER_Get_SlotIn( side , pt ) , changefile ,11 , 0 );
		//printf("Change Recipe  Res : %d [%s]\n",Res,changefile);
		
	}
	else if (  clnControl == 2) // MLT_2 
	{
		if(LPLAST_TYPE[ch]>0)
		{
			//Recipe Change
			IO_DIFF_CLN_RECIPE_GET(ch, LPLAST_TYPE[ch] ,sub_changefile); // "PM?.LOT_PRE_RECIPE?" IO에서 Recipe 읽어 오기 
			IO_DIFF_CLN_RECIPE_GET(ch, CUR_TYPE ,sub_changefile2); // "PM?.LOT_PRE_RECIPE?" IO에서 Recipe 읽어 오기 
			
			sprintf ( changefile , "\"%s|%s\"", sub_changefile , sub_changefile2 );

		}
		else
		{
			//Recipe Change - LPLAST_TYPE이 0이거나  -1이면 현재 Type 전달.
			IO_DIFF_CLN_RECIPE_GET(ch, CUR_TYPE ,changefile); // "PM?.LOT_PRE_RECIPE?" IO에서 Recipe 읽어 오기 
		}
		
		GET_CUR_RECIPE2(side ,ch, Buffer2 );

		printf("Change Recipe2  : %s \n",Buffer2);

		if(strcmp(changefile,Buffer2)!=0)
		{	
			sprintf ( Buffer , "DEFAULT_RECIPE_CHANGE2 READY0|PM%d|%s", ch - PM1 + 1 , changefile );
			//sprintf ( Buffer , "DEFAULT_RECIPE_CHANGE READY0|PM%d|%s", ch - PM1 + 1 , changefile );
			_dWRITE_FUNCTION_EVENT	(-1, Buffer );
			SET_CUR_RECIPE(side , changefile );
			SET_CUR_RECIPE2(side , ch, changefile );
			//printf("Change Recipe  : %s \n",changefile);
			//printf("Change Recipe2  : %s \n",Buffer);
		}

		//_SCH_LOG_LOT_PRINTF( side , "[%d]RECIPE_CHANGE_DATA [%s][CUR_TYPE=%d][LPLAST_TYPE =%d]\n" , clnControl,Buffer,CUR_TYPE ,LPLAST_TYPE[ch]);
		//Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , ch , _SCH_CLUSTER_Get_SlotIn( side , pt ) , changefile ,11 , 0 );
		//printf("Change Recipe  Res : %d [%s]\n",Res,changefile);
	}
	else if (  clnControl == 0)  //Normal 
	{
		GET_CUR_RECIPE2(side ,ch, Buffer2 );

		printf("Change Recipe2  : %s \n",Buffer2);
		
		if(strcmp(changefile,Buffer2)!=0 || strlen(Buffer2)<= 0)
		{	
			 //Recipe Change
			IO_DIFF_CLN_RECIPE_GET(ch, CUR_TYPE ,changefile); // "PM?.LOT_PRE_RECIPE?" IO에서 Recipe 읽어 오기 
			sprintf ( Buffer , "DEFAULT_RECIPE_CHANGE2 READY0|PM%d|%s", ch - PM1 + 1 , changefile );
			//sprintf ( Buffer , "DEFAULT_RECIPE_CHANGE READY0|PM%d|%s", ch - PM1 + 1 , changefile );
			//sprintf ( Buffer , "DEFAULT_RECIPE_CHANGE2 READY2|PM%d|%s", ch - PM1 + 1 , changefile );
			_dWRITE_FUNCTION_EVENT	(-1, Buffer );
			//printf("Change Recipe  : %s \n",changefile);
			SET_CUR_RECIPE(side , changefile );
			SET_CUR_RECIPE2(side , ch, changefile );
			_SCH_LOG_LOT_PRINTF( side , "[%d]RECIPE_CHANGE_DATA [%s][CUR_TYPE=%d][LPLAST_TYPE =%d]\n" , clnControl,Buffer,CUR_TYPE ,LPLAST_TYPE[ch]);
		}
	}
	else 
	{
		_SCH_LOG_LOT_PRINTF( side , "[%d]RECIPE_CHANGE_DATA IO ERROR [CUR_TYPE=%d][LPLAST_TYPE =%d]\n" , clnControl,CUR_TYPE ,LPLAST_TYPE[ch]);
	}
}

int LotPre_GET_CURRENT_TYPE_DATA( BOOL Check , int side , int pt , int ch , int *mode , int *type , char *recipe ) {
	//현재 공급할 Wafer 에 대한 LOT Pre 유무 확인 
	int Res;
	//int mode1;
	//
	char realfilename[256];

	//
	//if(!Check)//실제 Lotpre 를 위해 호출된 경우 
	//{
	//	printf("Change Recipe =======> \n");
	//	 RECIPE_CHANGE_DATA(side,pt,ch);  //해당 Chamber의 Recipe 가 변경 되어야 하는지 확인 후 Recipe 변경 
	//}

	//해당 Wafer가 공정할 Main Process Recipe 를 읽어 옴.
	strcpy( recipe , _SCH_CLUSTER_Get_PathProcessRecipe2( side , pt , 0 , ch , SCHEDULER_PROCESS_RECIPE_IN ) );
	//
	strcpy( realfilename , recipe );
	//
	//sprintf(realfilename , "Recipe\\RUN\\PROCESS\\PM%d\\%s",ch-PM1+1,recipe );
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM LotPre_GET_CURRENT_TYPE_DATA 1 [side=%d,pt=%d][ch=%d][Check=%d][%s]\n" , side , pt , ch , Check , recipe );
	//
	//1000이상인 경우 Recipe LOCKING된 File 경로를 가져옴 (2019.02.13 이전의버전에서는 LOCKING이 아닌 경우 경로를 가져오지 X)
	Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , ch , _SCH_CLUSTER_Get_SlotIn( side , pt ) , realfilename , 1000 , 0 );
//	Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , ch , _SCH_CLUSTER_Get_SlotIn( side , pt ) , realfilename , 11 , 0 );
	//printf( "SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING [%d][%s]\n" , Res ,realfilename);
//	Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , ch , _SCH_CLUSTER_Get_SlotIn( side , pt ) , realfilename , 1 , 0 );  //Post로 처리 
	//Res = 0;
	//
	if ( 0 > Res ) {
		_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM LotPre_GET_CURRENT_TYPE_DATA 2 [side=%d,pt=%d][ch=%d][Check=%d][Res=%d]\n" , side , pt , ch , Check , Res );
		return 1003;
	}
	//
	//Process Recipe 에서 LOT_PRE_TYPE 값을 가져 옴.
	//Res = K_RECIPE_STEP_ITEM_READ_3_INT( realfilename , 0 , "difflotpre_ctrl_type" , mode1 , 0 , "LOT_PRE_TYPE" , type , -1 , NULL , NULL ); 
	Res = K_RECIPE_STEP_ITEM_READ_3_INT( realfilename , 0 , "LOT_PRE_TYPE" , type , -1 , NULL , NULL , -1 , NULL , NULL);
	//"DiffClnOpt" IO  값을 읽어 온다. 만약 해당 IO가 정의 되어있지 않으면 기본 값으로 '1'을 가진다.
	*mode =  IO_DIFF_CLN_OPTION_GET();
	//
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM LotPre_GET_CURRENT_TYPE_DATA 3 [side=%d,pt=%d][ch=%d][Check=%d][Res=%d][mode=%d][type=%d][%s]\n" , side , pt , ch , Check , Res , *mode , *type , realfilename );
	//_SCH_LOG_LOT_PRINTF( side  , "USER_FLOW_CHECK_PICK_FROM_FM LotPre_GET_CURRENT_TYPE_DATA 3 [side=%d,pt=%d][ch=%d][Check=%d][Res=%d][mode=%d][type=%d][%s]\n" , side , pt , ch , Check , Res , *mode , *type , realfilename );
	//
	if ( Res != 0 || *mode == -1 ) {
		//printf( "K_RECIPE_STEP_ITEM_READ_3_INT has a Error [%d][%s]\n" , Res ,realfilename);
	}
	//
	return Res;

}


void LotPre_Last_Supply( int s , int p , int ch ,BOOL Mode ) {
	//마지막으로 공급한 Wafer Data 저장 
	//printf("LotPre_Last_Supply-A\n");
	//Wafer의 공급 Module이 CM이 아닌 BM인 경우 Dummy로 처리 함. 
	if(p<0) 
	{
		LPLAST_MODE[ch]=LPLAST_READY_STOP;
	}
	else
	{
		if ( _SCH_CLUSTER_Get_PathIn( s , p ) >= BM1 ) {
			LPLAST_MODE[ch] = LPLAST_DUMMY;
		}
		else {
			//
			//printf("LotPre_Last_Supply-A2\n");
			_SCH_LOG_DEBUG_PRINTF( s , 0 , "LOT_PRE_IF_DIFFER_LOT3 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][%s]\n" , s , p ,ch , LPLAST_MODE[ch],LPLAST_TYPE[ch],	LotPre_GET_CURRENT_TYPE_DATA( FALSE , s , p , ch , &(LPLAST_MODE[ch]) , &(LPLAST_TYPE[ch]),LPLAST_NAME[ch] ));
			//_SCH_LOG_LOT_PRINTF( s ,"LOT_PRE_IF_DIFFER_LOT3 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][%d]\n" , s , p ,ch , LPLAST_MODE[ch],LPLAST_TYPE[ch],	LotPre_GET_CURRENT_TYPE_DATA( FALSE , s , p , ch , &(LPLAST_MODE[ch]) , &(LPLAST_TYPE[ch]),LPLAST_NAME[ch] ));

			//printf("Change Recipe =======> \n");
			//RECIPE_CHANGE_DATA(s,p,ch);  //해당 Chamber의 Recipe 가 변경 되어야 하는지 확인 후 Recipe 변경

			if ( 0 == LotPre_GET_CURRENT_TYPE_DATA( FALSE , s , p , ch , &(LPLAST_MODE[ch]) , &(LPLAST_TYPE[ch]) , LPLAST_NAME[ch] ) ) {
			//printf("LotPre_Last_Supply-A3[%d][%d]\n",LPLAST_MODE[ch],LPLAST_TYPE[ch]);

			//	printf("Change Recipe =======> \n");
			//	RECIPE_CHANGE_DATA(s,p,ch,LPLAST_TYPE[ch]);  //해당 Chamber의 Recipe 가 변경 되어야 하는지 확인 후 Recipe 변경
			//	printf("LotPre_Last_Supply-A5[%d]\n",LPLAST_MODE[ch]);

				if(Mode)
				{
					LPLAST_MODE[ch] = LPLAST_PRODUCT;
					//printf("LotPre_Last_Supply-A6[%d]\n",LPLAST_MODE[ch]);
				}
				else 
				{
					LPLAST_MODE[ch] = LPLAST_PRODUCT_PRE;
					//printf("LotPre_Last_Supply-A7[%d]\n",LPLAST_MODE[ch]);
				}
			}
			else {
				//printf("LotPre_Last_Supply-A4[%d][%d]\n",LPLAST_MODE[ch],LPLAST_TYPE[ch]);
				LPLAST_MODE[ch] = LPLAST_READY;
			}
		}
	}
	//printf("[mode : %d]LotPre_Last_Supply[%d][%d]\n",Mode,LPLAST_MODE[ch],LPLAST_TYPE[ch]);
}



int LOT_PRE_IF_DIFFER_LOT( int side , int pt , int ch ) {
	//
	int CUR_MODE , CUR_TYPE , Res;
	int mode,count,max;
	char CUR_NAME[256];
	//

	Res = LotPre_GET_CURRENT_TYPE_DATA( TRUE , side , pt , ch , &CUR_MODE , &CUR_TYPE , CUR_NAME );

	_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT [ch =PM%d][ LPLAST_MODE =%d][LPLAST_TYPE =%d][CUR_TYPE = %d]\n" , ch-PM1+1, LPLAST_MODE[ch] ,LPLAST_TYPE[ch],CUR_TYPE);
	_SCH_LOG_DEBUG_PRINTF( side , 0 ,  "LOT_PRE_IF_DIFFER_LOT [ch =PM%d][ LPLAST_MODE =%d][LPLAST_TYPE =%d][CUR_TYPE = %d]\n" , ch-PM1+1, LPLAST_MODE[ch] ,LPLAST_TYPE[ch],CUR_TYPE);

	if ( LPLAST_MODE[ch] == LPLAST_PRODUCT_PRE )
	{
		if(LPLAST_TYPE[ch] == CUR_TYPE)
		{
			return 1004;
		}
	}
	//2020.03.03 위로 이동
	//Res = LotPre_GET_CURRENT_TYPE_DATA( TRUE , side , pt , ch , &CUR_MODE , &CUR_TYPE , CUR_NAME );
	
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "LOT_PRE_IF_DIFFER_LOT0 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][%d]\n" , side , pt ,ch,CUR_MODE,CUR_TYPE ,Res);
	_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT0 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][LPLAST_TYPE =%d][LPLAST_MODE =%d][%d]" , side , pt ,ch,CUR_MODE,CUR_TYPE ,LPLAST_TYPE[ch],LPLAST_MODE[ch],Res);

	if ( LPLAST_MODE[ch] == LPLAST_READY )
	{
		if(LPLAST_TYPE[ch]>=0)//2019.08.02
		{
			if(LPLAST_TYPE[ch] == CUR_TYPE)
			{
				//_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT3 [ch =PM%d][ rs = 1011]\n" , ch-PM1+1);
				_SCH_LOG_DEBUG_PRINTF( side , 0 ,  "LOT_PRE_IF_DIFFER_LOT3 [ch =PM%d][ rs = 1011]\n" , ch-PM1+1);
				return 1011;
			}
		}
	}
//	if ( LPLAST_MODE[ch] == LPLAST_DUMMY ) return 1012;
	//20109.08.08
	//if ( LPLAST_MODE[ch] != LPLAST_PRODUCT ) return 1001; // 2019.06.05 

	if ( LPLAST_MODE[ch] != LPLAST_PRODUCT ) 
	{
		if(LPLAST_TYPE[ch]<0 && LPLAST_MODE[ch] == LPLAST_READY) 
		{
			_SCH_LOG_DEBUG_PRINTF( side , 0 ,  "LOT_PRE_IF_DIFFER_LOT4 [ch =PM%d][ LPLAST_TYPE = %d ]\n",ch-PM1+1,LPLAST_TYPE[ch]);	
		}
		else if(LPLAST_TYPE[ch] != CUR_TYPE && LPLAST_MODE[ch] == LPLAST_READY) 
		{
			//_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT3-3 [ch =PM%d][PASS]\n" , ch-PM1+1);
			_SCH_LOG_DEBUG_PRINTF( side , 0 ,  "LOT_PRE_IF_DIFFER_LOT5 [ch =PM%d][ LPLAST_TYPE = %d ]\n",ch-PM1+1,LPLAST_TYPE[ch]);	
		}
		else 
		{
			//_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT3-2 [ch =PM%d][ rs = 1011]\n" , ch-PM1+1);
			return 1001;
		}
	}
	//

	// 2019.06.05
	if ( LPLAST_MODE[ch] != LPLAST_PRODUCT ) {
		if ( LPLAST_MODE[ch] != LPLAST_PRODUCT_PRE ) {
			if ( LPLAST_MODE[ch] != LPLAST_READY_STOP ) {
			
				if(LPLAST_TYPE[ch]>=0)
				{
					 if(LPLAST_TYPE[ch] != CUR_TYPE && LPLAST_MODE[ch] == LPLAST_READY) 
					{
						//_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT3-4 [ch =PM%d][PASS]\n" , ch-PM1+1);
						_SCH_LOG_DEBUG_PRINTF( side , 0 ,  "LOT_PRE_IF_DIFFER_LOT6 [ch =PM%d][ LPLAST_TYPE = %d ]\n",ch-PM1+1,LPLAST_TYPE[ch]);	
					}
					 else 
					 {
						if(LPLAST_TYPE[ch] != CUR_TYPE && LPLAST_MODE[ch] == LPLAST_READY) 
						{
							return 1005;   //2019.08.02
						}
					}
				}
				//if(LPLAST_TYPE[ch]>0)return 1005;
			}
		}
	}
	//


	//무조건 적으로 Lotpre가 돌아야 하는 조건 추가 
	//
	mode =-2; 
	count =-2; 


	//위치 이동 상단으로 
	/*
	//printf("LotPre_Last_Supply-B1\n");
	Res = LotPre_GET_CURRENT_TYPE_DATA( TRUE , side , pt , ch , &CUR_MODE , &CUR_TYPE , CUR_NAME );
	//printf("LotPre_Last_Supply-B2\n");
	//
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "LOT_PRE_IF_DIFFER_LOT0 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][%d]\n" , side , pt ,ch,CUR_MODE,CUR_TYPE ,Res);
	//_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT0 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][LPLAST_TYPE =%d][%d]" , side , pt ,ch,CUR_MODE,CUR_TYPE ,LPLAST_TYPE[ch],Res);
	//Recipe change 호출
	//
	*/
	//printf("Change Recipe =======> \n");
	RECIPE_CHANGE_DATA(side,pt,ch,CUR_TYPE);  //해당 Chamber의 Recipe 가 변경 되어야 하는지 확인 후 Recipe 변경
	

	//if(IO_DISABLE_CH_GET(ch)==1)
	//{
	//	_SCH_LOG_LOT_PRINTF( side , "[LOTPRE]  DISABLE MODE  PM%d [%d]\n" , ch - PM1 + 1,IO_DISABLE_CH_GET(ch) );
	//	IO_DISABLE_CH_SET(ch,0);
	//	return 0; 
	//}


	// 0 :	 OFF  1 : ON
	if(IO_FALVS_DLP_GET(ch) == 1)
	{
		_SCH_LOG_LOT_PRINTF( side , "[LOTPRE]  FALVS MODE  PM%d [%d]\n" , ch - PM1 + 1,IO_FALVS_DLP_GET(ch) );
		IO_FALVS_DLP_SET(ch,0);
		//_SCH_LOG_LOT_PRINTF( side , "[LOTPRE]  FALVS MODE  RESET PM%d [%d]\n" , ch - PM1 + 1,IO_FALVS_DLP_GET(ch) );
		return 0; 
	}

	
	//if(GET_DISABLE_PM_STATUS(ch))
	//{
	//	_SCH_LOG_LOT_PRINTF( side , "[LOTPRE]  DISAVLE  -> ENABLE  PM%d [%d]\n" , ch - PM1 + 1,GET_DISABLE_PM_STATUS(ch) );
	//	SET_DISABLE_PM_STATUS( side,ch ,FALSE); 
	//	//_SCH_LOG_LOT_PRINTF( side , "[LOTPRE]  FALVS MODE  RESET PM%d [%d]\n" , ch - PM1 + 1,IO_FALVS_DLP_GET(ch) );
	//	return 0; 
	//}
	//printf("LotPre_Last_Supply-B1\n");
	Res = LotPre_GET_CURRENT_TYPE_DATA( TRUE , side , pt , ch , &CUR_MODE , &CUR_TYPE , CUR_NAME );
	//printf("LotPre_Last_Supply-B2\n");
	////
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "LOT_PRE_IF_DIFFER_LOT1 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][%d]\n" , side , pt ,ch,CUR_MODE,CUR_TYPE ,Res);
	//_SCH_LOG_LOT_PRINTF( side , "LOT_PRE_IF_DIFFER_LOT1 [side=%d,pt=%d][ch=%d][CUR_MODE=%d][CUR_TYPE=%d][%d]\n" , side , pt ,ch,CUR_MODE,CUR_TYPE ,Res);
	if ( Res != 0 ) return Res;
	//
	
	//
	// 0 :	 Normal Clean Count에 상관 없이 Lotpre 진행
	// 1 : MLT  Clean Count가 0 이라면 Lotpre 진행 X
	mode= IO_DIFF_CLN_CONTROL_GET( ch);
	count =  IO_AUTO_LEAK_ACC_WF_GET( ch );
	max = IO_AUTO_LEAK_ACC_WF_MAX_GET( ch );

	if(count<0)
	{
		count =  IO_AUTO_LEAK_ACC_WF_GET( ch );
		_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-RECIPE] MLT1 MODE  - PM%d CLEAN COUNT ERROR (%d)\n" , ch - PM1 + 1,count);
	}
	
	if ( CUR_MODE == LPLAST_CHECK_RECIPE ) {
		if ( !STRCMP_L( LPLAST_NAME[ch] , CUR_NAME ) ) 
		{
			if(mode == 0)  //Normal Mode 
			{
				//IO_PM_REMAIN_TIME(ch,9999);
				_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-RECIPE] Normal MODE\n");
				return 0;
			}
			else    //MLT Mode 
			{
				if(mode == 1)
				{
					if(count == max || count == 0)  //Clean Count 가 max와 같은 경우
					{
						_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-RECIPE] MLT1 MODE  - PM%d CLEAN COUNT (%d)\n" , ch - PM1 + 1,count);
						return 1004;
					}
					else if(count > 0) //Clean Count 가 0인 이상 
					{
						//IO_PM_REMAIN_TIME(ch,9999);
						_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-RECIPE] MLT1 MODE  - PM%d CLEAN COUNT (%d / max  - %d )\n" , ch - PM1 + 1,count,max);
						return 0; //Clean Count 가 0이 X 경우 진행.
					}
				}
				else if(mode == 2)
				{
					_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-RECIPE] MLT2 MODE \n");
					return 0; //Clean Count 상관 없이 무조건 진행 
				}

			}
		}
	}
	else if ( CUR_MODE == LPLAST_CHECK_TYPE ) {
		
		_SCH_LOG_DEBUG_PRINTF( side , 0 , "LOT_PRE_IF_DIFFER_LOT2 [LPLAST_TYPE(ch)=%d][CUR_TYPE=%d]\n" , LPLAST_TYPE[ch] ,CUR_TYPE);

		if ( LPLAST_TYPE[ch] != CUR_TYPE )
		{
			if(mode == 0)
			{
				//IO_PM_REMAIN_TIME(ch,9999);
				_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-TYPE] Normal MODE.(2)\n");
				return 0;
			}
			else
			{
				if(mode == 1)
				{
					if(count == 0 || count == max ) //Clean Count 가 max와 같은 경우
					{
						_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-TYPE] MLT MODE  - PM%d CLEAN COUNT (%d)  --- LOTPRE SKIP \n" , ch - PM1 + 1,count);
						return 1005;
					}
					else
					{
						//IO_PM_REMAIN_TIME(ch,9999);
						_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-TYPE] MLT MODE  - PM%d CLEAN COUNT (%d)\n" , ch - PM1 + 1,count);
						return 0;
					}
				}
				else if(mode == 2)
				{
					_SCH_LOG_LOT_PRINTF( side , "[LOTPRE-RECIPE] MLT2 MODE.(2)\n");
					return 0; //Clean Count 상관 없이 무조건 진행 
				}

			}
		}
	}
	//
	//_SCH_LOG_LOT_PRINTF( side , "[LOTPRE]  MODE ERROR  - PM%d MODE (%d)\n" , ch - PM1 + 1,CUR_MODE);
	return 1002;
	//
}

//=======================================================================================================================

BOOL IS_THIS_FIRSTWAFER( int s , int p , int ch ) { // 2017.09.19
	int i , k;
	//
	for ( i = 0 ; i < MAX_CASSETTE_SLOT_SIZE ; i++ ) {
		//
		if ( i == p ) continue;
		//
		if ( _SCH_CLUSTER_Get_PathRange( s , i ) < 0 ) continue;
		if ( _SCH_CLUSTER_Get_PathStatus( s , i ) == SCHEDULER_READY ) continue;
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			if ( ch == _SCH_CLUSTER_Get_PathProcessChamber( s , i , 0 , k ) ) return FALSE;
			//
		}
		//
	}
	return TRUE;
}


BOOL NEED_LOT_PRE_SPAWN( int s , int p , int ch , char *GetRecipeFilename , int *GetLotPreSkipSide ) { // 2017.09.19

	if ( !IS_THIS_FIRSTWAFER( s , p , ch ) ) return FALSE;






















	return FALSE;
}
//
