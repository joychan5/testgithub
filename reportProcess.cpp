#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <client-libraries/cpp/redisclient.h>
#include "./appClass/parseMCDBKeyValue.hpp"
#include "./baselib/mysqlClass.hpp"
#include "./baselib/parseIniFileClass.hpp"
using namespace redis;

map< string, int > getMCDBInfo( string _mcdbStr )
{
	string mcdbStr = replace( _mcdbStr, "\"", "" );
	map< string, int > retMap;
	string split1 = "\\t";
	vector<string> mcdbInfo = splitStr( mcdbStr, split1 );
	vector<string>::iterator it1;
	for ( it1 = mcdbInfo.begin(); it1 != mcdbInfo.end(); it1++ )
	{
		string hostPortStr = *it1;
		cout<<"l="<<__LINE__<<"host:"<<*it1<<endl;
		vector<string> mcdbHostPort = splitStr( hostPortStr, ":" );
		cout<<"l="<<__LINE__<<"host:"<<mcdbHostPort[ 0 ]<<":"<<mcdbHostPort[ 1 ]<<endl;
		retMap.insert( pair< string, int >( mcdbHostPort[ 0 ], strToInt( mcdbHostPort[ 1 ] ) ) );
 	}
	return retMap;
}
time_t getHourTime( time_t tm )
{
	return  tm - ( tm%3600 );
	//return  tm - ( tm%3600 );
}
int main( ) 
{

	parseMCDBKeyValue parseMCDBObj;
	parseIniFileClass parseIniObj;
	mysqlClass	mysqlOBJ;

	parseIniObj.init( "./conf/sws.ini" );
	client redisOBJ( parseIniObj.getItemValue( "redisGather", "host" ), strToInt( parseIniObj.getItemValue( "redisGather", "port" ) ) );

	parseIniObj.initLogPath( parseIniObj.getItemValue( "log", "errLog" ) );
	if ( !mysqlOBJ.addConnect(	parseIniObj.getItemValue( "mysql", "billingDBHost" ).c_str(),
							parseIniObj.getItemValue( "mysql", "billingDBUser" ).c_str(),
							parseIniObj.getItemValue( "mysql", "billingDBPwd" ).c_str(),
							parseIniObj.getItemValue( "mysql", "billingDBName" ).c_str(),
							strToInt( parseIniObj.getItemValue( "mysql", "billingDBPort" ) )
						) )
	{
		parseIniObj.writeLog( LOG_ERR_SELF, "mysql", "connect mysql failed!("+parseIniObj.getItemValue( "mysql", "billingDBHost" )+")", __FILE__ );
		return 0;
	}

	parseIniObj.writeLog( LOG_ALERT_SELF, "mysql", "add mysql server success!("+parseIniObj.getItemValue( "mysql", "billingDBHost" )+")", __FILE__ );
	parseMCDBObj.init( &parseIniObj );
	
	//bool mysqlClass::addConnect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port,
	//							   const char *unix_socket, unsigned long client_flag) 
	vector<string> redisVec;
	vector<string> delKeyList;
	map<string,string> redisData;
	while ( 1 )//5w分钟扫一次redis
	{

		time_t tempTime;
		time( &tempTime );
		string regex;
		regex = "*_0";
		redisVec.clear();
		if ( redisOBJ.keys( regex, redisVec ) != 0 )
		{

			for ( unsigned int i = 0; i < redisVec.size(); i++ )
			{
				string key = redisVec[ i ];
				redisData.clear();
				string primaryKey = key, redisKey;
				primaryKey = primaryKey.substr( 0, primaryKey.size() - 1 );
				string value=primaryKey;

				for ( unsigned int j = 0; j < 6 ; j++ )
				{
					redisKey = primaryKey + itos( j );

					if ( j == 0 )
						value = value + redisOBJ.get( redisKey );
					else
					{
						
						value = value + "_";
						value = value + redisOBJ.get( redisKey );
					}
				}
				//cout<<"key("<<primaryKey<<")value("<<value<<")"<<endl;
				redisData.insert( pair< string, string >( primaryKey, value ) );
				parseMCDBObj.parseMCDBData( redisData );

				string sqlStr = parseMCDBObj.getSqlString();
				//cout<<"sqlStr="<<sqlStr<<endl;
				if ( sqlStr.size() == 0 )
				{
					vector<string> deleteMCDBKeyList = parseMCDBObj.getDeleteMCDBList();
					vector<string>::iterator it2;
					for ( it2 = deleteMCDBKeyList.begin(); it2 != deleteMCDBKeyList.end(); it2++ )
					{
							string keyTmp = *it2;
							try
							{
								redisOBJ.del( keyTmp );
								
							}
							catch ( redis_error & e )
							{
								parseIniObj.writeLog( LOG_ERR_SELF, "redis", "redis delete failed!key not exists!(key="+keyTmp+")error("+string(e)+")", __FILE__ );
							}

							parseIniObj.writeLog( LOG_ALERT_SELF, "mysql", "sqlStr is empty,mcdb delete success!(key="+keyTmp+")", __FILE__ );
					}

				}
				else
				{
					if ( !mysqlOBJ.query( sqlStr ) )
					{
						parseIniObj.writeLog( LOG_ERR_SELF, "mysql", "connect query failed!("+sqlStr+")", __FILE__ );

					}
					else
					{
						vector<string> deleteMCDBKeyList = parseMCDBObj.getDeleteMCDBList();
						vector<string>::iterator it2;
						for ( it2 = deleteMCDBKeyList.begin(); it2 != deleteMCDBKeyList.end(); it2++ )
						{
							string keyTmp = *it2;
							try
							{
								redisOBJ.del( keyTmp );
								
							}
							catch ( redis_error & e )
							{
								parseIniObj.writeLog( LOG_ERR_SELF, "redis", "redis delete failed!key not exists!(key="+keyTmp+")error("+string(e)+")", __FILE__ );
							}

							parseIniObj.writeLog( LOG_ALERT_SELF, "mysql", "sqlStr run success!sqlStr("+sqlStr+"),mcdb delete success!(key="+keyTmp+")", __FILE__ );
						}
					}
				}
			}
			parseIniObj.writeLog( LOG_ALERT_SELF, "debug", "run over and sleep 300 minutes!l("+itos( __LINE__ )+")size("+itos( redisVec.size() )+")", __FILE__ );
			sleep( 300 );
		}
		else
		{
			parseIniObj.writeLog( LOG_ALERT_SELF, "debug", "redis is null and sleep 60 minutes!l("+itos( __LINE__ )+")size("+itos( redisVec.size() )+")", __FILE__ );
			sleep( 60 );
		}
	}
}
