//===========================================================================================================================
#define _CRT_SECURE_NO_DEPRECATE
//===========================================================================================================================

#include <windows.h>

#include "INF_Scheduler.h"
#include "INF_CimSeqnc.h"
#include "INF_Sch_prm_Cluster.h"

//=======================================================================================================================

#include "io.h"

//=======================================================================================================================
/*

	Remain Time 의미

		100000	Pre(+Lot) Recipe 를 가지고 있음

		20000	Main Processing 시작

		60000	Pre Processing 시작

		10000	Post Processing 시작

		30000	Main Processing 중 Post Processing 예정

		40000	Main Processing 시작 + Post Recipe 가 예정

		50000	Placing 중

		?		Pre Processing 의 남은 시간
				Main Procesing 의 남은 시간
				Post Processing 의 남은 시간

*/

//=======================================================================================================================

#define RUNPLAN_UPDATE_BANDWIDTH	3

#define TIME_PRE_RUNPLAN_MORE_SEC						100000	// 2017.09.19

#define TIME_START_MAIN_PROCESSING_SEC					20000
#define TIME_START_PRE_PROCESSING_SEC					60000
#define TIME_START_POST_PROCESSING_SEC					10000
#define TIME_START_MAIN_and_POST_PROCESSING_SEC			40000
#define TIME_RUN_MAIN_and_POST_PROCESSING_SEC			30000
#define TIME_PLACE_RUNNING_SEC							50000

//=======================================================================================================================

int GET_USER_PLACING_PLAN_TIME() {
	return TIME_PLACE_RUNNING_SEC;
}

