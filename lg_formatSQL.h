#include <fstream>

const int SEGMENT_NAME_LN=30;
const int MAX_LOAD_OBJECTS=256;
const int MAX_INDEXES_PER_TABLE=24;
const int OFF=0;
const int ON=1;

class formatSQL 
{
  private:
     char tableName[SEGMENT_NAME_LN];
     std::string inStr, timeStampStr, DMLstr, predicateStr;
     std::string indxTableName[MAX_LOAD_OBJECTS], indxColumns[MAX_LOAD_OBJECTS];
     std::string errMsg;
  
     std::string indxColumnList[MAX_INDEXES_PER_TABLE];
     unsigned int currentPos, len;
     unsigned int indxColumnCount[MAX_LOAD_OBJECTS], indxListCounter;
     std::fstream inFile;
  public:
     formatSQL(); 
     int  initialize(const char indxFileName[]);
     int  getTableName(std::string inString);
     int  genPredicateList(unsigned int indxListIndex);
     int  update2insert();
     int  insert2update(); 
     int  update2update();            // format DML string with updated (truncated) predicate
     int  delete2delete();            // format DML string with updated (truncated) predicate
     char* sqlString();
     char* predicateString();
     char* tableNameString();
     char* timeStampString();
     char* errorMessage();
};
