//===========================================================================================================================

#include <windows.h>

#include "INF_Scheduler.h"
#include "INF_CimSeqnc.h"
#include "INF_Sch_prm_Cluster.h"

//=======================================================================================================================


int IOAddress_Sch_IO;
int IOAddress_Sch_PML;
int IOAddress_Sch_PMS[MAX_CHAMBER];
int IOAddress_Sch_PMS_MODE[MAX_CHAMBER];
int IOAddress_Sch_PMR[MAX_CHAMBER];
int IOAddress_Sch_PMM[MAX_CHAMBER];
int IOAddress_Sch_PMC[MAX_CHAMBER];

int IOAddress_Sch_RemainTime[MAX_CHAMBER];
int IOAddress_Sch_PostSkip[MAX_CHAMBER];

int IOAddress_Sch_PostX_Mode[MAX_CHAMBER];
int IOAddress_Sch_PostX_Count[MAX_CHAMBER];

//LOTPRE 관련 IO
int IOAddress_DiffClnOpt;
int IOAddress_DiffClnControl[MAX_CHAMBER];
int IOAddress_DiffClnRecipe[MAX_CHAMBER];
int IOAddress_FalvsDLP[MAX_CHAMBER];
int IOAddress_Auto_Leak_WF[MAX_CHAMBER];
int IOAddress_Auto_Leak_WF_MAX[MAX_CHAMBER];
//
int Address_FUNCTION_PROCESS[MAX_CHAMBER];

void EQUIP_INIT_READFILE()
{
	int i,j;
	char Buffer[256];
	
	for(i=PM1;i<BM1;i++)
	{
			Address_FUNCTION_PROCESS[i]  = -1;
	}
	
	for(j=PM1;j<BM1;j++)
	{
			sprintf(Buffer,"SCHEDULER_PROCESS_%s" , _SCH_SYSTEM_GET_MODULE_NAME_for_SYSTEM( j ) );
			Address_FUNCTION_PROCESS[j]  =  _FIND_FROM_STRING( _K_F_IO , Buffer );	
	}
}

void IO_PART_INIT() {
	int i;
	//
	IOAddress_Sch_IO = _FIND_FROM_STRING( _K_D_IO, "SMART_SCH_USE" );
	//
	IOAddress_Sch_PML = -2;
	IOAddress_DiffClnOpt = -2;


	for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
		IOAddress_DiffClnControl[i] =-2;
		IOAddress_DiffClnRecipe[i]=-2;
		IOAddress_FalvsDLP[i]=-2;
		IOAddress_Auto_Leak_WF[i] = -2;
		IOAddress_Auto_Leak_WF_MAX[i] = -2;

		IOAddress_Sch_PMS[i] = -2;
		IOAddress_Sch_PMR[i] = -2;
		IOAddress_Sch_PMM[i] = -2;
		IOAddress_Sch_PMC[i] = -2;
		//
		IOAddress_Sch_RemainTime[i] = -2;
		IOAddress_Sch_PostSkip[i] = -2;
		//
		IOAddress_Sch_PostX_Mode[i] = -2;
		IOAddress_Sch_PostX_Count[i] = -2;
		//
	}
	//
	EQUIP_INIT_READFILE();
}
//
int IO_CONTROL_USE() {
	int CommSts , Res;
	//
	if ( IOAddress_Sch_IO < 0 ) return 1;
	//
	Res = _dREAD_DIGITAL( IOAddress_Sch_IO, &CommSts );
	//
	if ( Res == 1 ) { // Off
		//
		_SCH_SUB_Make_ENABLE_TO_ENABLEN( 0 );
		//
	}
	else { // On
		//
		_SCH_SUB_Make_ENABLE_TO_ENABLEN( 3 );
		//
	}
	//
	return Res;
	//
}
//

