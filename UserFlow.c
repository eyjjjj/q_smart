//===========================================================================================================================
#define _CRT_SECURE_NO_DEPRECATE
//===========================================================================================================================

#include <windows.h>

#include "INF_Scheduler.h"
#include "INF_CimSeqnc.h"
#include "INF_Sch_prm_Cluster.h"

//=======================================================================================================================

extern BOOL MULTI_PM_MODE; // 2018.05.08
extern int  MULTI_PM_LAST; // 2018.05.08
extern int  MULTI_PM_COUNT; // 2018.06.01
extern int  MULTI_PM_CLIDX; // 2018.06.01
extern int  MULTI_PM_MAP[64]; // 2018.10.20
//=======================================================================================================================

#include "UserFlow.h"
#include "UserTime.h"
#include "io.h"
#include "LotPre.h"

#define REMAIN_TIME_SAME_GAP_SEC	1
#define NOWAYCHECK_GAP_SEC			5

DWORD LastRemainTimeSettingTime;

void Dummy_Set( int ch , BOOL log );


//=======================================================================================================================

// 2019.03.05


BOOL _DEPTH_INFO_FLAG[MAX_CASSETTE_SIDE];
int  _DEPTH_INFO_DATA[MAX_CASSETTE_SIDE][MAX_CASSETTE_SLOT_SIZE][MAX_PM_CHAMBER_DEPTH];


void DEPTH_INFO_INIT( int Side ) {

	_DEPTH_INFO_FLAG[Side] = FALSE;

}

void DEPTH_INFO_START( int Side ) {
	int i , j;
	//
	if ( _DEPTH_INFO_FLAG[Side] ) return;
	//
	_DEPTH_INFO_FLAG[Side] = TRUE;
	//
	for ( i = 0 ; i < MAX_CASSETTE_SLOT_SIZE ; i++ ) {
		//
		if ( _SCH_CLUSTER_Get_PathRange( Side , i ) < 0 ) {
			//
			for ( j = 0 ; j < MAX_PM_CHAMBER_DEPTH ; j++ ) _DEPTH_INFO_DATA[Side][i][j] = -1;
			//
			continue;
		}
		//
		if ( _SCH_CLUSTER_Get_PathProcessDepth( Side , i , 0 ) == NULL ) {
			//
			for ( j = 0 ; j < MAX_PM_CHAMBER_DEPTH ; j++ ) _DEPTH_INFO_DATA[Side][i][j] = 0;
			//
			continue;
		}
		//
		for ( j = 0 ; j < MAX_PM_CHAMBER_DEPTH ; j++ ) {
			_DEPTH_INFO_DATA[Side][i][j] = _SCH_CLUSTER_Get_PathProcessDepth2( Side , i , 0 , j );
		}
		//
	}
	//
}

int DEPTH_INFO_GET( int Side , int Pointer , int PMC ) {
	return _DEPTH_INFO_DATA[Side][Pointer][PMC - PM1];
}

void DEPTH_INFO_SET( int Side , int Pointer , int PMC , int slot ) {
	//
	if ( ( _SCH_CLUSTER_Get_PathProcessDepth( Side , Pointer , 0 ) == NULL ) && ( slot <= 0 ) ) return;
	//
	 _SCH_CLUSTER_Set_PathProcessDepth2( Side , Pointer , 0 , (PMC - PM1) , slot );
	//
}

//=======================================================================================================================

void Flow_Init() {
	LastRemainTimeSettingTime = GetTickCount();
}

BOOL GET_HAS_PROCESS_WAFER( int ch ) { // 2017.09.07
	int j;
	//
	if ( _SCH_MACRO_CHECK_PROCESSING_INFO( ch ) > 0 ) return TRUE;
	//
	for ( j = 0 ; j < _SCH_PRM_GET_MODULE_SIZE( ch ) ; j++ ) {
		//
		if ( _SCH_MODULE_Get_PM_WAFER( ch , j ) > 0 ) return TRUE;
	}
	//
	return FALSE;
}
//
//
int GET_PROCESS_REMAIN_TIME_PM( int ch , int *flag , int *s , int *p , int *w , int *mode , BOOL reading , BOOL *update ) {
	int j , k , m;
	BOOL haspost , hasplacing;
	//
	if ( update != NULL ) *update = TRUE;
	//
	*flag = 0;
	//
	*mode = _PMW_0_NOTHING;
	//
	*s = -1;
	*p = -1;
	*w = 0;
	//
	haspost = FALSE;
	hasplacing = FALSE;
	//
	for ( j = 0 ; j < _SCH_PRM_GET_MODULE_SIZE( ch ) ; j++ ) {
		//
		if ( _SCH_MODULE_Get_PM_WAFER( ch , j ) > 0 ) {
			//
			if ( _SCH_MODULE_Get_PM_PICKLOCK( ch , j ) > 0 ) { // Placing Wafer
				//
				if ( *mode == _PMW_0_NOTHING ) {
					*mode = _PMW_1_WILLPROCESS;
				}
				else if ( *mode == _PMW_2_PROCESSED ) {
					*mode = _PMW_3_ALL;
				}
				//
				hasplacing = TRUE;
				continue;
			}
			//
			if ( *mode == _PMW_0_NOTHING ) {
				*mode = _PMW_2_PROCESSED;
			}
			else if ( *mode == _PMW_1_WILLPROCESS ) {
				*mode = _PMW_3_ALL;
			}
			//
			// Processed/Processing Wafer
			//
			if ( *w == 0 ) {
				//
				*s = _SCH_MODULE_Get_PM_SIDE( ch , j );
				*p = _SCH_MODULE_Get_PM_POINTER( ch , j );
				*w = _SCH_MODULE_Get_PM_WAFER( ch , j );
				//
				for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
					//
					m = _SCH_CLUSTER_Get_PathProcessChamber( *s , *p , 0 , k );
					//
					if ( m == ch ) {
						//
						haspost = _SCH_CLUSTER_Check_HasProcessData_Post( *s , *p , 0 , k );
						//
					}
					//
				}
				//
			}
			//
		}
	}
	//
	if ( hasplacing ) return GET_USER_PLACING_PLAN_TIME();
	//
	return ( GET_USER_PROCESS_REMAIN_TIME( *s , *p , haspost , ch , flag , reading , update ) );
	//
}


int  USER_SELECT_PM[MAX_CASSETTE_SIDE][MAX_CASSETTE_SLOT_SIZE];

int Go_0_1_2_PM( int Side , int Pt ) {
	int k , c;
	//
	c = 0;
	//
	for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
		//
		if ( _SCH_CLUSTER_Get_PathProcessChamber( Side , Pt , 0 , k ) > 0 ) {
			//
			c++;
			//
			if ( c > 1 ) return 2;
			//
		}
		//
	}
	//
	return c;
	//
}

int Check_1_PM( int Side , int Pt ) {
	int k , c , m , l;
	//
	m = -1;
	c = 0;
	//
	for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
		//
		l = _SCH_CLUSTER_Get_PathProcessChamber( Side , Pt , 0 , k );
		//
		if ( l > 0 ) {
			//
			c++;
			//
			if ( c > 1 ) return -1;
			//
			m = l;
		}
		//
	}
	//
	return m;
	//
}

void Setting_1_PM( int Side , int Pt , int ch ) {
	int k , l;
	//
	for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
		//
		l = _SCH_CLUSTER_Get_PathProcessChamber( Side , Pt , 0 , k );
		//
		if ( l > 0 ) {
			if ( l != ch ) {
				_SCH_CLUSTER_Set_PathProcessChamber( Side , Pt , 0 , k , -l );
			}
		}
		else if ( l < 0 ) {
			if ( -l == ch ) {
				_SCH_CLUSTER_Set_PathProcessChamber( Side , Pt , 0 , k , -l );
			}
		}
		//
	}
	//
}


/*
//
// 2017.10.22
//

int DataHasPair( int ch , int side , int pt , int *DataMap_PMGoCount , int *DataMap_PMGoCount2 ) { // 2017.09.08
	int selid , pr , j , k , m , x;
	int lotprech[MAX_CHAMBER];
	int fail;
	//
	pr = _SCH_CLUSTER_Get_PathRange( side , pt );
	//
	if ( pr <= 0 ) return -1;
	//
	if ( !_SCH_PREPRCS_FirstRunPre_Check_PathProcessData( side , ch ) ) return -2;
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) lotprech[j] = -1;
	//
	selid = 0;
	//
	for ( j = 0 ; j < pr ; j++ ) {
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( side , pt , j , k );
			//
			if ( m > 0 ) {
				//
				if ( _SCH_CLUSTER_Get_PathProcessParallel( side , pt , j ) == NULL ) {
					lotprech[m] = 0;
				}
				else {
					//
					lotprech[m] = _SCH_CLUSTER_Get_PathProcessParallel2( side , pt , j , ( m - PM1 ) );
					//
					if ( ( m == ch ) && ( lotprech[m] > 1 ) && ( lotprech[m] < 100 ) ) {
						//
						selid = lotprech[m];
						//
					}
				}
				//
			}
		}
	}
	//
	for ( j = 0 ; j < pr ; j++ ) {
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( side , pt , j , k );
			//
			if ( m < 0 ) { // 2017.10.13
				//
				if ( _SCH_MODULE_Get_Mdl_Use_Flag( side , -m ) == MUF_98_USE_to_DISABLE_RAND ) {
					//
					if ( _SCH_CLUSTER_Get_PathProcessParallel( side , pt , j ) == NULL ) {
						//
						if ( 0 == selid ) {
							//
							DataMap_PMGoCount[ ch ] = 2;
							//
							return ( selid * 1000 ) + ( ch * 10 ) + 2;
							//
						}
					}
					else {
						//
						x = _SCH_CLUSTER_Get_PathProcessParallel2( side , pt , j , ( -m - PM1 ) );
						//
						if ( ( x == selid ) && ( selid > 1 ) && ( selid < 100 ) ) {
							//
							DataMap_PMGoCount[ ch ] = 3;
							//
							return ( selid * 1000 ) + ( ch * 10 ) + 3;
							//
						}
					}
				}
				//
			}
		}
	}
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
		//
		if ( lotprech[j] >= 0 ) {
			//
			if ( !_SCH_PREPRCS_FirstRunPre_Check_PathProcessData( side , j ) ) {
				//
				lotprech[j] = -2;
				//
			}
		}
	}
	//
	if ( ( selid > 1 ) && ( selid < 100 ) ) {
		//
		for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
			//
			if ( lotprech[j] > 1 ) {
				//
				if ( lotprech[j] != selid ) {
					//
					lotprech[j] = -3;
					//
				}
			}
		}
		//
	}
	//
	if ( lotprech[ch] < 0 ) {
		//
		DataMap_PMGoCount[ ch ] = 4;
		//
		switch( lotprech[ch] ) {
		case -2 :
			return ( selid * 1000 ) + ( ch * 10 ) + 5;
		case -3 :
			return ( selid * 1000 ) + ( ch * 10 ) + 6;
		}
		//
		return ( selid * 1000 ) + ( ch * 10 ) + 4;
		//
	}
	//
	fail = 0;
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
		//
		if ( lotprech[j] >= 0 ) {
			//
			if ( GET_HAS_PROCESS_WAFER( j ) ) {
				fail = ( selid * 1000 ) + ( j * 10 ) + 7;
				break;
			}
			if ( DataMap_PMGoCount[ j ] > 0 ) {
				fail = ( selid * 1000 ) + ( j * 10 ) + 8;
				break;
			}
			if ( DataMap_PMGoCount2[ j ] > 0 ) {
				fail = ( selid * 1000 ) + ( j * 10 ) + 9;
				break;
			}
			//
		}
	}
	//
	if ( fail != 0 ) {
		//
		for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
			//
			if ( lotprech[j] >= 0 ) {
				//
				DataMap_PMGoCount[ j ] = 7;
				//
			}
		}
		return fail;
	}
	//
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
		//
		if ( lotprech[j] >= 0 ) {
			//
			_SCH_MACRO_PRE_PROCESS_OPERATION( side , pt , 0 , 201000 + j );
			//
			DataMap_PMGoCount[j] = 9;
			//
		}
		//
	}
	//
	return ( selid * 1000 ) + ( ch * 10 ) + 0;
	//
}
*/

//
// 2017.10.22
//

int DataHasPair( int ch , int side , int pt , int *DataMap_PMGoCount , int *DataMap_PMGoCount2 , BOOL *LotPreUse , int *LotPreList , int *Res ) { // 2017.09.08
	int selid , pr , j , k , m , x;
	int lotprech[MAX_CHAMBER];
	int fail;
	//
	*Res = 0;
	//
	pr = _SCH_CLUSTER_Get_PathRange( side , pt );
	//
	if ( pr <= 0 ) return -1;
	//
	if ( !_SCH_PREPRCS_FirstRunPre_Check_PathProcessData( side , ch ) ) return -2;
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) lotprech[j] = -1;
	//
	selid = 0;
	//
	for ( j = 0 ; j < pr ; j++ ) {
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( side , pt , j , k );
			//
			if ( m > 0 ) {
				//
				if ( _SCH_CLUSTER_Get_PathProcessParallel( side , pt , j ) == NULL ) {
					lotprech[m] = 0;
				}
				else {
					//
					lotprech[m] = _SCH_CLUSTER_Get_PathProcessParallel2( side , pt , j , ( m - PM1 ) );
					//
					if ( ( m == ch ) && ( lotprech[m] > 1 ) && ( lotprech[m] < 100 ) ) {
						//
						selid = lotprech[m];
						//
					}
				}
				//
			}
		}
	}
	//
	for ( j = 0 ; j < pr ; j++ ) {
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( side , pt , j , k );
			//
			if ( m < 0 ) { // 2017.10.13
				//
				if ( _SCH_MODULE_Get_Mdl_Use_Flag( side , -m ) == MUF_98_USE_to_DISABLE_RAND ) {
					//
					if ( _SCH_CLUSTER_Get_PathProcessParallel( side , pt , j ) == NULL ) {
						//
						if ( 0 == selid ) {
							//
							*Res = 2;
							//
							return ( selid * 1000 ) + ( ch * 10 ) + 2;
							//
						}
					}
					else {
						//
						x = _SCH_CLUSTER_Get_PathProcessParallel2( side , pt , j , ( -m - PM1 ) );
						//
						if ( ( x == selid ) && ( selid > 1 ) && ( selid < 100 ) ) {
							//
							*Res = 3;
							//
							return ( selid * 1000 ) + ( ch * 10 ) + 3;
							//
						}
					}
				}
				//
			}
		}
	}
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
		//
		if ( lotprech[j] >= 0 ) {
			//
			if ( !_SCH_PREPRCS_FirstRunPre_Check_PathProcessData( side , j ) ) {
				//
				lotprech[j] = -2;
				//
			}
		}
	}
	//
	if ( ( selid > 1 ) && ( selid < 100 ) ) {
		//
		for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
			//
			if ( lotprech[j] > 1 ) {
				//
				if ( lotprech[j] != selid ) {
					//
					lotprech[j] = -3;
					//
				}
			}
		}
		//
	}
	//
	if ( lotprech[ch] < 0 ) {
		//
		*Res = 4;
		//
		switch( lotprech[ch] ) {
		case -2 :
			return ( selid * 1000 ) + ( ch * 10 ) + 5;
		case -3 :
			return ( selid * 1000 ) + ( ch * 10 ) + 6;
		}
		//
		return ( selid * 1000 ) + ( ch * 10 ) + 4;
		//
	}
	//
	fail = 0;
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
		//
		if ( lotprech[j] >= 0 ) {
			//
			if ( GET_HAS_PROCESS_WAFER( j ) ) {
				fail = ( selid * 1000 ) + ( j * 10 ) + 7;
				break;
			}
			if ( DataMap_PMGoCount[ j ] > 0 ) {
				fail = ( selid * 1000 ) + ( j * 10 ) + 8;
				break;
			}
			if ( DataMap_PMGoCount2[ j ] > 0 ) {
				fail = ( selid * 1000 ) + ( j * 10 ) + 9;
				break;
			}
			//
		}
	}
	//
	if ( fail != 0 ) {
		//
		*Res = 7;
		//
		return fail;
	}
	//
	*LotPreUse = TRUE;
	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
		//
		if ( lotprech[j] >= 0 ) {
			//
			LotPreList[j] = 1;
			//
		}
		else {
			//
			LotPreList[j] = 0;
			//
		}
		//
	}
	//
	return ( selid * 1000 ) + ( ch * 10 ) + 0;
	//
}


BOOL DataHasPair_LotPreCheck( int side , int pt , BOOL LotPreUse , int *LotPreList ) { // 2017.10.22
	int j;
	BOOL Res = FALSE;

	if ( !LotPreUse ) return FALSE;

	//
	for ( j = PM1 ; j < (MAX_PM_CHAMBER_DEPTH + PM1) ; j++ ) {
		//
		if ( LotPreList[j] ) {
			//
			_SCH_MACRO_PRE_PROCESS_OPERATION( side , pt , 0 , 201000 + j );
			//
			Res = TRUE;
			//
		}
		//
	}
	//

	return Res;

}


BOOL Parallel_Invalid_Enable( int s , int p , int selid ) { // 2017.10.23
	//
	int i , k , m , x , f , pr;
	//
	if ( selid <= 1 ) return FALSE;
	//
	pr = _SCH_CLUSTER_Get_PathRange( s, p );
	//
	for ( i = 1 ; i < pr ; i++ ) {
		//
		f = 0;
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( s, p , i , k );
			//
			if ( m > 0 ) {
				//
				if ( _SCH_CLUSTER_Get_PathProcessParallel( s , p , i ) == NULL ) {
					f = 2;
					break;
				}
				//
				x = _SCH_CLUSTER_Get_PathProcessParallel2( s , p , i , ( m - PM1 ) );
				//
				if ( x <= 1 ) {
					f = 3;
					break;
				}
				//
				if ( x == selid ) {
					f = 4;
					break;
				}
				//
			}
			//
		}
		//
		if ( f == 0 ) return TRUE;
	}
	//
	return FALSE;
	//
}



BOOL Parallel_OtherSide_Will_Go( int s , int m ) { // 2017.10.31
	int i , j , k;
	//
	for ( i = 0 ; i < MAX_CASSETTE_SIDE ; i++ ) {
		//
		if ( i == s ) continue;
		//
		if ( !_SCH_SYSTEM_USING_RUNNING(i) ) continue;
		if ( !_SCH_SUB_POSSIBLE_PICK_FROM_FM_NOSUPPLYCHECK( i , &k , &j ) ) continue;
		//
		if ( _SCH_MODULE_Get_Mdl_Use_Flag( i , m ) != MUF_01_USE ) continue;
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			if ( m == _SCH_CLUSTER_Get_PathProcessChamber( i , j , 0 , k ) ) return TRUE;
			//
		}
		//
	}
	//
	return FALSE;
	//
}


