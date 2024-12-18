// XQueue.h  Version 1.2
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// This software is released into the public domain.  You are free to use it
// in any way you like, except that you may not sell this source code.
//
// This software is provided "as is" with no expressed or implied warranty.
// I accept no liability for any damage or loss of business that this software
// may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XQUEUE_H
#define XQUEUE_H

#include "XMemmap.h"

#pragma pack(push, XQUEUE_INCLUDE_PACK, 1)


///////////////////////////////////////////////////////////////////////////////
// sizes, etc.
#define XQUEUE_DEFAULT_QUEUE_SIZE    (1  * 1024)            // 1K blocks
#define XQUEUE_MAX_QUEUE_SIZE        (64 * 1024)            // 64K blocks
#define XQUEUE_MIN_QUEUE_SIZE        (64)                   // 64 blocks
#define XQUEUE_DEFAULT_BLOCK_SIZE    (1024)                 // 1024 bytes per block
#define XQUEUE_MIN_BLOCK_SIZE        (128)                  // 128 bytes
#define XQUEUE_DATA_VARIABLE_SIZE    (6 * sizeof(DWORD))    // no. of DWORD variables in a data block
#define XQUEUE_DEFAULT_DATA_SIZE     (XQUEUE_DEFAULT_BLOCK_SIZE - XQUEUE_DATA_VARIABLE_SIZE)
#define XQUEUE_VARIABLE_BLOCK_NO     (0)                    // first block is used for queue variables
#define XQUEUE_FIRST_DATA_BLOCK_NO   (XQUEUE_VARIABLE_BLOCK_NO + 1)    // first data block
#define XQUEUE_INVALID_BLOCK         (-1)
#define XQUEUE_CONTINUATION_FLAG     0x80000000             // high bit of DataSize
                                                            // member - TRUE = ForwardLink
                                                            // is a continuation block


///////////////////////////////////////////////////////////////////////////////
// XQueue error codes
#define XQUEUE_ERROR_SUCCESS        0
#define XQUEUE_ERROR_NOT_OPEN       1
#define XQUEUE_ERROR_TOO_BIG        2
#define XQUEUE_ERROR_INTERNAL       3
#define XQUEUE_ERROR_NO_FREE        4
#define XQUEUE_ERROR_NO_DATA        5
#define XQUEUE_ERROR_MORE_DATA      6
#define XQUEUE_ERROR_TIMEOUT        7
#define XQUEUE_ERROR_PARAMETER      8
#define XQUEUE_ERROR_OPEN_FAILED    9
#define XQUEUE_ERROR_CREATE_FAILED  10
#define XQUEUE_ERROR_NO_SERVER      11


///////////////////////////////////////////////////////////////////////////////
// XQUEUE_VARIABLE_BLOCK - mmf block for variables shared by clients & server
struct XQUEUE_VARIABLE_BLOCK
{
    // ctor
    XQUEUE_VARIABLE_BLOCK()
    {
        Init();
    }

    void Init()
    {
        BlockSize           = 0;
        DataSize            = 0;
        NoBlocks            = 0;
        NoDataBlocks        = 0;
        FreeDataBlocks      = 0;
        UsedDataBlocks      = 0;
        NoClients           = 0;
        NoServers           = 0;
        ServerTID           = 0;
        ServerHwnd          = 0;
        NoWrites            = 0;
        NoReads             = 0;
        MaxDataSize         = 0;
        MessageId           = 0;
        FreeListForwardLink = 0;
        FreeListBackLink    = 0;
        UsedListForwardLink = 0;
        UsedListBackLink    = 0;
    }

    void Dump()
    {
#if _DEBUG_XQUEUE
        TRACE(_T("XQueue variable block:\n"));
        TRACE(_T("    BlockSize=%d\n"),           BlockSize);
        TRACE(_T("    DataSize=%d\n"),            DataSize);
        TRACE(_T("    NoBlocks=%d\n"),            NoBlocks);
        TRACE(_T("    NoDataBlocks=%d\n"),        NoDataBlocks);
        TRACE(_T("    FreeDataBlocks=%d\n"),      FreeDataBlocks);
        TRACE(_T("    UsedDataBlocks=%d\n"),      UsedDataBlocks);
        TRACE(_T("    NoClients=%d\n"),           NoClients);
        TRACE(_T("    NoServers=%d\n"),           NoServers);
        TRACE(_T("    ServerThreadId=0x%X\n"),    ServerTID);
        TRACE(_T("    ServerHwnd=0x%X\n"),        ServerHwnd);
        TRACE(_T("    NoWrites=%d\n"),            NoWrites);
        TRACE(_T("    NoReads=%d\n"),             NoReads);
        TRACE(_T("    MaxDataSize=%d\n"),         MaxDataSize);
        TRACE(_T("    MessageId=%d\n"),           MessageId);
        TRACE(_T("    FreeListForwardLink=%d\n"), FreeListForwardLink);
        TRACE(_T("    FreeListBackLink=%d\n"),    FreeListBackLink);
        TRACE(_T("    UsedListForwardLink=%d\n"), UsedListForwardLink);
        TRACE(_T("    UsedListBackLink=%d\n"),    UsedListBackLink);
#endif
    }