int IO_DIFF_CLN_OPTION_GET() {
	//
	// 0 :	Recipe
	// 1 :	Lot Type
	//
	int CommSts;
	//
	if ( IOAddress_DiffClnOpt == -2 ) {
		//
		IOAddress_DiffClnOpt= _FIND_FROM_STRING( _K_D_IO, "DiffClnOpt" );
		//
	}
	//
	if ( IOAddress_DiffClnOpt == -1 ) return -1;
	//
	return _dREAD_DIGITAL( IOAddress_DiffClnOpt, &CommSts );
	//
}
//IO_DIFF_CLN_RECIPE_GET(ch, LPLAST_TYPE[ch] ,changefile); 
int  IO_DIFF_CLN_RECIPE_GET(int ch, int type ,char *recipe)
{
	char Buffer[64];
	int CommSts;
	//
	//if ( IOAddress_DiffClnRecipe[ch] == -2 ) {
		//
		if(type ==1)
		{
			sprintf( Buffer , "PM%d.LOT_PRE_RECIPE" , ch - PM1 + 1);
			IOAddress_DiffClnRecipe[ch] = _FIND_FROM_STRING( _K_S_IO, Buffer );
		}
		else 
		{
			sprintf( Buffer , "PM%d.LOT_PRE_RECIPE%d" , ch - PM1 + 1,type );
			IOAddress_DiffClnRecipe[ch] = _FIND_FROM_STRING( _K_S_IO, Buffer );
		}
		//
	//}
	//
	if ( IOAddress_DiffClnRecipe[ch] == -1 ) return -3;
	//
	_dREAD_STRING(IOAddress_DiffClnRecipe[ch], recipe , &CommSts );
	//
	if(strlen(recipe)>0)return 0;
	else return -4;

}
int IO_FALVS_DLP_GET(int ch)
{
	//
	// 0 :	 OFF
	// 1 : ON
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_FalvsDLP[ch] == -2 ) {
		//
		switch(ch)
		{
			case 5: //PMA
			case 6:
				sprintf( Buffer , "%s","PMA_FALVS_DLP_CHECK");
				break; 
			case 7: //PMB
			case 8:
					sprintf( Buffer , "%s", "PMB_FALVS_DLP_CHECK");
				break;
			case 9: //PMC
			case 10:
				sprintf( Buffer , "%s", "PMC_FALVS_DLP_CHECK");
				break; 
			case 11: //PMD
			case 12:
				sprintf( Buffer , "%s","PMD_FALVS_DLP_CHECK");
				break;
			case 13: //PME
			case 14:
				sprintf( Buffer , "%s","PME_FALVS_DLP_CHECK");
				break;
			default:
				return -5;
		}
		IOAddress_FalvsDLP[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_FalvsDLP[ch] == -1 ) return -3;
	//
	return _dREAD_DIGITAL( IOAddress_FalvsDLP[ch], &CommSts );
	//
}

void IO_FALVS_DLP_SET(int ch,int data)
{
	//
	// 0 :	 OFF
	// 1 : ON
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_FalvsDLP[ch] == -2 ) {
		//
		switch(ch)
		{
			case 5: //PMA
			case 6:
				sprintf( Buffer , "%s","PMA_FALVS_DLP_CHECK");
				break; 
			case 7: //PMB
			case 8:
				sprintf( Buffer , "%s", "PMB_FALVS_DLP_CHECK");
				break;
			case 9: //PMC
			case 10:
				sprintf( Buffer , "%s", "PMC_FALVS_DLP_CHECK");
				break; 
			case 11: //PMD
			case 12:
				sprintf( Buffer , "%s", "PMD_FALVS_DLP_CHECK");
				break;
			case 13: //PME
			case 14:
				sprintf( Buffer , "%s", "PME_FALVS_DLP_CHECK");
				break;
			default:
				return;
		}
		IOAddress_FalvsDLP[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_FalvsDLP[ch] == -1 ) return ;
	//
	_dWRITE_DIGITAL( IOAddress_FalvsDLP[ch], data, &CommSts );
	//
}

int IO_AUTO_LEAK_ACC_WF_MAX_GET( int ch )
{
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Auto_Leak_WF_MAX[ch] == -2 ) {
		//
		sprintf( Buffer , "PM%d.MAX_ACC_WAFERCOUNT" , ch - PM1 + 1 );
		IOAddress_Auto_Leak_WF_MAX[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Auto_Leak_WF_MAX[ch] == -1 ) return -3;
	//
	return _dREAD_DIGITAL( IOAddress_Auto_Leak_WF_MAX[ch], &CommSts );

}

int IO_AUTO_LEAK_ACC_WF_GET( int ch )
{
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Auto_Leak_WF[ch] == -2 ) {
		//
		sprintf( Buffer , "PM%d.ACC_WAFERCOUNT" , ch - PM1 + 1 );
		IOAddress_Auto_Leak_WF[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Auto_Leak_WF[ch] == -1 ) return -3;
	//
	return _dREAD_DIGITAL( IOAddress_Auto_Leak_WF[ch], &CommSts );

}


int IO_DIFF_CLN_CONTROL_GET( int ch ) {
	//
	// 0 :	 Normal (Lottype에 맞는 Recipe 사용)
	// 1 : MLT  (선행 LOT의 Lottype에 맞는 Recipe 사용)
	// 2 : MLT (선행 LOT의 Lottype에 맞는 Recipe 와 Lottype에 맞는 Recipe 모두 사용) 

	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_DiffClnControl[ch] == -2 ) {
		//PM3.
		sprintf( Buffer , "PM%d.DiffLotPre_Ctrl_Type" , ch - PM1 + 1 );
		IOAddress_DiffClnControl[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_DiffClnControl[ch] == -1 ) return -3;
	//
	return _dREAD_DIGITAL( IOAddress_DiffClnControl[ch], &CommSts );
	//
}




