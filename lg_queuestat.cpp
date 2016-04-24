#include <iostream.h>
#include <errno.h>
#include <strings.h>
#include <string>
#include <fstream.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/stat.h>

const int TRUE=1;
const int FALSE=0;
#include "lg_common.cpp"

lg_Statistics lg_stat;
Environment Env;

int main(int argc, char* argv[])
{
  //---------------------------------------------
  // Buffer variables and configurables
  //---------------------------------------------
  unsigned long TotalBytes=0;
 
  //---------------------------------------------------------
  // Put argv stuff here because we need the name of the 
  // stat file and log file 
  //---------------------------------------------------------
  if(argc != 2)
  {
    std::cout << "Syntax: lg_queuestat [statfile] " << std::endl;
    exit(1);
  }
  std::string statFile=std::string(argv[1]);

  //------------------------------------------------------------------------------------------
  if (Env.mapIt_RO(statFile.c_str()) != TRUE)
  {
     std::cout<< "could not map stat file " << statFile << " in R/O mode" << std::endl; 
     exit(1);
  }
  if( Env.readStatsFromFile(lg_stat) != TRUE)
  {
     std::cout << "could not read stats from file "<< statFile << std::endl;
     exit(1);
  }
  else
  {
   std::cout << "offset   : " << lg_stat.fileOffSet  << std::endl 
             << "queuelog : " << lg_stat.queueNumber << std::endl  
             << "scn      : " << lg_stat.SCN         << std::endl; 
  }
  exit(0);
}
