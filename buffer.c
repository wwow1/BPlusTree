#include "buffer.h"

HeaderPage* head = NULL; //"声明"变量
bPlusTreePtr root = NULL;
FILE* fp = NULL;
FILE* dt = NULL;
/*
 * deleteFromBuffer - 从缓存池中删除节点
 */
void deleteFromBuffer(off_t deleteNode){  //从缓冲池中删除节点
    Buffer* tmp=BfPool.BufferHead;
    Buffer* pre=NULL;
    while(tmp!=NULL){
        if(tmp->BlockOffset==deleteNode){
            if(tmp==BfPool.BufferHead)
                BfPool.BufferHead=tmp->next;
            else
                pre->next=tmp->next;
            if(tmp==BfPool.BufferTail){
                BfPool.BufferTail=pre;
            }
            free(tmp);
            BfPool.BufferLen--;
            return;
        }
        tmp=tmp->next;
        if(pre==NULL)
            pre=BfPool.BufferHead;
        else
            pre=pre->next;
    }
}

/*
 * addIntoFreeBlockList - 将删除的节点加入空闲块链表，便于复用它们占用的磁盘空间
 */
extern void addIntoFreeBlockList(bPlusTreePtr node,int ptrIndex){  //将删除的节点加入空块链表，便于复用它们占用的磁盘空间
    if(node->isLeaf)    return; //不用管叶节点的子索引，因为它不是节点
    FreeBlockList* tmp =(FreeBlockList*)malloc(sizeof(FreeBlockList));
    if(ptrIndex==-1)    tmp->FreeBlockPos=node->me;
    else
        tmp->FreeBlockPos=node->Children[ptrIndex];
    if(head->fl==NULL)
        tmp->next=NULL;
    else
        tmp->next=head->fl;
    head->fl=tmp;
    deleteFromBuffer(tmp->FreeBlockPos);
}

/*
 * getFreeBlockOffset - 为新建的节点分配磁盘空间，应该尽可能分配空闲节点的位置给它，减少空洞
 * 如果空闲链表为空，则分配新的文件偏移量给它（相当于添加在当前文件末尾）
 */
extern off_t getFreeBlockOffset(){  //为新建的节点分配磁盘空间
    //应该尽可能分配空闲节点的位置给它，减少空洞
    off_t seekPos;
    if(head->FreeBlockNum==0){
        seekPos=sizeof(HeaderPage)+(head->NodeNum*head->BlockSize);
        head->NodeNum++;
    }else{
        seekPos=head->fl->FreeBlockPos;
        FreeBlockList* tmp=head->fl;
        head->fl=head->fl->next;
        free(tmp);
        head->FreeBlockNum--;
    }
    return seekPos;
}

/*
 * addIntoBuffer - 将新建的节点和读入的节点放入缓冲池中，最后程序结束时，通过
 * 遍历缓冲池来将这些数据写回文件
 */
off_t addIntoBuffer(bPlusTreePtr nodePtr,off_t BlockOffset){ 
    //将新建的节点和读入的节点放入缓冲池中
    if(BlockOffset==-1){
        BlockOffset=getFreeBlockOffset();
    }
    Buffer* tmp=(Buffer*)malloc(sizeof(Buffer));
    tmp->BlockOffset=BlockOffset;
    tmp->ptr=nodePtr;
    if(BfPool.BufferLen==0){
        BfPool.BufferHead=tmp;
        BfPool.BufferTail=tmp;
        tmp->next=NULL;
        BfPool.BufferLen++;
    } else {
        tmp->next=(BfPool.BufferTail)->next;
        (BfPool.BufferTail)->next=tmp;
        BfPool.BufferTail=tmp;
        BfPool.BufferLen++;
    }
    /*
    if (BfPool.BufferLen >= BUFFER_MAX) {
        //缓冲区大小不足，使用FIFO方式淘汰链首节点
        Buffer* eliminated = BfPool.BufferHead;
        if (eliminated->ptr->isWrite) 
            WriteBackToDisk(eliminated);
        BfPool.BufferHead = eliminated->next;
        free(eliminated);
    }*/
    return BlockOffset;
}

/*
 * ReadIntoBuffer - 根据偏移量将节点从文件读入缓存池
 */
bPlusTreePtr ReadIntoBuffer(off_t blkOffset){
    //从磁盘读出节点
    fseek(fp,blkOffset,SEEK_SET);
    bPlusTreePtr tmp=(bPlusTreePtr)malloc(sizeof(bPlusTreeNode));
    fread(tmp,sizeof(bPlusTreeNode),1,fp);
    //读入节点信息到缓冲池的tail处
    addIntoBuffer(tmp,blkOffset);
    return tmp;
}

/*
 * WriteBackToDisk - 将缓存池中的一个节点写回文件（磁盘）
 */
void WriteBackToDisk(Buffer* tmp){ 
    off_t seekPos=tmp->BlockOffset;
    tmp->ptr->isWrite = false;   //重置脏位
    //将缓冲池的某个节点写出到磁盘
    fseek(fp,seekPos,SEEK_SET);
    size_t t=fwrite(tmp->ptr,sizeof(bPlusTreeNode),1,fp);
}