BOOL Parallel_Invalid_xlotpre( int s , int p , int selid ) { // 2017.10.16 // 2017.10.20
	//
	int i , k , m , x , f , pr;
	//
	if ( selid <= 1 ) return FALSE;
	//
	pr = _SCH_CLUSTER_Get_PathRange( s, p );
	//
	for ( i = 1 ; i < pr ; i++ ) {
		//
		f = 0;
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( s , p , i , k );
			//
			if ( m < 0 ) {
				//
				if ( !Parallel_OtherSide_Will_Go( s , -m ) ) { // 2017.10.31
					//
					if ( _SCH_MODULE_Get_Mdl_Use_Flag( s , -m ) == MUF_98_USE_to_DISABLE_RAND ) {
						//
						if ( !_SCH_PREPRCS_FirstRunPre_Check_PathProcessData( s , -m ) ) {
							//
							if ( _SCH_CLUSTER_Get_PathProcessParallel( s , p , i ) == NULL ) {
								f = 1;
								break;
							}
							//
							x = _SCH_CLUSTER_Get_PathProcessParallel2( s , p , i , ( -m - PM1 ) );
							//
							if ( x <= 1 ) {
								f = 3;
								break;
							}
							//
							if ( x == selid ) {
								f = 4;
								break;
							}
							//
						}
					}
					//
				}
				//
			}
			//
		}
		//
		if ( f == 0 ) return TRUE;
	}
	//
	return FALSE;
	//
}



BOOL BM_is_Wating_for_Supply_Wafer( int side , int pt ) { // 2017.12.07
	int i , j , s , p;
	//
	for ( i = BM1 ; i < ( BM1 + MAX_BM_CHAMBER_DEPTH ) ; i++ ) {
		//
		if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) continue;
		//
		if ( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() == i ) continue;
		//
		if ( _SCH_PRM_GET_MODULE_GROUP( i ) != 0 ) continue;
		//
		if ( _SCH_MODULE_Get_BM_FULL_MODE( i ) != BUFFER_FM_STATION ) continue;
		if ( _SCH_MACRO_CHECK_PROCESSING_INFO( i ) > 0 ) continue;
		//
		for ( j = 1 ; j <= _SCH_PRM_GET_MODULE_SIZE( i ) ; j++ ) {
			//
			if ( ( _SCH_MODULE_Get_BM_WAFER( i , j ) > 0 ) && ( _SCH_MODULE_Get_BM_WAFER( i , j+1 ) <= 0 ) ) {
				//
				if ( _SCH_MODULE_Get_BM_STATUS( i , j ) != BUFFER_OUTWAIT_STATUS ) {
					//
					s = _SCH_MODULE_Get_BM_SIDE( i , j );
					p = _SCH_MODULE_Get_BM_POINTER( i , j );
					//
					if ( _SCH_CLUSTER_Get_PathDo( s , p ) == 0 ) {
						//
						if ( _SCH_CLUSTER_Get_ClusterIndex( s , p ) == _SCH_CLUSTER_Get_ClusterIndex( side , pt ) ) return TRUE;
						//
					}
					//
				}
				//
			}
			else if ( ( _SCH_MODULE_Get_BM_WAFER( i , j ) <= 0 ) && ( _SCH_MODULE_Get_BM_WAFER( i , j+1 ) > 0 ) ) {
				//
				if ( _SCH_MODULE_Get_BM_STATUS( i , j+1 ) != BUFFER_OUTWAIT_STATUS ) {
					//
					s = _SCH_MODULE_Get_BM_SIDE( i , j+1 );
					p = _SCH_MODULE_Get_BM_POINTER( i , j+1 );
					//
					if ( _SCH_CLUSTER_Get_PathDo( s , p ) == 0 ) {
						//
						if ( _SCH_CLUSTER_Get_ClusterIndex( s , p ) == _SCH_CLUSTER_Get_ClusterIndex( side , pt ) ) return TRUE;
						//
					}
					//
				}
				//
			}
			//
			j++;
			//
		}
	}
	//
	return FALSE;
}


int Align_Function_Address = -2;
//
void Set_PM_Slot_Info( int side , int pt , int pm , int l ) { // 2019.01.15
	char Buffer[256];
	if ( Align_Function_Address == -2 ) Align_Function_Address = _FIND_FROM_STRING( _K_F_IO + 1 , "FM.ALIGN_WAFER" );
	if ( Align_Function_Address == -1 ) return;
	//
	sprintf( Buffer , "PM_SLOT %s|%d|%s|%d" , _SCH_SYSTEM_GET_MODULE_NAME_for_SYSTEM( _SCH_CLUSTER_Get_PathIn( side , pt ) ) , _SCH_CLUSTER_Get_SlotIn( side , pt ) , _SCH_SYSTEM_GET_MODULE_NAME_for_SYSTEM( pm ) , l );
	//
	_dWRITE_FUNCTION_EVENT( Align_Function_Address , Buffer );
	//
}
//

int  Multi_is_PM_Max( int pm ) { // 2018.10.20
	//
	int i , c = 0;
	//
	for ( i = 0 ; i < _SCH_PRM_GET_MODULE_SIZE( pm ) ; i++ ) {
		if ( _SCH_PRM_GET_DFIM_SLOT_NOTUSE( pm , i + 1 ) ) continue;
		c++;
	}
	//
	return c;
	//
}

BOOL Multi_is_PM_Slot_NotUse( int pm , int side , int pt , int *l ) { // 2018.10.20
	//
	int sl;

	if ( l != NULL ) *l = 0;
	//
	sl = DEPTH_INFO_GET( side , pt , pm );
	//
	if ( sl < 0 ) return FALSE;
	//
	if ( l != NULL ) *l = sl;
	//
	if ( sl <= 0 ) return FALSE; // 2018.12.19
	//
	if ( _SCH_PRM_GET_DFIM_SLOT_NOTUSE( pm , sl ) ) return TRUE;
	//
	return FALSE;
	//
}


BOOL Multi_is_PM_Slot_Will_has_Space( int side , int pt , int pm , int count , int *map , int *slot ) { // 2018.12.06  ///SLot 변경 (1번이 1)
	int i , j , s , p , l;
	//
	if ( Multi_is_PM_Slot_NotUse( pm , side , pt , &l ) ) return FALSE;
	//
	if ( ( l > 0 ) && ( l <= _SCH_PRM_GET_MODULE_SIZE( pm ) ) ) { // 고정
		//
		for ( i = 0 ; i < count ; i++ ) {
			if ( map[i] == l ) return FALSE;
		}
		//
		s = l;
		//
	}
	else { // 비고정
		//
		s = 0;
		//



		for ( i = 0 ; i < _SCH_PRM_GET_MODULE_SIZE( pm ) ; i++ ) {
			//
			if ( _SCH_PRM_GET_DFIM_SLOT_NOTUSE( pm , i + 1 ) ) continue;
			//
			for ( j = 0 ; j < count ; j++ ) {
				if ( map[j] == ( i + 1 ) ) break;
			}
			//
			if ( j != count ) continue;
			//
			if ( s == 0 ) {
				s = i + 1;
				p = _SCH_PRM_GET_DFIM_SLOT_NOTUSE_DATA( pm , i + 1 );
			}
			else {
				if ( _SCH_PRM_GET_DFIM_SLOT_NOTUSE_DATA( pm , i + 1 ) < _SCH_PRM_GET_DFIM_SLOT_NOTUSE_DATA( pm , s ) ) {
					s = i + 1;
					p = _SCH_PRM_GET_DFIM_SLOT_NOTUSE_DATA( pm , i + 1 );
				}
				else if ( _SCH_PRM_GET_DFIM_SLOT_NOTUSE_DATA( pm , i + 1 ) == _SCH_PRM_GET_DFIM_SLOT_NOTUSE_DATA( pm , s ) ) {
					//
					switch( _SCH_PRM_GET_DFIM_SLOT_NOTUSE( pm , 0 ) ) {
					case 0 : // F-IN F-OUT
					case 1 : // F-IN L-OUT
						break;
					default :
						s = i + 1;
						p = _SCH_PRM_GET_DFIM_SLOT_NOTUSE_DATA( pm , i + 1 );
						break;
					}
					//
				}
				//
			}
			//
		}
		//
	}
	//
	if ( s == 0 ) return FALSE;
	//
	if ( slot != NULL ) *slot = s;
	//
	return TRUE;
	//
}


BOOL Multi_is_Wating_for_Supply_Wafer( int side , int pt , int lastpm , int lastcx , int count , int *map ) { // 2018.10.20
	//
	if ( lastpm <= 0 ) return FALSE;
	//
	if ( !_SCH_MODULE_GET_USE_ENABLE( lastpm , FALSE , -1 ) ) return FALSE;
	//
	if ( lastcx < 0 ) return FALSE;
	//
	if ( lastcx != _SCH_CLUSTER_Get_ClusterIndex( side , pt ) ) return FALSE;
	//
	if ( !Multi_is_PM_Slot_Will_has_Space( side , pt , lastpm , count , map , NULL ) ) return FALSE; // 2018.12.06
	//
	return TRUE;
	//
}

BOOL Multi_is_Checking_for_Supply_Wafer( int side , int pt , int pm , int *lastpm , int *lastcx , int *count , int *map ) { // 2018.10.20
	int l;
	//
	if ( !_SCH_MODULE_GET_USE_ENABLE( pm , FALSE , -1 ) ) {
		//
		*count = 0;
		*lastpm = 0;
		*lastcx = -1;
		//
		return FALSE;
	}
	//
	if ( !Multi_is_PM_Slot_Will_has_Space( side , pt , pm , *count , map , &l ) ) l = 0; // 2018.12.06
	//
	DEPTH_INFO_SET( side , pt , pm , l );
	//
	Set_PM_Slot_Info( side , pt , pm , l ); // 2019.01.15
	//
	map[ *count ] = l;
	//
	(*count)++;
	//
	if ( (*count) >= Multi_is_PM_Max( pm ) ) {
		//
		*count = 0;
		*lastpm = 0;
		*lastcx = -1;
		//
		return FALSE;
		//
	}
	else {
		*lastpm = pm;
		*lastcx = _SCH_CLUSTER_Get_ClusterIndex( side , pt );
		//
		return TRUE;
		//
	}
	//
}


void Multi_is_Checking_for_Supply_Wafer2( int side , int pt , int *lastpm , int *lastcx , int *count , int *map ) {
	//
	int l;
	//
	if ( !_SCH_MODULE_GET_USE_ENABLE( *lastpm , FALSE , -1 ) ) {
		//
		*count = 0;
		*lastpm = 0;
		*lastcx = -1;
		//
		return;
	}
	if ( Multi_is_PM_Slot_Will_has_Space( side , pt , *lastpm , *count , map , &l ) ) l = 0; // 2018.12.06
	//
	DEPTH_INFO_SET( side , pt , *lastpm , l );
	//
	Set_PM_Slot_Info( side , pt , *lastpm , l ); // 2019.01.15
	//
	map[ *count ] = l;
	//
	(*count)++;
	//
	if ( (*count) >= Multi_is_PM_Max( *lastpm ) ) {
		//
		*count = 0;
		*lastpm = 0;
		//
	}
	//
}
//


BOOL Multi_is_BM_Locking_for_FullSwap_Init = FALSE;
BOOL Multi_is_BM_Locking_for_FullSwap_Update = FALSE;
int  Multi_is_BM_Locking_for_FullSwap_PlaceData_F[MAX_CHAMBER];
int  Multi_is_BM_Locking_for_FullSwap_PlaceData_0[MAX_CHAMBER];
int  Multi_is_BM_Locking_for_FullSwap_PlaceData_1[MAX_CHAMBER];

void Multi_is_BM_Locking_for_FullSwapReset() { // 2018.11.22
	//
	int i;
	//
	if ( !Multi_is_BM_Locking_for_FullSwap_Init ) {
		//
		Multi_is_BM_Locking_for_FullSwap_Init = TRUE;
		//
		for ( i = BM1 ; i < ( BM1 + MAX_BM_CHAMBER_DEPTH ) ; i++ ) {
			Multi_is_BM_Locking_for_FullSwap_PlaceData_F[i] = _SCH_PRM_GET_MODULE_PLACE_GROUP( MAX_TM_CHAMBER_DEPTH , i );
			Multi_is_BM_Locking_for_FullSwap_PlaceData_0[i] = _SCH_PRM_GET_MODULE_PLACE_GROUP( 0 , i );
			Multi_is_BM_Locking_for_FullSwap_PlaceData_1[i] = _SCH_PRM_GET_MODULE_PLACE_GROUP( 1 , i );
		}
		//
		Multi_is_BM_Locking_for_FullSwap_Update = FALSE;
	}
	//
	if ( Multi_is_BM_Locking_for_FullSwap_Update ) {
		//
		for ( i = BM1 ; i < ( BM1 + MAX_BM_CHAMBER_DEPTH ) ; i++ ) {
			//
			_SCH_PRM_SET_MODULE_PLACE_GROUP( MAX_TM_CHAMBER_DEPTH , i , Multi_is_BM_Locking_for_FullSwap_PlaceData_F[i] );
			_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , i , Multi_is_BM_Locking_for_FullSwap_PlaceData_0[i] );
			_SCH_PRM_SET_MODULE_PLACE_GROUP( 1 , i , Multi_is_BM_Locking_for_FullSwap_PlaceData_1[i] );
			//
		}
		//
		Multi_is_BM_Locking_for_FullSwap_Update = FALSE;
		//
	}
}

BOOL Multi_is_BM_Locking_for_FullSwap() { // 2018.11.22
	int i , bm_1 , bm_2;
	int bm2_1 , bm2_2 , bm2_3 , bm2_4;
	//
	if ( !Multi_is_BM_Locking_for_FullSwap_Init ) {
		//
		Multi_is_BM_Locking_for_FullSwap_Init = TRUE;
		//
		for ( i = BM1 ; i < ( BM1 + MAX_BM_CHAMBER_DEPTH ) ; i++ ) {
			Multi_is_BM_Locking_for_FullSwap_PlaceData_F[i] = _SCH_PRM_GET_MODULE_PLACE_GROUP( MAX_TM_CHAMBER_DEPTH , i );
			Multi_is_BM_Locking_for_FullSwap_PlaceData_0[i] = _SCH_PRM_GET_MODULE_PLACE_GROUP( 0 , i );
			Multi_is_BM_Locking_for_FullSwap_PlaceData_1[i] = _SCH_PRM_GET_MODULE_PLACE_GROUP( 1 , i );
		}
		//
		Multi_is_BM_Locking_for_FullSwap_Update = FALSE;
	}
	//
	bm_1 = 0;
	bm_2 = 0;
	bm2_1 = 0;
	bm2_2 = 0;
	bm2_3 = 0;
	bm2_4 = 0;
	//
	for ( i = BM1 ; i < ( BM1 + MAX_BM_CHAMBER_DEPTH ) ; i++ ) {
		//
		if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) continue;
		//
		if ( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() == i ) continue;
		//
		if ( _SCH_PRM_GET_MODULE_GROUP( i ) == 0 ) {
			//
			if      ( bm_1 == 0 ) bm_1 = i;
			else if ( bm_2 == 0 ) bm_2 = i;
			//
		}
		else if ( _SCH_PRM_GET_MODULE_GROUP( i ) == 1 ) {
			if      ( bm2_1 == 0 ) bm2_1 = i;
			else if ( bm2_2 == 0 ) bm2_2 = i;
			else if ( bm2_3 == 0 ) bm2_3 = i;
			else if ( bm2_4 == 0 ) bm2_4 = i;
		}
	}
	//
	if ( bm_2 == 0 ) { 
		//
		for ( i = BM1 ; i < ( BM1 + MAX_BM_CHAMBER_DEPTH ) ; i++ ) {
			//
			_SCH_PRM_SET_MODULE_PLACE_GROUP( MAX_TM_CHAMBER_DEPTH , i , Multi_is_BM_Locking_for_FullSwap_PlaceData_F[i] );
			_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , i , Multi_is_BM_Locking_for_FullSwap_PlaceData_0[i] );
			_SCH_PRM_SET_MODULE_PLACE_GROUP( 1 , i , Multi_is_BM_Locking_for_FullSwap_PlaceData_1[i] );
			//
		}
		//
		Multi_is_BM_Locking_for_FullSwap_Update = FALSE;
		//
		return TRUE;
	}
	//
	_SCH_PRM_SET_MODULE_PLACE_GROUP( MAX_TM_CHAMBER_DEPTH , bm_1 , -1 );
	_SCH_PRM_SET_MODULE_PLACE_GROUP( MAX_TM_CHAMBER_DEPTH , bm_2 , 0 );
	//
	if      ( bm2_4 != 0 ) {
		//
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_1 , -1 );
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_2 , -1 );
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_3 , -1 );
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_4 , 0 );
		//
	}
	else if ( bm2_3 != 0 ) {
		//
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_1 , -1 );
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_2 , -1 );
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_3 , 0 );
		//
	}
	else if ( bm2_2 != 0 ) {
		//
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_1 , -1 );
		_SCH_PRM_SET_MODULE_PLACE_GROUP( 0 , bm2_2 , 0 );
		//
	}
	//
	Multi_is_BM_Locking_for_FullSwap_Update = TRUE;
	//
	return FALSE;
}


BOOL SCHEDULER_CONTROL_Use_LOT_Check_4_DUMMY_WAFER_SLOT_STYLE_6( int bmc , int slot ) {
	int s , i;
	for ( s = 0 ; s < MAX_CASSETTE_SIDE ; s++ ) {
		//========================================================================================================================
		if ( !_SCH_SYSTEM_USING_STARTING( s ) ) continue;
		//========================================================================================================================
//		for ( i = 0 ; i < MAX_CASSETTE_SLOT_SIZE ; i++ ) { // 2018.02.13
		for ( i = 0 ; i < (MAX_CASSETTE_SLOT_SIZE-1) ; i++ ) { // 2018.02.13
			//
			if ( _SCH_CLUSTER_Get_PathRange( s , i ) < 0 ) continue;
			//
			if ( _SCH_CLUSTER_Get_PathStatus( s , i ) == SCHEDULER_CM_END ) continue;
			//
			if ( ( _SCH_CLUSTER_Get_PathIn( s , i ) == bmc ) && ( _SCH_CLUSTER_Get_SlotIn( s , i ) == slot ) ) return TRUE;
			//
		}
		//========================================================================================================================
	}
	//
	return FALSE;
}


BOOL SCHEDULER_CONTROL_Get_SDM_4_DUMMY_WAFER_SLOT_STYLE_6( int side , int ch , int *slot1 , int *slot2 , int *ret ) {
	int i , mx1 , mx2 , s1 = -1 , s2 = -1;
	//
	*ret = 1;
	//
	if ( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() <= 0 ) return FALSE;
	//
	*ret = 2;
	//
	if ( !_SCH_MODULE_GET_USE_ENABLE( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() , FALSE , side ) ) return FALSE;
	//
	*ret = 3;
	//
	for ( i = 0 ; i < _SCH_PRM_GET_MODULE_SIZE( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() ) ; i++ ) {
		//
		if ( _SCH_SDM_Get_RUN_CHAMBER(i) == 0 ) continue;
		//
		if ( SCHEDULER_CONTROL_Use_LOT_Check_4_DUMMY_WAFER_SLOT_STYLE_6( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() , i + 1 ) ) continue; // 2010.07.02
		//
		if ( _SCH_SDM_Get_RUN_CHAMBER(i) != ch ) {
			if ( _SCH_PRM_GET_MODULE_MODE( _SCH_SDM_Get_RUN_CHAMBER(i) ) ) continue;
		}
		//
		if ( _SCH_IO_GET_MODULE_STATUS( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() , i + 1 ) <= 0 ) continue;
		//
		if ( s1 == -1 ) {
			mx1 = _SCH_SDM_Get_RUN_USECOUNT(i);
			s1 = i;
		}
		else {
			if ( mx1 > _SCH_SDM_Get_RUN_USECOUNT(i) ) {
				//
				mx2 = mx1;
				s2 = s1;
				//
				mx1 = _SCH_SDM_Get_RUN_USECOUNT(i);
				s1 = i;
				//
			}
			else {
				if ( s2 == -1 ) {
					mx2 = _SCH_SDM_Get_RUN_USECOUNT(i);
					s2 = i;
				}
				else {
					if ( mx2 > _SCH_SDM_Get_RUN_USECOUNT(i) ) {
						//
						mx2 = _SCH_SDM_Get_RUN_USECOUNT(i);
						s2 = i;
						//
					}
				}
			}
			//
		}
		//
	}
	//
	if ( s2 == -1 ) return FALSE;
	//
	*slot1 = s1 + 1;
	*slot2 = s2 + 1;
	return TRUE;
}



