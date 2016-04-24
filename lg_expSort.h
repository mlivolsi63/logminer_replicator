const int FILE_LIMIT=10000;
const int NAME_LN=32;

int Compare(const void *p1, const void *p2);

class FileList
{
 private:
        char name[FILE_LIMIT][NAME_LN];
        int  fileIndex;
 public:
	FileList();
        void    initialize();
	int 	setName(char ArgFileName[]);
	void 	sortList();
	char* 	getName();
	char* 	currentName();
};
