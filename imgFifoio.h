// Functions prototypes
int CreateFifo(char *fifoname);
int DeleteFifo(char *fifoname);
int OpenFifo(char *fifoname);
int CloseFifo(int fd_fifo);
int LinkFifo(int fd_fifo, GIOChannel *gch, GIOFunc func, gpointer user_data);
int UnLinkFifo(int tag, GIOChannel *gch);
