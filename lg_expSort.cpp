#include "lg_expSort.h"


int Compare(const void *p1, const void *p2)
{
  char* fn1; 
  char* fn2;
  fn1 = (char*)p1;
  fn2 = (char*)p2;
//  strcpy(fn1, p1);
//  strcpy(fn2, p2);
  return( strcmp(fn1, fn2)); 
}


FileList::FileList()
{
}

void FileList::initialize()
{
 int i;
 for(i=0; i< FILE_LIMIT; i++)
 {
   memset(name[i],'\0', NAME_LN);
 }
 fileIndex=0;
}

int FileList::setName(char ArgFileName[])
{
 strcpy(name[fileIndex], ArgFileName);
 fileIndex++;
 return(1);
}

void FileList::sortList()
{
 qsort(name, fileIndex, NAME_LN, Compare);
 fileIndex=0;
}

char* FileList::getName()
{
 int i;
 i = fileIndex;
 fileIndex++;
 return name[i]; 
}

char* FileList::currentName()
{
 return name[fileIndex-1]; 
}