BOOL Dummy_Request( int ch );
void Dummy_ReSet( int ch , BOOL msg );


void Dummy_Regist( int side , int pt , int rpt , int ch ) {
	int i;
	int f;
	//
	char recipe0[256];
	char recipe1[256];
	char recipe2[256];
	//
	if ( MULTI_PM_MODE ) return; // 2018.05.08
	//
	if ( pt == rpt ) return;
	//
	if ( _SCH_CLUSTER_Get_PathRange( side , rpt ) < 0 ) { // 처음
		//
		_SCH_CLUSTER_Copy_Cluster_Data( side , rpt , side , pt , 0 );
		//
		if ( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() >= BM1 ) { // 2018.11.21
			_SCH_CLUSTER_Set_PathIn( side , rpt , _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() );
			_SCH_CLUSTER_Set_PathOut( side , rpt , _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() );
		}
		//
		_SCH_CLUSTER_Set_SlotIn( side , rpt , 1 );
		_SCH_CLUSTER_Set_SlotOut( side , rpt , 1 );
		_SCH_CLUSTER_Set_PathStatus( side , rpt , SCHEDULER_READY );
		_SCH_CLUSTER_Set_PathDo( side , rpt , 0 );
		_SCH_CLUSTER_Set_PathRange( side , rpt , 1 );
		//
		_SCH_CLUSTER_Set_CPJOB_CONTROL( side , rpt , -1 );
		_SCH_CLUSTER_Set_CPJOB_PROCESS( side , rpt , -1 );
		//
	}
	else {
		//
		if ( ch == -1 ) return;
		//
		f = 0;
		//
		strcpy( recipe0 , "" );
		strcpy( recipe1 , "" );
		strcpy( recipe2 , "" );
		//
		for ( i = 0 ; i < MAX_PM_CHAMBER_DEPTH ; i++ ) {
			//
			if ( ch == _SCH_CLUSTER_Get_PathProcessChamber( side , pt , 0 , i ) ) {
				//
				strcpy( recipe0 , _SCH_CLUSTER_Get_PathProcessRecipe( side , pt , 0 , i , 0 ) );
				strcpy( recipe1 , _SCH_CLUSTER_Get_PathProcessRecipe( side , pt , 0 , i , 1 ) );
				strcpy( recipe2 , _SCH_CLUSTER_Get_PathProcessRecipe( side , pt , 0 , i , 2 ) );
				//
				break;
			}
			//
		}
		//
		for ( i = 0 ; i < MAX_PM_CHAMBER_DEPTH ; i++ ) {
			//
			if ( ch == _SCH_CLUSTER_Get_PathProcessChamber( side , rpt , 0 , i ) ) {
				//
				f = 1;
				//
				_SCH_CLUSTER_Set_PathProcessData( side , rpt , 0 , i , ch , recipe0 , recipe1 , recipe2 );
				//
				break;
			}
			//
		}
		//
		if ( f == 0 ) {
			//
			for ( i = 0 ; i < MAX_PM_CHAMBER_DEPTH ; i++ ) {
				//
				if ( 0 == _SCH_CLUSTER_Get_PathProcessChamber( side , rpt , 0 , i ) ) {
					//
					_SCH_CLUSTER_Set_PathProcessData( side , rpt , 0 , i , ch , recipe0 , recipe1 , recipe2 );
					//
					break;
				}
				//
			}
			//
		}
	}
}



BOOL Dummy_End( int side , int rpt ) {
	int  i , m , j;
	//
	// 2019.03.21
	//
	//
	for ( i = 0 ; i < MAX_CASSETTE_SIDE ; i++ ) {
		//
		if ( i == side ) continue;
		//
		if ( _SCH_SYSTEM_USING_STARTING( i ) ) {
			//
			_SCH_CLUSTER_Set_PathRange( side , rpt , -9 );
			//
			return TRUE;
			//
		}
		//
		if ( !_SCH_SYSTEM_USING_RUNNING(i) ) continue;
		//
		if ( !_SCH_SUB_POSSIBLE_PICK_FROM_FM_NOSUPPLYCHECK( i , &m , &j ) ) continue;
		//
		if ( j < ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) {
			//
			_SCH_CLUSTER_Set_PathRange( side , rpt , -9 );
			//
			return TRUE;
			//
		}
		//
	}
	//
	if ( _SCH_CLUSTER_Get_PathRange( side , rpt ) >= 0 ) {
		if ( _SCH_MODULE_Get_FM_InCount( side ) == _SCH_MODULE_Get_FM_OutCount( side ) ) {
			_SCH_CLUSTER_Set_PathRange( side , rpt , -9 );
			return TRUE;
		}
	}
	//
	return FALSE;
	//
}


BOOL Dummy_Add( BOOL StandaloneUse , BOOL nowaycheck , int ch , int side , int pt , int *rpt ) {
	int i , dumsel , dummyslot , dummyslot2 , clx;
//	int pathorder;
	char recipe[3][256];

	char Buffer[256]; // 2018.02.08
	int pw , ps , pp; // 2018.02.08

	int crxpt; // 2018.02.20

	*rpt = 0;

	if ( !SCHEDULER_CONTROL_Get_SDM_4_DUMMY_WAFER_SLOT_STYLE_6( side , ch , &dummyslot , &dummyslot2 , rpt ) ) return FALSE;
	//
	*rpt = 100;
	//
	dumsel = -1;
	//
//	for ( i = ( pt + 1 ) ; i < ( MAX_CASSETTE_SLOT_SIZE - 1 ) ; i++ ) { // 2018.02.12
	for ( i = ( pt + 1 ) ; i < ( MAX_CASSETTE_SLOT_SIZE - 2 ) ; i++ ) { // 2018.02.12
		//
		if ( _SCH_CLUSTER_Get_PathRange( side , i ) < 0 ) {
			if ( _SCH_CLUSTER_Get_PathRange( side , i + 1 ) < 0 ) {
				dumsel = i;
			}
		}
		//
	}
	//
	if ( dumsel == -1 ) { // 2018.02.15
		//
		for ( i = 0 ; i < ( MAX_CASSETTE_SLOT_SIZE - 2 ) ; i++ ) {
			//
			if ( _SCH_CLUSTER_Get_PathRange( side , i ) < 0 ) {
				if ( _SCH_CLUSTER_Get_PathRange( side , i + 1 ) < 0 ) {
					dumsel = i;
				}
			}
			//
		}
		//
		if ( dumsel == -1 ) return FALSE;
		//
	}
	//
	if ( nowaycheck ) return TRUE;
	//
	// 2018.02.08
	//
//	strcpy( recipe[0] , _SCH_SDM_Get_RECIPE( 0 , dummyslot , 0 , 0 ) );
//	strcpy( recipe[1] , _SCH_SDM_Get_RECIPE( 1 , dummyslot , 0 , 0 ) );
//	strcpy( recipe[2] , _SCH_SDM_Get_RECIPE( 2 , dummyslot , 0 , 0 ) );
//	//
//	if ( recipe[0][0] == 0 ) strcpy( recipe[0] , _SCH_SDM_Get_RECIPE( 0 , dummyslot2 , 0 , 0 ) );
//	if ( recipe[1][0] == 0 ) strcpy( recipe[1] , _SCH_SDM_Get_RECIPE( 1 , dummyslot2 , 0 , 0 ) );
//	if ( recipe[2][0] == 0 ) strcpy( recipe[2] , _SCH_SDM_Get_RECIPE( 2 , dummyslot2 , 0 , 0 ) );


	//
	// 2018.02.20
	//
	//
	crxpt = -1;
	//
	if ( _SCH_CLUSTER_Get_PathIn( side , pt ) >= BM1 ) {
		//
		crxpt = _SCH_CLUSTER_Get_PathRun( side , pt , 11 , 2 ) - 1; // Next
		//
		if ( crxpt < 0 ) {
			//
			if ( ( pt+1 ) < ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) {
				//
				if ( _SCH_CLUSTER_Get_PathIn( side , pt + 1 ) >= BM1 ) {
					//
					if ( _SCH_CLUSTER_Get_ClusterIndex( side , pt ) == _SCH_CLUSTER_Get_ClusterIndex( side , pt + 1 ) ) {
						//
						crxpt = _SCH_CLUSTER_Get_PathRun( side , pt + 1 , 11 , 2 ) - 1; // Next
						//
					}
				}
				//
			}
		}
	}
	//
	pw = _SCH_MODULE_Get_PM_WAFER( ch , 0 );
	ps = _SCH_MODULE_Get_PM_SIDE( ch , 0 );
	pp = _SCH_MODULE_Get_PM_POINTER( ch , 0 );
	//
	if ( ( pw == _SCH_MODULE_Get_PM_WAFER( ch , 0 ) ) && ( pw > 0 ) ) {
		//
		strcpy( Buffer , _SCH_CLUSTER_Get_PathProcessRecipe2( ps , pp , 0 , ch , 1 ) );
		//
		if      ( Buffer[0] == 0 ) {
			pw = 0;
		}
		else if ( ( Buffer[0] == '*' ) || ( Buffer[0] == '?' ) ) {
			if ( Buffer[1] == 0 ) {
				pw = 0;
			}
			else {
				//
				pw = 2;
				//
				strcpy( recipe[0] , Buffer + 1 );
				strcpy( recipe[1] , "" );
				strcpy( recipe[2] , "" );
			}
		}
		else {
			//
			pw = 1;
			//
			strcpy( recipe[0] , Buffer );
			strcpy( recipe[1] , "" );
			strcpy( recipe[2] , "" );
		}
	}
	else {
		pw = 0;
	}
	//
	if ( pw == 0 ) {
		//
		strcpy( recipe[0] , _SCH_SDM_Get_RECIPE( 0 , dummyslot , 0 , 0 ) );
		strcpy( recipe[1] , _SCH_SDM_Get_RECIPE( 1 , dummyslot , 0 , 0 ) );
		strcpy( recipe[2] , _SCH_SDM_Get_RECIPE( 2 , dummyslot , 0 , 0 ) );
		//
		if ( recipe[0][0] == 0 ) strcpy( recipe[0] , _SCH_SDM_Get_RECIPE( 0 , dummyslot2 , 0 , 0 ) );
		if ( recipe[1][0] == 0 ) strcpy( recipe[1] , _SCH_SDM_Get_RECIPE( 1 , dummyslot2 , 0 , 0 ) );
		if ( recipe[2][0] == 0 ) strcpy( recipe[2] , _SCH_SDM_Get_RECIPE( 2 , dummyslot2 , 0 , 0 ) );
		//
	}
	//
	*rpt = dumsel;
	//
	if ( _SCH_CLUSTER_Get_PathRange( side , pt ) < 0 ) {
		_SCH_CLUSTER_Copy_Cluster_Data( side , dumsel , side , pt , 4 );
	}
	else {
		_SCH_CLUSTER_Copy_Cluster_Data( side , dumsel , side , pt , 0 );
	}
	//
	clx = _SCH_CLUSTER_Get_ClusterIndex( side , dumsel ) + 1;
	//
	_SCH_CLUSTER_Set_PathProcessData( side , dumsel , 0 , 0 , ch ,
		recipe[0] ,
		recipe[1] ,
		recipe[2]
		);
	//
	for ( i = 1 ; i < MAX_PM_CHAMBER_DEPTH ; i++ ) {
		//
		_SCH_CLUSTER_Set_PathProcessData( side , dumsel , 0 , i , 0 , "" , "" , "" );
		//
	}
	//
	//
	_SCH_CLUSTER_Set_PathIn( side , dumsel , _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() );
	_SCH_CLUSTER_Set_PathOut( side , dumsel , _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() );
	_SCH_CLUSTER_Set_SlotIn( side , dumsel , dummyslot );
	_SCH_CLUSTER_Set_SlotOut( side , dumsel , dummyslot );
	_SCH_CLUSTER_Set_PathStatus( side , dumsel , SCHEDULER_READY );
	_SCH_CLUSTER_Set_PathDo( side , dumsel , 0 );
	_SCH_CLUSTER_Set_PathRange( side , dumsel , 1 );
	//
	_SCH_CLUSTER_Set_ClusterIndex( side , dumsel , clx );
	//
	_SCH_CLUSTER_Set_CPJOB_CONTROL( side , dumsel , -1 );
	_SCH_CLUSTER_Set_CPJOB_PROCESS( side , dumsel , -1 );
	//
	_SCH_CLUSTER_Set_PathRun( side , dumsel , 12 , 2 , 0 );
	_SCH_CLUSTER_Set_PathRun( side , dumsel , 11 , 2 , 0 ); // Next(다음)
	_SCH_CLUSTER_Set_PathRun( side , dumsel , 13 , 2 , 2 ); // pick후 11,2로 이동 / 완료후 삭제
	_SCH_CLUSTER_Set_PathRun( side , dumsel , 16 , 2 , 0 );
	//
	dumsel++;
	//
	_SCH_CLUSTER_Copy_Cluster_Data( side , dumsel , side , dumsel - 1 , 0 );
	//
	_SCH_CLUSTER_Set_SlotIn( side , dumsel , dummyslot2 );
	_SCH_CLUSTER_Set_SlotOut( side , dumsel , dummyslot2 );
	//
	if ( crxpt < 0 ) {
		//
		_SCH_CLUSTER_Set_PathRun( side , dumsel , 11 , 2 , pt+1 ); // Next
		//
	}
	else {
		//
		_SCH_CLUSTER_Set_PathRun( side , dumsel , 11 , 2 , crxpt+1 ); // Next
		//
		_SCH_CLUSTER_Set_PathRun( side , pt+1 , 11 , 2 , *rpt+1 ); // Next
		//
	}
	//
	Dummy_ReSet( ch , FALSE );
	//
	if ( recipe[0][0] != 0 ) _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , *rpt , ch , dummyslot , recipe[0] , 0 , 0 );
	if ( recipe[1][0] != 0 ) _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , *rpt , ch , dummyslot , recipe[1] , 1 , 0 );
	if ( recipe[2][0] != 0 ) _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , *rpt , ch , dummyslot , recipe[2] , 2 , 0 );
	//
	if ( recipe[0][0] != 0 ) _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , *rpt+1 , ch , dummyslot2 , recipe[0] , 0 , 0 );
	if ( recipe[1][0] != 0 ) _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , *rpt+1 , ch , dummyslot2 , recipe[1] , 1 , 0 );
	if ( recipe[2][0] != 0 ) _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , *rpt+1 , ch , dummyslot2 , recipe[2] , 2 , 0 );
	//
	if ( crxpt >= 0 ) {
		*rpt = -1;
	}
	//
	return TRUE;
	//
}


void RemainTimeSetting( BOOL pmincheck ) {
	//
	BOOL over , update;
	int i , j , k , rs , rp , rw , rm;
	//
	over = ( ( GetTickCount() - LastRemainTimeSettingTime ) >= 3000 );
	//
	for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
		//
		if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) 
		{	
			if(GET_DISABLE_PM_STATUS(i))
			{
				k= -1;
			}
			else continue;
		}
		//
		if(k==-1)
		{
			 k = 300000;
		}
		else 
		{
			k = GET_PROCESS_REMAIN_TIME_PM( i , &j , &rs , &rp , &rw , &rm , over , &update );
		}
		//
		if ( pmincheck && ( rw > 0 ) ) k = 300000;
		//
		if ( update ) IO_PM_REMAIN_TIME( i , k );
		//
	}
	//
	if ( over ) LastRemainTimeSettingTime = GetTickCount();
	//
}


BOOL PM_ENABLE_FOR_MULTI_TM_PLACE( int pm ) { // 2018.10.30
	//
	int grp , i;
	//
//	if ( pm < 0 ) return FALSE; // 2019.01.07
	if ( pm <= 0 ) return FALSE; // 2019.01.07
	//
	if ( !_SCH_MODULE_GET_USE_ENABLE( pm , FALSE , -1 ) ) return FALSE; // 2019.05.23
	//
	grp = _SCH_PRM_GET_MODULE_GROUP( pm );
	//
	for ( i = 0 ; i <= grp ; i++ ) {
		//
		if ( !_SCH_MODULE_GET_USE_ENABLE( TM + i , FALSE , -1 ) ) return FALSE;
		//
	}
	//
	return TRUE;
	//
}

BOOL HAS_DUMMY_WAFER_CHAMBER() { // 2019.01.29
	if ( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() >= BM1 ) {
		if ( _SCH_MODULE_GET_USE_ENABLE( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() , FALSE , -1 ) ) {
			return TRUE;
		}
	}
	return FALSE;
}


int Get_Run_List( int s , int p , BOOL *Dummy , int *LastDoneCh , int *WillList , int *WillCount , int *Will2List , int *Will2Count ) {
	int PathRange , i , j , k , m;
	//
	PathRange = _SCH_CLUSTER_Get_PathRange( s , p );
	//
	if ( Dummy != NULL ) {
		*Dummy = ( _SCH_CLUSTER_Get_PathIn( s , p ) >= BM1 );
	}
	//
	if ( LastDoneCh != NULL ) {
		//
		*LastDoneCh = 0;
		//
		for ( i = 0 ; i < PathRange ; i++ ) {
			//
			m = _SCH_CLUSTER_Get_PathRun( s , p , i , 0 );
			//
			if ( m != 0 ) *LastDoneCh = m;
			//
		}
		//
	}
	//
	if ( _SCH_CLUSTER_Get_PathDo( s , p ) >= PATHDO_WAFER_RETURN ) {
		//
		if ( ( WillList != NULL ) && ( WillCount != NULL ) ) {
			*WillCount = 0;
		}
		//
		if ( ( Will2List != NULL ) && ( Will2Count != NULL ) ) {
			*Will2Count = 0;
		}
		//
		return PathRange;
	}
	//
	if ( ( WillList != NULL ) && ( WillCount != NULL ) ) {
		//
		*WillCount = 0;
		//
		for ( i = 0 ; i < PathRange ; i++ ) {
			//
			if ( _SCH_CLUSTER_Get_PathRun( s , p , i , 0 ) != 0 ) continue;
			//
			*WillCount = 0;
			//
			for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
				//
				m = _SCH_CLUSTER_Get_PathProcessChamber( s , p , i , k );
				//
				if ( m > 0 ) {
					WillList[*WillCount] = m;
					(*WillCount)++;
				}
			}
			//
			break;
			//
		}
		//
	}
	//
	if ( ( Will2List != NULL ) && ( Will2Count != NULL ) ) {
		//
		*Will2Count = 0;
		//
		for ( i = 1 ; i < PathRange ; i++ ) {
			//
			if ( _SCH_CLUSTER_Get_PathRun( s , p , i , 0 ) != 0 ) continue;
			//
			for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
				//
				m = _SCH_CLUSTER_Get_PathProcessChamber( s , p , i , k );
				//
				if ( m > 0 ) {
					//
					for ( j = 0 ; j < *Will2Count ; j++ ) {
						if ( Will2List[j] == m ) break;
					}
					if ( j == *Will2Count ) {
						Will2List[*Will2Count] = m;
						(*Will2Count)++;
					}
					//
				}
			}
			//
		}
		//
	}
	//
	return PathRange;
	//
}