int GET_USER_PROCESS_REMAIN_TIME_USER( int s , int p , BOOL haspost , int ch , int *flag ) {
	int pdata , tdata;
	//
	pdata = _SCH_MACRO_CHECK_PROCESSING_INFO( ch );
	//
	tdata = 0;
	//
	if ( pdata > 0 ) { // prcs
		//
		if ( pdata < PROCESS_INDICATOR_PRE ) { // main
			//
			if ( _SCH_TIMER_GET_PROCESS_TIME_ELAPSED( 0 , ch ) < RUNPLAN_UPDATE_BANDWIDTH ) {
				//
				if ( haspost ) {
					tdata = TIME_START_MAIN_and_POST_PROCESSING_SEC;
				}
				else {
					tdata = TIME_START_MAIN_PROCESSING_SEC;
				}
				//
				*flag = 1;
				//
			}
			else {
				//
				tdata = (int) IO_PM_USER_REMAIN_TIME( ch );
				//
				if ( !haspost ) {
					//
					*flag = 3;
					//
				}
				else {
					//
					if ( IO_PM_USER_POST_SKIP( ch ) ) {
						//
						haspost = FALSE;
						//
						*flag = 5;
						//
					}
					else {
						//
						*flag = 7;
						//
						tdata += TIME_RUN_MAIN_and_POST_PROCESSING_SEC;
						//
					}
				}
				//
			}
			//
		}
		else if ( ( pdata >= PROCESS_INDICATOR_PRE ) && ( pdata < PROCESS_INDICATOR_POST ) ) { // pre
			//
			if ( _SCH_TIMER_GET_PROCESS_TIME_ELAPSED( 2 , ch ) < RUNPLAN_UPDATE_BANDWIDTH ) {
				//
				*flag = 9;
				//
				tdata = TIME_START_PRE_PROCESSING_SEC;
				//
			}
			else {
				//
				*flag = 11;
				//
				tdata = (int) IO_PM_USER_REMAIN_TIME( ch );
				//
			}
			//
		}
		else if ( pdata >= PROCESS_INDICATOR_POST ) { // post
			//
			if ( _SCH_TIMER_GET_PROCESS_TIME_ELAPSED( 1 , ch ) < RUNPLAN_UPDATE_BANDWIDTH ) {
				//
				*flag = 13;
				//
				tdata = TIME_START_POST_PROCESSING_SEC;
				//
			}
			else {
				//
				*flag = 15;
				//
				tdata = (int) IO_PM_USER_REMAIN_TIME( ch );
				//
			}
		}
		else {
			//
			*flag = 17;
			//
		}
	}
	else { // noprcs
		//
		if ( _SCH_SYSTEM_PLACING_TH_GET( ch ) > 0 ) { // 2017.01.02
			//
			if ( haspost ) {
				//
				*flag = 21;
				//
				tdata = TIME_START_MAIN_and_POST_PROCESSING_SEC;
				//
			}
			else {
				//
				*flag = 23;
				//
				tdata = TIME_START_MAIN_PROCESSING_SEC;
				//
			}
			//
		}
		else {
			if ( s >= 0 ) { // wafer
				//
				*flag = 25;
				//
				if ( haspost ) {
					//
					tdata = TIME_START_MAIN_and_POST_PROCESSING_SEC;
					//
				}
				else {
					//
					if ( _SCH_MODULE_Get_PM_PICKLOCK( ch , -1 ) > 0 ) {
						//
						*flag = 27;
						//
						tdata = TIME_PLACE_RUNNING_SEC; // 2018.10.20
						//
					}
					else {
						if ( _SCH_MACRO_CHECK_PROCESSING_INFO( ch ) > 0 ) {
							//
							*flag = 29;
							//
							tdata = TIME_START_MAIN_PROCESSING_SEC; // 2018.10.20
							//
						}
					}
					//
				}
			}
			else {
				//
				*flag = 31;
				//
			}
		}
	}
	//
	if ( haspost ) (*flag)++;
	//
	return tdata;
}
//
int GET_USER_PROCESS_REMAIN_TIME_SCH( int s , int p , BOOL haspost , int ch , int *flag ) {
	int pdata , tdata;
	//
	pdata = _SCH_MACRO_CHECK_PROCESSING_INFO( ch );
	//
	tdata = 0;
	//
	if ( pdata > 0 ) { // prcs
		//
		if ( pdata < PROCESS_INDICATOR_PRE ) { // main
			//
			*flag = 51;
			//
			tdata = _SCH_TIMER_GET_PROCESS_TIME_REMAIN( 0 , ch );
			//
			if ( haspost ) tdata += _SCH_TIMER_GET_PROCESS_TIME_RUNPLAN( 1 , ch );
			//
		}
		else if ( ( pdata >= PROCESS_INDICATOR_PRE ) && ( pdata < PROCESS_INDICATOR_POST ) ) { // pre
			//
			*flag = 53;
			//
			tdata = _SCH_TIMER_GET_PROCESS_TIME_REMAIN( 2 , ch );
			//
		}
		else if ( pdata >= PROCESS_INDICATOR_POST ) { // post
			//
			*flag = 55;
			//
			tdata = _SCH_TIMER_GET_PROCESS_TIME_REMAIN( 1 , ch );
			//
		}
		else {
			//
			*flag = 57;
			//
		}
	}
	else { // noprcs
		if ( _SCH_SYSTEM_PLACING_TH_GET( ch ) > 0 ) { // 2017.01.02
			//
			*flag = 61;
			//
			tdata = _SCH_TIMER_GET_PROCESS_TIME_RUNPLAN( 0 , ch );
			//
			if ( haspost ) tdata += _SCH_TIMER_GET_PROCESS_TIME_RUNPLAN( 1 , ch );
			//
		}
		else {
			if ( s >= 0 ) { // wafer
				//
				*flag = 63;
				//
				if ( haspost ) {
					//
					tdata = _SCH_TIMER_GET_PROCESS_TIME_RUNPLAN( 1 , ch );
					//
				}
				else {
					//
					if ( _SCH_MODULE_Get_PM_PICKLOCK( ch , -1 ) > 0 ) {
						//
						*flag = 65;
						//
						tdata = TIME_PLACE_RUNNING_SEC; // 2018.10.20
						//
					}
					else {
						if ( _SCH_MACRO_CHECK_PROCESSING_INFO( ch ) > 0 ) {
							//
							*flag = 67;
							//
							tdata = TIME_START_MAIN_PROCESSING_SEC; // 2018.10.20
							//
						}
					}
					//
				}
				//
			}
			else {
				//
				*flag = 69;
				//
			}
		}
	}
	//
	if ( haspost ) (*flag)++;
	//
	return tdata;
}
//


int GET_USER_PROCESS_REMAIN_TIME( int s , int p , BOOL haspost , int ch , int *flag , BOOL Reading , BOOL *update ) {
	int tdata;
	//
	if ( IO_PM_USER_REMAIN_TIME_HAS( ch ) ) {
		//
		if ( !Reading ) {
			if ( update != NULL ) *update = FALSE;
			return 0;
		}
		//
		tdata = GET_USER_PROCESS_REMAIN_TIME_USER( s , p , haspost , ch , flag );
	}
	else {
		tdata = GET_USER_PROCESS_REMAIN_TIME_SCH( s , p , haspost , ch , flag );
	}
	//
	if ( _SCH_MACRO_HAS_PRE_PROCESS_OPERATION( s , p , 300 + ch ) ) { // 2017.09.19
		tdata = tdata + TIME_PRE_RUNPLAN_MORE_SEC;
	}
	//
	return tdata;
}

