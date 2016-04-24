#include "lg_formatSQL.h"

//=========================================================================================================
// Copyright 2000-2009 Sonus Networks
// All Rights Reserved
// Use of copyright notice does not imply publication.
//
// This document contains Confidential Information Trade Secrets, or both
// which are the property of Sonus Networks. This document and the information
// it contains may not be used disseminated or otherwise disclosed without
// prior writen consent of Sonus Networks.
//
// Routine: formatSQL
// purpose: To take the SQL list that gets created when the lg_post.sh (ksh) program gets invoked.
//          two queries are issued in the ksh program that gets the list of indexes and stores them in a flat
//          file. Then when we call this class.. the first thing that needs to get done is to load
//          them into an array. Without this being done, absolutely no software conflicts can be resolved. 
// date:    Aug. 6, 2004
// Author:  MLV
// Return codes: None. This is the constructor 
//=========================================================================================================
formatSQL::formatSQL():
	inStr(), timeStampStr(), DMLstr(), predicateStr(),	
	errMsg(), currentPos(0), len(0), indxListCounter(0), inFile()
{
	tableName[0] = '\0';
	indxTableName[0] = '\0';
	indxColumns[0] = '\0';
     	indxColumnList[0] = '\0';
	memset(indxColumnCount, 0, MAX_LOAD_OBJECTS);
}

int formatSQL::initialize(const char indxFileName[])
{
  std::string inBuffer, prevTableName, newTableName;
  int BlankPos=0, i=0;

  indxListCounter=0;
  currentPos=0;

  inFile.open(indxFileName, std::ios::in);
  while(inFile)
  {
    getline(inFile, inBuffer, '\n');     
    if(inBuffer.empty() != TRUE)
    {
        BlankPos=inBuffer.find_first_of(" ");                                  // delimited by blanks
        newTableName = inBuffer.substr(0, BlankPos);                           // table name is the first column
        if(newTableName.compare(prevTableName) != 0)                           // New table... remember, we can have multiple rows
        {
          indxTableName[i] = inBuffer.substr(0, BlankPos); 
          indxColumnCount[i] = 1;
          BlankPos=inBuffer.find_first_of(" ", 31);
          indxColumns[i] = inBuffer.substr(31, BlankPos - 31); 
          prevTableName = std::string(newTableName);
          i++;
        }
        else                                                                   // we'll append the name delimited by '|'
        {
          BlankPos=inBuffer.find_first_of(" ", 31);
          indxColumns[i-1].append("|" + inBuffer.substr(31, BlankPos - 31)); 
          indxColumnCount[i-1]++;
        }
    }
  } 
  indxListCounter=i-1;

  //-----------------------------------------------------------------------------------------------------
  // DEBUG
  //  cout << "[I] Number of index lists loaded into memory ............ (" << indxListCounter+1 << ")\n";
  //  int j=0;
  //  for(j=0; j< i; j++) 
  //  {
  //   cout << "[I] " << j << "|" << indxColumns[j] << "| Columncount : " << indxColumnCount[j] << " \n";
  //  }
  //-----------------------------------------------------------------------------------------------------
  if(i == 0) return(FALSE);
  else       return(TRUE);

}
//=========================================================================================================
// Routine: genPredicateList
// purpose: To parse the index list for a table and load an array to be used in creating the predicate 
//          In english it means.. take the STRING "COLA|COLB|COLC" that we get when we load the index list 
//          and put each col (ie. COLA) into it's own indexed array.
//          so you would get.. indxColumnList[1] = COLA and indexColumnList[2] = COLB , etc..
// date:    Aug. 3, 2004
// Author:  MLV
// Return codes: 0 - Didn't do anything
//              -1 - Number of indexes parsed dosen't match the number of indexes expected 
//=========================================================================================================
int formatSQL::genPredicateList(unsigned int indxListIndex)
{
  unsigned int i=0, pos=0, indexCounter=0;
  std::string token;

  errMsg.clear();
  for(i=0; i < indxColumns[indxListIndex].size(); i++)               // For the number of columns in the list (ie. COLA| COLB | )
  {
    pos = indxColumns[indxListIndex].find_first_of("|",i);           // This information is delmited by '|'
    if(pos != std::string::npos)                                     // if we found a delimiter 
    {
      token = indxColumns[indxListIndex].substr(i, pos-i);           // Parse and assign to variable 'token'
      indxColumnList[indexCounter] = token;
      indexCounter++;
      i = pos;                                                       // move 'i' up to this delimiter so next time we find the next
    }   
    else                                                             // otherwise we must be at the last one (not terminated by | )
    {
      token = indxColumns[indxListIndex].substr(i);
      indxColumnList[indexCounter] = token;
      indexCounter++;
      break;
    }
  } 
  if(indexCounter != indxColumnCount[indxListIndex])                // just making sure we've parsed the right number of cols.
  {
    errMsg = std::string("The number of columns parsed dosen't match the number of columns expected in the index list");
    return(-1);
  }
  return(1);
}