int  USER_FLOW_CHECK_PICK_FROM_FM( int side , int pt , BOOL nowaycheck ) {
	int i , j , k , m , rs , rp , rw , rm , W;
	char recipe[256];
	char recipe2[256];
	char recipe2_s[256];
	char recipe2_s2[256];

	int Res;
	//
	int DataMap_PMSide[MAX_CHAMBER];
	int DataMap_PMGoCount[MAX_CHAMBER];
	int DataMap_PMRemainTime[MAX_CHAMBER];
	int DataMap_PMTag[MAX_CHAMBER];
	int DataMap_PMTag2[MAX_CHAMBER];
	int DataMap_PMSupplyTime[MAX_CHAMBER];
	int DataMap_WfrCRC[2];
	int DataMap_WfrCNT[2];
	//
	int DataMap_PMInCount[MAX_CHAMBER]; // 2018.11.23
	int DataMap_PMGoCount2[MAX_CHAMBER]; // 2017.09.19
	int DataMap_PMPairRes[MAX_CHAMBER]; // 2017.10.11
	int DataMap_PMPointer[MAX_CHAMBER]; // 2017.10.22
	//
	int DataMap_LotPreUse[MAX_CHAMBER]; // 2017.10.22
	int DataMap_LotPreList[MAX_CHAMBER][MAX_CHAMBER]; // 2017.10.22

	int DataMap_Select[MAX_CHAMBER]; // 2017.10.22

	int _G_WillList[MAX_PM_CHAMBER_DEPTH];
	int _G_WillCount;
	int _G_Will2List[MAX_PM_CHAMBER_DEPTH];
	int _G_Will2Count;

	int dummy , rpt,pathdo;

	int DataMap_PMReqMode[MAX_CHAMBER]; // 2018.02.10

	BOOL multireturning; // 2018.11.20
	BOOL multi1ArmBlock = FALSE; // 2018.11.20
	BOOL multiNextSupply = FALSE; // 2018.11.29

	int IO_CONTROL_MODE;

	int bmStatus,bs1,bp1,bs2,bp2;
	int bw1,bw2;

	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================

	if ( pt >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
		//
		if ( Dummy_End( side , ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) ) {
			return -9;
		}
		//
	}
	else {
		//
		if ( HAS_DUMMY_WAFER_CHAMBER() ) { // 2019.03.21
			//
			Dummy_Regist( side , pt , ( MAX_CASSETTE_SLOT_SIZE - 1 ) , -1 );
			//
		}
		//
	}
	//
	IO_CONTROL_MODE = IO_CONTROL_USE();
	//
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	
	if ( nowaycheck ) {//
		//BM이 FM Side 고 완료된 Wafer 가 있는 경우  
		
		bmStatus = _SCH_MODULE_Get_BM_FULL_MODE( BM1 ); 
		

		//if(bmStatus == BUFFER_FM_STATION || bmStatus == BUFFER_WAIT_STATION ) //BM1 FM Side or 전환 중 
		if(bmStatus == BUFFER_TM_STATION ) //BM1 TM Side 
		{

			bw1 = _SCH_MODULE_Get_BM_WAFER( BM1 , 1 );	
			
			if(bw1>0 )
			{
				bs1  = _SCH_MODULE_Get_BM_SIDE( BM1 , 1 );
				bp1 = _SCH_MODULE_Get_BM_POINTER( BM1 , 1 );
				pathdo  = _SCH_CLUSTER_Get_PathDo(bs1,bp1);

				if(pathdo > 0  )  //pathdo는 시작 시 0 이고 , 이후 돌아 올 Wafer 는 0 보다 큼 
				{
					//BM1의 첫번째 Slot에 회수할 Wafer 가 있는 경우 
					//_SCH_LOG_LOT_PRINTF( side , "Request Make FM Side [%d][%d]\n" , bmStatus,bmStatus2);
					if(_SCH_IO_GET_MODULE_RESULT(  BM1 , 1 ) > 0)
					{
						//	_SCH_LOG_LOT_PRINTF( side , "[BM1-1]Request Make FM Side1 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs1,bp1);
						return 1; 
					}
					else if(pathdo == PATHDO_WAFER_RETURN )
					{
						//_SCH_LOG_LOT_PRINTF( side , "[BM1-1]Request Make FM Side2 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs1,bp1);
						return 1; 
					}
				}

				else 
				{
					bw2 = _SCH_MODULE_Get_BM_WAFER( BM1 , 2 );	
					
					if(bw2>0)
					{
						bs2  = _SCH_MODULE_Get_BM_SIDE( BM1 , 2 );
						bp2 = _SCH_MODULE_Get_BM_POINTER( BM1 , 2 );

						pathdo  = _SCH_CLUSTER_Get_PathDo(bs2,bp2);
					
							
						if(pathdo > 0 )
						{
							//BM1의 두번째 Slot에 회수할 Wafer 가 있는 경우 
							if(_SCH_IO_GET_MODULE_RESULT(  BM1 , 2 ) > 0)
							{
								//	_SCH_LOG_LOT_PRINTF( side , "[BM1 - 2]Request Make FM Side1 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
								return 1; 
							}
							else if(pathdo == PATHDO_WAFER_RETURN )
							{
								//	_SCH_LOG_LOT_PRINTF( side , "[BM1 - 2]Request Make FM Side2 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
								return 1; 
							}
						}
					}

				}
			}
			else 
			{
				bw2 = _SCH_MODULE_Get_BM_WAFER( BM1 , 2 );
				
				if(bw2>0)
				{
					bs2  = _SCH_MODULE_Get_BM_SIDE( BM1 , 2 );
					bp2 = _SCH_MODULE_Get_BM_POINTER( BM1 , 2 );

					pathdo  = _SCH_CLUSTER_Get_PathDo(bs2,bp2);
					
						
					if(pathdo > 0 )
					{
						//BM1의 두번째 Slot에 회수할 Wafer 가 있는 경우 
						if(_SCH_IO_GET_MODULE_RESULT(  BM1 , 2 ) > 0)
						{
							//	_SCH_LOG_LOT_PRINTF( side , "[BM1 - 2]Request Make FM Side1 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
							return 1; 
						}
						else if(pathdo == PATHDO_WAFER_RETURN )
						{
							//	_SCH_LOG_LOT_PRINTF( side , "[BM1 - 2]Request Make FM Side2 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
							return 1; 
						}
					}
				}
			}
		}
		else 
		{
			bmStatus= _SCH_MODULE_Get_BM_FULL_MODE( BM1 +1); 
					
			//if( bmStatus == BUFFER_FM_STATION || bmStatus == BUFFER_WAIT_STATION ) //BM2 FM Side or 전환 중 	
			if(bmStatus == BUFFER_TM_STATION ) 
			{
				bw1 = _SCH_MODULE_Get_BM_WAFER( BM1+1 , 1 );	

				if(bw1>0)
				{

					bs1  = _SCH_MODULE_Get_BM_SIDE( BM1+1 , 1 );
					bp1 = _SCH_MODULE_Get_BM_POINTER( BM1+1 , 1 );
				
					pathdo  = _SCH_CLUSTER_Get_PathDo(bs1,bs1);
				
				

					if(pathdo>0 )
					{
						//BM2의 첫번째 Slot에 회수할 Wafer 가 있는 경우 
						if(_SCH_IO_GET_MODULE_RESULT(  BM1+1 , 1 ) > 0)
						{
						//	_SCH_LOG_LOT_PRINTF( side , "[BM2 - 1]Request Make FM Side1 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs1,bp1);
							return 1; 
						}
						else if(pathdo == PATHDO_WAFER_RETURN )
						{
						//	_SCH_LOG_LOT_PRINTF( side , "[BM2 - 1]Request Make FM Side3 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs1,bp1);
							return 1; 
						}
					}

					else 
					{
						bw2 = _SCH_MODULE_Get_BM_WAFER( BM1+1 , 2 );	

						if(bw2>0)
						{
							bs2  = _SCH_MODULE_Get_BM_SIDE( BM1+1 , 2 );
							bp2 = _SCH_MODULE_Get_BM_POINTER( BM1 +1, 2 );
					
							pathdo  = _SCH_CLUSTER_Get_PathDo(bs2,bp2);

							if(pathdo>0 )
							{
								//BM2의 두번째 Slot에 회수할 Wafer 가 있는 경우 
								if(_SCH_IO_GET_MODULE_RESULT(  BM1+1 , 2 ) > 0)
								{
									//_SCH_LOG_LOT_PRINTF( side , "[BM2 - 2]Request Make FM Side1 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
									return 1; 
								}
								else if(pathdo == PATHDO_WAFER_RETURN )
								{
									//_SCH_LOG_LOT_PRINTF( side , "[BM2 - 2]Request Make FM Side2 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
									return 1; 
								}
							}
						}
					}
				}
				else 
				{
					bw2 = _SCH_MODULE_Get_BM_WAFER( BM1+1 , 2 );	

					if(bw2>0)
					{
						bs2  = _SCH_MODULE_Get_BM_SIDE( BM1+1 , 2 );
						bp2 = _SCH_MODULE_Get_BM_POINTER( BM1 +1, 2 );
					
						pathdo  = _SCH_CLUSTER_Get_PathDo(bs2,bp2);

						if(pathdo>0 )
						{
							//BM2의 두번째 Slot에 회수할 Wafer 가 있는 경우 
							if(_SCH_IO_GET_MODULE_RESULT(  BM1+1 , 2 ) > 0)
							{
								//_SCH_LOG_LOT_PRINTF( side , "[BM2 - 2]Request Make FM Side1 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
								return 1; 
							}
							else if(pathdo == PATHDO_WAFER_RETURN )
							{
								//_SCH_LOG_LOT_PRINTF( side , "[BM2 - 2]Request Make FM Side2 [nowaycheck :%d][bmStatus :%d][pathdo :%d][%d:%d]\n" , nowaycheck,bmStatus,pathdo,bs2,bp2);
								return 1; 
							}
						}
					}

				}
			}
		}

	} //nowaycheck if 
	
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	if ( _SCH_PRM_GET_DFIX_TM_DOUBLE_CONTROL() > 0 ) { // 2017.02.01
		//
		if ( ( _SCH_MODULE_Get_FM_WAFER( 0 ) > 0 ) || ( _SCH_MODULE_Get_FM_WAFER( 1 ) > 0 ) ) {
			//
			for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) { // 2017.04.20
				//
				if ( IO_PM_REMAIN_GET( i ) == 4 ) IO_PM_REMAIN_SET( i , 5 );
				//
			}
			//
			RemainTimeSetting( FALSE );
			//
			_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SEL0 [side=%d,pt=%d][nowaycheck=%d]\n" , side , pt , nowaycheck );
			//
			return 1;
		}
		else { // 2017.12.01
			//
			if ( !_SCH_ROBOT_GET_FM_FINGER_HW_USABLE( TA_STATION ) || !_SCH_ROBOT_GET_FM_FINGER_HW_USABLE( TB_STATION ) ) {
				//
				if ( BM_is_Wating_for_Supply_Wafer( side , pt ) ) {
					//
					for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) { // 2017.04.20
						//
						if ( IO_PM_REMAIN_GET( i ) == 4 ) IO_PM_REMAIN_SET( i , 5 );
						//
					}
					//
					RemainTimeSetting( FALSE );
					//
					_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SEL0 [side=%d,pt=%d][nowaycheck=%d]\n" , side , pt , nowaycheck );
					//
					return 1;
				}
				//
			}

		}
		//
	}
	else { // 2018.05.08
		//
		if ( MULTI_PM_MODE ) {
			//
			if ( _SCH_PRM_GET_MODULE_BUFFER_SWITCH_SWAPPING() == BUFFER_SWITCH_FULLSWAP ) {
				if (
					!_SCH_ROBOT_GET_FINGER_HW_USABLE( 0 , TA_STATION ) || !_SCH_ROBOT_GET_FINGER_HW_USABLE( 0 , TB_STATION ) ||
					!_SCH_ROBOT_GET_FINGER_HW_USABLE( 1 , TA_STATION ) || !_SCH_ROBOT_GET_FINGER_HW_USABLE( 1 , TB_STATION )
					) { // 2018.11.22
					//
						if ( Multi_is_BM_Locking_for_FullSwap() ) multi1ArmBlock = TRUE;
					//
				}
				//
				else {
					Multi_is_BM_Locking_for_FullSwapReset();
				}
			}
			//
			if ( Multi_is_Wating_for_Supply_Wafer( side , pt , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP ) ) {
				//
				multiNextSupply = TRUE;
				//
			}
			//
		}
		//
	}
	//
	// Check Possible PMSide
	//
	//
	for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
		//
		DataMap_LotPreUse[i] = FALSE; // 2017.10.22
		//
		DataMap_PMSide[i] = -1;
		DataMap_PMPointer[i] = -1; // 2017.10.22
		//
		DataMap_PMReqMode[i] = MODE_0_UNKNOWN; // 2017.12.11
		//
	}
	//
	for ( i = 0 ; i < MAX_CASSETTE_SIDE ; i++ ) {
		//
		if ( i == side ) {
			j = pt;
		}
		else {
			//
			if ( !_SCH_SYSTEM_USING_RUNNING(i) ) continue;
			//
			if ( !_SCH_SUB_POSSIBLE_PICK_FROM_FM_NOSUPPLYCHECK( i , &m , &j ) ) continue;
			//
		}
		//
		if ( j >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) continue;
		//
		dummy = _SCH_CLUSTER_Get_PathIn( i , j ) >= BM1; // 2017.12.11
		//
		if ( ( 2 == IO_CONTROL_MODE ) && ( _SCH_CLUSTER_Get_PathRange( i , j ) > 1 ) ) { // 2017.10.23 // 2018.07.04
			//
			for ( k = 0 ; k < MAX_CHAMBER ; k++ ) {
				//
				DataMap_Select[k] = FALSE;
				//
			}
			//
			for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
				//
				m = _SCH_CLUSTER_Get_PathProcessChamber( i, j , 0 , k );
				//
				if ( PM_ENABLE_FOR_MULTI_TM_PLACE( m ) ) { // 2018.10.30
				//
//				if ( m > 0 ) { // 2018.10.30
					//
					if ( _SCH_CLUSTER_Get_PathProcessParallel( i , j , 0 ) != NULL ) {
						//
						if ( Parallel_Invalid_Enable( i , j , _SCH_CLUSTER_Get_PathProcessParallel2( i , j , 0 , ( m - PM1 ) ) ) ) continue;
						//
					}
					//
					DataMap_Select[m] = TRUE;
					//
					if ( DataMap_PMSide[m] == -1 ) {
						DataMap_PMSide[m] = i;
						DataMap_PMPointer[m] = j; // 2017.10.22
						//
						DataMap_PMReqMode[m] = dummy ? MODE_2_DUMMY_SUPPLY : MODE_1_NORMAL_SUPPLY; 
						//
					}
					else {
						if ( _SCH_SYSTEM_RUNORDER_GET( i ) < _SCH_SYSTEM_RUNORDER_GET( DataMap_PMSide[m] ) ) {
							DataMap_PMSide[m] = i;
							DataMap_PMPointer[m] = j; // 2017.10.22
							//
							DataMap_PMReqMode[m] = dummy ? MODE_2_DUMMY_SUPPLY : MODE_1_NORMAL_SUPPLY; 
							//
						}
					}
				}
				//
			}
			//
			for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
				//
				m = _SCH_CLUSTER_Get_PathProcessChamber( i, j , 0 , k );
				//
				if ( PM_ENABLE_FOR_MULTI_TM_PLACE( m ) ) { // 2018.10.30
				//
//				if ( m > 0 ) { // 2018.10.30
					//
					if ( !DataMap_Select[m] ) {

						if ( _SCH_CLUSTER_Get_PathProcessParallel( i , j , 0 ) != NULL ) {
							//
							if ( _SCH_PREPRCS_FirstRunPre_Check_PathProcessData( i , m ) ) continue;
							//
							if ( Parallel_Invalid_xlotpre( i , j , _SCH_CLUSTER_Get_PathProcessParallel2( i , j , 0 , ( m - PM1 ) ) ) ) continue;
							//
						}
						//
						if ( DataMap_PMSide[m] == -1 ) {
							DataMap_PMSide[m] = i;
							DataMap_PMPointer[m] = j; // 2017.10.22
							//
							DataMap_PMReqMode[m] = dummy ? MODE_2_DUMMY_SUPPLY : MODE_1_NORMAL_SUPPLY; 
							//
						}
						else {
							if ( _SCH_SYSTEM_RUNORDER_GET( i ) < _SCH_SYSTEM_RUNORDER_GET( DataMap_PMSide[m] ) ) {
								DataMap_PMSide[m] = i;
								DataMap_PMPointer[m] = j; // 2017.10.22
								//
								DataMap_PMReqMode[m] = dummy ? MODE_2_DUMMY_SUPPLY : MODE_1_NORMAL_SUPPLY; 
								//
							}
						}
					}
				}
				//
			}
			//
		}
		else {
			//
			for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
				//
				m = _SCH_CLUSTER_Get_PathProcessChamber( i, j , 0 , k );
				//
				if ( PM_ENABLE_FOR_MULTI_TM_PLACE( m ) ) { // 2018.10.30
				//
//				if ( m > 0 ) { // 2018.10.30
					//
					//
					if ( DataMap_PMSide[m] == -1 ) {
						DataMap_PMSide[m] = i;
						DataMap_PMPointer[m] = j; // 2017.10.22
						//
						DataMap_PMReqMode[m] = dummy ? MODE_2_DUMMY_SUPPLY : MODE_1_NORMAL_SUPPLY; 
						//
					}
					else {
						if ( _SCH_SYSTEM_RUNORDER_GET( i ) < _SCH_SYSTEM_RUNORDER_GET( DataMap_PMSide[m] ) ) {
							DataMap_PMSide[m] = i;
							DataMap_PMPointer[m] = j; // 2017.10.22
							//
							DataMap_PMReqMode[m] = dummy ? MODE_2_DUMMY_SUPPLY : MODE_1_NORMAL_SUPPLY; 
							//
						}
					}
				}
				//
			}
			//
		}
		//
	}
	//
	//----------------------------------------------------------------------
	//
	// 2018.02.10
	//
	for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
		//