void IO_PM_REMAIN_SET( int ch , int data ) {
	//
	// 0 :	Ready/Disable 상태
	// 1 :	해당 PM으로 공급할 Wafer가 없음					WX
	// 2 :	해당 PM으로 가고있는 Wafer가 있음				WO
	// 3 :	해당 PM의 Remain Time 조건을 만족하지 않음		RTX
	// 4 :	해당 PM의 Remain Time 조건을 만족함				RTO
	// 5 :	해당 PM의 Wafer를 Picking중	
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PMM[ch] == -2 ) {
		//
		sprintf( Buffer , "SMART_PM%d_RemainMode" , ch - PM1 + 1 );
		IOAddress_Sch_PMM[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PMM[ch] == -1 ) return;
	//
	_dWRITE_DIGITAL( IOAddress_Sch_PMM[ch], data , &CommSts );
	//
}

int IO_PM_REMAIN_GET( int ch ) {
	//
	// 0 :	Ready/Disable 상태
	// 1 :	해당 PM으로 공급할 Wafer가 없음					WX
	// 2 :	해당 PM으로 가고있는 Wafer가 있음				WO
	// 3 :	해당 PM의 Remain Time 조건을 만족하지 않음		RTX
	// 4 :	해당 PM의 Remain Time 조건을 만족함				RTO
	// 5 :	해당 PM의 Wafer를 Picking중	
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PMM[ch] == -2 ) {
		//
		sprintf( Buffer , "SMART_PM%d_RemainMode" , ch - PM1 + 1 );
		IOAddress_Sch_PMM[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PMM[ch] == -1 ) return 0;
	//
	return _dREAD_DIGITAL( IOAddress_Sch_PMM[ch], &CommSts );
	//
}

void IO_PM_SELECT_SET( int ch ) {
	//
	int CommSts;
	//
	if ( IOAddress_Sch_PML == -2 ) {
		//
		IOAddress_Sch_PML = _FIND_FROM_STRING( _K_D_IO , "SMART_PM_LastSelect" );
		//
	}
	//
	if ( IOAddress_Sch_PML == -1 ) return;
	//
	_dWRITE_DIGITAL( IOAddress_Sch_PML , ch - PM1 + 1 , &CommSts );
	//
}


void IO_PM_REMAIN_CM( int ch , int data ) {
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PMC[ch] == -2 ) {
		//
		sprintf( Buffer , "SMART_PM%d_RemainCM" , ch - PM1 + 1 );
		IOAddress_Sch_PMC[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PMC[ch] == -1 ) return;
	//
	_dWRITE_DIGITAL( IOAddress_Sch_PMC[ch], data , &CommSts );
	//
}
//
void IO_PM_REMAIN_TIME( int ch , int data ) {
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PMR[ch] == -2 ) {
		//
		sprintf( Buffer , "SMART_PM%d_RemainTime" , ch - PM1 + 1 );
		IOAddress_Sch_PMR[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PMR[ch] == -1 ) return;
	//
	_dWRITE_DIGITAL( IOAddress_Sch_PMR[ch], data , &CommSts );
	//
}
//

int IO_GET_PM_REMAIN_TIME( int ch  ) {
	//
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PMR[ch] == -2 ) {
		//
		sprintf( Buffer , "SMART_PM%d_RemainTime" , ch - PM1 + 1 );
		IOAddress_Sch_PMR[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PMR[ch] == -1 ) return -1;
	//
	return _dREAD_DIGITAL( IOAddress_Sch_PMR[ch], &CommSts );
	//
}

int  IO_PM_SUPPLY_POSSIBLE_TIME( int ch ) {
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PMS[ch] == -2 ) {
		//
		sprintf( Buffer , "WFRSTART_PMREMAIN_TIME_PM%d" , ch - PM1 + 1 );
		IOAddress_Sch_PMS[ch] = _FIND_FROM_STRING( _K_A_IO, Buffer );
		//
		if ( IOAddress_Sch_PMS[ch] == -1 ) {
			sprintf( Buffer , "SMART_PM%d_SupplyTime" , ch - PM1 + 1 );
			IOAddress_Sch_PMS[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
			IOAddress_Sch_PMS_MODE[ch] = 0;
		}
		else {
			IOAddress_Sch_PMS_MODE[ch] = 1;
		}
		//
	}
	//
	if ( IOAddress_Sch_PMS[ch] == -1 ) return 0;
	//
	if ( IOAddress_Sch_PMS_MODE[ch] == 1 ) {
		return (int) _dREAD_ANALOG( IOAddress_Sch_PMS[ch] , &CommSts );
	}
	else {
		return _dREAD_DIGITAL( IOAddress_Sch_PMS[ch] , &CommSts );
	}
	//
}