    DWORD BlockSize;                // block size (set by the server, checked
                                    // by the client to ensure consistency)
    DWORD DataSize;                 // data size = BlockSize - XQUEUE_DATA_VARIABLE_SIZE
    DWORD NoBlocks;                 // total no. of blocks =
                                    //     no. of data blocks +
                                    //     1  (XQueue variable block)
    DWORD NoDataBlocks;             // total no. of data blocks
    DWORD FreeDataBlocks;           // no. of free data blocks
    DWORD UsedDataBlocks;           // no. of used data blocks
    DWORD NoClients;                // no. of active clients
    DWORD NoServers;                // no. of active servers
    DWORD ServerTID;                // thread id of server    
    DWORD NoWrites;                 // no. of calls to Write() (by all clients)
    DWORD NoReads;                  // no. of calls to Read() (by server)
    DWORD MaxDataSize;              // size in bytes of largest write - statistic
                                    // that is updated during processing
    DWORD MessageId;                // id for this message (1st message = 1)
    DWORD FreeListForwardLink;      // list head for free list
    DWORD FreeListBackLink;         //
    DWORD UsedListForwardLink;      // list head for used list
    DWORD UsedListBackLink;         //

    HWND  ServerHwnd;               // handle of server's main window (if any)
};


///////////////////////////////////////////////////////////////////////////////
// XQUEUE_DATA_BLOCK - struct that defines contents of data blocks in mmf
struct XQUEUE_DATA_BLOCK
{
    // ctor
    XQUEUE_DATA_BLOCK()
    {
        Init();
    }

    void Init()
    {
        BlockNo     = XQUEUE_INVALID_BLOCK;
        BackLink    = XQUEUE_INVALID_BLOCK;
        ForwardLink = XQUEUE_INVALID_BLOCK;
        ThreadId    = 0;
        DataSize    = 0;
        MessageId   = 0;
        ZeroMemory(Data, XQUEUE_DEFAULT_DATA_SIZE);
    }

    void Dump()
    {
#if _DEBUG_XQUEUE
        TRACE(_T("XQueue data block:\n"));
        TRACE(_T("    BlockNo=%d\n"),     BlockNo);
        TRACE(_T("    BackLink=%d\n"),    BackLink);
        TRACE(_T("    ForwardLink=%d\n"), ForwardLink);
        TRACE(_T("    ThreadId=0x%X\n"),  ThreadId);
        TRACE(_T("    DataSize=%d\n"),    DataSize);
        TRACE(_T("    MessageId=%d\n"),   MessageId);
        TCHAR buf[100] = { 0 };
        int n = 0;
        for (int i = 0; i < 30; i++)
        {
            n += _stprintf_s(buf, 100, _T("%02X "), Data[i]);
        }
        TRACE(_T("    Data=<%s ...>\n"), buf);
#endif
    }

    // if you add members to this struct, be sure to update
    // the definition of XQUEUE_DATA_VARIABLE_SIZE

    DWORD BlockNo;                  // this block no.
    DWORD BackLink;                 // XQUEUE_INVALID_BLOCK = this is first block
    DWORD ForwardLink;              // XQUEUE_INVALID_BLOCK = this is last block
    DWORD ThreadId;                 // thread id of client
    DWORD DataSize;                 // no. of data bytes in this block;
                                    // bit 31 = 1 if data is continued in
                                    // next block
    DWORD MessageId;                // message id for this message;
                                    // continuation blocks have same id
                                    // as first block
    BYTE   Data[XQUEUE_DEFAULT_DATA_SIZE];    // data for this block -
                                    // note:  this array may be larger or smaller
                                    // than XQUEUE_DEFAULT_DATA_SIZE;  actual size
                                    // is given by
                                    //     m_nBlockSize - XQUEUE_DATA_VARIABLE_SIZE
};