//		if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) continue; // 2019.05.23
		//
		if ( !PM_ENABLE_FOR_MULTI_TM_PLACE( i ) ) continue; // 2018.10.30
		//
		if ( Dummy_Request( i ) ) {
			//
			if ( DataMap_PMSide[i] == -1 ) {
				//
				DataMap_PMSide[i] = side;
				DataMap_PMPointer[i] = pt;
				//
				DataMap_PMReqMode[i] = MODE_4_DUMMY_REQUEST_FROM_END; 
				//
			}
			else {
				//
				if ( DataMap_PMReqMode[i] == MODE_1_NORMAL_SUPPLY ) { // !dummy
					//
					DataMap_PMReqMode[i] = MODE_3_DUMMY_REQUEST_FROM_NORMAL;
					//
				}
			}
			//
		}
		//
	}

	//----------------------------------------------------------------------
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM 1 [NW=%d][%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n" , 
		//
		nowaycheck , multiNextSupply ,
		//
		DataMap_Select[PM1+0] , DataMap_Select[PM1+1] ,
		DataMap_Select[PM1+2] , DataMap_Select[PM1+3] ,
		DataMap_Select[PM1+4] , DataMap_Select[PM1+5] ,
		DataMap_Select[PM1+6] , DataMap_Select[PM1+7] ,
		DataMap_Select[PM1+8] , DataMap_Select[PM1+9] ,
		DataMap_Select[PM1+10] , DataMap_Select[PM1+11] ,
		DataMap_Select[PM1+12] , DataMap_Select[PM1+13] ,
		//
		DataMap_PMSide[PM1+0] , DataMap_PMSide[PM1+1] ,
		DataMap_PMSide[PM1+2] , DataMap_PMSide[PM1+3] ,
		DataMap_PMSide[PM1+4] , DataMap_PMSide[PM1+5] ,
		DataMap_PMSide[PM1+6] , DataMap_PMSide[PM1+7] ,
		DataMap_PMSide[PM1+8] , DataMap_PMSide[PM1+9] ,
		DataMap_PMSide[PM1+10] , DataMap_PMSide[PM1+11] ,
		DataMap_PMSide[PM1+12] , DataMap_PMSide[PM1+13] ,
		//
		DataMap_PMPointer[PM1+0] , DataMap_PMPointer[PM1+1] ,
		DataMap_PMPointer[PM1+2] , DataMap_PMPointer[PM1+3] ,
		DataMap_PMPointer[PM1+4] , DataMap_PMPointer[PM1+5] ,
		DataMap_PMPointer[PM1+6] , DataMap_PMPointer[PM1+7] ,
		DataMap_PMPointer[PM1+8] , DataMap_PMPointer[PM1+9] ,
		DataMap_PMPointer[PM1+10] , DataMap_PMPointer[PM1+11] ,
		DataMap_PMPointer[PM1+12] , DataMap_PMPointer[PM1+13] ,
		//
		DataMap_PMReqMode[PM1+0] , DataMap_PMReqMode[PM1+1] ,
		DataMap_PMReqMode[PM1+2] , DataMap_PMReqMode[PM1+3] ,
		DataMap_PMReqMode[PM1+4] , DataMap_PMReqMode[PM1+5] ,
		DataMap_PMReqMode[PM1+6] , DataMap_PMReqMode[PM1+7] ,
		DataMap_PMReqMode[PM1+8] , DataMap_PMReqMode[PM1+9] ,
		DataMap_PMReqMode[PM1+10] , DataMap_PMReqMode[PM1+11] ,
		DataMap_PMReqMode[PM1+12] , DataMap_PMReqMode[PM1+13]
		//
		);
	//----------------------------------------------------------------------
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//
	// Check Remain PM Time
	//
	while ( TRUE ) {
		//
		for ( W = 0 ; W < 2 ; W++ ) {
			//
			DataMap_WfrCRC[W] = 0;
			DataMap_WfrCNT[W] = 0;
			//
			if ( W != 0 ) {
				//
				multireturning = FALSE; // 2018.11.20
				//
				for ( i = 0 ; i < MAX_CHAMBER ; i++ ) {
					DataMap_PMGoCount[i] = 0;
					DataMap_PMRemainTime[i] = 0;
					//
					DataMap_PMTag[i] = 0;
					//
					DataMap_PMGoCount2[i] = 0; // 2017.09.19
					//
					DataMap_PMInCount[i] = _PMW_0_NOTHING; // 2017.11.23
					//
				}
			}
			//
			for ( i = BM1 ; i < ( BM1 + MAX_BM_CHAMBER_DEPTH ) ; i++ ) {
				//
				if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) continue;
				//
				if ( _SCH_PRM_GET_MODULE_DUMMY_BUFFER_CHAMBER() == i ) continue;
				//
				for ( j = 1 ; j <= _SCH_PRM_GET_MODULE_SIZE( i ) ; j++ ) {
					//
					if ( _SCH_PRM_GET_DFIX_TM_DOUBLE_CONTROL() > 0 ) {
						//
						if ( _SCH_MODULE_Get_BM_WAFER( i , j ) > 0 ) {
							//
							if ( _SCH_MODULE_Get_BM_WAFER( i , j+1 ) > 0 ) {
								//
								if      ( Go_0_1_2_PM( _SCH_MODULE_Get_BM_SIDE( i , j ) , _SCH_MODULE_Get_BM_POINTER( i , j ) ) == 1 ) {
									//
									rs = _SCH_MODULE_Get_BM_SIDE( i , j );
									rp = _SCH_MODULE_Get_BM_POINTER( i , j );
									//
									DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_BM_WAFER( i , j ) + _SCH_MODULE_Get_BM_WAFER( i , j+1 );
									//
								}
								else if ( Go_0_1_2_PM( _SCH_MODULE_Get_BM_SIDE( i , j+1 ) , _SCH_MODULE_Get_BM_POINTER( i , j+1 ) ) == 1 ) {
									//
									rs = _SCH_MODULE_Get_BM_SIDE( i , j+1 );
									rp = _SCH_MODULE_Get_BM_POINTER( i , j+1 );
									//
									DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_BM_WAFER( i , j ) + _SCH_MODULE_Get_BM_WAFER( i , j+1 );
									//
								}
								else {
									//
									rs = _SCH_MODULE_Get_BM_SIDE( i , j );
									rp = _SCH_MODULE_Get_BM_POINTER( i , j );
									//
									DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_BM_WAFER( i , j ) + _SCH_MODULE_Get_BM_WAFER( i , j+1 );
									//
								}
								//
							}
							else {
								//
								rs = _SCH_MODULE_Get_BM_SIDE( i , j );
								rp = _SCH_MODULE_Get_BM_POINTER( i , j );
								//
								DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_BM_WAFER( i , j );
								//
							}
						}
						else {
							if ( _SCH_MODULE_Get_BM_WAFER( i , j+1 ) > 0 ) {
								//
								rs = _SCH_MODULE_Get_BM_SIDE( i , j+1 );
								rp = _SCH_MODULE_Get_BM_POINTER( i , j+1 );
								//
								DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_BM_WAFER( i , j+1 );
								//
							}
							else {
								j++;
								continue;
							}
						}
						//
						j++;
						//
					}
					else {
						//
						if ( _SCH_MODULE_Get_BM_WAFER( i , j ) <= 0 ) continue;
						//
						rs = _SCH_MODULE_Get_BM_SIDE( i , j );
						rp = _SCH_MODULE_Get_BM_POINTER( i , j );
						//
						DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_BM_WAFER( i , j );
						//
					}
					//
					//
					if ( Get_Run_List( rs , rp , NULL , NULL , _G_WillList , &_G_WillCount , _G_Will2List , &_G_Will2Count ) < 1 ) continue;
					//
					if ( _G_WillCount == 0 ) {
						//
						if ( W != 0 ) multireturning = TRUE;
						//
						continue;
					}
					//
					if ( W != 0 ) {
						//
						for ( k = 0 ; k < _G_WillCount ; k++ ) {
							//
							DataMap_PMTag[_G_WillList[k]] += 1;
							//
							DataMap_PMGoCount[_G_WillList[k]]++;
							//
						}
						//
					}
					//
					DataMap_WfrCNT[W] += _G_WillCount;
					//
					if ( W != 0 ) {
						//
						for ( k = 0 ; k < _G_Will2Count ; k++ ) {
							DataMap_PMGoCount2[_G_Will2List[k]]++;
						}
						//
					}
					//
				}
			}
			//
			for ( i = 0 ; i < MAX_TM_CHAMBER_DEPTH ; i++ ) {
				//
				if ( !_SCH_PRM_GET_MODULE_MODE( TM + i ) ) continue;
				//
				for ( j = TA_STATION ; j <= TB_STATION ; j++ ) {
					//
					if ( _SCH_MODULE_Get_TM_WAFER( i , j ) <= 0 ) continue;
					//
					if ( _SCH_PRM_GET_DFIX_TM_DOUBLE_CONTROL() > 0 ) {

						k = _SCH_MODULE_Get_TM_WAFER( i , j );
						//
						if ( ( ( k % 100 ) > 0 ) && ( ( k / 100 ) > 0 ) ) {
							//
							if      ( Go_0_1_2_PM( _SCH_MODULE_Get_TM_SIDE( i , j ) , _SCH_MODULE_Get_TM_POINTER( i , j ) ) == 1 ) {
								//
								rs = _SCH_MODULE_Get_TM_SIDE( i , j );
								rp = _SCH_MODULE_Get_TM_POINTER( i , j );
								//
								DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_TM_WAFER( i , j );
								//
							}
							else if ( Go_0_1_2_PM( _SCH_MODULE_Get_TM_SIDE2( i , j ) , _SCH_MODULE_Get_TM_POINTER2( i , j ) ) == 1 ) {
								//
								rs = _SCH_MODULE_Get_TM_SIDE2( i , j );
								rp = _SCH_MODULE_Get_TM_POINTER2( i , j );
								//
								DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_TM_WAFER( i , j );
								//
							}
							else {
								//
								rs = _SCH_MODULE_Get_TM_SIDE( i , j );
								rp = _SCH_MODULE_Get_TM_POINTER( i , j );
								//
								DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_TM_WAFER( i , j );
								//
							}
							//
						}
						else {
							//
							rs = _SCH_MODULE_Get_TM_SIDE( i , j );
							rp = _SCH_MODULE_Get_TM_POINTER( i , j );
							//
							DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_TM_WAFER( i , j );
							//
						}
						//
					}
					else {
						//
						rs = _SCH_MODULE_Get_TM_SIDE( i , j );
						rp = _SCH_MODULE_Get_TM_POINTER( i , j );
						//
						DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + _SCH_MODULE_Get_TM_WAFER( i , j );
						//
					}
					//
					if ( Get_Run_List( rs , rp , NULL , NULL , _G_WillList , &_G_WillCount , _G_Will2List , &_G_Will2Count ) < 1 ) continue;
					//
					if ( _G_WillCount == 0 ) {
						//
						if ( W != 0 ) multireturning = TRUE;
						//
						continue;
					}
					//
					if ( W != 0 ) {
						//
						for ( k = 0 ; k < _G_WillCount ; k++ ) {
							//
							DataMap_PMTag[_G_WillList[k]] += 10;
							//
							DataMap_PMGoCount[_G_WillList[k]]++;
							//
						}
						//
					}
					//
					DataMap_WfrCNT[W] += _G_WillCount;
					//
					if ( W != 0 ) {
						//
						for ( k = 0 ; k < _G_Will2Count ; k++ ) {
							DataMap_PMGoCount2[_G_Will2List[k]]++;
						}
						//
					}
					//
				}
				//
			}
			//
			//-----------------------
			for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
				//
				if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) continue;
				//
				k = GET_PROCESS_REMAIN_TIME_PM( i , &j , &rs , &rp , &rw , &rm , TRUE , NULL );
				//
				if ( W != 0 ) {
					//
					DataMap_PMRemainTime[i] = DataMap_PMRemainTime[i] + k;
					//
					DataMap_PMTag[i] = DataMap_PMTag[i] + ( j * 1000 );
					//
				}
				//
				//
				DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + i + j + rm;
				//
				//
				// 2017.10.17
				//
				if ( W != 0 ) {
					//
					DataMap_PMInCount[i] = rm; // 2017.11.23
					//
					switch( rm ) {
					case _PMW_2_PROCESSED :
					case _PMW_3_ALL :
						multireturning = TRUE;
						break;
					}
					//
				}
				//
				DataMap_WfrCRC[W] = DataMap_WfrCRC[W] + rs + rp + rw; // 2017.11.02
				//
				if ( rw <= 0 ) continue;
				//
				if ( 2 != IO_CONTROL_MODE ) continue;
				//
				if ( W != 0 ) {
					//
					if ( Get_Run_List( rs , rp , NULL , NULL , NULL , NULL , _G_Will2List , &_G_Will2Count ) < 1 ) continue;
					//
					for ( k = 0 ; k < _G_Will2Count ; k++ ) {
						DataMap_PMGoCount2[_G_Will2List[k]]++;
					}
				}
				//
			}
			//
			//-----------------------
		}
		//
		//-------------------------------------------------------------------------------------
		//
		if ( ( DataMap_WfrCRC[0] == DataMap_WfrCRC[1] ) && ( DataMap_WfrCNT[0] == DataMap_WfrCNT[1] ) ) break;
		//
		//-------------------------------------------------------------------------------------
		//
		_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM RECHECK [side=%d,pt=%d] [%d/%d][%d/%d]\n" , side , pt , DataMap_WfrCRC[0] , DataMap_WfrCRC[1] , DataMap_WfrCNT[0] , DataMap_WfrCNT[1] );
		//
	}
	//
	//----------------------------------------------------------------------
	/*
	//
	// 2017.02.01
	//
	for ( i = TA_STATION ; i <= TB_STATION ; i++ ) {
		//
		if ( _SCH_MODULE_Get_FM_WAFER( i ) <= 0 ) continue;
		//
		rs = _SCH_MODULE_Get_FM_SIDE( i );
		rp = _SCH_MODULE_Get_FM_POINTER( i );
		//
		if ( _SCH_CLUSTER_Get_PathDo( rs , rp ) > 0 ) continue;
		if ( _SCH_CLUSTER_Get_PathRange( rs , rp ) < 1 ) continue;
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( rs , rp , 0 , k );
			//
			if ( m > 0 ) {
				//
				DataMap_PMTag[m] = DataMap_PMTag[m] + 100;
				//
				DataMap_PMGoCount[m]++;
				//
			}
			//
		}
		//
		if ( _SCH_PRM_GET_DFIX_TM_DOUBLE_CONTROL() > 0 ) break;
		//
	}
	//
	*/
	//
	//----------------------------------------------------------------------
	for ( i = 1 ; i <= 2 ; i++ ) {
		//
		if ( _SCH_MODULE_Get_MFALS_WAFER( i ) <= 0 ) continue;
		//
		rs = _SCH_MODULE_Get_MFALS_SIDE( i );
		rp = _SCH_MODULE_Get_MFALS_POINTER( i );
		//
		if ( Go_0_1_2_PM( rs , rp ) > 1 ) continue;
		//
		if ( Get_Run_List( rs , rp , NULL , NULL , _G_WillList , &_G_WillCount , _G_Will2List , &_G_Will2Count ) < 1 ) continue;
		//
		for ( k = 0 ; k < _G_WillCount ; k++ ) {
			//
			DataMap_PMTag[_G_WillList[k]] += 300;
			//
			DataMap_PMGoCount[_G_WillList[k]]++;
			//
		}
		//
		for ( k = 0 ; k < _G_Will2Count ; k++ ) {
			DataMap_PMGoCount2[_G_Will2List[k]]++;
		}
		//
		if ( _SCH_PRM_GET_DFIX_TM_DOUBLE_CONTROL() > 0 ) break;
		//
	}
	//
	//----------------------------------------------------------------------
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM 2 [NW=%d][%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n" , 
		//
		nowaycheck , multi1ArmBlock , multireturning ,

		DataMap_PMGoCount[PM1+0] , DataMap_PMGoCount[PM1+1] ,
		DataMap_PMGoCount[PM1+2] , DataMap_PMGoCount[PM1+3] ,
		DataMap_PMGoCount[PM1+4] , DataMap_PMGoCount[PM1+5] ,
		DataMap_PMGoCount[PM1+6] , DataMap_PMGoCount[PM1+7] ,
		DataMap_PMGoCount[PM1+8] , DataMap_PMGoCount[PM1+9] ,
		DataMap_PMGoCount[PM1+10] , DataMap_PMGoCount[PM1+11] ,
		DataMap_PMGoCount[PM1+12] , DataMap_PMGoCount[PM1+13] ,
		//
		DataMap_PMGoCount2[PM1+0] , DataMap_PMGoCount2[PM1+1] ,
		DataMap_PMGoCount2[PM1+2] , DataMap_PMGoCount2[PM1+3] ,
		DataMap_PMGoCount2[PM1+4] , DataMap_PMGoCount2[PM1+5] ,
		DataMap_PMGoCount2[PM1+6] , DataMap_PMGoCount2[PM1+7] ,
		DataMap_PMGoCount2[PM1+8] , DataMap_PMGoCount2[PM1+9] ,
		DataMap_PMGoCount2[PM1+10] , DataMap_PMGoCount2[PM1+11] ,
		DataMap_PMGoCount2[PM1+12] , DataMap_PMGoCount2[PM1+13] ,
		//
		DataMap_PMInCount[PM1+0] , DataMap_PMInCount[PM1+1] ,
		DataMap_PMInCount[PM1+2] , DataMap_PMInCount[PM1+3] ,
		DataMap_PMInCount[PM1+4] , DataMap_PMInCount[PM1+5] ,
		DataMap_PMInCount[PM1+6] , DataMap_PMInCount[PM1+7] ,
		DataMap_PMInCount[PM1+8] , DataMap_PMInCount[PM1+9] ,
		DataMap_PMInCount[PM1+10] , DataMap_PMInCount[PM1+11] ,
		DataMap_PMInCount[PM1+12] , DataMap_PMInCount[PM1+13] ,
		//
		DataMap_PMRemainTime[PM1+0] , DataMap_PMRemainTime[PM1+1] ,
		DataMap_PMRemainTime[PM1+2] , DataMap_PMRemainTime[PM1+3] ,
		DataMap_PMRemainTime[PM1+4] , DataMap_PMRemainTime[PM1+5] ,
		DataMap_PMRemainTime[PM1+6] , DataMap_PMRemainTime[PM1+7] ,
		DataMap_PMRemainTime[PM1+8] , DataMap_PMRemainTime[PM1+9] ,
		DataMap_PMRemainTime[PM1+10] , DataMap_PMRemainTime[PM1+11] ,
		DataMap_PMRemainTime[PM1+12] , DataMap_PMRemainTime[PM1+13] ,
		//
		DataMap_PMTag[PM1+0] , DataMap_PMTag[PM1+1] ,
		DataMap_PMTag[PM1+2] , DataMap_PMTag[PM1+3] ,
		DataMap_PMTag[PM1+4] , DataMap_PMTag[PM1+5] ,
		DataMap_PMTag[PM1+6] , DataMap_PMTag[PM1+7] ,
		DataMap_PMTag[PM1+8] , DataMap_PMTag[PM1+9] ,
		DataMap_PMTag[PM1+10] , DataMap_PMTag[PM1+11] ,
		DataMap_PMTag[PM1+12] , DataMap_PMTag[PM1+13]
		);
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//
	// 2018.11.27
	//
	if ( MULTI_PM_MODE ) {
		//
		_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM MULTI [side=%d,pt=%d][nowaycheck=%d][%d][%d][%d,%d,%d/%d,%d,%d,%d]\n" , side , pt , nowaycheck , multiNextSupply , multireturning , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
		//
		if ( !multiNextSupply ) {
			//
			if ( ( MULTI_PM_LAST >= PM1 ) && ( DataMap_PMSide[MULTI_PM_LAST] != -1 ) && ( side != DataMap_PMSide[MULTI_PM_LAST] ) ) {
				//
				if ( Multi_is_Wating_for_Supply_Wafer( DataMap_PMSide[MULTI_PM_LAST] , DataMap_PMPointer[MULTI_PM_LAST] , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP ) ) {
					//
					if ( ( DataMap_PMInCount[MULTI_PM_LAST] == _PMW_1_WILLPROCESS ) || ( DataMap_PMInCount[MULTI_PM_LAST] == _PMW_3_ALL ) || ( DataMap_PMGoCount[MULTI_PM_LAST] > 0 ) ) {
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-MMM [side=%d,pt=%d][nowaycheck=%d][%d,%d,%d/%d,%d,%d,%d]\n" , side , pt , nowaycheck , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
						//
						return -83;
						//
					}
					//
				}
				//
			}
			//
		}
		//
		//======================================================================================================================================================================
		//======================================================================================================================================================================
		if ( multi1ArmBlock ) { // 2018.11.22
			//
			if ( multireturning ) {
				//
				for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
					//
					if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) {
						IO_PM_REMAIN_CM( i , 0 );
						IO_PM_REMAIN_TIME( i , -1 );
						IO_PM_REMAIN_SET( i , 3 );
						continue;
					}
					//
					IO_PM_REMAIN_CM( i , 0 );
					IO_PM_REMAIN_TIME( i , 200000 );
					IO_PM_REMAIN_SET( i , 3 );
					//
				}
				//
				_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-81 [side=%d,pt=%d]\n" , side , pt );
				//
				return -81;
				//
			}
			//
			for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
				//
				if ( DataMap_PMInCount[i] > 0 ) {
					//
					DataMap_PMRemainTime[i] = 300000;
					//
				}
			}
			//
		}
		//
		if ( multiNextSupply ) {
			//
			if ( ( DataMap_PMInCount[MULTI_PM_LAST] == _PMW_1_WILLPROCESS ) || ( DataMap_PMInCount[MULTI_PM_LAST] == _PMW_3_ALL ) || ( DataMap_PMGoCount[MULTI_PM_LAST] > 0 ) ) {
				//
				if ( !nowaycheck ) {
					//
					USER_SELECT_PM[side][pt] = 0;
					//
				}
				//
				RemainTimeSetting( multi1ArmBlock );
				//
				_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELM [side=%d,pt=%d][nowaycheck=%d][%d,%d,%d/%d,%d,%d,%d]\n" , side , pt , nowaycheck , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
				//
				return 1;
				//
			}
			else {
				//
				_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELXM2 [side=%d,pt=%d][nowaycheck=%d][%d,%d,%d/%d,%d,%d,%d]\n" , side , pt , nowaycheck , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
				//
			}
			//
		}
		//
	}
	//
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//======================================================================================================================================================================
	//
	if ( pt < ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
		//
		if ( 2 == IO_CONTROL_MODE ) { // 2017.09.07
			//
			for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
				//
				DataMap_PMPairRes[i] = 0;
				//
				if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) continue;
				//
				if ( DataMap_PMReqMode[i] != MODE_1_NORMAL_SUPPLY ) continue; // 2018.02.12
				//
				if ( DataMap_PMGoCount[i] > 0 ) continue;
				//
				if ( DataMap_PMGoCount2[i] > 0 ) continue;
				//
				if ( DataMap_PMSide[i] < 0 ) continue; // 2017.10.22
				if ( DataMap_PMPointer[i] < 0 ) continue; // 2017.10.22
				//
				if ( _SCH_CLUSTER_Get_PathRange( DataMap_PMSide[i] , DataMap_PMPointer[i] ) < 2 ) continue; // 2018.07.04
				//

	//			DataMap_PMPairRes[i] = DataHasPair( i , side , pt , DataMap_PMGoCount , DataMap_PMGoCount2 ); // 2017.10.22
	//			DataMap_PMPairRes[i] = DataHasPair( i , DataMap_PMSide[i] , DataMap_PMPointer[i] , DataMap_PMGoCount , DataMap_PMGoCount2 ); // 2017.10.22

				DataMap_PMPairRes[i] = DataHasPair( i , DataMap_PMSide[i] , DataMap_PMPointer[i] , DataMap_PMGoCount , DataMap_PMGoCount2 , &DataMap_LotPreUse[i] , DataMap_LotPreList[i] , &rs ); // 2017.10.22

				if ( rs != 0 ) DataMap_PMGoCount2[i] = rs;

				//
			}
			//
			//----------------------------------------------------------------------
			_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM L [NW=%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n" , 
				//
				nowaycheck ,

				DataMap_PMGoCount[PM1+0] , DataMap_PMGoCount[PM1+1] ,
				DataMap_PMGoCount[PM1+2] , DataMap_PMGoCount[PM1+3] ,
				DataMap_PMGoCount[PM1+4] , DataMap_PMGoCount[PM1+5] ,
				DataMap_PMGoCount[PM1+6] , DataMap_PMGoCount[PM1+7] ,
				DataMap_PMGoCount[PM1+8] , DataMap_PMGoCount[PM1+9] ,
				DataMap_PMGoCount[PM1+10] , DataMap_PMGoCount[PM1+11] ,
				DataMap_PMGoCount[PM1+12] , DataMap_PMGoCount[PM1+13] ,
				//
				DataMap_PMGoCount2[PM1+0] , DataMap_PMGoCount2[PM1+1] ,
				DataMap_PMGoCount2[PM1+2] , DataMap_PMGoCount2[PM1+3] ,
				DataMap_PMGoCount2[PM1+4] , DataMap_PMGoCount2[PM1+5] ,
				DataMap_PMGoCount2[PM1+6] , DataMap_PMGoCount2[PM1+7] ,
				DataMap_PMGoCount2[PM1+8] , DataMap_PMGoCount2[PM1+9] ,
				DataMap_PMGoCount2[PM1+10] , DataMap_PMGoCount2[PM1+11] ,
				DataMap_PMGoCount2[PM1+12] , DataMap_PMGoCount2[PM1+13] ,
				//
				DataMap_PMPairRes[PM1+0] , DataMap_PMPairRes[PM1+1] ,
				DataMap_PMPairRes[PM1+2] , DataMap_PMPairRes[PM1+3] ,
				DataMap_PMPairRes[PM1+4] , DataMap_PMPairRes[PM1+5] ,
				DataMap_PMPairRes[PM1+6] , DataMap_PMPairRes[PM1+7] ,
				DataMap_PMPairRes[PM1+8] , DataMap_PMPairRes[PM1+9] ,
				DataMap_PMPairRes[PM1+10] , DataMap_PMPairRes[PM1+11] ,
				DataMap_PMPairRes[PM1+12] , DataMap_PMPairRes[PM1+13]
				);
				//----------------------------------------------------------------------
				//
		}
		//
	}
	//==============================================================================================
	//==============================================================================================
	//==============================================================================================
	//
	for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
		//
		if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) {
			IO_PM_REMAIN_CM( i , 0 );
			IO_PM_REMAIN_TIME( i , -1 );
			continue;
		}
		//
		if ( DataMap_PMSide[i] < 0 ) {
			IO_PM_REMAIN_CM( i , 0 );
		}
		else {
			IO_PM_REMAIN_CM( i , DataMap_PMSide[i] + 1 );
		}
		//
		IO_PM_REMAIN_TIME( i , DataMap_PMRemainTime[i] );
		//
	}
	//
	LastRemainTimeSettingTime = GetTickCount();
	//
	//==============================================================================================
	//==============================================================================================
	//==============================================================================================
	//
	m = -1;
	//
	for ( i = PM1 ; i < (MAX_PM_CHAMBER_DEPTH + PM1) ; i++ ) {
		//
		if ( !_SCH_MODULE_GET_USE_ENABLE( i , FALSE , -1 ) ) {
			IO_PM_REMAIN_SET( i , 0 );
			//
			DataMap_PMTag2[i] = -1;
			//
			DataMap_PMSupplyTime[i] = -1;
			//
			continue;
		}
		//
		if ( DataMap_PMSide[i] <= -1 ) {
			//
			IO_PM_REMAIN_SET( i , 1 );
			//
			DataMap_PMTag2[i] = -2;
			//
			DataMap_PMSupplyTime[i] = -2;
			//
			continue;
		}
		//
		if ( MULTI_PM_MODE ) {
			//
			if ( Multi_is_PM_Slot_NotUse( i , DataMap_PMSide[i] , DataMap_PMPointer[i] , NULL ) ) {
				//
				IO_PM_REMAIN_SET( i , 1 );
				//
				DataMap_PMTag2[i] = -4;
				//
				DataMap_PMSupplyTime[i] = -4;
				//
				continue;
			}
			//
			if ( multi1ArmBlock ) { // 2018.11.22
				//
				if ( DataMap_PMInCount[i] > 0 ) {
					//
					IO_PM_REMAIN_SET( i , 1 );
					//
					DataMap_PMTag2[i] = -5;
					//
					DataMap_PMSupplyTime[i] = -5;
					//
					continue;
				}
				//
			}
			//
			if ( !multiNextSupply ) { // 2019.01.29
				//
				if ( _SCH_MACRO_HAS_PRE_PROCESS_OPERATION( side , pt , 400 + i ) ) {
					//
					if ( ( DataMap_PMGoCount[i] + DataMap_PMGoCount2[i] + DataMap_PMInCount[i] ) > 0 ) {
						//
						IO_PM_REMAIN_SET( i , 1 );
						//
						DataMap_PMTag2[i] = -6;
						//
						DataMap_PMSupplyTime[i] = -6;
						//
						continue;
						//
					}
					//
				}
				//
			}
			//
		}
		//
		if ( ( DataMap_PMGoCount[i] + DataMap_PMGoCount2[i] ) > 0 ) { // 2017.10.17
			//
			IO_PM_REMAIN_SET( i , 2 );
			//
			if ( m == -1 ) m = -2;
			//
			DataMap_PMTag2[i] = -3;
			//
			DataMap_PMSupplyTime[i] = -3;
			//
			continue;
		}
		//
		//=============================================================================================================================
		//
		DataMap_PMTag2[i] = 1;
		//
		if ( IO_CONTROL_MODE == 1 ) {
			DataMap_PMSupplyTime[i] = 0;
		}
		else {
			DataMap_PMSupplyTime[i] = IO_PM_SUPPLY_POSSIBLE_TIME( i );
		}
		//
		k = 0;
		//
		if ( DataMap_PMSupplyTime[i] > 0 ) {
			//
			if ( DataMap_PMSupplyTime[i] >= DataMap_PMRemainTime[i] ) {
				//
				k = 1;
				//
			}
			//
		}
		else {
			k = 1;
		}
		//
		if ( nowaycheck ) {
			//
			DataMap_PMRemainTime[i] = DataMap_PMRemainTime[i] - NOWAYCHECK_GAP_SEC;
			//
			if ( k == 0 ) {
				//
				if ( DataMap_PMSupplyTime[i] >= DataMap_PMRemainTime[i] ) {
					k = 2;
				}
				//
			}
			//
		}
		//
		//
		if ( k != 0 ) { // 2018.02.20
			//
			if      ( DataMap_PMReqMode[i] == MODE_3_DUMMY_REQUEST_FROM_NORMAL ) { // appdummy
				//
				if ( !Dummy_Add( FALSE , TRUE , i , DataMap_PMSide[i] , DataMap_PMPointer[i] , &rpt ) ) {
					//
					k = 0;
					//
					DataMap_PMTag2[i] += ( k == 1 ) ? 20 : 30;
					//
				}
			}
			else if ( DataMap_PMReqMode[i] == MODE_4_DUMMY_REQUEST_FROM_END ) { // appdummy/blank
				//
				if ( !Dummy_Add( TRUE , TRUE , i , side , pt , &rpt ) ) {
					//
					k = 0;
					//
					DataMap_PMTag2[i] += ( k == 1 ) ? 40 : 50;
					//
				}
				//
			}
			else {
				if ( pt >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) {
					//
					k = 0;
					//
					DataMap_PMTag2[i] += ( k == 1 ) ? 60 : 70;
					//
				}
			}
			//
		}
		else {
			//
			DataMap_PMTag2[i] += 10;
			//
		}
		//
		//=============================================================================================================================
		//
		if ( k == 0 ) {
			//
			IO_PM_REMAIN_SET( i , 3 );
			//
			if ( m == -1 ) m = -2;
			//
			continue;
		}
		//
		//
		if ( k == 1 ) {
			//
			//
			// 2017.10.22
			//
			if ( !nowaycheck ) {
				IO_PM_REMAIN_SET( i , 4 );
			}
			//
			DataMap_PMTag2[i] = 2;
			//
		}
		else {
			IO_PM_REMAIN_SET( i , 3 );
			//
			DataMap_PMTag2[i] = 3;
			//
		}
		//
		//
		//---------------------------------------------------------------------------------------------------
		//---------------------------------------------------------------------------------------------------
		//---------------------------------------------------------------------------------------------------
		//
		if ( m < 0 ) {
			//
			m = i;
			//
		}
		else {
			//
			j = 0;
			//
			if      ( DataMap_PMRemainTime[i] < DataMap_PMRemainTime[m] ) {
				if ( ( DataMap_PMRemainTime[m] - DataMap_PMRemainTime[i] ) <= REMAIN_TIME_SAME_GAP_SEC ) {
					//
					j = 1;
					//
					DataMap_PMTag2[i] += 10;
					//
				}
				else {
					//
					m = i;				
					//
					DataMap_PMTag2[i] += 20;
					//
				}
			}
			else if ( DataMap_PMRemainTime[i] > DataMap_PMRemainTime[m] ) {
				if ( ( DataMap_PMRemainTime[i] - DataMap_PMRemainTime[m] ) <= REMAIN_TIME_SAME_GAP_SEC ) {
					//
					j = 1;
					//
					DataMap_PMTag2[i] += 30;
					//
				}
				else {
					//
					DataMap_PMTag2[i] += 40;
					//
				}
			}
			else {
				//
				j = 1; // 2018.11.21
				//
				DataMap_PMTag2[i] += 50;
				//
			}
			//
			if ( j == 1 ) { // same
				//
				if      ( ( DataMap_PMReqMode[i] == MODE_2_DUMMY_SUPPLY ) && ( DataMap_PMReqMode[m] != MODE_2_DUMMY_SUPPLY ) ) { // 2018.02.12 appdummy
					//
					m = i;
					//
					DataMap_PMTag2[i] += 100;
					//
				}
				else {
					if      ( ( DataMap_PMReqMode[i] >= MODE_3_DUMMY_REQUEST_FROM_NORMAL ) && ( DataMap_PMReqMode[m] == MODE_1_NORMAL_SUPPLY ) ) { // 2018.02.12 appdummy
						//
						m = i;
						//
						DataMap_PMTag2[i] += 200;
						//
					}
					else {
						//
						if ( _SCH_PRM_GET_MODULE_GROUP( i ) != _SCH_PRM_GET_MODULE_GROUP( m ) ) {
							//
							j = 0;
							//
							if      ( _SCH_PRM_GET_Getting_Priority( _SCH_PRM_GET_MODULE_GROUP( i ) + TM ) < _SCH_PRM_GET_Getting_Priority( _SCH_PRM_GET_MODULE_GROUP( m ) + TM ) ) {
								m = i;
								//
								DataMap_PMTag2[i] += 300;
								//
							}
							else if ( _SCH_PRM_GET_Getting_Priority( _SCH_PRM_GET_MODULE_GROUP( i ) + TM ) > _SCH_PRM_GET_Getting_Priority( _SCH_PRM_GET_MODULE_GROUP( m ) + TM ) ) {
								//
								DataMap_PMTag2[i] += 400;
								//
							}
							else {
								//
								j = 1;
								//
								DataMap_PMTag2[i] += 500;
								//
							}
						}
						else {
							//
							j = 1; // 2018.11.21
							//
							DataMap_PMTag2[i] += 600;
							//
						}
						//
						if ( j == 1 ) { // same
							//
							if ( ( DataMap_PMRemainTime[i] <= 0 ) || ( DataMap_PMRemainTime[m] <= 0 ) ) { // 2018.11.21
								//
								if ( _SCH_TIMER_GET_PROCESS_TIME_END( 0 , i ) < _SCH_TIMER_GET_PROCESS_TIME_END( 0 , m ) ) {
									//
									m = i;
									//
									DataMap_PMTag2[i] += 1000;
									//
								}
								//
							}
							//
						}
						//
					}
				}
			}
			//
		}
		//
		//---------------------------------------------------------------------------------------------------
		//---------------------------------------------------------------------------------------------------
		//---------------------------------------------------------------------------------------------------
		//
	}
	//
	//----------------------------------------------------------------------
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM 3 [M=%d][P=%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d][%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n" , 
		//
		m ,
		//
		USER_SELECT_PM[side][((pt-1)<0)?pt:pt-1] ,
		USER_SELECT_PM[side][pt] ,
		USER_SELECT_PM[side][((pt+1)>=MAX_CASSETTE_SLOT_SIZE?pt:pt+1)] ,
		//
		DataMap_PMTag2[PM1+0] , DataMap_PMTag2[PM1+1] ,
		DataMap_PMTag2[PM1+2] , DataMap_PMTag2[PM1+3] ,
		DataMap_PMTag2[PM1+4] , DataMap_PMTag2[PM1+5] ,
		DataMap_PMTag2[PM1+6] , DataMap_PMTag2[PM1+7] ,
		DataMap_PMTag2[PM1+8] , DataMap_PMTag2[PM1+9] ,
		DataMap_PMTag2[PM1+10] , DataMap_PMTag2[PM1+11] ,
		DataMap_PMTag2[PM1+12] , DataMap_PMTag2[PM1+13] ,
		//
		DataMap_PMRemainTime[PM1+0] , DataMap_PMRemainTime[PM1+1] ,
		DataMap_PMRemainTime[PM1+2] , DataMap_PMRemainTime[PM1+3] ,
		DataMap_PMRemainTime[PM1+4] , DataMap_PMRemainTime[PM1+5] ,
		DataMap_PMRemainTime[PM1+6] , DataMap_PMRemainTime[PM1+7] ,
		DataMap_PMRemainTime[PM1+8] , DataMap_PMRemainTime[PM1+9] ,
		DataMap_PMRemainTime[PM1+10] , DataMap_PMRemainTime[PM1+11] ,
		DataMap_PMRemainTime[PM1+12] , DataMap_PMRemainTime[PM1+13] ,
		//
		DataMap_PMSupplyTime[PM1+0] , DataMap_PMSupplyTime[PM1+1] ,
		DataMap_PMSupplyTime[PM1+2] , DataMap_PMSupplyTime[PM1+3] ,
		DataMap_PMSupplyTime[PM1+4] , DataMap_PMSupplyTime[PM1+5] ,
		DataMap_PMSupplyTime[PM1+6] , DataMap_PMSupplyTime[PM1+7] ,
		DataMap_PMSupplyTime[PM1+8] , DataMap_PMSupplyTime[PM1+9] ,
		DataMap_PMSupplyTime[PM1+10] , DataMap_PMSupplyTime[PM1+11] ,
		DataMap_PMSupplyTime[PM1+12] , DataMap_PMSupplyTime[PM1+13]
		//
		);
	//----------------------------------------------------------------------
	if ( m == -1 ) {
		//
		// 2018.11.23
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			if ( pt < ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
				m = _SCH_CLUSTER_Get_PathProcessChamber( side, pt , 0 , k );
			}
			else {
				if ( DataMap_PMReqMode[k] != MODE_0_UNKNOWN ) { // 2018.02.10
					m = k;
				}
				else {
					m = -1;
				}
			}
			//
//			if ( m > 0 ) { // 2019.01.03
			if ( PM_ENABLE_FOR_MULTI_TM_PLACE( m ) ) { // 2019.01.03
				//
				if ( pt < ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
					//
					if ( DataMap_PMReqMode[m] == MODE_1_NORMAL_SUPPLY ) { // 2018.02.12
						//
						if ( DataMap_PMSide[m] == side ) { // 2017.10.22
							//
							if ( DataHasPair_LotPreCheck( side , pt , DataMap_LotPreUse[m] , DataMap_LotPreList[m] ) ) {

								continue;

							}
							//
						}
					}
					//
				}
				//
				if ( DataMap_PMTag2[m] == -4 ) continue; // 2019.03.05
				if ( DataMap_PMTag2[m] == -6 ) continue; // 2019.02.01
				//
				if ( !nowaycheck ) {
					//
					// 2017.10.22
					//
					if ( DataMap_PMReqMode[m] == MODE_1_NORMAL_SUPPLY ) { // 2018.02.10
						//
						if ( !_SCH_MACRO_HAS_PRE_PROCESS_OPERATION( side , pt , 400 + m ) ) { // 2017.09.19
							//
							rs = LOT_PRE_IF_DIFFER_LOT( side , pt , m );
							_SCH_LOG_LOT_PRINTF( side , "USER_FLOW_CHECK_PICK_FROM_FM [rs :%d]\n",rs);
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM [rs :%d]\n",rs);
							//
							if ( 0 == rs ) {
								//
								if ( HAS_DUMMY_WAFER_CHAMBER() ) { // 2019.01.29
									//
									Dummy_Set( m , TRUE );
									//
									_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY1 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
									//
									IO_PM_REMAIN_SET( m , 3 );
									//
									return -9999;
									//
								}
								else { // 2019.01.29
									//
									_SCH_PREPRCS_FirstRunPre_Set_PathProcessData( m , 2 );
									//
									_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY1-1 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
									//
									IO_PM_REMAIN_SET( m , 3 );
									//
									return -9999;
									//
								}
								//
							}
							else {
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY1-2 [side=%d,pt=%d][m=%d] [%d]\n" , side , pt , m , rs );
							}
							//
						}
						else {
							//
							if ( HAS_DUMMY_WAFER_CHAMBER() ) { // 2019.01.29
								//
								_SCH_PREPRCS_FirstRunPre_Done_PathProcessData( side , m );
								//
								Dummy_Set( m , TRUE );
								//
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY2 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
								//
								IO_PM_REMAIN_SET( m , 3 );
								//
								return -9999;
								//
							}
							else {
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY2-2 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
							//	printf("LotPre 강제 설정 - 0\n");
								//Wafer 유무 확인 
								//if(HAS_PM_WAFER_STATUS(m)) {
								if ( ( DataMap_PMGoCount[m] + DataMap_PMGoCount2[m] + DataMap_PMInCount[m] ) > 0 ) {
								//	printf("LotPre 강제 설정 - 01\n");
									return -16;
								}
								else
								{
									//LotPre 강제 설정. 
									//printf("LotPre 강제 설정 - 1\n");
									//strcpy( recipe , _SCH_CLUSTER_Get_PathProcessRecipe2( side , pt , 0 , m , SCHEDULER_PROCESS_RECIPE_IN ) );
									//strcpy( recipe2 , _SCH_CLUSTER_Get_PathProcessRecipe2( side , pt , 0 , m , SCHEDULER_PROCESS_RECIPE_OUT ) );
									strcpy( recipe , _SCH_PREPOST_Get_inside_READY_RECIPE_NAME( side , m ));
									//printf("LotPre 강제 설정 - 2[%s][%s]\n",recipe,recipe2);
									//Res = _SCH_MACRO_LOTPREPOST_PROCESS_OPERATION(side , m, side,  -1, -1, recipe, 0,0, "", 0, 2, 0); 
									
									GET_CUR_RECIPE(side,recipe2 );
									
									//_SCH_LOG_LOT_PRINTF( side  ,"LotPre 강제 설정 - 2[%s][%s]\n",recipe,recipe2);
									
									if ( recipe2[0] == '"' ) 
									{
										STR_SEPERATE_CHAR( recipe2 + 1 , '"' , recipe2_s, recipe2_s2 , 512 );
										sprintf(recipe2,"%s",recipe2_s);
										//_SCH_LOG_LOT_PRINTF( side  ,"LotPre 강제 설정 - 22[%s][%s][%s]\n",recipe2,recipe2_s,recipe2_s2);
									}

									if(strcmp(recipe,recipe2)!=0)
									{
										sprintf(recipe,"%s",recipe2);
									}

									Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , m , _SCH_CLUSTER_Get_SlotIn( side , pt ) , recipe ,11 , 0 );
									//printf("Change Recipe  Res : %d [%s]\n",Res,recipe);
								//	_SCH_LOG_LOT_PRINTF( side  , "[SlotIn :%d]_SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING[%s]\n" , _SCH_CLUSTER_Get_SlotIn( side , pt ) ,recipe );
									
									//Process 중이면 LotPre 강제 설정 전 우선 Pick을 막기 위해 Process가 끝날 때 까지 지연.  
									if( _SCH_MACRO_CHECK_PROCESSING(m) == 1)  
									{  
										//_SCH_LOG_LOT_PRINTF( side  ,"LotPre 강제 설정 - 2C[Process 중 ][%d]\n",_SCH_MACRO_CHECK_PROCESSING( m) );
										return -26;
									}

									//Res = _SCH_MACRO_LOTPREPOST_PROCESS_OPERATION(side , m, side+1,  0, 0, recipe, 11,0, "", 1, 2, 0); 
									Res = _SCH_MACRO_LOTPREPOST_PROCESS_OPERATION(side , m, side+1,  0, 0, recipe, 99,0, "", 1, 2, 0); 

									//printf("LotPre 강제 설정 - 3[%d][%s]\n",Res,recipe);
									//Lotpre Cancel 
									_SCH_PREPRCS_FirstRunPre_Done_PathProcessData( side , m );
									//printf("LotPre 강제 설정 - 4[%s]\n",recipe);
									_SCH_LOG_LOT_PRINTF( side  , "_SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING1[%s]\n" , recipe );
									//LAST_SUPPLY 값 설정.
									//printf("LotPre 강제 설정 - 5[%s]\n",recipe);
									LotPre_Last_Supply(side , pt , m ,FALSE);
									return -17;
								}
							}
							//
						}
						//
					}
					//
					// 2018.02.10
					//
					if      ( DataMap_PMReqMode[m] == MODE_3_DUMMY_REQUEST_FROM_NORMAL ) { // appdummy
						//
						if ( Dummy_Add( FALSE , nowaycheck , m , DataMap_PMSide[m] , DataMap_PMPointer[m] , &rpt ) ) {
							//
							if ( rpt >= 0 ) _SCH_MODULE_Set_FM_DoPointer( DataMap_PMSide[m] , rpt );
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-101 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -14; // 2018.02.12
							//
						}
						else {
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-102 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -4;
							//
						}
					}
					else if ( DataMap_PMReqMode[m] == MODE_4_DUMMY_REQUEST_FROM_END ) { // appdummy/blank
						//
						if ( Dummy_Add( TRUE , nowaycheck , m , side , pt , &rpt ) ) {
							//
							if ( rpt >= 0 ) _SCH_MODULE_Set_FM_DoPointer( side , rpt );
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-103 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -15; // 2018.02.12
							//
						}
						else {
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-104 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -5;
							//
						}
						//
					}
					else {
						if ( pt >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-105 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -25;
							//
						}
					}
					//
					if ( !nowaycheck ) { // 2018.12.07
						//
						Dummy_Regist( side , pt , ( MAX_CASSETTE_SLOT_SIZE - 1 ) , m );
						//
						IO_PM_SELECT_SET( m );
						//
						USER_SELECT_PM[side][pt] = m;
						//
					}
					//
				}
				else {
					//
					// 2019.06.05
					//
					if ( MULTI_PM_MODE ) {
						//
						if ( DataMap_PMReqMode[m] == MODE_1_NORMAL_SUPPLY ) {
							//
							if ( _SCH_MACRO_HAS_PRE_PROCESS_OPERATION( side , pt , 400 + m ) ) {
								//
								if ( !HAS_DUMMY_WAFER_CHAMBER() ) {
									//
									_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY2-2-2 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
									//
									IO_PM_REMAIN_SET( m , 3 );
									//
									return -27;
									//
								}
								//
							}
							//
						}
						//
					}
					//
					//
					// 2018.02.10
					//
					if      ( DataMap_PMReqMode[m] == MODE_3_DUMMY_REQUEST_FROM_NORMAL ) { // appdummy
						//
						if ( !Dummy_Add( FALSE , nowaycheck , m , DataMap_PMSide[m] , DataMap_PMPointer[m] , &rpt ) ) {
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-201 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -4;
							//
						}
					}
					else if ( DataMap_PMReqMode[m] == MODE_4_DUMMY_REQUEST_FROM_END ) { // appdummy/blank
						//
						if ( !Dummy_Add( TRUE , nowaycheck , m , side , pt , &rpt ) ) {
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-202 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -5;
							//
						}
						//
					}
					else {
						if ( pt >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-203 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
							//
							return -25;
							//
						}
					}
					//
					//
					//

				}
				//
				_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SEL1 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
				//
				return 2;
			}
			//
		}
		//
		return -1;
	}
	//
	if ( m == -2 ) return -2;
	//
	if ( DataMap_PMSide[m] == side ) {
		//
//		if ( DataHasPair_LotPreCheck( side, pt , DataMap_LotPreUse[m] , DataMap_LotPreList[m] ) ) { // 2017.10.22
		if ( ( pt < ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) && DataHasPair_LotPreCheck( side, pt , DataMap_LotPreUse[m] , DataMap_LotPreList[m] ) ) { // 2018.02.12
			//
			_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SEL2X [side=%d,pt=%d][m=%d]\n" , side , pt , m );
			//
			return -3;
			//

		}
		else {
			//
			_SCH_LOG_DEBUG_PRINTF( side , 0 , "[NW: %d]USER_FLOW_CHECK_PICK_FROM_FM FORCE_CHECK [side=%d,pt=%d][m=%d][PMreqMode : %d]\n" , nowaycheck,side , pt , m ,DataMap_PMReqMode[m] );
			if ( !nowaycheck ) {
				//
				// 2017.10.22
				//
				if ( DataMap_PMReqMode[m] == MODE_1_NORMAL_SUPPLY ) { // 2018.02.10
					//
					if ( !_SCH_MACRO_HAS_PRE_PROCESS_OPERATION( side , pt , 400 + m ) ) { // 2017.09.19
						//
						rs = LOT_PRE_IF_DIFFER_LOT( side , pt , m );
						_SCH_LOG_LOT_PRINTF( side , "USER_FLOW_CHECK_PICK_FROM_FM [rs :%d]\n",rs);
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM2 [rs :%d]\n",rs);
						//
						if ( 0 == rs ) {
							//
							if ( HAS_DUMMY_WAFER_CHAMBER() ) { // 2019.01.29
								//
								Dummy_Set( m , TRUE );
								//
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY3 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
								//
								IO_PM_REMAIN_SET( m , 3 );
								//
								return -9999;
								//
							}
							else { // 2019.01.29
								//
								_SCH_PREPRCS_FirstRunPre_Set_PathProcessData( m , 2 );
								//
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY3-1 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
								//
								IO_PM_REMAIN_SET( m , 3 );
								//
								return -9999;
								//
							}
							//
						}
						else {
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY3-2 [side=%d,pt=%d][m=%d] [%d]\n" , side , pt , m , rs );
						}
						//
					}
					else {
						//
						if ( HAS_DUMMY_WAFER_CHAMBER() ) { // 2019.01.29
							//
							_SCH_PREPRCS_FirstRunPre_Done_PathProcessData( side , m );
							//
							Dummy_Set( m , TRUE );
							//
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY4 [side=%d,pt=%d][m=%d]]\n" , side , pt , m );
							//
							IO_PM_REMAIN_SET( m , 3 );
							//
							return -9999;
							//
						}
						else {
							_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY4-2 [side=%d,pt=%d][m=%d]\n" , side , pt , m );

							//COde 추가 
							//printf("LotPre 강제 설정 - 0A\n");
							//Wafer 유무 확인 
							//if(HAS_PM_WAFER_STATUS(m)) {
							if ( ( DataMap_PMGoCount[m] + DataMap_PMGoCount2[m] + DataMap_PMInCount[m] ) > 0 ) {
							//	printf("LotPre 강제 설정 - 01A\n");
								return -16;
							}
							else
							{
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY4-3 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
								//LotPre 강제 설정. 
								//printf("LotPre 강제 설정 - 1A\n");
								strcpy( recipe , _SCH_PREPOST_Get_inside_READY_RECIPE_NAME( side , m ));
								strcpy( recipe2 , _SCH_CLUSTER_Get_PathProcessRecipe2( side , pt , 0 , m , SCHEDULER_PROCESS_RECIPE_OUT ) );
								//printf("LotPre 강제 설정 - 2A[%s][%s]\n",recipe,recipe2);
								//Res = _SCH_MACRO_LOTPREPOST_PROCESS_OPERATION(side , m, side,  -1, -1, recipe, 0,0, "", 0, 2, 0);
								
								GET_CUR_RECIPE(side,recipe2 );
									
								_SCH_LOG_LOT_PRINTF( side  ,"LotPre 강제 설정 - 2A[%s][%s]\n",recipe,recipe2);
								
								if ( recipe2[0] == '"' ) 
								{
									STR_SEPERATE_CHAR( recipe2 + 1 , '"' , recipe2_s, recipe2_s2 , 512 );
									sprintf(recipe2,"%s",recipe2_s);
									//_SCH_LOG_LOT_PRINTF( side  ,"LotPre 강제 설정 - 22[%s][%s][%s]\n",recipe2,recipe2_s,recipe2_s2);
								}

								if(strcmp(recipe,recipe2)!=0)
								{
									if(strlen(recipe2) > 0 )
									{
										sprintf(recipe,"%s",recipe2);
									}
								}

								Res = _SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING( side , pt , m , _SCH_CLUSTER_Get_SlotIn( side , pt ) , recipe ,11 , 0 );
								//printf("Change Recipe  ResA : %d [%s]\n",Res,recipe);
								//_SCH_LOG_LOT_PRINTF( side  , "[SlotIn :%d]_SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING[%s]\n" , _SCH_MACRO_CHECK_PROCESSING(m) ,recipe );
								//_SCH_LOG_LOT_PRINTF( side  ,"LotPre 강제 설정 - 2B[Process ?? ][%d][D : %d]\n", _SCH_MACRO_CHECK_PROCESSING(m) ,_SCH_MACRO_CHECK_PROCESSING_INFO( m) );
								//Process 중이면 LotPre 강제 설정 전 우선 Pick을 막기 위해 Process가 끝날 때 까지 지연.  
								if( _SCH_MACRO_CHECK_PROCESSING(m) == 1)  
								{  
									_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY4-4 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
									_SCH_LOG_LOT_PRINTF( side  ,"LotPre 강제 설정 - 2C[Process 중 ][%d]\n",_SCH_MACRO_CHECK_PROCESSING( m) );
									return -26;
								}
									
								_SCH_LOG_LOT_PRINTF( side  , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY4-5 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY4-5 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
								//Process 이상 
								//Res = _SCH_MACRO_LOTPREPOST_PROCESS_OPERATION(side , m, side+1,  0, 0, recipe, 11,0, "", 1, 2, 0); 
								Res = _SCH_MACRO_LOTPREPOST_PROCESS_OPERATION(side , m, side+1,  0, 0, recipe, 99,0, "", 1, 2, 0); 
								_SCH_LOG_LOT_PRINTF( side  , "[m:%d]_SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING2- %s]\n" , m,recipe );
								//printf("LotPre 강제 설정 - 3A[%d][%s][%s]\n",Res,recipe,recipe2);
								//Lotpre Cancel 
								_SCH_PREPRCS_FirstRunPre_Done_PathProcessData( side , m );
								//printf("LotPre 강제 설정 - 4A[%s]\n",recipe);
								_SCH_LOG_LOT_PRINTF( side  , "[m:%d]_SCH_SUB_COPY_LOCKING_FILE_WHILE_RUNNING2[%s]\n" ,m, recipe );
								//LAST_SUPPLY 값 설정.
								//printf("LotPre 강제 설정 - 5A[%s]\n",recipe);
								LotPre_Last_Supply(side , pt , m,FALSE );
								return -18;
							}


						}
						//
					}
					//
				}
				//
				//
				// 2018.02.10
				//
				if      ( DataMap_PMReqMode[m] == MODE_3_DUMMY_REQUEST_FROM_NORMAL ) { // appdummy
					//
					if ( Dummy_Add( FALSE , nowaycheck , m , DataMap_PMSide[m] , DataMap_PMPointer[m] , &rpt ) ) {
						//
						if ( rpt >= 0 ) _SCH_MODULE_Set_FM_DoPointer( DataMap_PMSide[m] , rpt );
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-301 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -14;
						//
					}
					else {
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-302 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -4;
						//
					}
					//
				}
				else if ( DataMap_PMReqMode[m] == MODE_4_DUMMY_REQUEST_FROM_END ) { // appdummy/blank
					//
					if ( Dummy_Add( TRUE , nowaycheck , m , side , pt , &rpt ) ) {
						//
						if ( rpt >= 0 ) _SCH_MODULE_Set_FM_DoPointer( side , rpt );
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-303 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -15;
						//
					}
					else {
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-304 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -5;
						//
					}
					//
				}
				else {
					//
					if ( pt >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-305 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -25;
						//
					}
					//
				}
				//
				Dummy_Regist( side , pt , ( MAX_CASSETTE_SLOT_SIZE - 1 ) , m );
				//
				IO_PM_SELECT_SET( m );
				//
				USER_SELECT_PM[side][pt] = m;
				//
			}
			else {
				//
				//
				// 2019.06.05
				//
				//
				if ( MULTI_PM_MODE ) {
					//
					if ( DataMap_PMReqMode[m] == MODE_1_NORMAL_SUPPLY ) {
						//
						if ( _SCH_MACRO_HAS_PRE_PROCESS_OPERATION( side , pt , 400 + m ) ) {
							//
							if ( !HAS_DUMMY_WAFER_CHAMBER() ) {
								//
								_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM FORCE_DUMMY4-5-2 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
								//
								IO_PM_REMAIN_SET( m , 3 );
								//
								return -28;
								//
							}
							//
						}
						//
					}
					//
				}
				//
				//
				//
				// 2018.02.10
				//
				if      ( DataMap_PMReqMode[m] == MODE_3_DUMMY_REQUEST_FROM_NORMAL ) { // appdummy
					//
					if ( !Dummy_Add( FALSE , nowaycheck , m , DataMap_PMSide[m] , DataMap_PMPointer[m] , &rpt ) ) {
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-401 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -4;
						//
					}
					//
				}
				else if ( DataMap_PMReqMode[m] == MODE_4_DUMMY_REQUEST_FROM_END ) { // appdummy/blank
					//
					if ( !Dummy_Add( TRUE , nowaycheck , m , side , pt , &rpt ) ) {
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-402 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -5;
						//
					}
					//
				}
				else {
					//
					if ( pt >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) { // 2018.02.12
						//
						IO_PM_REMAIN_SET( m , 3 );
						//
						_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SELX-403 [side=%d,pt=%d][m=%d][%d]\n" , side , pt , m , rpt );
						//
						return -25;
						//
					}
					//
				}
				//
				//
			}
			//
		}
		//
		_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_CHECK_PICK_FROM_FM SEL2 [side=%d,pt=%d][m=%d]\n" , side , pt , m );
		//
		return 3;
	}

	return 0;
}