BOOL IO_PM_USER_REMAIN_TIME_HAS( int ch ) {
	char Buffer[64];
	//
	if ( IOAddress_Sch_RemainTime[ch] == -2 ) {
		//
		sprintf( Buffer , "CTC.PM%d_RECIPE_REMAIN_TIME" , ch - PM1 + 1 );
		IOAddress_Sch_RemainTime[ch] = _FIND_FROM_STRING( _K_A_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_RemainTime[ch] == -1 ) return FALSE;
	//
	return TRUE;
	//
}


double IO_PM_USER_REMAIN_TIME( int ch ) {
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_RemainTime[ch] == -2 ) {
		//
		sprintf( Buffer , "CTC.PM%d_RECIPE_REMAIN_TIME" , ch - PM1 + 1 );
		IOAddress_Sch_RemainTime[ch] = _FIND_FROM_STRING( _K_A_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_RemainTime[ch] == -1 ) return 0;
	//
	return _dREAD_ANALOG( IOAddress_Sch_RemainTime[ch] , &CommSts );
	//
}

BOOL IO_PM_USER_POST_SKIP( int ch ) {
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PostSkip[ch] == -2 ) {
		//
		sprintf( Buffer , "CTC.PM%d_AUTOCLN_RUN" , ch - PM1 + 1 );
		IOAddress_Sch_PostSkip[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PostSkip[ch] == -1 ) return FALSE;
	//
	return _dREAD_DIGITAL( IOAddress_Sch_PostSkip[ch] , &CommSts );
	//
}



int IO_PM_POSTX_GET_MODE( int ch ) {
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PostX_Mode[ch] == -2 ) {
		//
		sprintf( Buffer , "PM%d_POSTX_MODE" , ch - PM1 + 1 );
		IOAddress_Sch_PostX_Mode[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PostX_Mode[ch] == -1 ) return 0;
	//
	return _dREAD_DIGITAL( IOAddress_Sch_PostX_Mode[ch] , &CommSts );
	//
}

int IO_PM_POSTX_GET_COUNT( int ch ) {
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PostX_Count[ch] == -2 ) {
		//
		sprintf( Buffer , "PM%d_POSTX_COUNT" , ch - PM1 + 1 );
		IOAddress_Sch_PostX_Count[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PostX_Count[ch] == -1 ) return 0;
	//
	return _dREAD_DIGITAL( IOAddress_Sch_PostX_Count[ch] , &CommSts );
	//
}


void IO_PM_POSTX_SET_COUNT( int ch , int c ) {
	char Buffer[64];
	int CommSts;
	//
	if ( IOAddress_Sch_PostX_Count[ch] == -2 ) {
		//
		sprintf( Buffer , "PM%d_POSTX_COUNT" , ch - PM1 + 1 );
		IOAddress_Sch_PostX_Count[ch] = _FIND_FROM_STRING( _K_D_IO, Buffer );
		//
	}
	//
	if ( IOAddress_Sch_PostX_Count[ch] == -1 ) return;
	//
	_dWRITE_DIGITAL( IOAddress_Sch_PostX_Count[ch] , c , &CommSts );
	//
}


void IO_CCW_Recipe_Type(int chamber,int RcpType)
{
	char IOName[64];
	int IOAddress,CommSts;

	//printf("IO_CCW_Recipe_Type [%d][%d]\n",chamber,RcpType);

	sprintf(IOName,"CTC.PM%d_CCWRcpType",chamber);

	//printf("IOName [%s]\n",IOName);

	IOAddress = _FIND_FROM_STRING(_K_D_IO,IOName);

	_dWRITE_DIGITAL(IOAddress ,RcpType, &CommSts );
}

BOOL HAS_PM_WAFER_STATUS(int ch)
{
	int Module_Size;
	int i;

	Module_Size = _SCH_PRM_GET_MODULE_SIZE(ch);
	
	for (i=0;i<Module_Size;i++)
	{
		if(_SCH_IO_GET_MODULE_STATUS( ch,i) > 0)return TRUE;
	}
	return FALSE;
}

int EQUIP_STATUS_PROCESS( int Chamber ) {

	if ( Chamber >= BM1 ) {
		return SYS_SUCCESS;
	}
	else {
		if ( Address_FUNCTION_PROCESS[Chamber]< 0 ) return SYS_SUCCESS;
		return _dREAD_FUNCTION( Address_FUNCTION_PROCESS[Chamber] );
	}
	return SYS_SUCCESS;
}