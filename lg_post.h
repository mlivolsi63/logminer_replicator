//------------------------------------------------------------------------
// NOTE: Due to pro*C, we're using # defines instead of const in
// Maximum number of select-list items or bind variables.   
//------------------------------------------------------------------------
#define MAX_ITEMS         64                                                   // Changed from 40 to 64

// Maximum lengths of the _names_ of the select-list items or indicator variables. 
#define MAX_VNAME_LEN     30
#define MAX_INAME_LEN     30
#define LONG_SQL_STRING 16384

#ifndef NULL
#define NULL  0
#endif

/* Prototypes */
#if defined(__STDC__)
  void sql_error(const int action, char inMsg[]);
  int oracle_connect(void);
  int alloc_descriptors(int, int, int);
  int get_dyn_statement(void);
  void set_bind_variables(void);
  void process_select_list(void);
  //-----------------------------------------------------------------------------
  int getDBTimeStamp(char inTableName[], char inPredicate[], char inTimeStamp[], const char schemaOwner[]);
  int resolveConflict(const std::string schemaOwner, const char timeNow[]);
  int switchQueue(int numberOfQueues, char queueDir[]);
  void Cleanup(int);
  void Wakeup(int);
  //-----------------------------------------------------------------------------
#else
  void sql_error(const int action, char inMsg[]);
  int oracle_connect(/*_ void _*/);
  int alloc_descriptors(/*_ int, int, int _*/);
  int get_dyn_statement(/* void _*/);
  void set_bind_variables(/*_ void -*/);
  void process_select_list(/*_ void _*/);
  //-----------------------------------------------------------------------------
  int getDBTimeStamp(char inTableName[], char inPredicate[], char inTimeStamp[], const char schemaOwner[]);
  int resolveConflict(const std::string schemaOwner, const char timeNow[]);
  int switchQueue(int numberOfQueues, char queueDir[]);
  void Cleanup(int);
  void Wakeup(int);
  //-----------------------------------------------------------------------------
#endif

char *dml_commands[] = {"SELECT", "select", "INSERT", "insert",
                        "UPDATE", "update", "DELETE", "delete"};

SQLDA *bind_dp;
SQLDA *select_dp;

jmp_buf jmp_continue;
int parse_flag = 0;

const int   TRUE            =    1;
const int   FALSE           =    0;

const int   EXIT            =    1;
const int   CONTINUE        =    0;

const int   FN_LEN          =  128;
const int   MAX_IDLE_TIME   =   10;
const int   COMMIT          =    1;

const int TIME_BUF_LN=20;

const int INSERT=1;
const int UPDATE=2;
const int DELETE=3;

//-------------------------
// Used for processing flow 
//-------------------------
const int NORMAL=0;
const int RESOLVE=1;
const int IGNORE=2;
const int IDLE=3;
const int COMMIT_WORK=4;
const int DDL=5;                                                               // New support - for v7.0 we'll support sequences 
const int ENDSQL=6;

const unsigned int MOD_ROWS=250;


std::fstream  logfile; 
std::ifstream datafile;

int globalRC=0;
char          TimeBuffer[20], TimeBuffer2[64];
unsigned long queueOffSet=0;
std::string   SCN, timeStr;

long sqlRc=0;
unsigned long dmlOffset = 0;
unsigned long commitOffset = 0;

int           glb_switchState=0;                                                  // when a signal is received
std::fstream  glb_queuefile;