//=========================================================================================================
// Routine: getTableName
// purpose: To get the table name from a DML string and via return code, return the type of DML action
// date:    Aug. 3, 2004
// Author:  MLV
// Return codes: 1 
//===========================================================================================================
int formatSQL::getTableName(std::string inString)
{
   unsigned int i, Rc=0, pos=0, pos2=0;
   unsigned int tokenLen=0;
   std::string Token, tableNameStr;

   errMsg.clear();
   inStr = std::string(inString);
 
   len = inStr.size();
   memset(tableName, '\0', sizeof(tableName));

   Rc=0;
   for(i = 0; i < len; i++)
   {
      pos=inStr.find_first_of(" ",i);
      if( pos != std::string::npos )
      {
         Token = inStr.substr(i, pos - i);
         if( (Token.compare("INSERT") == 0) || (Token.compare("insert") == 0) )
         {
            i=pos+1;
            pos=inStr.find_first_of(" ",i);          // go beyond the word
            Token = inStr.substr(i, pos - i);        // "INTO"
    
            i=pos+1; 
            pos=inStr.find_first_of("(",i);      
            Token = inStr.substr(i, pos - i);       // "TABLE_NAME"
        
            i=pos; 
            pos=Token.find_first_of(".",0);         // Is the table_name qualified with the table_owner
            if (pos != std::string::npos)           // If yes ...
            {
              Token = Token.substr(pos+1);          // .. then get the table_name
            }
            currentPos = i;                         // NOTE: This is handled differently below
            Rc=1;
            break;
         }
         else
         {
            if ( (Token.compare("UPDATE") == 0) || (Token.compare("update") == 0) )
            {
               Rc=2;
               i=pos+1; 
               pos=inStr.find_first_of(" ",i);                       // go beyond the word
               Token = inStr.substr(i, pos - i);                     // "TABLE_NAME"
               i=pos+1;                                              // We are going fishing for the "set" keyword
               pos=Token.find_first_of(".",0);                       // Is the table_name qualified with the owner
               if (pos != std::string::npos)                         // if yes
               {
                   Token = Token.substr(pos+1);                      // parse it 
               }
               pos=inStr.find(" set ",i);                            // "SET"
               currentPos = i+4;
               break;
            }
            else
            {
               if( (Token.compare("DELETE") == 0) || (Token.compare("delete") == 0) )
               {
                  Rc=3;
                  i=pos+1;
                  pos=inStr.find_first_of(" ",i);                    // go beyond the word
                  Token = inStr.substr(i, pos - i);                  // .. "FROM"
                  i=pos+1;                                           // then set the index to the first occurence of the next word
                  pos=inStr.find_first_of(" ",i);
                  Token = inStr.substr(i, pos - i);                  // "TABLE_NAME"
                  i=pos+1;                                           // we are going fishing for the "where" keyword
                  pos=Token.find_first_of(".",0);                    // is table_name qualified with the owner ?
                  if (pos != std::string::npos)
                  {
                    Token = Token.substr(pos+1);                     // if yes, then get the table_name
                  }
                  pos=inStr.find_first_of(" ",i);
                  currentPos = i+4;
                  break;
                }
            }
         }
         i = pos;
      }
   }                                                       // end for loop

   //-------------------------------------------------
   // Strip out the double quotes
   //-------------------------------------------------
   pos=Token.find_first_of("\"",0);                       // Are there any double quotes
   tokenLen=Token.size();
   if(pos != std::string::npos)
   {
      if(pos == tokenLen)
      {
         Token = Token.substr(0,tokenLen-1);        // If for some reason it's only on the last word, strip
      }
      else                                          // .. otherwise start with the first quote
      {
         pos2=Token.find_last_of("\"",tokenLen);    // and get the last quote
         if(pos != pos2)
         {
            Token = Token.substr(pos+1, (pos2-pos)-1);
         }
         else
         {
            Token = Token.substr(pos);
         }
      }
   }

   strcpy(tableName, Token.c_str());
   return(Rc);
}
//===========================================================================================================
// Routine: update2insert
// purpose: To convert an update statement to an insert statement
// date:    Aug. 3, 2004
// Author:  MLV
// Return codes: 0 - Didn't do anything
//               1 - parsed cleanly
//              -1 - Error encounted - errMsg set 
// Changes:
//           2.9.2005 - set initializations on variables when declaring them
//===========================================================================================================
int formatSQL::update2insert()
{
 int foundFlag=FALSE, timeFlagFound=FALSE;
 unsigned idx, wordCount=0;
 unsigned int  eqPos=0, quoPos1=0, quoPos2=0, pos1=0, pos2=0, timeStampIdx=0;
 std::string Token, ColumnList[128], ValueList[128];

 errMsg.clear(); 
 DMLstr       = std::string("INSERT INTO ") + tableName  + " (";       // this will be used to create an insert
 predicateStr = std::string(" WHERE ");                                // we may also need a stripped down updt.
 //--------------------------------------------------------------------------
 // Need to search tablelist and index list to prepare to form the predicate
 //--------------------------------------------------------------------------

 //------------------------------------------------------------------------
 // DEBUG
 //cout << "[DEBUG] Looking for table name " << tableName << endl;
 //------------------------------------------------------------------------
 unsigned int indxListIndex=0;                                         // where in the index list is the table
 unsigned int i = 0;                                                   // go through the index column list
 for(i=0; i <= indxListCounter; i++)
 {
     if(indxTableName[i].compare(tableName) ==0)                       // Compare tablenames to the list 
     {
         indxListIndex=i;                                              // This points to the list of columns
         if (genPredicateList(indxListIndex) != 1)                     // did not find table in the index list
         {
            return(-1);
         }
         break;
     }
 }
 if ( indxListIndex== 0)                                               // This table does not have an index 
 {
     errMsg = std::string("Could not find ") + tableName + " in index list";
     return(-1);
 }
 //--------------------------------------------------------------------------
 // We need to find out the postion of the 'where' clause. In that way, we 
 // process the first name/value pairs first, then we'll process the name value
 // pairs in the predicate
 //--------------------------------------------------------------------------
 unsigned int wherePos   = inStr.find(" where ", currentPos);          // find the SQL keyword "where"
 if(wherePos == std::string::npos)
 {
   wherePos = inStr.find(" WHERE ", currentPos);
   if( wherePos == std::string::npos)                                  // no 'where'. Maybe not an update or delete
   {
      errMsg = std::string("insert2update: Could not find the begining of the predicate.");
      return(-1);
   }
 }
 //------------------------------------------------------------------------
 // DEBUG
 // std::cout << "[DEBUG] 'where' is at position " << wherePos << std::endl; 
 //------------------------------------------------------------------------

 int predSwitch=OFF;                                                 // initialize the predicate switch
 for(i = currentPos; i < len; i++)
 {
   //=================================================================================================
   if(predSwitch == OFF)                                             // are we at the first 1/2 of the update statement
   {
     pos1=inStr.find_first_of("(",i);                                // need this if a function is being used on a col. 
     pos2=inStr.find_first_of(",",i);                                // pre-predicate pairs are seperated by commas
     //-------------------------------------------------------------------------------------------------------
     if( (pos2 != std::string::npos ) && (pos2 < wherePos))      
     {
        if (pos2 > pos1)                                             // theres a left parenthesis before the comman . Function ?
        {
           pos2 = inStr.find_first_of(",", pos2+1);                  // there's a function going on, so pass by the first ','
           if( (pos2 == std::string::npos) || (pos2 > wherePos) )
           {
              predSwitch = ON;
              pos2 = inStr.find_first_of(")", pos1);                 // if we are at the end, go back a little
              if(pos2 != std::string::npos) pos2++;
           }
        }

        eqPos=inStr.find_first_of("=",i);                            // and the pairs are matched with an equal sign
        Token   = std::string(inStr.substr(i, eqPos-i));             // the left hand of the '=' is the column name
        quoPos1 = Token.find("\"");                                  // .. which is usually in doule quotes
        if(quoPos1 != std::string::npos)
        {
            Token.erase(quoPos1,1);
            quoPos1 = Token.find("\"");                              // .. which is usually in doule quotes
            if(quoPos1 != std::string::npos) Token.erase(quoPos1);
            ColumnList[wordCount] = Token; 
        }
        else                                                         // but maybe it won't be, so we should just make sure
        {
            quoPos1 = Token.find(" ");                              // .. which is usually in doule quotes
            if(quoPos1 != std::string::npos)
            {
               Token.erase(quoPos1,1);
               quoPos1 = Token.find(" ");                              // .. which is usually in doule quotes
               if(quoPos1 != std::string::npos) Token.erase(quoPos1);
            }
            ColumnList[wordCount] = Token; 
        }
        ValueList[wordCount] = std::string( inStr.substr(eqPos+1, (pos2-eqPos) -1 ));
        //-------------------------------
        // DEBUG
        //-------------------------------
        //cout << "[DEBUG 1] Adding Column |" << ColumnList[wordCount] << "|  and value " << ValueList[wordCount] << std::endl;
        //-------------------------------
        wordCount++;
        i = pos2+1;
     }
     //------------------------------------------------------------------------------------
     // PRE PREDICATE.. but possibly the last part before the 'where' condition
     //------------------------------------------------------------------------------------
     else
     {
       predSwitch = ON;                                              // set the switch, so we process the 'where' clause
       eqPos=inStr.find_first_of("=",i);
       Token = std::string(inStr.substr(i, eqPos-i));

       quoPos1 = Token.find_first_of("\"");
       if(quoPos1 != std::string::npos)
       {
          quoPos2 = Token.find_last_of("\"");
          ColumnList[wordCount] = std::string(Token.substr(quoPos1+1, (quoPos2 - quoPos1)-1)); 
       }
       else 
       {
          ColumnList[wordCount] = std::string(inStr.substr(i, eqPos-i));
       }
           
       quoPos1 = inStr.find_first_of("'", eqPos+1);
       if( (quoPos1 != std::string::npos) && (quoPos1 < wherePos) )
       {
          quoPos2 = inStr.find_first_of("'", quoPos1+1);
          ValueList[wordCount]  = std::string( inStr.substr(quoPos1, (quoPos2-quoPos1)+1 ));
       }
       else
       {
          ValueList[wordCount]  = std::string( inStr.substr(eqPos+2, wherePos-(eqPos+2) ));
       }
       //-------------------------------
       // DEBUG
       //-------------------------------
       //cout << "[DEBUG2] Adding Column |" << ColumnList[wordCount] << "|  and value " << ValueList[wordCount] << endl;
       //-------------------------------
       wordCount++;
       i=wherePos+6;                                                 // plus the length of the word ' where' 
     }                                                               // end else
   }                                                                 // end if predswitch
   //================================================================
   // We are now in the 'where' clause (aka predicate)
   // NOTE: in the predicate, name/value pairs are grouped by an '='
   //       but seperated by an 'AND'
   //================================================================
   else                                                                // we are in the predicate section
   {
     pos2=inStr.find(" and ",i);                                       // in the predicate, pairs are grouped by the 'and' keyword
     if( pos2 == std::string::npos )                                   // are we at the end of the line ?
     {
       pos2=len;                                                       // if so, set the pos2 to the end position
     }
        
     eqPos=inStr.find_first_of("=",i);                                 // Pairs are then grouped by an '=' sign or.
     if (eqPos > pos2)                                                 // sometimes they are paired with "IS NULL"
     {
        eqPos = inStr.find("IS NULL",i);                               // is it paired with "IS NULL"
        if(eqPos != std::string::npos)
        {
          eqPos++; 
          Token = "NULL"; 
        }
        else
        {
          return(-1);
        } 
     }
     else
     {
        Token = std::string(inStr.substr(i, (eqPos-i)-1));             // Column to the right of the equal sign.  
     }
     //--------------------------
     // STRIP THE DOUBLE QUOTES
     //--------------------------
     quoPos1 = Token.find_first_of("\"");                              // is this thing wrapped in double quotes ?
     if( quoPos1 != std::string::npos)                                 // yup.. we found double quotes
     {
           quoPos2 = Token.find_last_of("\"");                         // Find the last quote
           Token = std::string(Token.substr(quoPos1+1, (quoPos2 - quoPos1)-1)); 
     }
                                                                       // NO ELSE - Do nothing to the token
     
     //----------------------------------------------------------------
     // Some columns in the 'where' clause  are not always referenced
     // in the first part of an update/delete statement
     // example: update a set foo=a where bar=b. bar is before the 'where'
     // so we need to add any column that might be missing 
     //----------------------------------------------------------------
     foundFlag=0;
     for(idx = 0; idx < wordCount; idx++)                              // Looking to see if this column has already been added 
     {
        if( ColumnList[idx].compare(Token) == 0)
        {
           foundFlag=1;                                              // yea, it already has been referenced and added to the list
        } 
     }
     if(foundFlag == 0)                                                // nope.. new column found in the predicate
     {
           ColumnList[wordCount] = std::string(Token);
           Token   = inStr.substr(eqPos+1, pos2 - (eqPos + 1));      // column Value is to the right of the '=' sign
           quoPos1 = Token.find("'", 0);                             // look for a single quote
           pos1    = Token.find("(",0);                              // used to identify a possible function 
           if( quoPos1 != std::string::npos)                         // 
           {
             if(quoPos1 > pos1) 
             {
                quoPos1 = 0;
                quoPos2 = Token.find_last_of(")");
             }
             else
             { 
               quoPos2 = Token.find("'", quoPos1+1);
             }

              ValueList[wordCount]  = Token .substr(quoPos1, (quoPos1-quoPos2)+1);
           }
           else
           {
              quoPos1 = Token.find(" ");                             // let's get rid of the blanks  
              if(quoPos1 != std::string::npos)
              {
                 Token.erase(quoPos1,1);
                 quoPos1 = Token.find(" ");                          // This should ditch any trailing blanks  
                 if(quoPos1 != std::string::npos) Token.erase(quoPos1);
               }
              ValueList[wordCount]  = Token; 
           }
           //-------------------------------
           // DEBUG 
           //-------------------------------
           //cout << "[3] Adding Column |" << ColumnList[wordCount] << "| and value |" << ValueList[wordCount] << "|\n";
           //-------------------------------
           wordCount++;
     }
     i = pos2+4;                                                     //  the length of 'and' and the following blank = 4
   }
 }
 //----------------------
 // END PREDICATE <<<<<<<
 //---------------------------------------------------------------------------------------------------
 // NOW - Let's build our NEW INSERT sql statement 
 // >>>> NOTE <<<< Even though this is an UPDDATE 2 INSERT ROUTINE
 //                we are building a predicate (not used in an update statement) for the alternate update
 //                statement that will only have the UQ columns in the predicate
 //---------------------------------------------------------------------------------------------------
 int i2=0, commaFlag=OFF;
 timeStampIdx=0;

 int freshNewPredicateFlag=TRUE;                                    // first name/value pair does not have an "AND"
 for(i=0; i < wordCount; i++)                                       // for i < the number of words in our new list
 {
   if(ColumnList[i].compare("UPDATE_TIME")  == 0)                   // we have the update_time and now we know where it is
   {
     timeFlagFound=TRUE;
     timeStampIdx=i;
   }
   //--------------------------------------------------------------
   // BUILD NEW predicate in this ELSE statement
   //--------------------------------------------------------------
   else                                                             
   {
     //--------------------------------------------------
     // DEBUG 
     //--------------------------------------------------
     // std::cout << "[DEBUG] MORPHING: COLUMN -> " << ColumnList[i] << " VALUE -> " << ValueList[i] << std::endl;
     //--------------------------------------------------
     for(i2=0; i2 < (signed)indxColumnCount[indxListIndex]; i2++)              // For the number of columns in the UQ do
     {
        if( ColumnList[i].compare(indxColumnList[i2]) == 0)            // If this column is part of the UQ 
        {
           //--------------------------------------------------
           // DEBUG
           //std::cout << "[DEBUG] ADDING COLUMN TO NEW PREDICATE " << ColumnList[i] << std::endl;
           //--------------------------------------------------
           if(freshNewPredicateFlag == FALSE)                          // For the first pair, we don't have an 'and'
           {                                                           // .. (i.e set foo=bar WHERE a = b )
              predicateStr.append(" AND ");                            // otherwise we do have the 'AND'
           }
           else
           {
             freshNewPredicateFlag=FALSE;
           }
           if(ValueList[i].compare(" NULL")==0)                  // If the value in the INSERT is a NULL, the ansi standard 
           {                                                      // is to use 'IS NULL' (something can't be equal to null)
              predicateStr.append( ColumnList[i] + " IS " + ValueList[i]) + " "; 
           }
           else
           {
             predicateStr.append( ColumnList[i] + " = " + ValueList[i]) + " "; 
           }
        }
     }
   }

   //-----------------------------------------------------------------
   // INSERT STATEMENT ONLY
   //-----------------------------------------------------------------
   if(ValueList[i].compare("NULL") != 0)                             // Can't insert a null.., so ignore column
   {
      if(commaFlag == OFF)                                           // columns are seperated by commas
      {
         DMLstr.append(ColumnList[i]);
         commaFlag = ON;                                             // next time around, we'll end up using commas 
      }
      else
      {
         DMLstr.append("," + ColumnList[i]);
      }
   }
 }

 if(timeFlagFound == FALSE)
 {
   errMsg = std::string("Insert2update: Could not find 'UPDATE_TIME' column anywhere in the SQL");
   return(-1);
 }
 DMLstr.append(") values (");

 //---------------------------------------------------------------------------------------------------------------------
 // Now we are going to run through the list again, building the 'values' clause of the insert statement
 // (ie. insert into foo (a,b,c) VALUES (d,e,f) ) 
 //---------------------------------------------------------------------------------------------------------------------
 commaFlag=OFF;                                                      // values in the list are delimited by commas
 for(i=0; i < wordCount; i++)
 {
   if(i == timeStampIdx)
   {
     timeStampStr = std::string(ValueList[i]);
   }
   if(ValueList[i].compare("NULL") != 0)                             // Can't insert a null.. only can omit the column.. duhhh 
   {
      if(commaFlag == OFF)
      {
         DMLstr.append(ValueList[i]);
         commaFlag = ON;
      }
      else
      {
         DMLstr.append("," + ValueList[i]);
      }
   }
 }
 DMLstr.append(")");                                                 // ')' parenthesis the values clause

 return(1);
}

