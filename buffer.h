#ifndef buffer_H //防止多次引入
#define buffer_H

#include "bPlusTree.h"

#define BUFFER_MAX 10

typedef struct _FreeBlockList_{
    off_t FreeBlockPos;
    struct _FreeBlockList_* next;
}FreeBlockList;

typedef struct _BufferPool_{
    int BufferLen;  
    struct _Buffer_* BufferHead;
    struct _Buffer_* BufferTail;
}BufferPool;

typedef struct _Buffer_{
    bPlusTreePtr ptr;
    off_t BlockOffset;
    //bool isWritten;  //记录节点是否被修改，如果修改需要回写
    struct _Buffer_* next;
}Buffer;

BufferPool BfPool; //初始化缓冲池,缓冲池长度后面再调整

void deleteFromBuffer(off_t deleteNode);
void addIntoFreeBlockList(bPlusTreePtr node,int ptrIndex);
off_t getFreeBlockOffset();
off_t addIntoBuffer(bPlusTreePtr nodePtr,off_t BlockOffset);
bPlusTreePtr ReadIntoBuffer(off_t blkOffset);
void WriteBackToDisk(Buffer* tmp);

#endif