///////////////////////////////////////////////////////////////////////////////
// CXQueue - definition of CXQueue object
class CXQueue
{
// Construction / Deconstruction
public:
    CXQueue();
    virtual ~CXQueue();

// Attributes
public:
    DWORD   GetBlockSize() const { return IsOpen() ? m_nBlockSize : 0; }
    DWORD   GetQueueSize() const { return IsOpen() ? m_nQueueSize : 0; }    
    BOOL    IsOpen() const { return m_MMF.IsOpen(); }
    XQUEUE_VARIABLE_BLOCK * GetVariableBlock() const { return m_pVariableBlock; }
// Operations
public:
    LONG    Close();
    LONG    Create(LPCTSTR lpszQueueName, DWORD nQueueSize = XQUEUE_DEFAULT_QUEUE_SIZE, DWORD nBlockSize = XQUEUE_DEFAULT_BLOCK_SIZE);
    void    Dump();
    LONG    Open(LPCTSTR lpszQueueName);
    LONG    Read(LPBYTE lpBuf, DWORD * lpcbSize, DWORD * lpnMsgId, DWORD * pdwTID, DWORD dwTimeout = INFINITE);
    LONG    WaitForQueueWrite(DWORD dwTimeOut);
    LONG    Write(LPBYTE lpData, DWORD nDataSize, DWORD dwTimeOut = INFINITE);
    LONG    Write(LPCTSTR lpszString, DWORD dwTimeout = INFINITE);

// Implementation
private:
    void    Add(DWORD &ForwardLink, DWORD &BackLink, DWORD &BlockCount, XQUEUE_DATA_BLOCK * pXDB);
    void    AddFree(XQUEUE_DATA_BLOCK * pXDB);
    void    AddUsed(XQUEUE_DATA_BLOCK * pXDB);
    DWORD   CalculateNoBlocks(DWORD nDataBytes);
    LONG    CreateSyncObjects();
    DWORD   GetEntrySize(XQUEUE_DATA_BLOCK *pXDB);    
    LONG    Lock(DWORD dwTimeOut);
    LONG    OpenSyncObjects();
    LONG    ReadMMF(LPBYTE lpData, DWORD * lpcbSize, DWORD dwTimeOut);
    LONG    ReleaseEntry();
    LONG    SetQueueAvailable();
    LONG    Unlock();
    LONG    WaitForQueueIdle(DWORD dwTimeOut);
    LONG    WriteToMMF(LPBYTE lpData, DWORD nDataSize);
    XQUEUE_DATA_BLOCK * GetBlock(DWORD &ForwardLink, DWORD &BackLink, DWORD &BlockCount);
    XQUEUE_DATA_BLOCK * GetFreeBlock();
    XQUEUE_DATA_BLOCK * GetUsedBlock();
    ///////////////////////////////////////////////////////////////////////////
    // data members

    CXMemMapFile m_MMF;                         // memory mapped file object
    HANDLE    m_hWriteMutex;                    // handle for global write mutex -
                                                // set to nonsignaled when a
                                                // client or server is writing
    HANDLE    m_hClientWriteEvent;              // handle for client write block
                                                // event - set to nonsignaled
                                                // to block clients from writing
                                                // (server can still write)
    HANDLE    m_hServerNotifyEvent;             // handle for server notification
                                                // event - set to signaled to
                                                // notify server that the queue
                                                // needs servicing
    BOOL      m_bClient;                        // TRUE = queue client
    TCHAR     m_szQueueName[_MAX_PATH*2];       // queue name
    LPBYTE    m_pQueue;                         // pointer to queue mapped memory
                                                // (returned by MapViewOfFile)
    XQUEUE_VARIABLE_BLOCK * m_pVariableBlock;   // pointer to XQueue variable block
    LPBYTE   m_pDataBlock;                      // pointer to first data block
    DWORD    m_nQueueSize;                      // no. of data blocks in queue
    DWORD    m_nTotalBlocks;                    // no. of total blocks in queue -
                                                //   = m_nQueueSize + 1
    DWORD    m_nBlockSize;                      // block size - must be the same
                                                // for the client & server
    DWORD    m_nDataSize;                       // m_nBlockSize - XQUEUE_DATA_VARIABLE_SIZE

    SECURITY_ATTRIBUTES m_tDefSecAttr;
    SECURITY_DESCRIPTOR m_tDefSecDesc;
};


#pragma pack(pop, XQUEUE_INCLUDE_PACK)

#endif //XQUEUE_H