//=========================================================================================================
// Routine: insert2update
// purpose: To convert an insert statement to an update statement
// date:    Aug. 3, 2004
// Author:  MLV
// Return codes: 0 - Didn't do anything
//              -2 - Could not find update_time column
//              -3 - Could not find the table from the sql in the index list
//              -4 - Problem encountered when parsing the column list to populate the array used in creating the predicate 
//===========================================================================================================
int formatSQL::insert2update()
{
    int valuesSwitch=OFF;
    unsigned int  i=0, i2=0, pos1=0, pos2=0, rParenPos=0, lParenPos=0, quoPos1=0; 
    unsigned int  timeStampIdx=0, wordCount=0, valueWordPos=0, indxListIndex=0;
    std::string   Token, ColumnList[128], ValueList[128];

    errMsg.clear();
    DMLstr    = std::string("UPDATE ") + tableName  + " SET ";
    predicateStr = std::string(" WHERE ");

    //--------------------------------------------------------------------------
    // Need to search tablelist and index list to prepare to form the predicate
    //--------------------------------------------------------------------------
    indxListIndex=0;
    for(i=0; i <= indxListCounter; i++)
    {
       if(indxTableName[i].compare(tableName) ==0)                   // Comparing table names to the list we added in memory
       {
         indxListIndex=i;                                            // This points to the list of columns
         if (genPredicateList(indxListIndex) != 1)
         {
            return(-1);
         }
         break;
       }
    }

    if ( indxListIndex== 0)                                          // Does this table have an index associated with it
    {
      errMsg = std::string("insert2update: Could not find ") +  tableName + " in index list";
      return(-1);                                                   
    }
    //--------------------------------------------------------------------------------------------
    // in an insert statement, the columns being affected are wrapped in left/right parenthesis
    //--------------------------------------------------------------------------------------------

    lParenPos = inStr.find_first_of("(", currentPos);
    rParenPos = inStr.find_first_of(")", currentPos);

    currentPos = lParenPos +1;                                       // The for loop will start at this position. 
  
    //--------------------------------------------------------------------------------------------- 
    // The 'for' loop has two parts. One that processes the column list (ie. insert into foo (cola, colb, colc) )
    // where 'cola ... colc ' is considered the first part and the second part 'values (val1, val2, val3)'
    //--------------------------------------------------------------------------------------------- 
    for(i = currentPos; i < len; i++)
    {
        //----------------------------------------------------------------------------------------
        if(valuesSwitch == OFF)                                      // right now , let's process the column names
        {
           pos2=inStr.find_first_of(",",i);                          // column names are seperated by commas
           //----------------------------------------------------------------------------------
           if( ( pos2 != std::string::npos ) && (pos2 < rParenPos) ) // most likely the rParenPos condidition will be met
           {
             Token = inStr.substr(i, pos2 - i);                      // Most of the time, columns will be wrapped in double quotes
             quoPos1 = Token.find("\"");
             if(quoPos1 != std::string::npos)
             {
                Token.erase(quoPos1,1);
                quoPos1 = Token.find("\"");                          // .. which is usually in doule quotes
                if(quoPos1 != std::string::npos) Token.erase(quoPos1);
                quoPos1 = Token.find(" ");                          // .. which is usually in doule quotes
                if(quoPos1 != std::string::npos) Token.erase(quoPos1,1);
                quoPos1 = Token.find(" ");                          // .. which is usually in doule quotes
                if(quoPos1 != std::string::npos) Token.erase(quoPos1);
             }
             ColumnList[wordCount] = Token;
             wordCount++;
             i=pos2;
           }
           //----------------------------------------------------------------------------------
           else                                                      // ok.. either a problem with the sql or we are at  last col
           {
             if(pos2 > rParenPos)                                    // We'll have to process the last column 
             {
                 pos2 = rParenPos;                          
                 Token = inStr.substr(i, pos2 - i);
                 quoPos1 = Token.find("\"");
                 if(quoPos1 != std::string::npos)
                 {
                    Token.erase(quoPos1,1);
                    quoPos1 = Token.find("\"");                          // .. which is usually in doule quotes
                    if(quoPos1 != std::string::npos) Token.erase(quoPos1);
                    quoPos1 = Token.find(" ");                          // .. which is usually in doule quotes
                    if(quoPos1 != std::string::npos) Token.erase(quoPos1,1);
                    quoPos1 = Token.find(" ");                          // .. which is usually in doule quotes
                    if(quoPos1 != std::string::npos) Token.erase(quoPos1);
                 }
                 ColumnList[wordCount] = std::string(Token);
                 wordCount++;
                 i=pos2;

                 pos2 = inStr.find("values");                        // Now let's get it ready for the 'values' section
                 if(pos2 != std::string::npos)
                 {
                    i = pos2+6;                                      //
                    rParenPos = inStr.find_last_of(")");             // Reset the right parenthesis to the last paren 
                 }
                 else
                 {
                    errMsg = std::string("Replication found SQL syntax error in the 'values' portion of the SQL");
                    return(-1);
                 }
                 pos2 = inStr.find_first_of("(",i);                 // value are also bounded by parenthesis.. be careful
                 if(pos2 != std::string::npos)                      // .. functions have parenthesis too !
                 {
                    i = pos2;                 
                    valuesSwitch=ON;
                 }
                 else
                 {
                    errMsg = std::string("Replication found SQL syntax error in the 'values' portion of the SQL");
                    return(-1);
                 }
             }
             else
             {
                 errMsg = std::string("Replication is encountering a non-expected error");
                 return(-1);
             }
           }
        }
        //----------------------------------------------------------------------------------------
        // Everything in this section is the values clause
        //----------------------------------------------------------------------------------------
        else                                                  // we are processing the values area
        {
           pos1 = inStr.find_first_of("(", i);                // does a right parenthesis exist
           pos2 = inStr.find_first_of(",", i);                // and look for the next comma
           if( ( pos2 != std::string::npos ) && (pos2 < rParenPos) )  // is there really a comma ?
           {
             if (pos2 > pos1)                                         // theres a left parenthesis before the comman . Function ?
             {
                pos2 = inStr.find_first_of(",", pos2+1);              // there's a function going on, so pass by the first ','
                if(pos2 == std::string::npos) 
                {
                  pos2 = inStr.find_first_of(")", pos1); // if we are at the end, go back a little
                  if(pos2 != std::string::npos) pos2++;
                }
             
                Token = inStr.substr(i, pos2 - i );
             }
             Token = inStr.substr(i, pos2 - i );
             ValueList[valueWordPos] = std::string(Token);
             i=pos2;
           }
           else
           {
               pos2 = rParenPos;                          
               Token = inStr.substr(i, rParenPos - i);
               ValueList[valueWordPos] = std::string(Token);
               i=pos2;
           }
           valueWordPos++;
        }
    }
    //----------------------------------------------------------------------------------------
    // Build the SQL
    // This is the section where all our SQL and PREDICATE is built.
    //----------------------------------------------------------------------------------------
    for(i=0; i < wordCount; i++)
    {
      if( ColumnList[i].compare("UPDATE_TIME") == 0)             // Was one of our tokens the value 'UPDATE_TIME'
      {
         timeStampIdx=i;
         timeStampStr = std::string(ValueList[i]);               // if so, then we need the value to compare against the DB
      }
      else
      {                                                          // PREIDCATE BUILD BEING DONE IN THE ELSE STATEMENT 
        for(i2=0; i2 < indxColumnCount[indxListIndex]; i2++)     // For the number of columns in the UQ do
        {
          if( ColumnList[i].compare(indxColumnList[i2]) == 0)    // If this column is part of the UQ 
          {
             if(i2 != 0)                                         // For the first  coupled pair, we don't have an 'and'
             {
               predicateStr.append(" AND ");                     // otherwise we do have the 'AND'
             }
             if(ValueList[i].compare("NULL")==0)                 // If the value in the INSERT is a NULL, the ansi standard 
             {                                                   // is to use 'IS NULL' (something can't be equal to null)
               predicateStr.append( ColumnList[i] + " IS " + ValueList[i]); 
             }
             else
             {
               predicateStr.append( ColumnList[i] + " = " + ValueList[i]); 
             }
          }
        }
      }
      if(i == 0)
      {
        DMLstr.append( ColumnList[i] + " = " + ValueList[i]);
      }
      else
      {
        DMLstr.append( "," + ColumnList[i] + " = " + ValueList[i]);
      }
    }
    DMLstr.append(predicateStr);                                 // Putting all the pieces of the string together
  
    if(timeStampIdx == 0)                                        // Never did find update_time.. which means a problem
    {
      errMsg = std::string("insert2update: Could not find 'UPDATE_TIME' column referenced anywhere in the DML");
      return(-1);
    } 
    return(1);
}
//=========================================================================================================
// Routine: update2update 
// purpose: Sets the DML string to a truncated version of itself, only incorporating the columns of an unique index 
// date:    Aug. 3, 2004
// Author:  MLV
// Returns: 
//         -1 - an error occured
//          1 - Everything is fine 
// NOTE:    update2insert must have already ran
//===========================================================================================================
int formatSQL::update2update()
{
  unsigned int wherePos=0;
  errMsg.clear();

  wherePos   = inStr.find(" where ", currentPos);
  if(wherePos == std::string::npos)
  {
    wherePos = inStr.find(" WHERE ", currentPos);
    if( wherePos == std::string::npos)
    {
     errMsg = std::string("update2update: Could not find the begining of the predicate");
     return(-1);
    }
  }
  DMLstr = std::string(inStr.substr(0,wherePos)) + predicateStr;
  return(1);
}
//=========================================================================================================
// Routine: delete2delete 
// purpose: Sets the DML string to a truncated version of itself, only incorporating the columns of an unique index 
// date:    Aug. 9, 2004
// Author:  MLV
// Returns: 
//         -4 - could not find '=' or 'IS' in processing predicate pairs
//         -3 - could not find index in the list 
//         -2 - Could not find update_time column
//          1 - Everything is fine 
// NOTE:    update2insert must have already ran
//===========================================================================================================
int formatSQL::delete2delete()
{
    int foundFlag=FALSE;
    unsigned int i, i2=0, idx, wordCount=0, offSet;
    unsigned int wherePos=0, eqPos=0, quoPos1, quoPos2, pos1, pos2, timeStampIdx=0, indxListIndex=0;
    std::string Token, ColumnList[128], ValueList[128];

    DMLstr    = std::string("DELETE FROM ") + tableName + " ";
    predicateStr = std::string(" WHERE ");

    //--------------------------------------------------------------------------
    wherePos  = inStr.find(" where ", 0);
    if(wherePos == std::string::npos)
    {
       wherePos = inStr.find(" WHERE ", 0);
       if( wherePos == std::string::npos)
       {
          errMsg = std::string("delete2delete: Could not find the begining of the predicate");
          return(-1);
       }
    }
    currentPos = wherePos + 7;
 
    //--------------------------------------------------------------------------
    // Need to search tablelist and index list to prepare to form the predicate
    //--------------------------------------------------------------------------
    indxListIndex=0;
    for(i=0; i <= indxListCounter; i++)
    {
       if(indxTableName[i].compare(tableName) ==0)               // Comparing table names to the list we added in memory
       {
           indxListIndex=i;                                      // This points to the stringed list of columns
           if (genPredicateList(indxListIndex) != 1)
           {
              return(-1);
           }
           break;
       }
    }
    //--------------------------------------------------------------------------
    if ( indxListIndex== 0)
    {
       errMsg = std::string("delete2delete: Could not find ") +  tableName +" in index list";
       return(-1);
    }
    //--------------------------------------------------------------------------
    for(i = currentPos; i < len; i++)
    {
       pos2=inStr.find(" and ",i);                               // 'and' is what seperates each pair from another pair
       if( pos2 == std::string::npos )                           // if there is no more 'and', then we must be at the last pair
       {
         pos2=len;
       }
       
       offSet=2;                                                 // when parsing the right hand side, we will need to know this
       eqPos=inStr.find_first_of("=",i);                         // usually pairs are assigned with '=' ...
       if((eqPos == std::string::npos) || (eqPos > pos2)) 
       {
         eqPos=inStr.find("IS",i);                               // .. and sometimes they are assigned with 'IS'
         if(eqPos == std::string::npos)                          // .. but this situation should never occur
         {
           errMsg = std::string("delete2delete: error parsing DML while looking for '=' or 'IS'");
           return(-1);
         }
         eqPos++;
         offSet=3;
       }
       Token = std::string(inStr.substr(i, (eqPos-i)-1));                 // The left hand side of the pair is the column name 
       quoPos1 = Token.find_first_of("\"");                               // and most times the column is wrapped in double quotes
                                                                          // stripping the quotes makes it easier to search for idx 
       if(( quoPos1 != std::string::npos) && (quoPos1 < pos2))
       {
           quoPos2 = Token.find_last_of("\"");
           if(quoPos2 > pos2)                                             // for some reason, there's a problem with the quotes
           {
             errMsg = std::string("delete2delete: error in parsing DML - unmatched quotes :") + Token; 
             return(-1);  
           }
           Token = std::string(Token.substr(quoPos1+1, (quoPos2 - quoPos1)-1)); 
       }
     //------------
     //else { } - No Else Statement - If there isn't quotes, we won't do any stripping
     //------------
       foundFlag=0;
       for(idx = 0; idx < wordCount; idx++)                               // This really kills.. I really don't like looping
       {
          if( ColumnList[idx].compare(Token) == 0)
          {
             foundFlag=1;
          } 
       }
       //----------------------------------------
     if(foundFlag == 0)                                              // nope.. new column found in the predicate
     {
           ColumnList[wordCount] = std::string(Token);
           Token   = inStr.substr(eqPos+1, pos2 - (eqPos + 1));      // column Value is to the right of the '=' sign
           quoPos1 = Token.find("'", 0);                             // look for a single quote
           pos1    = Token.find("(",0);                              // used to identify a possible function 
           if( quoPos1 != std::string::npos)                         // there is a single quote that denotes a char value 
           {
             if(quoPos1 > pos1) 
             {
                quoPos1 = 0;
                quoPos2 = Token.find_last_of(")");
             }
             else
             { 
               quoPos2 = Token.find("'", quoPos1+1);
             }

              ValueList[wordCount]  = Token .substr(quoPos1, (quoPos1-quoPos2)+1);
           }
           else
           {
              quoPos1 = Token.find(" ");                              // let's get rid of the blanks  
              if(quoPos1 != std::string::npos)
              {
                 Token.erase(quoPos1,1);
                 quoPos1 = Token.find(" ");                          // This should ditch any trailing blanks  
                 if(quoPos1 != std::string::npos) Token.erase(quoPos1);
               }
              ValueList[wordCount]  = Token; 
           }
           //-------------------------------
           // DEBUG 
           //-------------------------------
           //cout << "[DEBUG3] Adding Column |" << ColumnList[wordCount] << "|  and value |" << ValueList[wordCount] << "|\n";
           //-------------------------------
           wordCount++;
      }
      i = pos2+4;
    }

    timeStampIdx=0;
    //----------------------------------------------------------------------------------------
    // Build the SQL
    // This is the section where all our SQL and PREDICATE is built.
    //----------------------------------------------------------------------------------------
    for(i=0; i < wordCount; i++)
    {
      if( ColumnList[i].compare("UPDATE_TIME") == 0)               // Was one of our tokens the value 'UPDATE_TIME'
      {
         timeStampIdx=i;
         timeStampStr = std::string(ValueList[i]);               // if so, then we need the value to compare against the DB
      }
      else
      {                                                          // PREIDCATE BUILD BEING DONE IN THE ELSE STATEMENT 
        for(i2=0; i2 < indxColumnCount[indxListIndex]; i2++)     // For the number of columns in the UQ do
        {
          if( ColumnList[i].compare(indxColumnList[i2]) == 0)    // If this column is part of the UQ 
          {
             if(i2 != 0)                                         // For the first  coupled pair, we don't have an 'and'
             {
               predicateStr.append(" AND ");                     // otherwise we do have the 'AND'
             }
             if( (ValueList[i].compare("NULL")==0) || (ValueList[i].compare("NOT NULL")==0) )  
             {                                                   // is to use 'IS NULL' (something can't be equal to null)
               predicateStr.append( ColumnList[i] + " IS " + ValueList[i]); 
             }
             else
             {
               predicateStr.append( ColumnList[i] + " = " + ValueList[i]); 
             }
          }
        }
      }
    }
    DMLstr.append(predicateStr);                                 // Putting all the pieces of the string together

    if(timeStampIdx == 0)                                        // Never did find update_time.. which means a problem
    {
      errMsg = std::string("delete2delete: could not find 'UPDATE_TIME' column in the DML");
      return(-1);
    }
    return(1);
}
//=========================================================================================================
// Routine: sqlString 
// purpose: return the entire NEW SQL string 
// date:    Aug. 3, 2004
// Author:  MLV
// Returns: NULL or the SQL
//===========================================================================================================
char* formatSQL::sqlString()
{
  if(DMLstr.size() == 0) return(NULL);
  return((char*) DMLstr.c_str());
}

