class MakeSocket
{
  private:
  public:
     int passiveTCP(char *service, int qlen);
     int passivesock(char *service, char *protocol, int qlen);
};
