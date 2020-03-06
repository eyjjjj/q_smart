//===========================================================================================================================
#define _CRT_SECURE_NO_DEPRECATE
//===========================================================================================================================

#include <windows.h>

#include "INF_Scheduler.h"
#include "INF_CimSeqnc.h"
#include "INF_Sch_prm_Cluster.h"

#include "IO.h"
#include "UserFlow.h"
#include "LotPre.h"


BOOL MULTI_PM_MODE = FALSE; // 2018.05.08
int  MULTI_PM_LAST = 0; // 2018.05.08
int  MULTI_PM_COUNT = 0; // 2018.06.01
int  MULTI_PM_MAP[64]; // 2018.10.20
int  MULTI_PM_CLIDX = -1; // 2018.06.01

void LotPre_Init();
void Flow_Init();

void IO_CCW_Recipe_Type(int chamber,int RcpType);
int USER_EVENT_ANALYSIS( char *FullMessage , char *RunMessage , char *ElseMessage , char *ErrorMessage );

void DEPTH_INFO_INIT( int Side ); // 2019.03.05
void DEPTH_INFO_START( int Side ); // 2019.03.05

int USE_PM_CHK[MAX_CASSETTE_SIDE];

BOOL Dummy_Data[MAX_CHAMBER];
BOOL Disable_PM[MAX_CHAMBER];
BOOL Disable_PM_Event[MAX_CHAMBER];


SLOTTemplate SLOT[MAX_CASSETTE_SIDE];
void Dummy_Init() {
	int i;
	for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
		Dummy_Data[i] = FALSE;
	}
}

BOOL Dummy_Request( int ch ) {
	return ( Dummy_Data[ch] );
}

void Dummy_ReSet( int ch , BOOL msg ) {
	int i;
	//
	Dummy_Data[ch] = FALSE;
	//
	for ( i = 0 ; i < MAX_CASSETTE_SIDE ; i++ ) {
		//
		if ( !_SCH_SYSTEM_USING_RUNNING(i) ) continue;
		//
		if ( msg ) {
			_SCH_LOG_LOT_PRINTF( i , "DUMMY Reset from MESSAGE for PM%d\n" , ch - PM1 + 1 );
		}
		else {
			_SCH_LOG_LOT_PRINTF( i , "DUMMY Reset from APPENDING for PM%d\n" , ch - PM1 + 1 );
		}
		//
	}
}
void Dummy_Set( int ch , BOOL log ) {
	int i;
	//
	 Dummy_Data[ch] = TRUE;
	//
	for ( i = 0 ; i < MAX_CASSETTE_SIDE ; i++ ) {
		//
		if ( !_SCH_SYSTEM_USING_RUNNING(i) ) continue;
		//
		if ( log ) {
			_SCH_LOG_LOT_PRINTF( i , "DUMMY Set from MESSAGE for PM%d\n" , ch - PM1 + 1 );
		}
		else {
		}
		//
	}
}
void Dummy_Status() {
	int i;
	for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
		if ( Dummy_Data[i] ) {
			//printf( "PM%d Dummy_Data has been Setted\n" , i - PM1 + 1 );
		}
	}
}
void Disable_PM_Init() {
	int i;
	for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
		Disable_PM[i] = FALSE;
	}
}

BOOL GET_DISABLE_PM_STATUS( int ch ) {
	return ( Disable_PM[ch] );
}

BOOL GET_DISABLE_PM_STATUS_EVENT( int ch ) {
	return ( Disable_PM_Event[ch] );
}

void SET_DISABLE_PM_STATUS( int side , int ch ,BOOL mode )  {
		
	if(mode)
	{
		Disable_PM[ch]  = TRUE; 
		//_SCH_LOG_LOT_PRINTF( side , "SET_DISABLE_PM_STATUS PM%d - TRUE\n" , ch - PM1 + 1 );
	}
	else 
	{
		Disable_PM[ch] = FALSE;
		//_SCH_LOG_LOT_PRINTF( side, "SET_DISABLE_PM_STATUS PM%d - FALSE\n" , ch - PM1 + 1 );
	}
}

void SET_DISABLE_PM_STATUS_EVENT( int ch ,BOOL mode )  {
		
	if(mode)
	{
		Disable_PM_Event[ch]  = TRUE; 
		//_IO_CIM_PRINTF(  "SET_DISABLE_PM_STATUS_EVENT PM%d - TRUE\n" , ch - PM1 + 1 );
	}
	else 
	{
		Disable_PM_Event[ch] = FALSE;
		//_IO_CIM_PRINTF("SET_DISABLE_PM_STATUS_EVENT PM%d - FALSE\n" , ch - PM1 + 1 );
	}
}
int  RETURN_SLOT_INDEX(int ch, char *Data)
{
	int ENDINDEX;
	int SS;
	int slotdata;

	ENDINDEX = _SCH_PRM_GET_MODULE_SIZE( ch);

	//
	slotdata =-1; 

	for ( SS = 0 ; SS < ENDINDEX ; SS++ ) {
		switch( Data[SS]) {
		case '1' :
				break;
		case '2' :
				slotdata = SS+1;
				break;
			case '3' :
				break;
			case '4' :
				break;
			case '5' :
				break;
			case '6' :
				break;
			case '7' :
				break;
			case '8' :
				break;
			case '9' :
				break;
			}

		if(slotdata >0 ) return slotdata;

		}
	return -1;
}
//=======================================================================================================================

void _SCH_COMMON_REVISION_HISTORY( int mode , char *data , int count ) {
//	sprintf( data , "SM170104(%s %s)" , __DATE__ , __TIME__ );
	sprintf( data , "SM200211(%s %s)" , __DATE__ , __TIME__ );
}

void _SCH_COMMON_SYSTEM_DATA_LOG( char *filename , int mode ) {
	int i;
	FILE *fpt;
	//
	fpt = fopen( filename , "a" );
	//
	if ( fpt == NULL ) return;
	//
	for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
		if ( Dummy_Data[i] ) {
			fprintf( fpt , "PM%d Dummy_Data has been Setted\n" , i - PM1 + 1 );
		}
	}
	fclose( fpt );
}
void Reset_LOTPRE_LASTRTYPE(int side)
{
	int j;

	for(j=PM1; j<MAX_PM_CHAMBER_DEPTH + PM1;j++)
	{
		if(_SCH_MODULE_Get_Mdl_Use_Flag( side,j) == MUF_01_USE )
		{
			if(Get_LPLAST_Data (j) == 0)
			{
				_IO_CIM_PRINTF ("Set_LPLAST_Data  [ch  : %d ][-88]\n",j); 
				Set_LPLAST_Data (j, -88);
			}
		}
	}
}

