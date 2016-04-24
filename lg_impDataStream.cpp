#include "lg_impDataStream.h"

//---------------------------------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------------------------------
DataStream::DataStream()
{
  initialConnectFlag=TRUE; 
}
//---------------------------------------------------------------------------------------------
int DataStream::TCPechod(int fd, int verbose, char queueDir[], int numberOfQueues)
{
 char          inbuf[MSG_SIZE];                     // messages coming in
 std::string   inRecord;                            // after we read from the socket, build a record 
 unsigned int  inBytes;                             // expected # of bytes 
 std::string   outbuf;                              // messages out
 char          timeBuffer[TIME_BUF_LN];             // Used by time routine to get the current time
 int           readLooper=TRUE; 
 int           pFlag=FALSE;                         // used to keep track on wether we've done any previous work
 unsigned long TotalBytes=0;                        // total bytes read in from the socket
 unsigned long BytesLeft=0;                         // bytes expected - bytes read = bytes left

 //---------------------------------------------------------------------
 // Set the queue to the last recorded queue
 //---------------------------------------------------------------------
 char          tempBuffer[16];                      // temporary work space

 memset(tempBuffer, '\0', sizeof(tempBuffer));
 sprintf(tempBuffer, "%05d", lg_stat.queueNumber);
 std::string FileName = std::string(queueDir) + "/queue" + tempBuffer;

 //---------------------------------------------------------------------
 // Open the queue file for processing
 //---------------------------------------------------------------------
 glb_queuefile.open(FileName.c_str(), std::ios::out | std::ios::app);
 if(! glb_queuefile)
 {
    Env.logIt(INFO, "ERROR : Could not open for writing: ");
    Env.logIt(INFO,  (char*)FileName.c_str());
    outbuf = std::string("Error.");
    return(FALSE);
 }
 
 Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
 Env.logIt(INFO_CR, ">>>>> Begin lg_imp server");

 int Rc=0;                                              // Used to keep track of the queue number
 int cc=0;                                              // # of bytes read from the socket - 0 or negative is a bad cc
 int looper=TRUE;                                       // Infinite loop
 while(looper)
 {
    //--------------------------------------------------
    // Reset all buffers and variables
    //--------------------------------------------------
    memset(inbuf,'\0', sizeof(inbuf));                  // Wipe out inbuffer
    inRecord.clear();                                   // Wipe out the record construct
    inBytes=0;                                          // Wipe out expected # of bytes we should be receiving 
    pFlag=FALSE;                                        // Set this flag to false, we're not in the middle of processing a statement
   
    //--------------------------------------------------
    // READ THE BEGINING OF THE INBUFFER 
    //--------------------------------------------------
    readLooper=TRUE;                                    // Read until done reading
    while(readLooper)
    {
       cc = read(fd, inbuf, 1);                          // This is the instruction on how many bytes to read
       if( cc <= 0)                                      // we may get this on a client hangup or another type of signal 
       {
          if(glb_switchState == TRUE)                    // Yup, we've been told to switch via signal 
          {
            Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
            Env.logIt(INFO_CR, "\t\tsignal received - calling queue switch");
 
            Rc=switchQueue(numberOfQueues, queueDir);
            Env.writeStatsToFile(lg_stat);
            Env.logIt(TIME, Env.timeNow(timeBuffer,20) );
            Env.logIt(INFO, "queue group advanced to # ");
            Env.logIt(INFO_CR, Rc);
            Env.writeStatsToFile(lg_stat);

            readLooper      = FALSE;                     // Break out of this loop 
            glb_switchState = FALSE;
          }
          else                                           // we received a signal , or a hangup from the client
          {
            Env.logIt(INFO,"ERROR : Invalid return code from cc - aborting child."); 
            close(fd);
            exit(1); 
          }
       }
       //----------------------------------------------------
       // a positive CC means there is someting in the buffer 
       //----------------------------------------------------
       else
       {
          inBytes++;
          if(strlen(inbuf) != 0)                            // There's something in the inbuffer
          {
             if( inbuf[0] == '|' ) readLooper=FALSE;        // Its the first line, and we've  instructions on how many bytes to read
             else inRecord = std::string(inRecord) + inbuf; // didn't get enough bytes from the socket, so we'll append and continue
          }
          if(inBytes > 7)                                   // If this value starts incrementing, we have a problem  
          {                                                 // .. NOTE: 7 is an semi-arbitrary number.  
             std::cerr << "ERROR : Loop control. This spawned child is terminating\n";
             exit(1);
          }
       }
    }                                                    // end while

    //--------------------------------------------------
    // START READING THE REST
    //--------------------------------------------------
    inBytes = atoi(inRecord.c_str());                    // How many bytes were we told to expect ?
    
    if(inBytes != 0)                                     // we're expecting 'x' number of bytes to read
    {
       memset(inbuf,'\0', sizeof(inbuf));                // wipe out the in buffer
       cc = read(fd, inbuf, inBytes);                    // read from socket into the inbuffer

       if(cc <= 0)                                       // This is bad, so we'll exit immediately if it happens
       {
          Env.logIt(INFO, "ERROR : invalid code from reading socket ");
          close(fd);
          exit(1); 
        }
          
       //-------------------------------
       // IN PING - SEND PONG  
       //-------------------------------
       if(strncmp(inbuf,"_ping", 5)==0) 
       {
         pFlag = TRUE;
         outbuf.clear();
         outbuf = std::string("_pong");
         if( SendConsoleMessages(fd, outbuf, 1, verbose) != 0) Env.logIt(INFO, "ERROR : in pong.\n");
       }
       
       //------------------------------------------
       // IN VERSION REQUEST - SEND VERSION NUMBER
       //------------------------------------------
       if(strncmp(inbuf,"_version",8)==0) 
       {
         pFlag = TRUE;
         outbuf.clear();
         outbuf = std::string(serverVersion);
         if( SendConsoleMessages(fd, outbuf, 1, verbose) != 0) Env.logIt(INFO, "ERROR : sending version request.\n");
       }

       //------------------------------------------
       // Defunct routine ?
       //------------------------------------------
       if(pFlag == FALSE) 
       {
         if(strncmp(inbuf,"filename", 8)==0)  
         {
              Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
              Env.logIt(INFO, "<--- Filename command from client.\n");
              pFlag = TRUE;
              TotalBytes = 0;

              outbuf = std::string("_message_accepted.");
              Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
              Env.logIt(INFO, "---> sending message accepted to client.\n");
              if( SendConsoleMessages(fd, outbuf, 1, verbose) != 0) Env.logIt(INFO, "ERROR : in sending message.\n");
         }
       }

       //--------------------------------------------
       // TOLD BY CLIENT TO TERMINATE ALL PROCESSING
       //--------------------------------------------
       if(pFlag == FALSE) 
       {
         if(strcmp(inbuf,"fin")==0) 
         {
              pFlag = TRUE;
              Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
              Env.logIt(INFO, "<--- end command from client.\n");
              sprintf(tempBuffer,"%ld", TotalBytes) ;
              Env.logIt(TIME, Env.timeNow(timeBuffer, TIME_BUF_LN));
              Env.logIt(INFO,  "---> sending confirm to client.\n");
              if( SendConsoleMessages(fd, tempBuffer, 1, verbose) != 0) 
                   Env.logIt(INFO, "ERROR : In sending confirm message.\n");
         }
       }
            
       //--------------------------------------------
       // DRAIN THE INBUFFER UNTIL WE HAVE EVERYTHING
       //--------------------------------------------
       if(pFlag == FALSE)                                              // No previous processing above
       {
         BytesLeft=inBytes - cc;                                       // bytes told to expect - bytes we've received
         TotalBytes+=cc;
         glb_queuefile.write((char *)inbuf,  strlen(inbuf));           // write-out the current contents of our buffer
         glb_queuefile.flush();                                          // Flush so it shows up for the poster
         while(BytesLeft >= 1)
         {
             Env.logIt(INFO, "more data on the queue\n");
             memset(inbuf,'\0', sizeof(inbuf));                        // wipe out the inbuffer
             cc = read(fd, inbuf, BytesLeft);                          // read from the socket
             Env.logIt(INFO, "Number of bytes from cc read:");
             memset(tempBuffer,'\0', sizeof(tempBuffer));              // wipe out the temporary buffer
             sprintf(tempBuffer,"%d", cc) ;                            // take an int and make it a char so we have something to say
             Env.logIt(INFO,  tempBuffer);                             // report on the number of bytes read
             glb_queuefile.write((char *)inbuf,  strlen(inbuf));       // write-out the contents to the queue
             glb_queuefile.flush();                                    // Flush so it shows up for the poster
             BytesLeft -= cc;                                          // calculate the number of bytes we need to read
             TotalBytes+=cc;
          } 
       }
       glb_queuefile.flush();                                          // Flush so it shows up for the poster
    }
 } 
 glb_queuefile.close();
 return 0;
}
//---------------------------------------------------------------------------------------------
// strcpy is equal
// strcat is +
//---------------------------------------------------------------------------------------------
int DataStream::SendConsoleMessages(int fd, std::string outbuf, int rc, int verbose)
{
  //------------------------
  if(verbose >= 1)  std::cout << "[I] Replying to client with message type " << rc << std::endl;
  //------------------------
  switch(rc)
  {
    case -1:  
              if(write (fd, outbuf.c_str(), outbuf.size()) < 0) return 1;
              break;
    case 0 :
              if(write (fd, NULL, 0) < 0) return 1;
              break;
    case 1 :
              if( write (fd, outbuf.c_str(), outbuf.size()) < 0) return 1;     // Write message back to client
              if(fflush(NULL) != 0 && verbose >= 1) std::cout << "could not flush output stream\n";
              break;
    case 2 :
              if(write (fd, outbuf.c_str(),outbuf.size()) < 0) return 1;
              if(fflush(NULL) != 0 && verbose >= 1) std::cout << "could not flush output stream\n";
              break;
 
    default:  std::cout << "unknown command to server\n"; 
              break; 
   }

   return(0);
}
//=============================================================================
// Routine: switchQueue
// purpose: when we receive the signal, and we are idle, the come here and
//          switch to the next queue. We used to terminate, wait until the queue cleared
//          but this is a better approach as we can continue to process
//
//=============================================================================
int DataStream::switchQueue(int numberOfQueues, char queueDir[])
{
  int queueNumber=(lg_stat.queueNumber + 1) % numberOfQueues;
  if ( queueNumber == 0)
  {
     queueNumber=numberOfQueues;                               //  ie. if the previous was 2 (out of 3), now its 0
  }                                                        //  so we set it to the max number of logs supported
  lg_stat.queueNumber=queueNumber;

  char tempString[MAX_TMP_LN];
  memset(tempString, '\0', MAX_TMP_LN);
  sprintf(tempString,"%05d", queueNumber);

  std::string queueFile = std::string(queueDir) + "/queue" + tempString;

  glb_queuefile.close();
  glb_queuefile.open(queueFile.c_str(), std::ios::out | std::ios::app);

  return(queueNumber);
}


