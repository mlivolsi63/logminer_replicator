#include <iostream.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <strings.h>
#include <string>
#include <unistd.h>
#include <fstream.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "lg_exp.h"
#include "lg_common.cpp"

//---------------------------------------------------------------------------------------
// lg_exp (export client)
//
// NOTE: When logging, do no prefix an INFO_PAD log with a timestamp (TIME) as the output
//       does not lineup exactly
//---------------------------------------------------------------------------------------

lg_Statistics lg_stat;
Environment Env;

int main(int argc, char* argv[])
{
  //---------------------------------------------
  // Buffer variables and configurables
  //---------------------------------------------
  char ReadBuffer[MAX], TempString[TMP_LN];
  int           Connected=FALSE, i=0, fileCounter=0;
  unsigned int  n=0, Rc=0, pingCounter=0, totalPings=0;
  unsigned long bytesSent=0, bytesReceived=0;
              
  memset(ReadBuffer,'\0', sizeof(ReadBuffer));
  memset(TempString,'\0', sizeof(TempString));
 
  //---------------------------------------------
  // File Stuff
  //---------------------------------------------
  struct  dirent  *dptr;
  DIR     *dirp;

  //--------------------------------------------------------------------------------------
  // What to do when we get a signal
  //--------------------------------------------------------------------------------------
  signal(SIGUSR1, &cleanup);
  sigset(SIGUSR1, &cleanup);

  signal(SIGPIPE, &cleanup);
  sigset(SIGPIPE, &cleanup);

  signal(SIGUSR2, &wakeup);
  sigset(SIGUSR2, &wakeup);
  sigignore(SIGHUP);
  sigignore(SIGINT);
  sigignore(SIGALRM);

  //---------------------------------------------------------
  // Initialize the enviroment
  //---------------------------------------------------------
  std::string lg_home, oracleBase;
  if( Env.getEnv(lg_home, oracleBase) != TRUE)
  {
    std::cerr << Env.error() << std::endl;
    exit(1);
  }
 
  //---------------------------------------------------------
  // Set the programing environment with the OS environment
  //---------------------------------------------------------
  if( Env.setEnv("lg_exp", lg_home, oracleBase) != TRUE)
  {
    std::cerr << "Could not set the environment" << std::endl;
    exit(1);
  }
 
  //---------------------------------------------------------
  // Read the logfile from the configuration file
  //---------------------------------------------------------
  std::string logFileName;
  if( Env.readConfig("logfile", logFileName) != TRUE)
  {
    std::cerr << "Terminating: logfile is not specified in the configuration file." << std::endl;
    exit(1);
  }
  Env.openLog(logFileName);
  
  
  //---------------------------------------------------------
  // Read the statfile from the configuration file
  //---------------------------------------------------------
  std::string statFile;
  if( Env.readConfig("statfile", statFile) != TRUE)
  {
    std::cerr << "Terminating: Stat file is not specified in the configuration file." << std::endl;
    exit(1);
  }

  //---------------------------------------------------------
  // Put argv stuff here because we need the name of the 
  // stat file and log file 
  //---------------------------------------------------------
  if(argc > 1)
  {
     if(argc == 3)
     {
       if(strcmp(argv[1],"-i")==0)
       {
          if ( Env.initStatFile(statFile.c_str(), atol(argv[2])) != TRUE)
          {
            Env.logIt(INFO, "WARNING : Could not initialize stat file.\n");
            exit(1);
          }
       }
       else
       {
          std::cerr << "Initalizing failure: Invalid flag sent to program\n" ;
          exit(1);
       }
       Env.logIt(INFO, "\t\tInitialized new stat file\n");
       exit(0);
     }
     else
     {
        std::cerr << "Invalid number of arguments passed to lg_exp\n"; 
        exit(1);
     }
   }
  
  //---------------------------------------------------------
  // Read the lockfile from the configuration file
  //---------------------------------------------------------
  std::string lockFile;
  if( Env.readConfig("lockfile", lockFile) != TRUE)
  {
    std::cerr << "Terminating: lock file is not specified in the configuration file." << std::endl;
    exit(1);
  }
  //-----------------------------------------------------
  // WHat's the max number of queue logs we support
  //-----------------------------------------------------
  std::string configFileBuffer;
  int queueFileTotal=0;
  if( Env.readConfig("queue_groups", configFileBuffer)  != TRUE)
  {
     queueFileTotal=3;
     Env.logIt(INFO, "\tThe number of queue files is not specified in config file: Defaulting to 3\n");
  }
  else
  {
      queueFileTotal=atoi(configFileBuffer.c_str());
      if((queueFileTotal < 1) || (queueFileTotal > 12))
      {
         Env.logIt(INFO, "\tInvalid number of queue files specified in config file: Defaulting to 3\n");
         queueFileTotal=3;
      }
  }

  //---------------------------------------------------------
  // Get this hosts information
  //---------------------------------------------------------
  if(uname(&hostname) < 0)
  {
     Env.logIt(INFO, "ERROR : Could not get host information\n");
     exit(1);
  }
  
  //---------------------------------------------------------
  // Check the lockfile
  //---------------------------------------------------------
   char  timeBuffer[TIME_BUF_LN];

   Env.logIt(TIME,  Env.timeNow(timeBuffer, TIME_BUF_LN));
   Env.logIt(INFO_PAD, "Checking locks ");
   if( Env.checkLock(lockFile.c_str()) == TRUE)
   {
      Env.logIt(INFO, "(error)\n\t\tTerminating due to lockfile");
      exit(1);
   }
   else
   { 
      Env.logIt(INFO_CR, "(ok)");
   }
  
  //---------------------------------------------------------
  // Read some configuration information
  //---------------------------------------------------------
  std::string service;
  if( Env.readConfig("service", service) != TRUE)
  {
    std::cerr << "ERROR Terminating: IP service is not specified in the configuration file." << std::endl;
    exit(1);
  }
  Env.logIt(INFO_PAD, "\t\tService");
  Env.logIt(INFO_CR, (char*)service.c_str());
  SERV_TCP_PORT=atoi(service.c_str());
  
  //---------------------------------------------------------
  // Read hostaddress 
  //---------------------------------------------------------
  std::string address;
  if( Env.readConfig("hostaddr", address) != TRUE)
  {
    std::cerr << "Terminating: TARGET HOST IP service is not specified in the configuration file." << std::endl;
    exit(1);
  }
  SERV_HOST_ADDR=(char*)address.c_str();
  Env.logIt(INFO_PAD, "\t\ttarget address");
  Env.logIt(INFO_CR, (char*)address.c_str());
 
  //------------------------------------------------------------------------------------------
  // Set up communication link to server
  //------------------------------------------------------------------------------------------
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR); 
  serv_addr.sin_port        = htons(SERV_TCP_PORT); 
    

  //------------------------------------------------------------------------------------------
  // Read the stats
  //------------------------------------------------------------------------------------------
  if (Env.mapIt(statFile.c_str()) != TRUE)
  {
     Env.logIt(INFO, Env.error() );
     exit(1);
  }
  if( Env.readStatsFromFile(lg_stat) != TRUE)
  {
     Env.logIt(INFO, "ERROR : Could not read stats from file.\n");
     exit(1);
  }
  else
  {
     Env.logIt(INFO_PAD, "\t\tLast Recorded SCN");
     Env.logIt(INFO_CR, lg_stat.SCN); 
     Env.logIt(INFO_PAD, "\t\tLast Recorded Offset");
     Env.logIt(INFO_CR, lg_stat.fileOffSet); 
     Env.logIt(INFO_PAD, "\t\tLast Recorded queue file");
     Env.logIt(INFO_CR, lg_stat.queueNumber); 
  }
 
  //-----------------------------------------------------------
  // Get  the queue file name (aka datafile) from the conf file
  //-----------------------------------------------------------
  std::string queueDir;
  if( Env.readConfig("queue_directory", queueDir)  != TRUE)
  {
     Env.logIt(INFO, "(ERROR)\n\t\tData directory not specified in the configuration file\n");
     exit(1);
  }
  //---------------------------------------------------------
  // Check to see if we can access the queue
  //---------------------------------------------------------
  char tempString[MAX_TMP_LN];
  memset(tempString,'\0', MAX_TMP_LN);                                      // Reside on a word boundary 4, 8, 12

  sprintf(tempString,"%05d", lg_stat.queueNumber);
  std::string queueFile=std::string(queueDir) + "/queue" + tempString;
  glb_queuefile.open(queueFile.c_str(), std::ios::in);
  if(! glb_queuefile)
  {
      Env.logIt(INFO, "\t\tERROR - Could not open for writing: ");
      Env.logIt(INFO_CR,  (char*)queueFile.c_str());
      exit(1);
   }


  glb_queuefile.seekg(0, std::ios::end);                                       // go to the end
  unsigned long fileLength = glb_queuefile.tellg();                            // how big am I ?
  if(fileLength < lg_stat.fileOffSet)                                          // did we reset ?
  {
     lg_stat.fileOffSet=0;                                                     // Yes, start at begining of file
  }
  glb_queuefile.seekg(lg_stat.fileOffSet);                                     // Set the offset from the last ti


 //============================================================================
 // MAIN 
 //============================================================================
 int Looper=TRUE; 
 int sleepLooper=TRUE;                                                         // go to sleep - true or false
 int sleepSeconds=1;                                                           // how many seconds to sleep

 //---------------------------------------------------------
 // We are starting our main processing 
 //---------------------------------------------------------
 Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
 Env.logIt(INFO_CR, ">>>>> Begin lg_exp");

 while(Looper)								       // WHILE FOREVER
 {
    //-----------------------------------------------------------------------
    // Try to connect here
    //-----------------------------------------------------------------------
    if(Connected == FALSE)
    {
      Env.logIt(TIME,  Env.timeNow(timeBuffer, TIME_BUF_LN));
      Env.logIt(INFO_CR, "Attempting to open socket");
      openSocket();
      Connected = TRUE;

      //-----------------------------------------------------------------------
      // Verify version (only if we've reconnected)
      // - If the server and client don't match, then there's a possibility
      //   that we can run into problems
      //-----------------------------------------------------------------------
      glb_outbuffer = std::string("_version");				       // outbuffer = "_version"
      memset(TempString,'\0', sizeof(TempString));
      sprintf(TempString,"%d", (int)glb_outbuffer.length());		       // tempstring = "8"
      glb_outbuffer = std::string(TempString) + "|" + glb_outbuffer;	       // outbuffer = "8|_version"

      if( (unsigned int)write(socketFD, glb_outbuffer.c_str(),  glb_outbuffer.length()) != glb_outbuffer.length()) 
      {
         Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
         Env.logIt(INFO_CR, "WARNING client: Could not send version request to server");
         Env.logIt(INFO_CR, "client: Terminating. Will be re-spawned by cop");
         cleanup(1);
      }
      else
      {
         Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
         Env.logIt(INFO, "Server version: ");
         if( (n=read(socketFD, ReadBuffer, sizeof(ReadBuffer))))              // ======> READ
         {
            ReadBuffer[strlen(ReadBuffer)] = '\0';
            if(strncmp(ReadBuffer, serverVersion, strlen(serverVersion)) != 0)
            {
              Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
              Env.logIt(INFO_CR, "ERROR : Invalid version release from server");
              cleanup(1);
            }
            else
            {
              Env.logIt(INFO_CR, ReadBuffer); 
              memset(ReadBuffer,'\0', sizeof(ReadBuffer));                    // wipe out the buffer since we're done
            }
         }
         else
         {
            Env.logIt(INFO_CR, "\t\tERROR : server not responding to version request");
            cleanup(1);
         }
      }
    }                   // END IF CONNECTED==FALSE
  
    //=========================================================================
    // Imbedded while loop 
    // Sleep until the file has grown or we've received a signal
    //=========================================================================
    sleepLooper=TRUE; 
    Env.logIt(TIME,  Env.timeNow(timeBuffer, TIME_BUF_LN));
    Env.logIt(INFO, "Going to sleep \n");
    while(sleepLooper)                                                         // default to sleep, otherwise, wakeup immediately
    {
       glb_queuefile.clear();                                                  // clear the eof or error flag
       glb_queuefile.seekg(0, std::ios::end);                                  // go to the end
       fileLength = glb_queuefile.tellg();                                     // how big am I ?
       glb_queuefile.seekg(lg_stat.fileOffSet);                                // Go back to where we last left off

       if(fileLength > lg_stat.fileOffSet)                                     // are there new records ?
       {
         Env.logIt(TIME,  Env.timeNow(timeBuffer, TIME_BUF_LN));
         Env.logIt(INFO, "Waking up (on queue growth) \n");
         sleepLooper=FALSE;
         sleepSeconds=0;                                                       // we don't want to sleep
       }
       else
       {
          //-------------------------------------------------------------------
          // We only want to switch queues when we are going into a sleep state
          // If the queue has stopped growing (ext is writting to a different 
          // queue), and we've reached the bottom of our current working queue
          // then we'll go into a sleep state, of which then , based on the
          // signal, we can switch to a different queue.
          //-------------------------------------------------------------------
          if( (glb_switchState == TRUE) && (sleepLooper == TRUE )  )           // A signal was sent to this program
          {                                                                    // ... and we can sleep
             Rc=switchQueue(queueFileTotal, (char*)queueDir.c_str() ); 
             lg_stat.fileOffSet = 0;
             lg_stat.queueNumber = Rc;
             Env.writeStatsToFile(lg_stat);
   
             Env.logIt(TIME, Env.timeNow(timeBuffer,20) );
             Env.logIt(INFO, "Waking up (on signal) - Queue group advanced to # ");
             Env.logIt(INFO_CR, Rc);
   
             glb_switchState = FALSE;                                          // we're done with pausing 
             glb_queuefile.clear();                                            // clear the eof or error flag
             fileLength   = 0; 
             sleepSeconds = 0;
             sleepLooper  = TRUE;
          }                                                                    // the 'else' to this is that they are equal
       }
       sleep(sleepSeconds);                                                    // if 0, nothing happens, otherwise we'll sleep
       sleepSeconds=1;                                                         // reset back to 1
 
       //--------------------------------------------------------------------------
       // Ping: 
       // If we don't ping and we are idle, we'll have no way of knowing if our 
       // connection is still up. The following sends a little ping to the server
       // and checks to see if the connection is still active.
       //--------------------------------------------------------------------------
       pingCounter++;

       if( (pingCounter == (unsigned int)PING_COUNT) && (Connected == TRUE) && (sleepLooper == TRUE ) )
       {									       // ...but only if we are connected.

          //Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
          Env.logIt(INFO, "*");
          pingCounter=0;							       // Build the Ping message
          glb_outbuffer = std::string("_ping");				               // outbuffer = "_ping"
          memset(TempString,'\0', sizeof(TempString));
          sprintf(TempString,"%d", (int)glb_outbuffer.length());		       // tempstring = "5"
          glb_outbuffer = std::string(TempString) + "|" + glb_outbuffer;	       // outbuffer = "5|ping"
       
          //----------------------------------------------------
          // SEND THE PING 
          // 6.1.2007 - We now try to get a response back
          //----------------------------------------------------
          if( (unsigned int)write(socketFD, glb_outbuffer.c_str(),  glb_outbuffer.length()) != glb_outbuffer.length()) 
          {
             Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
             Env.logIt(INFO_CR, "WARNING client: Could not send ping to server");
             Env.logIt(INFO_CR, "client: Terminating. Will be re-spawned by cop");
             Connected=FALSE; 
             sleepLooper=FALSE;
             cleanup(1);
          }
          else
          {
            totalPings++;
            if(totalPings%80 == 0)  
            {
              Env.logIt(INFO_CR, " ");
              totalPings=0;
            }
          }
         
          //---------------------------------------------------------------
          // WAIT FOR RESPONSE FROM SERVER THAT IT GOT THE 'PING'
          //---------------------------------------------------------------

          memset(ReadBuffer,'\0', sizeof(ReadBuffer));
          if( (n=read(socketFD, ReadBuffer, sizeof(ReadBuffer))))              // ======> READ
          {
             ReadBuffer[strlen(ReadBuffer)] = '\0';
             if(strncmp(ReadBuffer,"_pong",5) != 0)
             { 
               Env.logIt(INFO, "\t\tERROR : Invalid response from ping\n\t\t Received:");
               Env.logIt(INFO, ReadBuffer);
               cleanup(1);
             }
          }
          else
          {
             Env.logIt(INFO_CR, "\t\tERROR : server not responding");
             cleanup(1);
          }
       }
    }                                                                          // END sleeplooper

    //=========================================================================
    // Imbedded while loop
    //=========================================================================
    unsigned int stmtSignalMsgFlag=FALSE;                                      // Want to put out a message when a signal is sent
                                                                               // .. but only 1x
    unsigned int recordWrittenFlag=TRUE;                                       // Make sure the record was written even though a signal interupt

    unsigned int statementLooper=TRUE; 
    recordWrittenFlag=TRUE;
    unsigned int resendFlag=FALSE;

    while( statementLooper == TRUE)
    {
       //---------------------------------------------------------------
       // READ THE FILE AND SEND THE CONTENTS OVER THE SOCKET
       //---------------------------------------------------------------
       if(Connected == TRUE)
       {
         if(resendFlag == FALSE)
         {
           if (get_dyn_statement() == FALSE) break;

           //-----------------------------------------------------
           // Before every message, put the length of the message
           //-----------------------------------------------------
           memset(TempString,'\0', sizeof(TempString));                          // clear out the contents
           sprintf(TempString,"%d", (int)glb_outbuffer.length());
           glb_outbuffer = std::string(TempString) + "|" + glb_outbuffer;

           recordWrittenFlag=FALSE; 
         }

         Rc = write(socketFD, glb_outbuffer.c_str(),  glb_outbuffer.length()); // <===== WRITE
         if( Rc != glb_outbuffer.length() )
         {
            if ( glb_switchState == FALSE )                                    // A signal can give us a false state. so we check the state
            {
               Env.logIt(INFO_CR, "\t\tWARNING client: Could not write message to socket");
               cleanup(1);
            }
         }
         else
         {
            recordWrittenFlag=TRUE; 
         }
            
         bytesSent+=(Rc-3);
         Env.writeStatsToFile(lg_stat);
         
         if( (glb_switchState == TRUE) && (stmtSignalMsgFlag == FALSE) )
         {
           Env.logIt(INFO_CR, "\t\tSignal received while processing statements\n");
           stmtSignalMsgFlag=TRUE;
           Env.logIt(INFO_CR, (char*)glb_outbuffer.c_str());
         }
       }
       if(recordWrittenFlag == FALSE ) 
       {
          Env.logIt(INFO_CR, "\t\t WARNING: A record was interupted by the signal and did not make it");
          Env.logIt(INFO_CR, "\t\t          Re-transmitting ");
          resendFlag=TRUE;
       }
       else
       {
          resendFlag=FALSE;
       }
     }                                                                         // end get_dyn_statement 
     //---------------------------------------------------------------
     // fin:
     // The following piece of code sends a 'fin' command to the server
     // letting it know we are finished reading files
     //---------------------------------------------------------------
     if(Connected == TRUE)
     {
          Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
          Env.logIt(INFO_CR, "\t ---> Sending finish command to server");
          glb_outbuffer = std::string("fin");
          memset(TempString,'\0', sizeof(TempString));
          sprintf(TempString,"%d", (int)glb_outbuffer.length());
          glb_outbuffer = std::string(TempString) + "|" + glb_outbuffer;

          Rc = write(socketFD, glb_outbuffer.c_str(),  glb_outbuffer.length());// <===== WRITE
          if(Rc != glb_outbuffer.length())
          {
             if ( glb_switchState == FALSE )                                   // ignore signal to switch queues
             { 
               Env.logIt(INFO_CR, "\t\tWARNING client: Could not write fin message to socket");
               cleanup(1); 
             }
          }
          
          bytesSent += (Rc-3);
          
          //---------------------------------------------------------------
          // WAIT FOR RESPONSE FROM SERVER THAT IT GOT THE 'FIN' COMMAND
          //---------------------------------------------------------------

          Env.logIt(INFO_CR,  "\t\t<--- Waiting for response");
          if( (n=read(socketFD, ReadBuffer, sizeof(ReadBuffer))))	       // ======> READ
          {
             ReadBuffer[strlen(ReadBuffer)] = '\0';
             Env.logIt(INFO_PAD,"\t\tBytes Sent");  
             sprintf(TempString, "%d", bytesSent);
             Env.logIt(INFO_CR, TempString); 


             Env.logIt(INFO_PAD,"\t\tBytes Received"); 
             sprintf(TempString, "%ld", (atol(ReadBuffer)-bytesReceived));
             Env.logIt(INFO_CR,  TempString );
             bytesReceived=atol(ReadBuffer);

             Env.writeStatsToFile(lg_stat);
             bytesSent=0;
          }
          else
          {
             Env.logIt(INFO_CR, "\t\tWARNING : File NOT Processed on Server Side");
             Connected=FALSE; 
          }
     }
  }					// END WHILE FOREVER

  //----------------------------------------------------------------
  // Finish: The loop is over, go in peace.
  //----------------------------------------------------------------
 
  if(close(socketFD) != 0) Env.logIt(INFO_CR, "\t\tWARNING : Error closing socket");

  Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
  Env.logIt(INFO_CR, "\t\tClient ends ");
  exit(Rc);
}
//------------------------------------------------------------------------------------------
// Open the socket
//
// - Return only if we've got a connection
// - Exit if an error
// - otherwise loop until connected
//------------------------------------------------------------------------------------------
int openSocket()
{
  int Looper=TRUE;
  while(Looper)
  {
    if ( (socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      Env.logIt(INFO, "WARNING client: Cannot open stream socket\n");
      exit(1);
    }
    Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
    Env.logIt(INFO, "Connecting to server on port ");
    Env.logIt(INFO_CR, SERV_TCP_PORT);

    if (connect (socketFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
       Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
       Env.logIt(INFO_CR, "ERROR : FAILED to connect");
       close(socketFD);
       sleep(30);
    }
    else
    {
       Env.logIt(INFO_CR, "\t\tConnected");
       Looper=FALSE;
    }
    fflush(NULL);
  }
  return(0);
}

//-----------------------------------------------------------------------------
//
// Returns: FALSE = eof
//          TRUE  = got the statement
//-----------------------------------------------------------------------------
int get_dyn_statement()
{
    std::string linebuf;   linebuf.clear();
    
    std::string SCN;       SCN.clear();

    glb_outbuffer.clear();                                                     // clear the eof marker

    getline(glb_queuefile, linebuf, '\n');
    if(glb_queuefile.eof() == 1)                                               // At the EOF ?
    { 
       return(FALSE);                                                          // rc=1 means EOF to calling function
    }
    else 
    {
       glb_outbuffer=std::string(linebuf) + "\n\0"; 
       lg_stat.fileOffSet+=linebuf.length()+1; 
    }

    unsigned int delimiter=0;
    
    if (linebuf.substr(0,2) == "--")                                           // if its a comment, get the SCN 
    {
       delimiter = linebuf.find("|", 0);
       SCN=std::string(linebuf.substr(3,delimiter-4));
       strcpy(lg_stat.SCN, SCN.c_str());
    }
    
    return(TRUE);
}

//------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------
void cleanup(int signum)
{
   char tempStr[16];
   sprintf(tempStr, "%d", signum);
   if(close(socketFD) != 0)
   {
      Env.logIt(INFO_CR, "\t\tWARNING : could not close socket");
   }
   if(signum != 1)
   {
     Env.logIt(INFO, "\t\tINFO : lg_exp terminating on signal :");
     Env.logIt(INFO_CR, tempStr);
   }
   exit(signum);
}

//-----------------------------------------------------------------------------
// Routine:  wakeup
// purpose:  Semd a message to the console and set the global signal flag
//-----------------------------------------------------------------------------
void wakeup(int signum)
{
   Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
   Env.logIt(INFO_CR, "Wakeup:\t on signal 17");

   Env.logIt(INFO_PAD, "\t\tLast Recorded SCN");
   Env.logIt(INFO_CR, lg_stat.SCN); 
   glb_switchState = TRUE;                                  // Global switch state is set to true
}

//=============================================================================
// Routine: switchQueue
// purpose: when we receive the signal, and we are idle, the come here and
//          switch to the next queue. We used to terminate, wait until the queue cleared
//          but this is a better approach as we can continue to process
//
//=============================================================================
int switchQueue(int numberOfQueues, char queueDir[])
{
  int queueNumber=(lg_stat.queueNumber + 1) % numberOfQueues;
  if ( queueNumber == 0)
  {
     queueNumber=numberOfQueues;                           //  ie. if the previous was 2 (out of 3), now its 0
  }                                                        //  so we set it to the max number of logs supported
  lg_stat.queueNumber=queueNumber;

  char tempString[MAX_TMP_LN];
  memset(tempString, '\0', MAX_TMP_LN);
  sprintf(tempString,"%05d", queueNumber);

  std::string queueFile = std::string(queueDir) + "/queue" + tempString;

  glb_queuefile.close();
  glb_queuefile.open(queueFile.c_str(), std::ios::in);

  return(queueNumber);
}