//=========================================================================================================
// Routine: predicateString 
// purpose: return the newly created predicate string 
// date:    Aug. 3, 2004
// Author:  MLV
// Returns: NULL or the SQL
//===========================================================================================================
char* formatSQL::predicateString()
{
  if(predicateStr.size() == 0) return(NULL);
  return((char*) predicateStr.c_str());
}
//=========================================================================================================
// Routine: tableName 
// purpose: return the tableName from the DML string 
// date:    Aug. 3, 2004
// Author:  MLV
// Returns: NULL or the SQL
//===========================================================================================================
char* formatSQL::tableNameString()
{
  if(strlen(tableName)==0) return(NULL);
  return((char*) tableName);
}

//=========================================================================================================
// Routine: timeStampString 
// purpose: return the timeStamp from the DML string 
// date:    Aug. 3, 2004
// Author:  MLV
// Returns: NULL or the SQL
//===========================================================================================================
char* formatSQL::timeStampString()
{
  if(timeStampStr.size() == 0) return(NULL);
  return((char*) timeStampStr.c_str());
}
//=========================================================================================================
// Routine: errorMessage 
// purpose: return the errorMessage if any 
// date:    Aug. 9, 2004
// Author:  MLV
// Returns: NULL or the error message 
//===========================================================================================================
char* formatSQL::errorMessage()
{
  if(errMsg.size() == 0) return(NULL);
  return((char*) errMsg.c_str());
  return(NULL);
}
