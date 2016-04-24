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
#include<iostream>

const int TRUE=1;
const int FALSE=0;
#include "lg_common.cpp"

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
  if(argc != 3)
  {
     std::cout << "Syntax: lg_queueseek [filename] [offset] " << std::endl;
     exit(1);
  }
  std::string fileName=std::string(argv[1]);
  unsigned long offset=atoi(argv[2]);

  std::string  lineBuffer;
  std::ifstream infile;    
  infile.open(fileName.c_str());
  if(infile)                                                                  // if it exists
  {
     infile.seekg(offset);
     while(infile)                                                           // read while we have records
     {
         getline(infile, lineBuffer, '\n');                                    // populate the bufffer
         if(lineBuffer.empty() != TRUE)
         {
           std::cout << lineBuffer << std::endl;
         }
     }
  }
  else
  {
     std::cout << "Could not open input file.\n";
  }

  //------------------------------------------------------------------------------------------
  exit(0);
}