//=======================================================================================================================
// Initialize Callback Function
//
void _SCH_COMMON_INITIALIZE_PART( int apmode , int Side ) {
	int i,j; // 2017.04.20
	//
	if ( apmode == 0 ) {
		//
		IO_PART_INIT();

		for(i=0; i<MAX_CASSETTE_SIDE;i++)
		{
			USE_PM_CHK[i]=FALSE;
		}

		for(j=PM1; j<MAX_PM_CHAMBER_DEPTH + PM1;j++)
		{
			SET_DISABLE_PM_STATUS_EVENT(j,FALSE); 
			//Disable_PM_Event[j]= FALSE;
		}
		//
	}
	//
	//2019.08.14
	/*
	if(( apmode == 3 ) ) //가장 먼저 시작 하는 Lot 인 경우 다시 한번 초기화 (Loading 시 Evant로 Disable 하는 부분이 있으므로 )
	{
		for(j=PM1; j<MAX_PM_CHAMBER_DEPTH + PM1;j++)
		{
			SET_DISABLE_PM_STATUS_EVENT(j,FALSE); 
			//Disable_PM_Event[j]= FALSE;
			//Reset_LPLAST_Data(j,-3);	
		}
	}
	*/
	if(( apmode == 1 ) || ( apmode == 3 ) )
	{
		//printf("\nSCH_COMMON_INITIALIZE_PART [%d]\n" , apmode);
		USE_PM_CHK[Side]=TRUE;
	}
	//
	if(apmode == 1)
	{
		//초기화 추가 
		Reset_LOTPRE_LASTRTYPE(Side);
	}

	if ( ( apmode == 3 ) || ( apmode == 4 ) ) { // 2017.04.20
		//
		_SCH_PRM_SET_MFI_INTERFACE_FLOW_USER_OPTION( 6 , 1 ); // 2018.02.11
		//
		Dummy_Init();
		//
		LotPre_Init();
		//
		Flow_Init();
		//
		Disable_PM_Init();
		//
		MULTI_PM_MODE = FALSE; // 2018.05.08
		MULTI_PM_LAST = 0; // 2018.05.08
		MULTI_PM_COUNT = 0; // 2018.06.01
		MULTI_PM_CLIDX = -1; // 2018.06.01
		//
		for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
			//
			IO_PM_REMAIN_SET( i , 0 );
			//
			if ( _SCH_PRM_GET_MODULE_SIZE( i ) > 1 ) MULTI_PM_MODE = TRUE; // 2018.05.08
			//
		}
		//
	}
	//
	if ( apmode == 1 ) {
		if ( MULTI_PM_MODE ) DEPTH_INFO_INIT( Side ); // 2019.03.05

	}
	//
	if ( apmode == 2 || apmode == 4 ) {
	
			if(_SCH_SYSTEM_MODE_END_GET( Side )>0)
			{
				_SCH_LOG_LOT_PRINTF( Side , "END - [%d]\n" , _SCH_SYSTEM_MODE_END_GET( Side ) );
				
				for(j=PM1; j<MAX_PM_CHAMBER_DEPTH + PM1;j++)
				{
					if(_SCH_MODULE_Get_Mdl_Use_Flag( Side,j) == MUF_01_USE )
					{
						_SCH_LOG_LOT_PRINTF( Side , "END - [ch : %d]\n" , j ); 
					}
				}
			}
	}
}
int _SCH_COMMON_CHECK_PICK_FROM_FM( int side , int pt , int subchk ) { // 2016.05.10 : 이곳에서 DB를 변경하는 작업은 아래 _SCH_COMMON_USER_FLOW_NOTIFICATION()에서 진행한다.
	int iResult , Res;
	
	BOOL No_Wafer =TRUE;
	//
	//---------------------------------------------------------------------------------
	//
	Res = IO_CONTROL_USE();
	//
	if ( !MULTI_PM_MODE && ( Res == 1 ) ) return TRUE;
	//
	if ( MULTI_PM_MODE ) DEPTH_INFO_START( side ); // 2019.03.05
	//
	//---------------------------------------------------------------------------------
	//
	if ( ( subchk < -1 ) || ( subchk >= 100 ) ) {
		return TRUE;
	}
	//
	//---------------------------------------------------------------------------------

	/*
	if(USE_PM_CHK[side])
	{
		//사용 하는 PM 중 Disable 상태인 PM 을 찾아서 IO 변경
		for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
		//
			if( _SCH_MODULE_Get_Mdl_Use_Flag(side, i ) == MUF_99_USE_to_DISABLE ){//||_SCH_MODULE_Get_Mdl_Use_Flag(side, i ) == MUF_98_USE_to_DISABLE_RAND ){
				//IO_FALVS_DLP_SET(i,1);
				//_SCH_MODULE_Get_Mdl_Use_Flag(Side, i );
				//_SCH_LOG_LOT_PRINTF( side , "_SCH_COMMON_CHECK_PICK_FROM_FM-1  PM%d - [%d]\n" , i-PM1+1,GET_DISABLE_PM_STATUS( i ));
				if(!GET_DISABLE_PM_STATUS(i))
				{
					SET_DISABLE_PM_STATUS( side,i ,TRUE); 
					RemainTimeSetting( TRUE );
					//_SCH_LOG_LOT_PRINTF( side , "_SCH_COMMON_CHECK_PICK_FROM_FM-2 PM%d - [%d]\n" , i-PM1+1,_SCH_MODULE_Get_Mdl_Use_Flag(side, i ));
				}
			}
			USE_PM_CHK[side]=FALSE;
		}
		//return FALSE; 
	}
	else
	{
		for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
		//
//		_SCH_LOG_LOT_PRINTF( side , "[%d][%d]_SCH_COMMON_CHECK_PICK_FROM_FM-3 PM%d - [%d]\n" , GET_DISABLE_PM_STATUS(i),GET_DISABLE_PM_STATUS_EVENT(i),i-PM1+1,_SCH_MODULE_Get_Mdl_Use_Flag(side, i ));
		
		if (_SCH_MODULE_Get_Mdl_Use_Flag(side, i ) == MUF_01_USE && GET_DISABLE_PM_STATUS(i) 
				 || _SCH_MODULE_Get_Mdl_Use_Flag(side, i ) == MUF_01_USE &&GET_DISABLE_PM_STATUS_EVENT(i))
		{
			//문제 부분
				 RemainTimeSetting( TRUE );
				//강제로 Lotpre 진행
				 if( _SCH_MACRO_CHECK_PROCESSING(i) == 1) 
				 {
					// _SCH_LOG_LOT_PRINTF( side  , "_SCH_MACRO_CHECK_PROCESSING[%d]\n" , _SCH_MACRO_CHECK_PROCESSING(i) );
					//Process 중 Pick 거절 
					 return 1;
					 // continue;
				 }
				 else
				 {
					 //Code 보강 위치 
					//TM이 해당 PM으로 동작 중인 경우 Pick 거절
					
					 _SCH_SYSTEM_RUNCHECK_GET()
					 for(j=0;j<4;j++)
					 {
						if(_SCH_MODULE_Get_PM_WAFER( i , j )>0) No_Wafer = FALSE;
					 }

					if(No_Wafer)
					{
						strcpy( recipe , _SCH_PREPOST_Get_inside_READY_RECIPE_NAME( side , i ));
						//_SCH_LOG_LOT_PRINTF( side  , "[%d][_SCH_COMMON_CHECK_PICK_FROM_FM]_SCH_PREPOST_Get_inside_READY_RECIPE_NAME[%s]\n" ,Res, recipe );
						Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , i , _SCH_CLUSTER_Get_SlotIn( side , pt ) , recipe ,11 , 0 );
						//_SCH_LOG_LOT_PRINTF( side  , "[%d][_SCH_COMMON_CHECK_PICK_FROM_FM]_SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING[%s]\n" ,Res, recipe );
					
						Res = _SCH_MACRO_LOTPREPOST_PROCESS_OPERATION(side , i, side+1,  0, 0, recipe,99,0, "", 1, 2, 0); 
			
						//	
						_SCH_LOG_LOT_PRINTF( side , "[LOTPRE]  DISABLE  -> ENABLE  PM%d [%d]\n" , i - PM1 + 1,GET_DISABLE_PM_STATUS(i) );
						LotPre_Last_Supply(side , pt , i,FALSE );
				
						if(GET_DISABLE_PM_STATUS(i) )
						{
							SET_DISABLE_PM_STATUS( side,i ,FALSE);
						}

						if(GET_DISABLE_PM_STATUS_EVENT(i))
						{
							SET_DISABLE_PM_STATUS_EVENT(i,FALSE); 
						}
					}
				}
			}
		}
	}
	*/
	//
//	iResult = USER_FLOW_CHECK_PICK_FROM_FM( side , pt , ( subchk == -1 ) ); // 2017.09.19
	//
	while ( TRUE ) { // 2017.09.19
		//
		iResult = USER_FLOW_CHECK_PICK_FROM_FM( side , pt , ( subchk == -1 ) ); // 2017.09.19
		//
	//	_SCH_LOG_LOT_PRINTF( side , "USER_FLOW_CHECK_PICK_FROM_FM-iResult[%d]\n" , iResult);
		//
		if ( iResult != -9999 ) break; // 2017.09.19
		//

	} // 2017.09.19

	if ( iResult > 0 ) {
		return TRUE;
	}
	//
	return FALSE;
}
//SCH_COMMON_USER_FLOW_NOTIFICATION_MREQ( MACRO_PICK , 102 , CHECKING_SIDE , pt , SCH_Chamber , _SCH_CLUSTER_Get_PathDo( CHECKING_SIDE , pt ) , -1 , tms , cf ) 
int _SCH_COMMON_USER_FLOW_NOTIFICATION( int mode , int usertag , int side , int pt , int opt1 , int opt2 , int opt3 , int opt4 , int opt5 ) 
{										// mode = 0(pick인 경우) , usertag = 1(CM)
										// side(Arm A의 Side) , pt(Arm A의 Pointer) , opt1(Arm A의 Wafer) , opt2(Arm B의 Side) , opt3(Arm B의 Pointer) , opt4(Arm B의 Wafer)
										// return : abort(0) , reject(1) , continue(-1)

	int iResult = 0;
	int ts, tr, td,tt; 


	if ( ( mode == 0 ) && ( usertag == 1 ) ) {
		//
		if ( !MULTI_PM_MODE && ( IO_CONTROL_USE() == 1 ) ) return -1; // Continue;
		//
		iResult = USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM( side , pt , opt1 , opt2 , opt3 , opt4 );
		//
		if ( iResult == 2 ) { // A only
			return -2; // Continue;
		}
		if ( iResult > 0 ) {
			return -1; // Continue;
		}
		//
		return 1; // Reject
		//
	}
	else if ( ( mode == 0 ) && ( usertag == 102 ) ) {
		//
		if ( !MULTI_PM_MODE && ( IO_CONTROL_USE() == 1 ) ) return -1; // Continue;
		//
		//2020.02.11===========================================================================
		if(opt4 > 0)
		{
			//Arm Wafer 가 있거나 B Arm 
			if(_SCH_MODULE_Get_TM_WAFER( opt4 , TA_STATION ) >0)
			{
				ts = _SCH_MODULE_Get_TM_SIDE( opt4 , TA_STATION );
				tt =_SCH_MODULE_Get_TM_POINTER( opt4 , TA_STATION );
				td = _SCH_CLUSTER_Get_PathDo( ts , tt );
				tr = _SCH_CLUSTER_Get_PathRange( ts , tt );
				
				if(tr > td ) return 1; // Reject				
			}
			else if(_SCH_MODULE_Get_TM_WAFER( opt4 , TB_STATION ) >0)
			{
				ts = _SCH_MODULE_Get_TM_SIDE( opt4 , TB_STATION );
				tt =_SCH_MODULE_Get_TM_POINTER( opt4 , TB_STATION );
				td = _SCH_CLUSTER_Get_PathDo( ts , tt );
				tr = _SCH_CLUSTER_Get_PathRange( ts , tt );
				
				if(tr > td ) return 1; // Reject			
			}
		}
		//============================================================================

		iResult = USER_FLOW_NOTIFICATION_FOR_PICK_FROM_BM( side , pt , opt1 );
		//
		if ( iResult > 0 ) {

			return -1; // Continue;
		}
		//
		return 1; // Reject
		//
	}
	else if ( ( mode == 1 ) && ( usertag == 10002 ) ) {
		//
		if ( !MULTI_PM_MODE && ( IO_CONTROL_USE() == 1 ) ) return -1; // Continue;
		//
		iResult = USER_FLOW_NOTIFICATION_FOR_PLACE_TO_BM( side , pt , opt1 );
		//
		if ( iResult > 0 ) {
			return -1; // Continue;
		}
		//
		return 1; // Reject
		//
	}
	else if( ( mode == 1 ) && ( usertag == 101 ) ) 
	{
		_SCH_LOG_LOT_PRINTF( side  , "LotPre_Last_Supply-TURE5\n");
		//LotPre_Last_Supply( side , pt , opt1 ,TRUE);
		return -1; // Continue;
	}
	else {
		return -1; // Continue;
	}
}