void Parallel_Set_Enable( int s , int p , int ch ) { // 2017.10.23
	int selid , j , k , m , x , pr;
	//
	if ( _SCH_CLUSTER_Get_PathProcessParallel( s , p , 0 ) == NULL ) return;
	//
	selid = _SCH_CLUSTER_Get_PathProcessParallel2( s , p , 0 , ( ch - PM1 ) );
	//
	if ( selid <= 1 ) return;
	//
	pr = _SCH_CLUSTER_Get_PathRange( s , p );
	//
	if ( pr <= 0 ) return;
	//
	for ( j = 1 ; j < pr ; j++ ) {
		//
		for ( k = 0 ; k < MAX_PM_CHAMBER_DEPTH ; k++ ) {
			//
			m = _SCH_CLUSTER_Get_PathProcessChamber( s , p , j , k );
			//
			if ( m < 0 ) {
				//
				if ( _SCH_MODULE_Get_Mdl_Use_Flag( s , -m ) == MUF_98_USE_to_DISABLE_RAND ) {
					//
					if ( _SCH_CLUSTER_Get_PathProcessParallel( s , p , j ) != NULL ) {
						//
						x = _SCH_CLUSTER_Get_PathProcessParallel2( s , p , j , ( -m - PM1 ) );
						//
						if ( x == selid ) {
							//
							_SCH_CLUSTER_Set_PathProcessChamber_Enable( s , p , j , k );
							//
						}
					}
				}
				//
			}
		}
	}
	//
}
int  USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM_Sub( int side1 , int pt1 , int slot1 , int side2 , int pt2 , int slot2 ) {
	int g , pm;
	//
	g = -1;
	//
	if ( MULTI_PM_MODE ) { // 2018.05.08
		//
		if ( ( _SCH_MODULE_Get_FM_WAFER( 0 ) <= 0 ) && ( _SCH_MODULE_Get_FM_WAFER( 1 ) <= 0 ) ) {
			if ( slot1 > 0 ) {
				//
				//-----------------------------------------------------------------------------------------------------
				//
				pm = USER_SELECT_PM[side1][pt1];
				//
				if ( pm <= 0 ) {
					pm = MULTI_PM_LAST;
				}
				else { // 2018.10.20
					MULTI_PM_COUNT = 0;
				}
				//
				//_SCH_LOG_LOT_PRINTF( side1 , "_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable1][side  : %d][pt : %d][pm : %d]\n",side1,pt1,pm);

				//Pathsts ->Supply 
				_SCH_CLUSTER_Set_PathStatus( side1 , pt1,SCHEDULER_SUPPLY );

				_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable( side1 , pt1 , 0 , pm );

				//
				g = pm + 100;
				//
				//-----------------------------------------------------------------------------------------------------
				//
				IO_PM_REMAIN_SET( pm , 5 ); // 2017.04.17
				//
				//-----------------------------------------------------------------------------------------------------
				//
				if ( 2 == IO_CONTROL_USE() ) { // 2017.10.23
					if ( _SCH_CLUSTER_Get_PathRange( side1 , pt1 ) >= 2 ) { // 2018.07.04
						Parallel_Set_Enable( side1 , pt1 , pm ); // 2017.10.23
					}
				}
				//
				//-----------------------------------------------------------------------------------------------------
				//
				_SCH_LOG_LOT_PRINTF( side1  , "LotPre_Last_Supply--1\n");
				LotPre_Last_Supply( side1 , pt1 , pm ,TRUE); //2020.02.27
				//LotPre_Last_Supply( side1 , pt1 , pm ,FALSE);
				//
				//-----------------------------------------------------------------------------------------------------
				//
				_SCH_LOG_DEBUG_PRINTF( side1 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 11 [%d][side=%d,pt=%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , USER_SELECT_PM[side1][pt1] , side1 , pt1 , slot1 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
				//
				if ( Multi_is_Checking_for_Supply_Wafer( side1 , pt1 , pm , &MULTI_PM_LAST , &MULTI_PM_CLIDX , &MULTI_PM_COUNT , MULTI_PM_MAP ) ) {
					_SCH_LOG_DEBUG_PRINTF( side1 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 12 [%d][side=%d,pt=%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , USER_SELECT_PM[side1][pt1] , side1 , pt1 , slot1 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
				}
				else {
					_SCH_LOG_DEBUG_PRINTF( side1 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 13 [%d][side=%d,pt=%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , USER_SELECT_PM[side1][pt1] , side1 , pt1 , slot1 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
					//
					return 2;
					//
				}
				//
				//-----------------------------------------------------------------------------------------------------
				//
			}
			//
			if ( slot2 > 0 ) {
				//
				//-----------------------------------------------------------------------------------------------------
				if ( slot1 <= 0 ) {
					//
					pm = USER_SELECT_PM[side2][pt2];
					//
					if ( pm <= 0 ) {
						pm = MULTI_PM_LAST;
					}
					else { // 2018.10.20
						MULTI_PM_COUNT = 0;
					}
				}
				else { // 2019.03.06
					if ( _SCH_CLUSTER_Get_ClusterIndex( side1 , pt1 ) != _SCH_CLUSTER_Get_ClusterIndex( side2 , pt2 ) ) {
						_SCH_LOG_DEBUG_PRINTF( side2 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 41 [%d][side=%d,pt=%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , USER_SELECT_PM[side1][pt1] , side1 , pt1 , slot1 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
						return 2;
					}
				}
				//
				//
				//_SCH_LOG_LOT_PRINTF( side2 , "_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable2]\n");
				//Pathsts ->Supply 
				_SCH_CLUSTER_Set_PathStatus( side2 , pt2,SCHEDULER_SUPPLY );
				_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable( side2 , pt2 , 0 , pm );
				//
				g = pm + 200;
				//
				//-----------------------------------------------------------------------------------------------------
				//
				IO_PM_REMAIN_SET( pm , 5 ); // 2017.04.17
				//
				//-----------------------------------------------------------------------------------------------------
				//
				if ( 2 == IO_CONTROL_USE() ) { // 2017.10.23
					if ( _SCH_CLUSTER_Get_PathRange( side2 , pt2 ) >= 2 ) { // 2018.07.04
						Parallel_Set_Enable( side2 , pt2 , pm ); // 2017.10.23
					}
				}
				//
				//-----------------------------------------------------------------------------------------------------
				//
				_SCH_LOG_LOT_PRINTF( side2  , "LotPre_Last_Supply--2\n");
				LotPre_Last_Supply( side2 , pt2 , pm ,TRUE); //2020.02.27
				//LotPre_Last_Supply( side2 , pt2 , pm ,FALSE); 
				//
				//-----------------------------------------------------------------------------------------------------
				//
				_SCH_LOG_DEBUG_PRINTF( side1 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 21 [%d][side=%d,pt=%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , USER_SELECT_PM[side2][pt2] , side2 , pt2 , slot2 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
				//
				Multi_is_Checking_for_Supply_Wafer( side2 , pt2 , pm , &MULTI_PM_LAST , &MULTI_PM_CLIDX , &MULTI_PM_COUNT , MULTI_PM_MAP );
				//
				_SCH_LOG_DEBUG_PRINTF( side1 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 22 [%d][side=%d,pt=%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , USER_SELECT_PM[side2][pt2] , side2 , pt2 , slot2 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
				//
				//-----------------------------------------------------------------------------------------------------
				//
			}
		}
		else {
			//
			_SCH_LOG_DEBUG_PRINTF( side1 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 31 [%d,%d,%d/%d,%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , side1 , pt1 , slot1 , side2 , pt2 , slot2 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
			//
			Multi_is_Checking_for_Supply_Wafer2( ( slot1 > 0 ) ? side1 : side2 , ( slot1 > 0 ) ? pt1 : pt2 , &MULTI_PM_LAST , &MULTI_PM_CLIDX , &MULTI_PM_COUNT , MULTI_PM_MAP );
			//
			_SCH_LOG_DEBUG_PRINTF( side1 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM 32 [%d,%d,%d/%d,%d,%d][%d,%d,%d/%d,%d,%d,%d]\n" , side1 , pt1 , slot1 , side2 , pt2 , slot2 , MULTI_PM_LAST , MULTI_PM_CLIDX , MULTI_PM_COUNT , MULTI_PM_MAP[0] , MULTI_PM_MAP[1] , MULTI_PM_MAP[2] , MULTI_PM_MAP[3] );
			//
		}
		//

	}
	else {
		//
		if ( ( _SCH_MODULE_Get_FM_WAFER( 0 ) <= 0 ) && ( _SCH_MODULE_Get_FM_WAFER( 1 ) <= 0 ) ) {
			if ( slot1 > 0 ) {
				//
				//-----------------------------------------------------------------------------------------------------
				//
				pm = USER_SELECT_PM[side1][pt1];
				//
				//Pathsts ->Supply 
				_SCH_CLUSTER_Set_PathStatus( side1 , pt1,SCHEDULER_SUPPLY );
				//_SCH_LOG_LOT_PRINTF( side1 , "_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable3]\n");
				_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable( side1 , pt1 , 0 , pm );
				
				//
				g = pm + 100;
				//
				//-----------------------------------------------------------------------------------------------------
				//
				IO_PM_REMAIN_SET( pm , 5 ); // 2017.04.17
				//
				//-----------------------------------------------------------------------------------------------------
				//
				if ( 2 == IO_CONTROL_USE() ) { // 2017.10.23
					if ( _SCH_CLUSTER_Get_PathRange( side1 , pt1 ) >= 2 ) { // 2018.07.04
						Parallel_Set_Enable( side1 , pt1 , pm ); // 2017.10.23
					}
				}
				//
				//-----------------------------------------------------------------------------------------------------
				//
				_SCH_LOG_LOT_PRINTF( side1  , "LotPre_Last_Supply--3\n");
				LotPre_Last_Supply( side1 , pt1 , pm ,TRUE);//2020.02.27
				//LotPre_Last_Supply( side1 , pt1 , pm ,FALSE);
				//
				//-----------------------------------------------------------------------------------------------------
				//
				MULTI_PM_LAST = pm; // 2018.05.08
				//
				//-----------------------------------------------------------------------------------------------------
				//
				// 2019.03.21
				//
				if ( slot2 > 0 ) {
					//
					if ( pt2 >= ( MAX_CASSETTE_SLOT_SIZE - 1 ) ) {
						return 2;
					}
					//
				}
				//
			}
			else {
				if ( slot2 > 0 ) {
					//
					//-----------------------------------------------------------------------------------------------------
					//
					pm = USER_SELECT_PM[side2][pt2];
					//
					//_SCH_LOG_LOT_PRINTF( side1 , "_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable4]\n");
					//Pathsts ->Supply 
					_SCH_CLUSTER_Set_PathStatus( side2 , pt2,SCHEDULER_SUPPLY );
					_SCH_CLUSTER_Set_PathProcessChamber_Select_Other_Disable( side2 , pt2 , 0 , pm );

					//
					g = pm + 200;
					//
					//-----------------------------------------------------------------------------------------------------
					//
					IO_PM_REMAIN_SET( pm , 5 ); // 2017.04.17
					//
					//-----------------------------------------------------------------------------------------------------
					//
					if ( 2 == IO_CONTROL_USE() ) { // 2017.10.23
						if ( _SCH_CLUSTER_Get_PathRange( side2 , pt2 ) >= 2 ) { // 2018.07.04
							Parallel_Set_Enable( side2 , pt2 , pm ); // 2017.10.23
						}
					}
					//
					//-----------------------------------------------------------------------------------------------------
					//
					_SCH_LOG_LOT_PRINTF( side2  , "LotPre_Last_Supply--4\n");
					LotPre_Last_Supply( side2 , pt2 , pm ,TRUE); //2020.02.27
					//LotPre_Last_Supply( side2 , pt2 , pm ,FALSE); 
					//
					//-----------------------------------------------------------------------------------------------------
					//
					MULTI_PM_LAST = pm; // 2018.05.08
					//
					//-----------------------------------------------------------------------------------------------------
					//
				}
				else {
					g = 0;
				}
			}
		}
	}
	//
	//_SCH_LOG_LOT_PRINTF( side1 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM PM%d [%d]\n" , pm - PM1 + 1,pm );

	if(GET_DISABLE_PM_STATUS(pm) )
	{
		SET_DISABLE_PM_STATUS( side1,pm,FALSE);
	}
	//if(GET_DISABLE_PM_STATUS_EVENT(pm))
	//{
	//	SET_DISABLE_PM_STATUS_EVENT(pm,FALSE); 
	//}

	//
	_SCH_LOG_DEBUG_PRINTF( ( slot1 > 0 ) ? side1 : side2 , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM RUN [s=%d,p=%d,w=%d][s=%d,p=%d,w=%d][F=%d,%d][%d]\n" , side1 , pt1 , slot1 , side2 , pt2 , slot2 , _SCH_MODULE_Get_FM_WAFER( 0 ) , _SCH_MODULE_Get_FM_WAFER( 1 ) , g );
	//
	return 1;
}


int  USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM( int side1 , int pt1 , int slot1 , int side2 , int pt2 , int slot2 ) {

	int Res;


	_SCH_SYSTEM_ENTER_MDL_CRITICAL();

	Res =  USER_FLOW_NOTIFICATION_FOR_PICK_FROM_CM_Sub(  side1 ,  pt1 ,  slot1 ,  side2 ,  pt2 ,  slot2 );


	_SCH_SYSTEM_LEAVE_MDL_CRITICAL();


	return Res;
}

int  USER_FLOW_NOTIFICATION_FOR_PICK_FROM_BM( int side , int pt , int ch ) { // 2017.01.25
	int j , g1 , g2 , s1 , p1 , s2 , p2 , m1 , m2;
	//
	if ( _SCH_PRM_GET_DFIX_TM_DOUBLE_CONTROL() <= 0 ) return TRUE;
	//
	g1 = 0;
	g2 = 0;
	//
	for ( j = 1 ; j <= _SCH_PRM_GET_MODULE_SIZE( ch ) ; j++ ) {
		//
		if ( ( _SCH_MODULE_Get_BM_WAFER( ch , j ) > 0 ) && ( _SCH_MODULE_Get_BM_WAFER( ch , j+1 ) > 0 ) ) {
			//
			s1 = _SCH_MODULE_Get_BM_SIDE( ch , j );
			p1 = _SCH_MODULE_Get_BM_POINTER( ch , j );
			//
			s2 = _SCH_MODULE_Get_BM_SIDE( ch , j+1 );
			p2 = _SCH_MODULE_Get_BM_POINTER( ch , j+1 );
			//
			m1 = Check_1_PM( s1 , p1 );
			m2 = Check_1_PM( s2 , p2 );
			//
			if ( m1 > 0 ) {
				if ( m1 != m2 ) {
					Setting_1_PM( s2 , p2 , m1 );
					g1 = ( g1 * 100 ) + j;
					g2 = ( g2 * 100 ) + m1;
				}
			}
			else {
				if ( m2 > 0 ) {
					Setting_1_PM( s1 , p1 , m2 );
					g1 = ( g1 * 100 ) + (j+1);
					g2 = ( g2 * 100 ) + m2;
				}
			}
			//
		}
		//
		j++;
		//
	}
	//
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_NOTIFICATION_FOR_PICK_FROM_BM RUN [s=%d,p=%d,ch=%d][%d,%d]\n" , side , pt , ch , g1 , g2 );
	//
	return TRUE;
}

int  USER_FLOW_NOTIFICATION_FOR_PLACE_TO_BM( int side , int pt , int ch ) { // 2017.02.01
	int j , g1 , g2 , s1 , p1 , s2 , p2 , m1 , m2;
	//
	if ( _SCH_PRM_GET_DFIX_TM_DOUBLE_CONTROL() <= 0 ) return TRUE;
	//
	g1 = 0;
	g2 = 0;
	//
	for ( j = 1 ; j <= _SCH_PRM_GET_MODULE_SIZE( ch ) ; j++ ) {
		//
		if ( ( _SCH_MODULE_Get_BM_WAFER( ch , j ) > 0 ) && ( _SCH_MODULE_Get_BM_WAFER( ch , j+1 ) > 0 ) ) {
			//
			s1 = _SCH_MODULE_Get_BM_SIDE( ch , j );
			p1 = _SCH_MODULE_Get_BM_POINTER( ch , j );
			//
			s2 = _SCH_MODULE_Get_BM_SIDE( ch , j+1 );
			p2 = _SCH_MODULE_Get_BM_POINTER( ch , j+1 );
			//
			m1 = Check_1_PM( s1 , p1 );
			m2 = Check_1_PM( s2 , p2 );
			//
			if ( m1 > 0 ) {
				if ( m1 != m2 ) {
					Setting_1_PM( s2 , p2 , m1 );
					g1 = ( g1 * 100 ) + j;
					g2 = ( g2 * 100 ) + m1;
				}
			}
			else {
				if ( m2 > 0 ) {
					Setting_1_PM( s1 , p1 , m2 );
					g1 = ( g1 * 100 ) + (j+1);
					g2 = ( g2 * 100 ) + m2;
				}
			}
			//
		}
		//
		j++;
		//
	}
	//
	_SCH_LOG_DEBUG_PRINTF( side , 0 , "USER_FLOW_NOTIFICATION_FOR_PLACE_TO_BM RUN [s=%d,p=%d,ch=%d][%d,%d]\n" , side , pt , ch , g1 , g2 );
	//
	return TRUE;
}

