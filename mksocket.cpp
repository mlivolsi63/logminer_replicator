#include "mksocket.h"

//-----------------------------------------------------------------------------
int MakeSocket::passiveTCP(char *service, int qlen)
{
  return passivesock(service, "tcp", qlen);
}

//-----------------------------------------------------------------------------
// The following routine comes from the following:
// "Internetworking with TCP/IP" Volume III Copyright 1993 - pp. 117-118
//-----------------------------------------------------------------------------
int MakeSocket::passivesock(char *service, char *protocol, int qlen)
{
 struct servent *pse;
 struct protoent *ppe;

 struct sockaddr_in sin;

 int s, type;

 bzero(( char *) &sin, sizeof(sin));
 sin.sin_family = AF_INET;
 sin.sin_addr.s_addr = INADDR_ANY;

 pse = getservbyname(service, protocol);
 if( pse != 0 )
 {
    sin.sin_port = htons(ntohs (( u_short)pse->s_port) + portbase);
 }
 else 
 {
     if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )      // if argument was specified as a port
     {
        std::cerr << "\t\tCannot get " << service << " service entry\n";
        exit(1);
     }
 }
 if ( (ppe = getprotobyname(protocol)) == 0)
 {
   std::cerr << "\t\tCannot get " << protocol << "protocol entry \n";
   exit(1);
 }


 if(strcmp(protocol, "udp") == 0 )
    type = SOCK_DGRAM;
 else
    type = SOCK_STREAM;

 s = socket(PF_INET, type, ppe->p_proto);
 if(s < 0)
 {
  std::cerr << "[E] Can't create a socket\n" << s;
  exit(1);
 }

 if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0 )
 { 
   std::cerr << "[E] Can't bind to port " << service << std::endl;
   exit(1);
 }

 if(type == SOCK_STREAM && listen(s, qlen) < 0)
 {
  std::cerr << "[E] Can't listen on port " << service << std::endl;
  exit(1);
 }

 //  << "[I] Listening on port " << sin.sin_port << endl;
 
 fflush(NULL);

 return s;
}