BOOL _SCH_COMMON_SIDE_SUPPLY( int side , BOOL EndCheck , int Supply_Style , int *Result ) {
	//
	if ( MULTI_PM_MODE || ( IO_CONTROL_USE() != 1 ) ) {
		//
		*Result = TRUE;

		return TRUE;
		//
	}
	//
	return FALSE;
	//
}


BOOL _SCH_COMMON_CASSETTE_CHECK_INVALID_INFO( int mode , int Side , int Cm1_Chamber , int Cm2_Chamber , int option ) { // 2018.11.07
	if ( mode == 10 ) {
		if ( ( Cm1_Chamber > TM ) && ( Cm1_Chamber < FM ) ) return FALSE;
	}
	return TRUE;
}

BOOL _SCH_COMMON_METHOD_CHECK_SKIP_WHEN_START( int mode , int side , int ch , int slot , int option ) {
	if ( mode == 0 ) {
		if ( ( ch > TM ) && ( ch < FM ) ) {
			if ( !_SCH_MODULE_GET_USE_ENABLE( ch , FALSE , -1 ) ) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

int Serch_Use_Side_for_PM(int ch)
{
	int i;
	int _pre_runorder; 
	int	runorder; 
	int _side;

	_pre_runorder =-1;
	_side = -1;
	for(i=0;i<MAX_CASSETTE_SIDE;i++)
	{
		if(_SCH_MODULE_Get_Mdl_Use_Flag( i, ch ) == MUF_99_USE_to_DISABLE )
		{
			runorder =_SCH_SYSTEM_RUNORDER_GET(i);

			if( runorder ==1000)
			{
				return i;
			}
			else 
			{
				if(_pre_runorder<0)
				{
					_side = i;
					_pre_runorder = runorder;
				}
				else
				{
					if(	_pre_runorder > runorder)
					{
						_side = i;
						_pre_runorder = runorder;
					}
				}
			}
		}

	}
	return _side;
}


BOOL _SCH_COMMON_GET_UPLOAD_MESSAGE( int mode , int ch , char *message ) { // 2017.01.03
	//
	int pmm , c;
	//
	if ( ( mode / 10 ) > 0 ) { // simulation
		//
		switch( mode % 10 ) {
		case 0 : // postrecv
			//
			pmm = IO_PM_POSTX_GET_MODE( ch );
			//
			if      ( pmm == 0 ) {
			}
			else if ( pmm == 1 ) {
				strcpy( message , "POSTSKIP" );
				return TRUE;
			}
			else {
				//
				c = IO_PM_POSTX_GET_COUNT( ch );
				//
				c++;
				//
				if ( c >= ( pmm - 1 ) ) {
					IO_PM_POSTX_SET_COUNT( ch , 0 );
				}
				else {
					IO_PM_POSTX_SET_COUNT( ch , c );
					strcpy( message , "POSTSKIP" );
					return TRUE;
				}
				//
			}
			//
			break;

		case 1 : // process

			break;
		}
		//
	}
	//
	return FALSE;

}

int  _SCH_COMMON_EVENT_ANALYSIS( char *FullMessage , char *RunMessage , char *ElseMessage , char *ErrorMessage )
{
int i,pch;
	char chamberStr[1024] , typeStr[1024];
	char NewFullMessage[1024],NewElseMessage[1024];

	/*	printf ("_SCH_COMMON_EVENT_ANALYSIS - FullMessage - {%s}\n",FullMessage);
		printf ("_SCH_COMMON_EVENT_ANALYSIS - RunMessage - {%s}\n", RunMessage);
		printf ("_SCH_COMMON_EVENT_ANALYSIS - ElseMessage - {%s}\n",ElseMessage);*/



	if(  STRCMP_L( RunMessage , "DUMMY_SUPPLY_REQUEST" ) ) {
	//
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - FullMessage - {%s}\n",FullMessage);
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - RunMessage - {%s}\n", RunMessage);
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - ElseMessage - {%s}\n",ElseMessage);


		STR_SEPERATE_CHAR( ElseMessage , '|' , chamberStr   , typeStr , 1024 );
	
	
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - chamberStr - {%s}\n", chamberStr);
		

		pch = atoi( ElseMessage + 2 );
		if(strlen(typeStr)==0)
		{
			sprintf(typeStr,"0");
		}
		
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - pch-{%d}  typeStr - {%s}\n",pch,typeStr);

		IO_CCW_Recipe_Type(pch,atoi(typeStr));
		//printf("----------\n");
		sprintf(NewFullMessage,"DUMMY_SUPPLY_REQUEST PM%d",pch);
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - NewFullMessage - {%s}\n",NewFullMessage);
		sprintf(NewElseMessage,"PM%d",pch);
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - NewElseMessage - {%s}\n",NewElseMessage);
		
		
		i = USER_EVENT_ANALYSIS(NewFullMessage,RunMessage,NewElseMessage,ErrorMessage);

	}

	else 
	{
		i = USER_EVENT_ANALYSIS(FullMessage,RunMessage,ElseMessage,ErrorMessage);
	}
	return i;
}
BOOL RUN_STATUS(int ch)
{
	int i,j;
	int useFlag;
	int us;

	for(i=0; i<MAX_CASSETTE_SIDE;i++)
	{
		useFlag = -1; 
		us = _SCH_SYSTEM_USING_GET(i);

		_IO_CIM_PRINTF("RUN_STATUS PM%d - [%d][%d]\n" ,ch - PM1 + 1,  _SCH_SYSTEM_USING_RUNNING(i),_SCH_SYSTEM_EQUIP_RUNNING_GET(i));
		if( _SCH_SYSTEM_USING_RUNNING(i))  //해당 Side 사용 중 
		{
			useFlag = 	 _SCH_MODULE_Get_Mdl_Use_Flag( i, ch );

			_IO_CIM_PRINTF("[SIDE : %d]RUN_STATUS PM%d - [%d]\n" , i,ch - PM1 + 1, useFlag);

			if(useFlag = MUF_99_USE_to_DISABLE) return TRUE;
		}
		else if(us>0 && us < 10)
		{
			if(_SCH_SYSTEM_USING_ANOTHER_RUNNING(i))
			{
				for(j=0; j<MAX_CASSETTE_SIDE;j++)
				{
					if(i == j )continue;
					
					if( _SCH_SYSTEM_USING_RUNNING(j))
					{
						if( _SCH_MODULE_Get_Mdl_Use_Flag( j, ch ) == MUF_00_NOTUSE)
						{
							useFlag = 	 _SCH_MODULE_Get_Mdl_Use_Flag( i, ch );
	
							_IO_CIM_PRINTF("[SIDE : %d]RUN_STATUS2 PM%d - [%d]\n" , i,ch - PM1 + 1, useFlag);

							if(useFlag = MUF_99_USE_to_DISABLE) return TRUE;				

						}

					}
				}	
			}
			else 
			{
				useFlag = 	 _SCH_MODULE_Get_Mdl_Use_Flag( i, ch );

				_IO_CIM_PRINTF("[SIDE : %d]RUN_STATUS3 PM%d - [%d]\n" , i,ch - PM1 + 1, useFlag);

				if(useFlag = MUF_99_USE_to_DISABLE) return TRUE;
			}
		}
	}

	return FALSE;
}

int  _SCH_COMMON_EVENT_PRE_ANALYSIS(char *FullMessage , char *RunMessage , char *ElseMessage , char *ErrorMessage )
{
	int pch,pch2;
	int s,i;
	int Cu_LastIndex;
	char chamberStr[256] , typeStr[256];
	int s2;
	//char MsgStr[1024];

	//printf ("_SCH_COMMON_EVENT_PRE_ANALYSIS - FullMessage - {%s}\n",FullMessage);
	//printf ("_SCH_COMMON_EVENT_PRE_ANALYSIS - RunMessage - {%s}\n", RunMessage);
	//printf ("_SCH_COMMON_EVENT_PRE_ANALYSIS - ElseMessage - {%s}\n",ElseMessage);
	 	
	if ( STRCMP_L( RunMessage , "SET_MODULE_INFO_LOCAL(F)" ) ||STRCMP_L( RunMessage , "SET_MODULE_INFO_LOCAL" ) || 	STRCMP_L( RunMessage , "SET_MODULE_INFO" )) 
	{
	//
		STR_SEPERATE_CHAR( ElseMessage , '|' , chamberStr   , typeStr , 1024 );
			
		//printf ("_SCH_COMMON_EVENT_ANALYSIS - chamberStr-{%s}  typeStr - {%s}\n",chamberStr,typeStr);

		if ( STRNCMP_L( chamberStr , "PM" , 2 ) ) {

			pch = atoi( chamberStr + 2 ) - 1 + PM1;
			pch2 = atoi( chamberStr + 2 );
			
			if(pch2%2 == 0) return 1;

			//if( _SCH_MODULE_Get_Mdl_Use_Flag(side, pch ) == MUF_00_NOTUSE ) return 1;

			if(STRCMP_L( typeStr , "DISABLE" ) ||STRCMP_L( typeStr , "DISABLEHW" ) )
			{
				//printf ("_SCH_COMMON_EVENT_ANALYSIS - pch-{%d}  typeStr - {%s}\n",pch,typeStr);
				
				//추가 
				 IO_PM_REMAIN_SET( pch , 0 ); 

				if(!GET_DISABLE_PM_STATUS_EVENT(pch))
				{
				
					SET_DISABLE_PM_STATUS_EVENT(pch,TRUE); 
					RemainTimeSetting( TRUE );

					_IO_CIM_PRINTF("[Event]PM%d Enable -> Disable  \n" , pch-PM1+1);
					//printf("[Event]PM%d Enable -> Disable  \n" , pch-PM1+1);
				}
			}
			else if(STRCMP_L( typeStr , "ENABLE" ))
			{
				s2 = Serch_Use_Side_for_PM(pch);

				if(  EQUIP_STATUS_PROCESS(pch) == SYS_RUNNING) //Process 상태인 경우 
				{
					_IO_CIM_PRINTF("[Event]PM%d Disable ->  Enable (PROCESS RUNNING)\n" , pch-PM1+1);
				}
				else
				{
								
					if(GET_DISABLE_PM_STATUS_EVENT(pch))
					{
						SET_DISABLE_PM_STATUS_EVENT(pch,FALSE); 
						//RemainTimeSetting( TRUE );
						//Reset_LPLAST_Data(pch,-1);		

						Cu_LastIndex = Get_LPLAST_Data (pch);

						if (Cu_LastIndex == 0 ||Cu_LastIndex<0)
						{
							Cu_LastIndex = 99;
						}
						
						_IO_CIM_PRINTF ("Set_LPLAST_Data2  [ch  : %d ][%d]\n",pch,Cu_LastIndex * -1); 
						Set_LPLAST_Data(pch,Cu_LastIndex * -1);	
						
						_IO_CIM_PRINTF("[Event side : %d]PM%d Disable ->  Enable (%d,%d)\n" ,s2, pch-PM1+1,Cu_LastIndex,Cu_LastIndex * -1);
						
						//printf("[Event]PM%dDisable ->  Enable \n" , pch-PM1+1);

					}
					else
					{
						if(RUN_STATUS(pch))
						{
							Cu_LastIndex = Get_LPLAST_Data (pch);

							if (Cu_LastIndex == 0 ||Cu_LastIndex<0)
							{
								Cu_LastIndex =99;
							}

							_IO_CIM_PRINTF ("Set_LPLAST_Data3  [ch  : %d ][%d]\n",pch,Cu_LastIndex * -1); 
							Set_LPLAST_Data(pch,Cu_LastIndex * -1);	

							_IO_CIM_PRINTF("[Event2 side : %d]PM%d Disable ->  Enable (%d,%d)\n" ,s2, pch-PM1+1,Cu_LastIndex,Cu_LastIndex * -1);
						}
						_IO_CIM_PRINTF("[Event3   side : %d]PM%d Disable ->  Enable (%d,%d)\n" ,s2, pch-PM1+1,Cu_LastIndex,Cu_LastIndex * -1);
					}
					_IO_CIM_PRINTF("[Event4   side : %d]PM%d Disable ->  Enable (%d,%d)\n" ,s2, pch-PM1+1,Cu_LastIndex,Cu_LastIndex * -1);
				}
			}

		}
		//
	}

	else if ( STRNCMP_L( RunMessage , "STOP" , 4 ))
	{
		if ( strlen( RunMessage ) > 4 ) {

		s=  atoi( RunMessage + 4 ) - 1 ;

		//printf("_SCH_COMMON_EVENT_PRE_ANALYSIS s : %d\n",s);
		}
		else 
		{
			s = 0;
		}

		for( i=PM1;i<BM1;i++)
		{
			//printf("_SCH_COMMON_EVENT_PRE_ANALYSIS Use_Flag: %d\n",_SCH_MODULE_Get_Mdl_Use_Flag(s,i));
			if( _SCH_MODULE_Get_Mdl_Use_Flag(s,i) == MUF_01_USE  )
			{
				LotPre_Last_Supply( s , -1 ,i ,FALSE );
			}
		}
	}

   return 1;

}
//2020.03.06
void FM_ROBOT(int Arm,int ch)
{
		int fs, fp,fd,fr,fpo;

		if (_SCH_MODULE_Get_FM_WAFER(Arm)>0)
		{
			fs=_SCH_MODULE_Get_FM_SIDE( Arm );
			fp=_SCH_MODULE_Get_FM_POINTER( Arm );

			fd = _SCH_CLUSTER_Get_PathDo( fs , fp );
			fr = _SCH_CLUSTER_Get_PathRange( fs , fp );

			if (fr > fd )
			{
				fpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			
				if( _SCH_CLUSTER_Get_PathProcessChamber(fs , fp , 0, fpo) > 0 )
				{
					_SCH_CLUSTER_Set_PathDo( fs , fp , PATHDO_WAFER_RETURN );
					_SCH_CLUSTER_Set_PathStatus ( fs , fp ,SCHEDULER_RETURN_REQUEST);
				}
			}
		}
}
void TM_ROBOT(int tms,int Arm ,int ch)
{
	int tr,td, ts,tt;
	int tpo; 
	int tst;

	if(_SCH_MODULE_Get_TM_WAFER( tms , Arm ) >0)
	{
		ts = _SCH_MODULE_Get_TM_SIDE( tms , Arm );
		tt =_SCH_MODULE_Get_TM_POINTER( tms , Arm );
		td = _SCH_CLUSTER_Get_PathDo( ts , tt );
		tr = _SCH_CLUSTER_Get_PathRange( ts , tt );

		if (tr > td )
		{
			tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
	
			if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
			{
				_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
				_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
			}
		}
		else if(tr == td)
		{
			tst  = _SCH_CLUSTER_Get_PathStatus( ts , tt );

			if(tst < SCHEDULER_BM_END )
			{
				tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			
				if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
				{
					_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
					_SCH_CLUSTER_Set_PathStatus (ts , tt ,SCHEDULER_RETURN_REQUEST);
				}
			}
		}
	}
}
void MAKE_RETURN_WAFER(int ch)
{
	int tms;
	int i,j,size;
	int bs,bt;
	int po;
	//int tpo; 
	int bd, br;
	//int tr,td, ts,tt;
	int k; 
	int bst;
	//int tst;
	//int fs, fp,fd,fr,fpo;
	int	als, alp,ald,alr ,alpo;
////	int time_data,t;
//
//	t = IO_GET_PM_REMAIN_TIME( ch );
//	IO_PM_REMAIN_TIME( ch , t );
//
	tms = _SCH_PRM_GET_MODULE_GROUP( ch );
	//t = IO_GET_PM_REMAIN_TIME( ch );
	//time_data = t+50000;

	//IO_PM_REMAIN_TIME( ch  , time_data );

	if(tms ==0) //TM1 
	{
		for (i = BM1 ; i<BM1+MAX_BM_CHAMBER_DEPTH; i++)
		{
			if ( !_SCH_PRM_GET_MODULE_MODE( i ) ) continue;
			if( tms !=_SCH_PRM_GET_MODULE_GROUP( i )) continue;  //TM1에 속해 있는 BM일 경우 TM2에 속해 있는 BM check Skip 

			if(i ==  _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER( )) continue; // Dummy 공급 용 BM check Skip 

			size = _SCH_PRM_GET_MODULE_SIZE( i );

			for (j = 1; j<size+1 ;j++)
			{
				if(_SCH_MODULE_Get_BM_WAFER( i , j ) >0)
				{
					bs = _SCH_MODULE_Get_BM_SIDE( i , j );
					bt =_SCH_MODULE_Get_BM_POINTER( i , j );
					bd = _SCH_CLUSTER_Get_PathDo( bs , bt );
					br = _SCH_CLUSTER_Get_PathRange( bs , bt );
					bst  = _SCH_CLUSTER_Get_PathStatus( bs , bt );

					if (bd > br )continue;

					if(bd == br && bst > SCHEDULER_WAITING ) continue;

					po  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
					
					if( _SCH_CLUSTER_Get_PathProcessChamber(bs , bt , 0, po) > 0 )
					{
						_SCH_CLUSTER_Set_PathDo( bs , bt , PATHDO_WAFER_RETURN );
						_SCH_CLUSTER_Set_PathStatus ( bs , bt ,SCHEDULER_RETURN_REQUEST);
					}

				}
			}
		}
		
		if ( _SCH_ROBOT_GET_FINGER_HW_USABLE( tms, TA_STATION ) )
		{

			TM_ROBOT(tms,TA_STATION,ch);

			//if(_SCH_MODULE_Get_TM_WAFER( tms , TA_STATION ) >0)
			//{
			//	ts = _SCH_MODULE_Get_TM_SIDE( tms , TA_STATION );
			//	tt =_SCH_MODULE_Get_TM_POINTER( tms , TA_STATION );
			//	td = _SCH_CLUSTER_Get_PathDo( ts , tt );
			//	tr = _SCH_CLUSTER_Get_PathRange( ts , tt );

			//	if (tr > td )
			//	{
			//		tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			//
			//		if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
			//		{
			//			_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
			//			_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
			//		}
			//	}
			//	else if(tr == td)
			//	{
			//			tst  = _SCH_CLUSTER_Get_PathStatus( ts , tt );

			//			if(tst < SCHEDULER_BM_END )
			//			{
			//				tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			//
			//				if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
			//				{
			//					_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
			//					_SCH_CLUSTER_Set_PathStatus (ts , tt ,SCHEDULER_RETURN_REQUEST);
			//				}
			//			}
			//	}
			//}
		}
		if ( _SCH_ROBOT_GET_FINGER_HW_USABLE( tms, TB_STATION ) )
		{
			TM_ROBOT(tms,TA_STATION,ch);

			//if(_SCH_MODULE_Get_TM_WAFER( tms , TB_STATION ) >0)
			//{
			//	ts = _SCH_MODULE_Get_TM_SIDE( tms , TB_STATION );
			//	tt =_SCH_MODULE_Get_TM_POINTER( tms , TB_STATION );
			//	td = _SCH_CLUSTER_Get_PathDo( ts , tt );
			//	tr = _SCH_CLUSTER_Get_PathRange( ts , tt );

			//	if (tr > td )
			//	{
			//		tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			//
			//		if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
			//	{
			//		_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
			//		_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
			//	}
			//}
			//else if(tr == td)
			//{
			//		tst  = _SCH_CLUSTER_Get_PathStatus( ts , tt );

			//		if(tst < SCHEDULER_BM_END )
			//		{
			//			tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			//
			//			if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
			//			{
			//				_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
			//				_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
			//			}
			//		}
			//	}
			//}
		}	
	}
	else if (tms ==1 ) //TM2
	{
		for (i = BM1 ; i<BM1+MAX_BM_CHAMBER_DEPTH; i++)
		{
			if ( !_SCH_PRM_GET_MODULE_MODE( i ) ) continue;

			size = _SCH_PRM_GET_MODULE_SIZE( i );

			for (j = 1; j<size+1 ;j++)
			{
				if(_SCH_MODULE_Get_BM_WAFER( i , j ) >0)
				{
					bs = _SCH_MODULE_Get_BM_SIDE( i , j );
					bt =_SCH_MODULE_Get_BM_POINTER( i , j );
					bd = _SCH_CLUSTER_Get_PathDo( bs , bt );
					br = _SCH_CLUSTER_Get_PathRange( bs , bt );			
					bst  = _SCH_CLUSTER_Get_PathStatus( bs , bt );

					if (bd > br )continue;

					if(bd == br && bst > SCHEDULER_WAITING ) continue;


					po  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
					
					if( _SCH_CLUSTER_Get_PathProcessChamber(bs , bt , 0, po) > 0 )
					{
							_SCH_CLUSTER_Set_PathDo( bs , bt , PATHDO_WAFER_RETURN );
							_SCH_CLUSTER_Set_PathStatus ( bs , bt ,SCHEDULER_RETURN_REQUEST);
					}

				}
			}
		}
		for(k=0; k<tms+1;k++)
		{
			TM_ROBOT(k,TA_STATION,ch);

/*			if(_SCH_MODULE_Get_TM_WAFER( k , TA_STATION ) >0)
			{
				ts = _SCH_MODULE_Get_TM_SIDE( k , TA_STATION );
				tt =_SCH_MODULE_Get_TM_POINTER( k , TA_STATION );
				td = _SCH_CLUSTER_Get_PathDo( ts , tt );
				tr = _SCH_CLUSTER_Get_PathRange( ts , tt );

				if (tr >td )
				{
					tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			
					if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
					{
						_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
						_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
					}
				}
				else if(tr == td)
				{
					tst  = _SCH_CLUSTER_Get_PathStatus( ts , tt );

					if(tst < SCHEDULER_BM_END )
					{
						tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			
						if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
						{
							_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
							_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
						}
					}
				}
			}*/	
			TM_ROBOT(k,TB_STATION,ch);
			//if(_SCH_MODULE_Get_TM_WAFER( k , TB_STATION ) >0)
			//{
			//	ts = _SCH_MODULE_Get_TM_SIDE( k , TB_STATION );
			//	tt =_SCH_MODULE_Get_TM_POINTER( k , TB_STATION );
			//	td = _SCH_CLUSTER_Get_PathDo( ts , tt );
			//	tr = _SCH_CLUSTER_Get_PathRange( ts , tt );

			//	if (tr > td )
			//	{
			//		tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			//
			//		if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
			//		{
			//			_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
			//			_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
			//		}
			//	}
			//	else if(tr == td)
			//	{
			//		tst  = _SCH_CLUSTER_Get_PathStatus( ts , tt );

			//		if(tst < SCHEDULER_BM_END )
			//		{
			//			tpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			//
			//			if( _SCH_CLUSTER_Get_PathProcessChamber(ts , tt , 0, tpo) > 0 )
			//			{
			//				_SCH_CLUSTER_Set_PathDo( ts , tt , PATHDO_WAFER_RETURN );
			//				_SCH_CLUSTER_Set_PathStatus ( ts , tt ,SCHEDULER_RETURN_REQUEST);
			//			}
			//		}
			//	}
			//}
		}
	}
	else 
	{
		//현재 Error
	}

	//FM
	if ( _SCH_ROBOT_GET_FM_FINGER_HW_USABLE( TA_STATION ) )
	{
		FM_ROBOT(TA_STATION , ch ); //2020.03.06
		//2020.03.06
		//if (_SCH_MODULE_Get_FM_WAFER(TA_STATION)>0)
		//{
		//	fs=_SCH_MODULE_Get_FM_SIDE( TA_STATION );
		//	fp=_SCH_MODULE_Get_FM_POINTER( TA_STATION );

		//	fd = _SCH_CLUSTER_Get_PathDo( fs , fp );
		//	fr = _SCH_CLUSTER_Get_PathRange( fs , fp );

		//	if (fr > fd )
		//	{
		//		fpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
		//	
		//		if( _SCH_CLUSTER_Get_PathProcessChamber(fs , fp , 0, fpo) > 0 )
		//		{
		//			_SCH_CLUSTER_Set_PathDo( fs , fp , PATHDO_WAFER_RETURN );
		//			_SCH_CLUSTER_Set_PathStatus ( fs , fp ,SCHEDULER_RETURN_REQUEST);
		//		}
		//	}
		//}
	}
	if ( _SCH_ROBOT_GET_FM_FINGER_HW_USABLE( TB_STATION ) )
	{
		FM_ROBOT(TB_STATION , ch ); //2020.03.06
		//2020.03.06
		//if (_SCH_MODULE_Get_FM_WAFER(TB_STATION)>0)
		//{
		//	fs=_SCH_MODULE_Get_FM_SIDE( TB_STATION );
		//	fp=_SCH_MODULE_Get_FM_POINTER( TB_STATION );

		//	fd = _SCH_CLUSTER_Get_PathDo( fs , fp );
		//	fr = _SCH_CLUSTER_Get_PathRange( fs , fp );

		//	if (fr > fd )
		//	{
		//		fpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
		//	
		//		if( _SCH_CLUSTER_Get_PathProcessChamber(fs , fp , 0, fpo) > 0 )
		//		{
		//			_SCH_CLUSTER_Set_PathDo( fs , fp , PATHDO_WAFER_RETURN );
		//			_SCH_CLUSTER_Set_PathStatus ( fs , fp ,SCHEDULER_RETURN_REQUEST);
		//		}
		//	}
		//}
	}

	//AL
	if(_SCH_MODULE_Get_MFALS_WAFER( 1 )>0)
	{
		als =_SCH_MODULE_Get_MFALS_SIDE(1);
		alp =_SCH_MODULE_Get_MFALS_POINTER(1);

		ald = _SCH_CLUSTER_Get_PathDo( als , alp );
		alr = _SCH_CLUSTER_Get_PathRange( als , alp );

		if (alr > ald )
		{
			alpo  =  _SCH_MODULE_Get_PM_PATHORDER( ch,0 ); 
			
			if( _SCH_CLUSTER_Get_PathProcessChamber(als , alp , 0, alpo) > 0 )
			{
				_SCH_CLUSTER_Set_PathDo( als , alp , PATHDO_WAFER_RETURN );
				_SCH_CLUSTER_Set_PathStatus ( als , alp ,SCHEDULER_RETURN_REQUEST);
			}
		}
	}
}

int Serch_PT(int side, int slot)
{
	int i;

	for(i=0;i<MAX_CLUSTER_DEPTH;i++)
	{
		if(_SCH_CLUSTER_Get_SlotIn( side, i )   == slot ) return i;
	}

	return -1;
}
int USER_EVENT_ANALYSIS( char *FullMessage , char *RunMessage , char *ElseMessage , char *ErrorMessage ) {
	//
	int pch;
	int slot;
	int t;
	int side;
	char sideStr[256];
	char chamberStr[256];
	char slotStr[256];
	char mapStr[256];
	int pt,slotD;
	
		//printf ("USER_EVENT_ANALYSIS - FullMessage - {%s}\n",FullMessage);
		//printf ("USER_EVENT_ANALYSIS - RunMessage - {%s}\n", RunMessage);
		//printf ("USER_EVENT_ANALYSIS - ElseMessage - {%s}\n",ElseMessage);

	//
	if      ( STRCMP_L( RunMessage , "DUMMY_SUPPLY_REQUEST" ) ) {
		//
		if ( STRNCMP_L( ElseMessage , "PM" , 2 ) ) {
			pch = atoi( ElseMessage + 2 ) - 1 + PM1;
		}
		else {
			sprintf( ErrorMessage , "DUMMY_SUPPLY_REQUEST Fail 2 [%s]" , FullMessage );
			return 0;
		}
		//
		Dummy_Set( pch , TRUE );
		//
		return 1;
	}
	else if ( STRCMP_L( RunMessage , "DUMMY_SUPPLY_CANCEL" ) ) {
		//
		if ( STRNCMP_L( ElseMessage , "PM" , 2 ) ) {
			pch = atoi( ElseMessage + 2 ) - 1 + PM1;
		}
		else {
			sprintf( ErrorMessage , "DUMMY_SUPPLY_REQUEST Fail 2 [%s]" , FullMessage );
			return 0;
		}
		//
		Dummy_ReSet( pch , TRUE );
		//
		return 1;
	}
	else if ( STRCMP_L( RunMessage , "DUMMY_CONTROL" ) ) {
		Dummy_Status();
		return 1;
	}
	else 	if      ( STRCMP_L( RunMessage , "PROCESS_MAKE_PM_DISABLE" ) ) {
		//
		if ( STRNCMP_L( ElseMessage , "PM" , 2 ) ) {
			pch = atoi( ElseMessage + 2 ) - 1 + PM1;


			 IO_PM_REMAIN_SET( pch , 0 ); 

			if(!GET_DISABLE_PM_STATUS_EVENT(pch))
			{
				SET_DISABLE_PM_STATUS_EVENT(pch,TRUE); 
				RemainTimeSetting( TRUE );

				_IO_CIM_PRINTF("[PM PROCESS FAIL]PM%d Enable -> Disable  \n" , pch-PM1+1);
				//printf("[Event]PM%d Enable -> Disable  \n" , pch-PM1+1);
			}
		}
		return 1;
	}
	else  if      ( STRCMP_L( RunMessage , "PM_SUPPLY_WAFER_RETURN_REQUEST" ) ) {
		//
		_IO_CIM_PRINTF("[Event]%s %s\n",RunMessage,ElseMessage);

		if ( STRNCMP_L( ElseMessage , "PM" , 2 ) ) {
			pch = atoi( ElseMessage + 2 ) - 1 + PM1;
		}
		else {
			sprintf( ErrorMessage , "PM_SUPPLY_WAFER_RETURN_REQUEST Fail 2 [%s]" , FullMessage );
			return 0;
		}
		//
		//	int time_data;

		t = IO_GET_PM_REMAIN_TIME( pch );
		IO_PM_REMAIN_TIME( pch , t );

		MAKE_RETURN_WAFER(pch);
		//
		return 1;
	}

	else  if ( STRCMP_L( RunMessage , "SLOT_MAP_NOTUSE_DB" ) ) {
		//evevt 예시 SLOT_MAP_NOTUSE_DB PM1|0|1|1222
		//
		_IO_CIM_PRINTF("[Event]%s %s\n",RunMessage,ElseMessage);

		STR_SEPERATE_CHAR4( ElseMessage , '|' , chamberStr , sideStr,slotStr ,mapStr,256 );


		if ( STRNCMP_L( chamberStr , "PM" , 2 ) ) {
			pch = atoi( chamberStr + 2 ) - 1 + PM1;
			slot  = atoi(slotStr);
			side   = atoi(sideStr);
		}
		else {
			sprintf( ErrorMessage , "SLOT_MAP_NOTUSE_DB Fail 2 [%s]" , FullMessage );
			return 0;
		}
		//
		pt =Serch_PT(side,slot);
		slotD = RETURN_SLOT_INDEX(pch,mapStr);

		DEPTH_INFO_SET( side  , pt , pch , slotD );
		//
		_IO_CIM_PRINTF("DEPTH_INFO_SET  Result Side : %d Pt :%d ch :%d slot :%d [%d]\n" ,side ,pt, pch ,slot, slotD ,DEPTH_INFO_GET(side,pt,pch ) );

		return 1;
	}
	//2019.12.02
	/*
	else 	if      ( STRCMP_L( RunMessage , "PROCESS_LOTTYPE_UPDATE" ) ) {
		//
		STR_SEPERATE_CHAR( ElseMessage , '|' , chamberStr   , typeStr , 256 );

		if ( STRNCMP_L( chamberStr , "PM" , 2 ) ) {
			pch = atoi( chamberStr + 2 ) - 1 + PM1;
			type =  atoi( typeStr );

			_IO_CIM_PRINTF("[PROCESS_LOTTYPE_UPDATE]PM%d %d -> %d  \n" , pch-PM1+1 ,Get_LPLAST_Data (pch),type);
			
			Set_LPLAST_Data(pch,type);
		}
		return 1;
	}*/
	//--------------------------------------------------------------------------------
	else {
		return -1;
	}
}
int FIRST_PT(int side, int ch)
{
	int i , k;
	//
	for ( i = 0 ; i < MAX_CASSETTE_SLOT_SIZE ; i++ ) {
		//
		if ( _SCH_CLUSTER_Get_PathRange( side , i ) < 0 ) continue;
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			if ( ch == _SCH_CLUSTER_Get_PathProcessChamber( side , i , 0 , k ) ) return i;
			//
		}
		//
	}
	return -1;
}
//2019.12.02
void _SCH_COMMON_LOTPREPOST_PROCESS_NOTIFICATION( int side , int ch ,  int incm , int slot ,  int arm , char *recipe , int modeid, int puttime , char *nextpm , int mintime ,  int mode , int optdata ,int Result )
{
	int type;
	int firstpt; 
	char recipe2[256];
	char realfilename[1024];
	int Res;

	
	_SCH_LOG_LOT_PRINTF( side , "_SCH_COMMON_LOTPREPOST_PROCESS_NOTIFICATION-[side : %d][ch : %d][mode : %d][%s]\n" ,side,ch,mode,recipe);
	//_IO_CIM_PRINTF("_SCH_COMMON_LOTPREPOST_PROCESS_NOTIFICATION-[side : %d][ch : %d][mode : %d][%s]\n" ,side,ch,mode,recipe);
	
	if( mode == 1)
	{	
		firstpt = FIRST_PT( side, ch);

		if(firstpt<0) return;

		strcpy( recipe2 , _SCH_CLUSTER_Get_PathProcessRecipe2( side , firstpt , 0 , ch , SCHEDULER_PROCESS_RECIPE_IN ) );
		
		strcpy( realfilename , recipe2 );
	//	Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , firstpt , ch , _SCH_CLUSTER_Get_SlotIn( side , firstpt ) , realfilename , 0 , 0 );
		
		Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , firstpt , ch , _SCH_CLUSTER_Get_SlotIn( side , firstpt ) , realfilename , 1000 , 0 );

		Res = K_RECIPE_STEP_ITEM_READ_3_INT( realfilename , 0 , "LOT_PRE_TYPE" , &type , -1 , NULL , NULL , -1 , NULL , NULL);
	//	Res = K_RECIPE_STEP_ITEM_READ_3_INT( realfilename , 0 , "LOT_PRE_TYPE" , type , -1 , NULL , NULL , -1 , NULL , NULL);

	
	//1000이상인 경우 Recipe LOCKING된 File 경로를 가져옴 (2019.02.13 이전의버전에서는 LOCKING이 아닌 경우 경로를 가져오지 X)

		_SCH_LOG_LOT_PRINTF( side , "_SCH_COMMON_LOTPREPOST_PROCESS_NOTIFICATION2-[firstpt : %d][type : %d][Res : %d]\n" ,firstpt,type,Res);
		_SCH_LOG_LOT_PRINTF( side , "_SCH_COMMON_LOTPREPOST_PROCESS_NOTIFICATION2-2[recipe2 : %s]\n" ,recipe2);
		_SCH_LOG_LOT_PRINTF( side , "_SCH_COMMON_LOTPREPOST_PROCESS_NOTIFICATION2-3[realfilename : %s]\n" ,realfilename);
				
		//_IO_CIM_PRINTF ("Set_LPLAST_Data4  [ch  : %d ][%d]\n",ch,type); 
		Set_LPLAST_Data(ch,type);

		_SCH_LOG_LOT_PRINTF( side , "PM%d LOTPRE TYPE SET (%d) (pt  : %d)\n" , ch-PM1+1,type,firstpt);
//	_IO_CIM_PRINTF("PM%d LOTPRE TYPE SET (%d) (pt  : %d)\n" , ch-PM1+1,type,firstpt);
	}
}