// XQueue.cpp  Version 1.2
//
// Author: Hans Dietrich
//         hdietrich@gmail.com
//
// Description:
//     XQueue.cpp implements CXQueue, a class to create and manage an
//     interprocess queue.  CXQueue is intended to offload operations
//     from client app to server app - for example, performing logging
//     actions;  or server might function as network gateway, so
//     that details would be transparent to client.  CXQueue supports
//     one-way FIFO (first-in first-out) communication between multiple writers
//     (clients) and one reader (server) that are running on same machine.
//     Communication is one-way only - from client to server.  CXQueue offers
//     no delivery guarantee.
//
//     CXQueue may be used in either MFC or SDK programs.  It has been tested
//     on Windows 98, Windows 2000, and Windows XP.
//
// Background:
//     Although Windows NT-based platforms provide mechanisms such as
//     mailslots and named pipes that can be used to emulate queue, these
//     are not available on Win9x systems.  There have been some superficial
//     implementations of queues using shared memory and other techniques,
//     but these have been limited in one of two ways:  1) they are limited
//     to only intraprocess, not interprocess communication;  or 2) they are
//     synchronous, meaning that queue is limited to one entry, and the
//     server must remove and process entry before client can add
//     another.
//
// An Alternative:
//     If you are reading this, you are probably also familiar with the
//     Microsoft queue solution:  MSMQ.  This is an extremely powerful tool
//     that is mandatory in some situations.  However, it is also very complex,
//     non-trivial to install and set up, and has some issues on Win9x systems.
//
//     For more information on MSMQ, please refer to Microsoft site at
//         http://www.microsoft.com/windows2000/technologies/communications/msmq
//
//     For information concerning issues with MSMQ, please refer to the
//     Usenet news groups:
//         microsoft.public.msmq.programming
//         microsoft.public.msmq.setup
//         microsoft.public.msmq.deployment
//         microsoft.public.msmq.networking
//
// Concept & Facilities:
//     The CXQueue class incorporates both client and server API
//     set.  The server calls CXQueue::Create(), while client(s) call
//     CXQueue::Open().
//
//     CXQueue is based on shared memory using memory-mapped files.  Access
//     to shared memory is controlled by means of two kernel objects: an
//     event (used by clients to ensure quiescence, to allow server to
//     free some space); and mutex, to control actual writing to queue.
//
//     The shared memory (hereafter called MMF, for memory-mapped file),
//     is composed of fixed-size blocks.  The fixed size ensures that there
//     will be no fragmentation, and eliminates need for garbage
//     collection.  The first block of MMF is used for CXQueue variables.
//     Beginning with second block, all further blocks may be used for
//     data blocks.
//
//     The first (variable) block holds data that is used to manage and monitor
//     queue.  The data blocks are managed by use of two doubly-linked
//     lists:  free list and used list.  Initially, all blocks are
//     assigned to free list.  When client writes to queue, one or
//     more blocks are allocated from free list and transferred to the
//     used list.  This involves simply relinking block numbers; no
//     physical block copying is done, so it is very fast.
//
//     The client may write any number of bytes to queue.  The default
//     queue block size is 1024 bytes, but this may be easily changed by
//     modifying one line of code.  Typically client-->server message is
//     much less than 1024 bytes - usually, it is between 10 and 200 bytes.
//     To assist in determining optimal block size, CXQueue monitors
//     and stores maximum size of client write in queue variable
//     block.  You can then use this information to adjust size of
//     queue blocks.
//
//     Regardless of block size chosen, it should be expected that there
//     will be client writes that exceed block size.  When this happens,
//     CXQueue determines how many blocks will be needed, and writes data
//     to blocks, splitting data as necessary.  (Note:  data blocks
//     each have header with back and forward links and other information,
//     so there is less than 1024 bytes available in each block for data).
//     If multiple blocks are necessary, continuation flag is set in the
//     block header, to indicate that there is another block (which can be
//     found by means of forward link).
//
//     The links that have been mentioned are block numbers.  Data blocks
//     are numbered from 0 to N-1, although first data block is actually
//     second block in MMF.  To calculate byte address for any
//     block, formula is:
//         block_address = (block_no * BLOCK_SIZE) + base_address
//     where
//         block_no is zero-based number of data block,
//         BLOCK_SIZE is fixed number of bytes in a block,
//         and base_address is byte address of first data block,
//         which is simply address returned by MapViewOfFile(), plus
//         BLOCK_SIZE (to account for variable block).
//
//     Note that there is no guarantee that blocks will be contiguous
//     in multi-block client write.
//
//     When client has added an entry to queue, notification event
//     informs server that queue needs to be serviced.  The server
//     then performs read on queue (usually, two reads, with first
//     read returning only size of queue entry).  Then server
//     reads data and returns blocks to free list.  Because
//     only server manipulates blocks already in used list, it
//     is not necessary to lock queue (using mutex) until server
//     actually frees blocks.  This optimization helps to reduce the
//     time that clients are prevented from writing.
//
//     As mentioned in previous paragraph, mutex is used to control
//     write access to queue.  An event object that was mentioned
//     earlier is also used to synchronize queue access.  This event
//     object is used only by clients.  Here is why:  when client wants
//     to write, first thing it must do is determine if there are
//     enough free blocks to accommodate entire write.  If there are,
//     it can then write.  But if there are not, it would do no good to use
//     mutex at this point, because using mutex would lock out
//     server as well as all clients, so server would not be able
//     to free any blocks.  The client would then be waiting for server
//     to free some blocks, and server would be waiting for client
//     to release mutex.
//
//     What event object does is to prevent clients from starting any
//     new writes.  Since no one will be adding anything to queue, the
//     server will have chance to free some blocks.  When enough blocks
//     become free, waiting client can complete write, and then
//     set event to allow other clients to write.
//
//     To ensure that messages are processed in first-in first-out manner,
//     newly-written blocks are always appended to end of used list.
//     When server processes queue entries, it always removes entries
//     beginning at front of used list.  Server apps may verify the
//     proper ordering of message entries by inspecting sequential message
//     id that is stored in each queue entry;  multi-block entries will have
//     same message id stored in each block.
//
//     In current implementation, CXQueue has been tested with multiple
//     clients, but only one server.
//
// History
//     Version 1.2 - 2005 January 17
//                 - Initial public release
//
// Public APIs:
//        NAME                      DESCRIPTION
//     ----------  -------------------------------------------------------
//     Close()     Close XQueue object and clean up.
//     Create()    Create() is called by the XQueue server to create the
//                 system-wide named XQueue object.
//     Dump()      Dump (TRACE) info about the XQueue object.
//     Open()      Open() is called by XQueue clients to open an existing
//                 XQueue object.
//     Read()      Read message from the queue.  Typically used only by
//                 XQueue servers.
//     Write()     Write message to the queue.  Typically used only by
//                 XQueue clients.
//
// License:
//     This software is released into the public domain.  You are free to use
//     it in any way you like, except that you may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this
//     software may cause.
//
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// include the following line if compiling an MFC app
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////

#ifndef _MFC_VER
#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>
#include <tchar.h>
#include <stddef.h>
#pragma message("    compiling for Win32")
#else
#pragma message("    compiling for MFC")
#endif


// determine number of elements in an array (not bytes)
#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#pragma warning(disable : 4127)        // conditional expression is constant
                                    // (needed for _ASSERTE)

///////////////////////////////////////////////////////////////////////////////
//
// If you do not want TRACE output from XQueue.cpp you can
// uncomment the following lines:
//
//#undef  TRACE
//#define TRACE ((void)0)
//

#include "XQueue.h"

///////////////////////////////////////////////////////////////////////////////
// prefixes for named objects
#define XQUEUE_NAME                 _T("Global\\XQueue_")
#define WRITE_MUTEX_NAME            _T("_Write")
#define CLIENT_WRITE_EVENT_NAME     _T("_ClientWrite")
#define SERVER_NOTIFY_EVENT_NAME    _T("_ServerNotify")

///////////////////////////////////////////////////////////////////////////////
//
// CXQueue()
//
// Purpose:     Construct CXQueue object.
//
// Parameters:  None
//
// Returns:     None
//
CXQueue::CXQueue() :
    m_nBlockSize(XQUEUE_DEFAULT_BLOCK_SIZE),
    m_nDataSize(XQUEUE_DEFAULT_DATA_SIZE),
    m_nQueueSize(0),
    m_bClient(TRUE),
    m_hWriteMutex(NULL),
    m_hClientWriteEvent(NULL),
    m_hServerNotifyEvent(NULL),
    m_pQueue(NULL),
    m_pVariableBlock(NULL),
    m_pDataBlock(NULL)
{
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::CXQueue for thread %04X\n"), GetCurrentThreadId());
#endif
    ZeroMemory(m_szQueueName, sizeof(m_szQueueName));

    ZeroMemory(&m_tDefSecAttr, sizeof(SECURITY_ATTRIBUTES));

    m_tDefSecAttr.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    InitializeSecurityDescriptor(m_tDefSecAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // ACL is set as NULL in order to allow all access to the object.
    SetSecurityDescriptorDacl(m_tDefSecAttr.lpSecurityDescriptor, TRUE, NULL, FALSE);
    m_tDefSecAttr.nLength        = sizeof(m_tDefSecAttr);
    m_tDefSecAttr.bInheritHandle = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// ~CXQueue()
//
// Purpose:     Destroy CXQueue object.
//
// Parameters:  None
//
// Returns:     None
//
CXQueue::~CXQueue()
{
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::~CXQueue for thread %04X\n"), GetCurrentThreadId());
#endif
    if (m_tDefSecAttr.lpSecurityDescriptor) {
        free(m_tDefSecAttr.lpSecurityDescriptor);
    }

    Close();
}

///////////////////////////////////////////////////////////////////////////////
//
// Close()
//
// Purpose:     Close XQueue object and clean up.
//
// Parameters:  None
//
// Returns:     LONG - XQueue error code (see XQueue.h)
//
LONG CXQueue::Close()
{    
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::Close\n"));
#endif

    Dump();

    m_MMF.Dump();

    if (IsOpen()) {
#if _DEBUG_XQUEUE
        TRACE(_T("IsOpen\n"));
#endif        
        if (m_bClient) {
#if _DEBUG_XQUEUE
            TRACE(_T("Client\n"));
#endif             
            if (m_pVariableBlock->NoClients > 0) {
                m_pVariableBlock->NoClients--;
            }
        }
        else {
            if (m_pVariableBlock->NoServers > 0) {
                m_pVariableBlock->NoServers--;
            }
        }
    }

    if (m_pVariableBlock) {
        m_pVariableBlock->Dump();
    }

    m_MMF.UnMap();

    m_nQueueSize        = 0;
    m_pQueue            = NULL;
    m_pVariableBlock    = NULL;
    m_pDataBlock        = NULL;

    if (m_hWriteMutex) {
        CloseHandle(m_hWriteMutex);
        m_hWriteMutex = NULL;
    }

    if (m_hClientWriteEvent) {
        CloseHandle(m_hClientWriteEvent);
        m_hClientWriteEvent = NULL;
    }

    if (m_hServerNotifyEvent) {
        CloseHandle(m_hServerNotifyEvent);
        m_hServerNotifyEvent = NULL;
    }

    ZeroMemory(m_szQueueName, sizeof(m_szQueueName));

    return XQUEUE_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// Create()
//
// Purpose:     Create() is called by the XQueue server to create the
//              system-wide named XQueue object.
//
// Parameters:  lpszQueueName - [in] name of XQueue object (must be specified)
//              nQueueSize    - [in] size of queue in blocks
//              nBlockSize    - [in] size of a block in bytes
//
// Returns:     LONG          - XQueue error code (see XQueue.h)
//
LONG CXQueue::Create(LPCTSTR lpszQueueName /*= NULL*/,
                     DWORD   nQueueSize    /*= XQUEUE_DEFAULT_QUEUE_SIZE*/,
                     DWORD   nBlockSize    /*= XQUEUE_DEFAULT_BLOCK_SIZE*/)
{
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::Create\n"));
#endif     

    if (IsOpen()) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  already open\n"));
#endif        
        _ASSERTE(FALSE);
        return XQUEUE_ERROR_CREATE_FAILED;
    }

    _ASSERTE(lpszQueueName && (lpszQueueName[0] != _T('\0')));
    if (!lpszQueueName || (lpszQueueName[0] == _T('\0')))
        return XQUEUE_ERROR_PARAMETER;

    ZeroMemory(m_szQueueName, sizeof(m_szQueueName));
    _tcscpy_s(m_szQueueName, MAX_PATH * 2, XQUEUE_NAME);    // copy prefix
    size_t len = _tcslen(m_szQueueName);
    _tcsncat_s(m_szQueueName, MAX_PATH, lpszQueueName, _countof(m_szQueueName) - len - 1);

    m_nQueueSize = nQueueSize;
    if (m_nQueueSize == 0) {
#if _DEBUG_XQUEUE
        TRACE(_T("WARNING:  setting to default queue size\n"));
#endif        
        m_nQueueSize = XQUEUE_DEFAULT_QUEUE_SIZE;
    }

    _ASSERTE(m_nQueueSize <= XQUEUE_MAX_QUEUE_SIZE);
    if (m_nQueueSize > XQUEUE_MAX_QUEUE_SIZE)
        return XQUEUE_ERROR_PARAMETER;

    _ASSERTE(m_nQueueSize >= XQUEUE_MIN_QUEUE_SIZE);
    if (m_nQueueSize < XQUEUE_MIN_QUEUE_SIZE)
        return XQUEUE_ERROR_PARAMETER;

    m_nBlockSize = nBlockSize;
    if (m_nBlockSize < XQUEUE_MIN_BLOCK_SIZE) {
#if _DEBUG_XQUEUE
        TRACE(_T("WARNING:  block size too small - setting to minimum block size\n"));
#endif 
        m_nBlockSize = XQUEUE_MIN_BLOCK_SIZE;
    }

    // round up to next multiple of XQUEUE_MIN_BLOCK_SIZE
    m_nBlockSize = ((m_nBlockSize + (XQUEUE_MIN_BLOCK_SIZE-1)) / XQUEUE_MIN_BLOCK_SIZE) * XQUEUE_MIN_BLOCK_SIZE;

    m_nDataSize = m_nBlockSize - XQUEUE_DATA_VARIABLE_SIZE;
#if _DEBUG_XQUEUE
    TRACE(_T("m_nBlockSize=%d  m_nDataSize=%d\n"), m_nBlockSize, m_nDataSize);
#endif 

    _ASSERTE((offsetof(XQUEUE_DATA_BLOCK, Data) + m_nDataSize) == m_nBlockSize);

    m_bClient = FALSE;                    // only servers call Create()

    m_nTotalBlocks = 1 + m_nQueueSize;    // include queue variable block

    LONG lRet = XQUEUE_ERROR_SUCCESS;

    // this is the server
    if (m_MMF.MapMemory(m_szQueueName, (DWORD)(m_nTotalBlocks * m_nBlockSize), FALSE, &m_tDefSecAttr))
    {
        m_MMF.Dump();

        lRet = CreateSyncObjects();        // create mutex and events

        if (lRet == XQUEUE_ERROR_SUCCESS)
        {
            m_pQueue = (LPBYTE) m_MMF.GetMappedAddress();
            _ASSERTE(m_pQueue);

            if (m_pQueue)
            {
                // set up queue variable block (first block in mmf)
                m_pVariableBlock = (XQUEUE_VARIABLE_BLOCK *) m_pQueue;
                ZeroMemory(m_pVariableBlock, m_nBlockSize);            // zero all variables

                // first data block is second block in mmf
                m_pDataBlock = m_pQueue + XQUEUE_FIRST_DATA_BLOCK_NO * m_nBlockSize;

                m_pVariableBlock->BlockSize      = m_nBlockSize;
                _ASSERTE(m_nBlockSize > XQUEUE_DATA_VARIABLE_SIZE);
                m_pVariableBlock->DataSize       = m_nBlockSize - XQUEUE_DATA_VARIABLE_SIZE;
                m_pVariableBlock->NoBlocks       = m_nTotalBlocks;    // includes variable block
                m_pVariableBlock->NoDataBlocks   = m_nQueueSize;    // no. data blocks (not
                                                                    // including variable block
                m_pVariableBlock->FreeDataBlocks = m_nQueueSize;    // initially all are free
                m_pVariableBlock->ServerTID      = GetCurrentThreadId();
                m_pVariableBlock->MessageId      = 1;

                // set up list of free blocks
                XQUEUE_DATA_BLOCK *pXDB = NULL;
                XQUEUE_DATA_BLOCK *pPrevious = NULL;
                DWORD back_link = XQUEUE_INVALID_BLOCK;

                for (DWORD i = 0; i < m_nQueueSize; i++)
                {
                    pXDB = (XQUEUE_DATA_BLOCK *) (m_pDataBlock + i * m_nBlockSize);
                    pXDB->BlockNo     = i;
                    pXDB->BackLink    = back_link;
                    pXDB->ForwardLink = XQUEUE_INVALID_BLOCK;
                    pXDB->ThreadId    = 0;
                    pXDB->DataSize    = 0;
                    pXDB->MessageId   = 0;
                    pXDB->Data[0]     = 0;
                    pXDB->Data[1]     = 0;

                    back_link = i;

                    if (pPrevious)
                        pPrevious->ForwardLink = i;
                    pPrevious = pXDB;
                }

                // free block list has all the blocks;
                // note:  data blocks are numbered from 0
                m_pVariableBlock->FreeListForwardLink = 0;
                m_pVariableBlock->FreeListBackLink    = m_nQueueSize - 1;

                // used block list is empty
                m_pVariableBlock->UsedListForwardLink = XQUEUE_INVALID_BLOCK;
                m_pVariableBlock->UsedListBackLink    = XQUEUE_INVALID_BLOCK;

                // this server is ready
                m_pVariableBlock->NoServers++;

                m_pVariableBlock->Dump();
            }
            else
            {
#if _DEBUG_XQUEUE
                TRACE(_T("ERROR:  GetMappedAddress returned NULL\n"));
#endif                 
                lRet = XQUEUE_ERROR_INTERNAL;
            }
        }
        else
        {
            _ASSERTE(FALSE);
#if _DEBUG_XQUEUE
            TRACE(_T("ERROR:  CreateSyncObjects failed\n"));
#endif            
        }
    }
    else
    {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  cannot create mmf %s\n"), m_szQueueName);
#endif         
        _ASSERTE(FALSE);
        lRet = XQUEUE_ERROR_CREATE_FAILED;
    }

    if (lRet != XQUEUE_ERROR_SUCCESS)
        Close();

    return lRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// Dump()
//
// Purpose:     Dump (TRACE) info about the XQueue object.
//
// Parameters:  None
//
// Returns:     None
//
void CXQueue::Dump()
{
#if _DEBUG_XQUEUE
    TRACE(_T("CXQueue dump:\n"));
    TRACE(_T("    m_hWriteMutex=0x%08X\n"),        m_hWriteMutex);
    TRACE(_T("    m_hClientWriteEvent=0x%08X\n"),  m_hClientWriteEvent);
    TRACE(_T("    m_hServerNotifyEvent=0x%08X\n"), m_hServerNotifyEvent);
    TRACE(_T("    m_szQueueName=<%s>\n"),          m_szQueueName);
    TRACE(_T("    m_nQueueSize=%d\n"),             m_nQueueSize);
    TRACE(_T("    m_nTotalBlocks=%d\n"),           m_nTotalBlocks);
    TRACE(_T("    m_nBlockSize=%d\n"),             m_nBlockSize);
    TRACE(_T("    m_nDataSize=%d\n"),              m_nDataSize);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// Open()
//
// Purpose:     Open() is called by XQueue clients to open an existing XQueue
//              object.
//
// Parameters:  lpszQueueName - [in] name of XQueue object (must be specified)
//
// Returns:     LONG          - XQueue error code (see XQueue.h)
//
LONG CXQueue::Open(LPCTSTR lpszQueueName)
{
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::Open -------\n"));
#endif

    if (IsOpen()) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  already open\n"));
#endif        
        _ASSERTE(FALSE);
        return XQUEUE_ERROR_OPEN_FAILED;
    }

    _ASSERTE(lpszQueueName && (lpszQueueName[0] != _T('\0')));
    if (!lpszQueueName || (lpszQueueName[0] == _T('\0'))) {
        return XQUEUE_ERROR_PARAMETER;
    }

    ZeroMemory(m_szQueueName, sizeof(m_szQueueName));
    _tcscpy_s(m_szQueueName, MAX_PATH * 2, XQUEUE_NAME);    // copy prefix

    size_t len = _tcslen(m_szQueueName);
    _tcsncat_s(m_szQueueName, MAX_PATH * 2, lpszQueueName, _countof(m_szQueueName) - len - 1);

    LONG lRet = XQUEUE_ERROR_SUCCESS;

    if (m_MMF.MapExistingMemory(m_szQueueName)) {

        m_MMF.Dump();

        lRet = OpenSyncObjects();

        if (lRet == XQUEUE_ERROR_SUCCESS) {
            m_bClient = TRUE;
            m_nQueueSize = 0;

            m_pQueue = (LPBYTE) m_MMF.GetMappedAddress();
            _ASSERTE(m_pQueue);

            if (m_pQueue) {
                m_pVariableBlock  = (XQUEUE_VARIABLE_BLOCK *) m_pQueue;

                // set the client block sizes to the same size as the server
                m_nBlockSize   = m_pVariableBlock->BlockSize;
                m_nDataSize    = m_pVariableBlock->DataSize;
#if _DEBUG_XQUEUE
                TRACE(_T("m_nBlockSize=%d  m_nDataSize=%d\n"), m_nBlockSize, m_nDataSize);
#endif
                m_nQueueSize   = m_pVariableBlock->NoDataBlocks;
                m_nTotalBlocks = m_pVariableBlock->NoBlocks;
                m_pDataBlock   = m_pQueue + XQUEUE_FIRST_DATA_BLOCK_NO * m_nBlockSize;

                m_pVariableBlock->NoClients++;

                m_pVariableBlock->Dump();
            }
            else {
#if _DEBUG_XQUEUE
                TRACE(_T("ERROR:  GetMappedAddress returned NULL\n"));
#endif
                lRet = XQUEUE_ERROR_INTERNAL;
            }
        }
    }
    else {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  cannot open mmf %s\n"), m_szQueueName);
#endif
        lRet = XQUEUE_ERROR_OPEN_FAILED;
    }

    if (lRet != XQUEUE_ERROR_SUCCESS) {
        Close();
    }

    return lRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read()
//
// Purpose:     Read message from the queue.  Typically used only by XQueue
//              servers.
//
// Parameters:  lpData       - [out] Pointer to the buffer that receives the
//                             data read from the file
//              lpcbSize     - [in/out] size in bytes of lpData;  if lpData
//                             is NULL, the size of the required buffer will
//                             returned in lpcbSize.
//              lpnMessageId - [out] the message id contained in the queue block
//              pdwThreadId  - [out] the thread id of the thread that queued
//                             the message
//              dwTimeOut    - [in] specifies the time-out interval, in
//                             milliseconds
//
// Returns:     LONG         - XQueue error code (see XQueue.h)
//
LONG CXQueue::Read(LPBYTE   lpData,
                   DWORD  * lpcbSize,
                   DWORD  * lpnMessageId,
                   DWORD  * pdwThreadId,
                   DWORD    dwTimeOut /*= INFINITE*/)
{
    _ASSERTE(lpcbSize);
    if (!lpcbSize)
        return XQUEUE_ERROR_PARAMETER;

    _ASSERTE(lpnMessageId);
    if (!lpnMessageId)
        return XQUEUE_ERROR_PARAMETER;

    _ASSERTE(m_pVariableBlock);
    if (!m_pVariableBlock) {
        return XQUEUE_ERROR_NOT_OPEN;
    }

    *lpnMessageId = 0;

    LONG lRet = XQUEUE_ERROR_NO_DATA;

    if ((m_pVariableBlock) &&
        (m_pVariableBlock->FreeDataBlocks < m_pVariableBlock->NoDataBlocks))
    {
        DWORD block_no = m_pVariableBlock->UsedListForwardLink;
        if (block_no != XQUEUE_INVALID_BLOCK) {
            XQUEUE_DATA_BLOCK *pXDB =
                (XQUEUE_DATA_BLOCK *) (m_pDataBlock + block_no * m_nBlockSize);

            *lpnMessageId = pXDB->MessageId;
            if (pdwThreadId) {
                *pdwThreadId  = (DWORD)pXDB->ThreadId;
            }

            if (!lpData) {
                // caller just wants the size
                *lpcbSize = GetEntrySize(pXDB);
#if _DEBUG_XQUEUE
                TRACE(_T("GetEntrySize returned %d for block %d\n"),
                      *lpcbSize, block_no);
#endif
                lRet = XQUEUE_ERROR_SUCCESS;
            } 
            else {
                // caller wants the data
                lRet = ReadMMF(lpData, lpcbSize, dwTimeOut);
            }
        }
    }

    if (lRet == XQUEUE_ERROR_NO_DATA) {
        *lpcbSize = 0;
    }

#if _DEBUG_XQUEUE
    TRACE(_T("Read returning %d bytes -----\n"), *lpcbSize);
#endif 
    
    return lRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// Write()
//
// Purpose:     Write string to the queue.  Typically used only by XQueue
//              clients.
//
// Parameters:  lpszString - [in] Pointer to the buffer that contains the
//                           nul-terminated string to queue
//              dwTimeOut  - [in] specifies the time-out interval, in
//                           milliseconds
//
// Returns:     LONG       - XQueue error code (see XQueue.h)
//
LONG CXQueue::Write(LPCTSTR lpszString,
                    DWORD dwTimeOut /*= INFINITE*/)
{
    if (!IsOpen()) {
        return XQUEUE_ERROR_NOT_OPEN;
    }

    _ASSERTE(lpszString);
    if (!lpszString) {
        return XQUEUE_ERROR_PARAMETER;
    }

    size_t nDataSize = (_tcslen(lpszString)+1) * sizeof(TCHAR);    // no. of bytes

    return Write((LPBYTE) lpszString, (DWORD)nDataSize, dwTimeOut);
}

///////////////////////////////////////////////////////////////////////////////
//
// Write()
//
// Purpose:     Write byte data to the queue.  Typically used only by XQueue
//              clients.
//
// Parameters:  lpData    - [in] Pointer to the buffer that contains the
//                          data to queue
//              nDataSize - [in] number of bytes of data in lpData
//              dwTimeOut - [in] specifies the time-out interval, in
//                          milliseconds
//
// Returns:     LONG      - XQueue error code (see XQueue.h)
//
LONG CXQueue::Write(LPBYTE lpData,
                    DWORD nDataSize,
                    DWORD dwTimeOut /*= INFINITE*/)
{
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::Write:  %d bytes -----\n"), nDataSize);
#endif    

    _ASSERTE(lpData);
    _ASSERTE(nDataSize > 0);
    if (!lpData || (nDataSize == 0))
        return XQUEUE_ERROR_PARAMETER;

    // check size of write
    _ASSERTE(nDataSize < (m_nBlockSize * (m_nQueueSize-1)));
    if (nDataSize >= (m_nBlockSize * (m_nQueueSize-1))) {
        return XQUEUE_ERROR_PARAMETER;
    }

    _ASSERTE(m_pVariableBlock);
    if (!m_pVariableBlock) {
        return XQUEUE_ERROR_NOT_OPEN;
    }

    if (m_pVariableBlock->NoServers == 0) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  no server\n"));
#endif        
       // return XQUEUE_ERROR_NO_SERVER;
    }

    // wait until no _new_ client writes are allowed - it doesn't
    // matter that there are still client writes going on; we just want
    // to prevent any new ones, so the server will have a chance to
    // free some blocks (if necessary).  Note:  this is auto reset event.

    LONG lWaitResult = WaitForQueueIdle(INFINITE);

    if (lWaitResult != XQUEUE_ERROR_SUCCESS) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  WaitForQueueIdle failed for thread %X -----\n"), GetCurrentThreadId());
#endif 
        _ASSERTE(FALSE);
        return lWaitResult;
    }

    // determine how many blocks we will need
    DWORD nBlocks = CalculateNoBlocks(nDataSize);

    BOOL bFound = TRUE;
    LONG lRet = XQUEUE_ERROR_SUCCESS;

    // are there enough free blocks?
    if (m_pVariableBlock->FreeDataBlocks < nBlocks) {
        // no, we must wait until server frees some

        bFound = FALSE;

        if (dwTimeOut == 0) {
#if _DEBUG_XQUEUE
            TRACE(_T("ERROR:  not enough free blocks\n"));
#endif 
            lRet = XQUEUE_ERROR_NO_FREE;
        }
        else {
            DWORD dwFreeTimeOut = dwTimeOut;
            DWORD dwSpinTime = 1;

            // loop until server frees some blocks
            do
            {
                Sleep(dwSpinTime);

                if ((lRet == XQUEUE_ERROR_SUCCESS) &&
                    (m_pVariableBlock->FreeDataBlocks >= nBlocks))
                {
                    // there are now enough free blocks, we can write
                    bFound = TRUE;
                    break;
                }

                if (dwFreeTimeOut > dwSpinTime)
                    dwFreeTimeOut -= dwSpinTime;
                else
                    dwFreeTimeOut = 0;

            } while (dwFreeTimeOut > 0);
        }
    }

    if (!bFound) {
        // timeout expired, not enough free blocks
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  timeout waiting for enough free blocks\n"));
#endif 
        lRet = XQUEUE_ERROR_TIMEOUT;
    }
    else {
        // there are enough free blocks

        lRet = Lock(dwTimeOut);    //===================================

        if (lRet == XQUEUE_ERROR_SUCCESS) {
            lRet = WriteToMMF(lpData, nDataSize);

            if (lRet == XQUEUE_ERROR_SUCCESS) {
                // update some XQueue statistics
                m_pVariableBlock->NoWrites++;

                if (nDataSize > m_pVariableBlock->MaxDataSize)
                    m_pVariableBlock->MaxDataSize = nDataSize;

                // increment message id (all blocks within a message
                // will have the same id)
                m_pVariableBlock->MessageId++;
            }

            Unlock();            //===================================
        }

        if (lRet == XQUEUE_ERROR_SUCCESS) {
            // tell server something has been queued
            if (m_hServerNotifyEvent)
                SetEvent(m_hServerNotifyEvent);
        }
    }

    // allow other clients to write
    SetEvent(m_hClientWriteEvent);

    return lRet;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  PRIVATE FUNCTIONS
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
// Add() - private function
//
// Purpose:     Add a block to list
//
// Parameters:  ForwardLink - [out] reference to forward link
//              BackLink    - [out] reference to backward link
//              BlockCount  - [in/out] number of blocks already in list
//              pXDB        - [in] pointer to Xqueue data block
//
// Returns:     None
//
void CXQueue::Add(DWORD &ForwardLink,
                  DWORD &BackLink,
                  DWORD &BlockCount,
                  XQUEUE_DATA_BLOCK * pXDB)
{
    _ASSERTE(pXDB);
    if (!pXDB)
        return;

    // make sure list isn't already full - caller should have checked this
    _ASSERTE(BlockCount < m_pVariableBlock->NoDataBlocks);
    if (BlockCount >= m_pVariableBlock->NoDataBlocks) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  list full\n"));
#endif 
        return;
    }

    // add block to end of list
    DWORD block_no = BackLink;
    XQUEUE_DATA_BLOCK * pLast = NULL;
    if (block_no != XQUEUE_INVALID_BLOCK)
        pLast = (XQUEUE_DATA_BLOCK *) (m_pDataBlock + block_no * m_nBlockSize);

    // at this point pLast is NULL if list is empty,
    // or pLast points to last block in list

    if (pLast == NULL) {
        // used list is empty
        ForwardLink = pXDB->BlockNo;
        BackLink    = pXDB->BlockNo;
        pXDB->ForwardLink = XQUEUE_INVALID_BLOCK;
        pXDB->BackLink    = XQUEUE_INVALID_BLOCK;
    }
    else {
        // used list not empty, pLast points to last block;
        // add block to end of list
        pLast->ForwardLink = pXDB->BlockNo;
        pXDB->BackLink     = pLast->BlockNo;
        pXDB->ForwardLink  = XQUEUE_INVALID_BLOCK;
        BackLink = pXDB->BlockNo;
    }

    // bump count for this list
    BlockCount++;
    if (BlockCount > m_pVariableBlock->NoDataBlocks) {
        BlockCount = m_pVariableBlock->NoDataBlocks;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// AddFree() - private function
//
// Purpose:     Add a block to the free list
//
// Parameters:  pXDB - [in] pointer to Xqueue data block
//
// Returns:     None
//
void CXQueue::AddFree(XQUEUE_DATA_BLOCK * pXDB)
{
    Add(m_pVariableBlock->FreeListForwardLink,
        m_pVariableBlock->FreeListBackLink,
        m_pVariableBlock->FreeDataBlocks,
        pXDB);
}

///////////////////////////////////////////////////////////////////////////////
//
// AddUsed() - private function
//
// Purpose:     Add a block to the used list
//
// Parameters:  pXDB - [in] pointer to Xqueue data block
//
// Returns:     None
//
void CXQueue::AddUsed(XQUEUE_DATA_BLOCK * pXDB)
{
    Add(m_pVariableBlock->UsedListForwardLink,
        m_pVariableBlock->UsedListBackLink,
        m_pVariableBlock->UsedDataBlocks,
        pXDB);
}

///////////////////////////////////////////////////////////////////////////////
//
// GetBlock() - private function
//
// Purpose:     Get a block from a list, delink it from the list
//
// Parameters:  ForwardLink - [out] reference to forward link
//              BackLink    - [out] reference to backward link
//              BlockCount  - [in/out] number of blocks in list
//
// Returns:     XQUEUE_DATA_BLOCK *  - pointer to XQueue data block, or NULL
//
XQUEUE_DATA_BLOCK * CXQueue::GetBlock(DWORD &ForwardLink,
                                      DWORD &BackLink,
                                      DWORD &BlockCount)
{
    // check if list is empty
    if (BlockCount == 0)
        return NULL;

    // check list head forward link
    DWORD block_no = ForwardLink;
    if (block_no == XQUEUE_INVALID_BLOCK)
        return NULL;

    _ASSERTE(m_pDataBlock);
    _ASSERTE(m_pVariableBlock);
    _ASSERTE(block_no < m_nQueueSize);

    XQUEUE_DATA_BLOCK * pXDB = (XQUEUE_DATA_BLOCK *)(m_pDataBlock + block_no * m_nBlockSize);

    // check if block is valid
    _ASSERTE(pXDB->BlockNo == block_no);
    if (pXDB->BlockNo != block_no) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  bad data block %d\n"), block_no);
#endif 
        return NULL;
    }

    DWORD new_first_block = pXDB->ForwardLink;
    ForwardLink = new_first_block;

    if (new_first_block == XQUEUE_INVALID_BLOCK) {
        BackLink = XQUEUE_INVALID_BLOCK;
    }
    else {
        XQUEUE_DATA_BLOCK * pFirst = (XQUEUE_DATA_BLOCK *) (m_pDataBlock + new_first_block * m_nBlockSize);

        pFirst->BackLink = XQUEUE_INVALID_BLOCK;
    }

    // one less entry in list
    if (BlockCount > 0) {
        BlockCount--;
    }

    // store thread id for debugging
    pXDB->ThreadId = GetCurrentThreadId();

#if _DEBUG_XQUEUE
    TRACE(_T("GetBlock returning %d\n"), block_no);
#endif

    return pXDB;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetFreeBlock() - private function
//
// Purpose:     Get a block from the free list
//
// Parameters:  None
//
// Returns:     XQUEUE_DATA_BLOCK *  - pointer to XQueue data block, or NULL
//
XQUEUE_DATA_BLOCK * CXQueue::GetFreeBlock()
{
    return GetBlock(m_pVariableBlock->FreeListForwardLink,
                    m_pVariableBlock->FreeListBackLink,
                    m_pVariableBlock->FreeDataBlocks);
}

///////////////////////////////////////////////////////////////////////////////
//
// GetUsedBlock() - private function
//
// Purpose:     Get a block from the used list
//
// Parameters:  None
//
// Returns:     XQUEUE_DATA_BLOCK *  - pointer to XQueue data block, or NULL
//
XQUEUE_DATA_BLOCK * CXQueue::GetUsedBlock()
{
    return GetBlock(m_pVariableBlock->UsedListForwardLink,
                    m_pVariableBlock->UsedListBackLink,
                    m_pVariableBlock->UsedDataBlocks);
}

///////////////////////////////////////////////////////////////////////////////
//
// GetEntrySize() - private function
//
// Purpose:     Get the size in bytes of a block (including any continuation
//              blocks)
//
// Parameters:  pXDB   - [in] pointer to queue block
//
// Returns:     DWORD - size in bytes
//
DWORD CXQueue::GetEntrySize(XQUEUE_DATA_BLOCK *pXDB)
{
    _ASSERTE(pXDB);
    if (!pXDB) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  pXDB is NULL\n"));
#endif
        return 0;
    }

    DWORD nSize = 0;

    for(;;) {
        nSize += pXDB->DataSize & 0x7FFFFFFF;    // mask off continuation bit

        // is there another block?
        if ((pXDB->DataSize & XQUEUE_CONTINUATION_FLAG) == 0) {
            break;
        }

        DWORD block_no = pXDB->ForwardLink;
        if (block_no == XQUEUE_INVALID_BLOCK) {
            break;
        }

        pXDB = (XQUEUE_DATA_BLOCK *) (m_pDataBlock + block_no * m_nBlockSize);
    }

    return nSize;
}

///////////////////////////////////////////////////////////////////////////////
//
// ReadMMF() - private function
//
// Purpose:     Read memory-mapped file
//
// Parameters:  lpData       - [out] Pointer to the buffer that receives the
//                             data read from the file
//              lpcbSize     - [in/out] size in bytes of lpData;  if lpData
//                             is NULL, the size of the required buffer will
//                             returned in lpcbSize.
//              dwTimeOut    - [in] specifies the time-out interval, in
//                             milliseconds
//
// Returns:     LONG         - XQueue error code (see XQueue.h)
//
LONG CXQueue::ReadMMF(LPBYTE  lpData,
                      DWORD * lpcbSize,
                      DWORD   dwTimeOut)
{
    LONG lRet = XQUEUE_ERROR_SUCCESS;

    // verify parameters
    if ((lpData == NULL) ||
        (lpcbSize == NULL) ||
        (*lpcbSize == 0) ||
        IsBadWritePtr(lpData, *lpcbSize))
    {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR: bad parameters\n"));
#endif
        _ASSERTE(FALSE);
        *lpcbSize = 0;
        return XQUEUE_ERROR_PARAMETER;
    }

    DWORD block_no = m_pVariableBlock->UsedListForwardLink;
    if (block_no == XQUEUE_INVALID_BLOCK) {
        *lpcbSize = 0;
        return XQUEUE_ERROR_NO_DATA;
    }

    _ASSERTE(m_pDataBlock);
    _ASSERTE(m_pVariableBlock);
    _ASSERTE(block_no < m_nQueueSize);

    XQUEUE_DATA_BLOCK * pXDB = NULL;

    DWORD nSize = 0;

    while (nSize < *lpcbSize) {
        if (block_no == XQUEUE_INVALID_BLOCK) {
            break;
        }

        pXDB = (XQUEUE_DATA_BLOCK *) (m_pDataBlock + block_no * m_nBlockSize);

        // check if block is valid
        _ASSERTE(pXDB->BlockNo == block_no);
        if (pXDB->BlockNo != block_no) {
#if _DEBUG_XQUEUE
            TRACE(_T("ERROR:  bad data block %d\n"), block_no);
#endif
            nSize = 0;
            lRet = XQUEUE_ERROR_INTERNAL;
            break;
        }

        DWORD nBytesToCopy = pXDB->DataSize & 0x7FFFFFFF;    // mask off continuation bit
        if ((nSize + nBytesToCopy) > *lpcbSize) {
            // too much data for buffer - only copy what will fit
            nBytesToCopy = *lpcbSize - nSize;
            lRet = XQUEUE_ERROR_MORE_DATA;
        }
        nSize += nBytesToCopy;

        if (nBytesToCopy > 0) {
            memcpy(lpData, pXDB->Data, nBytesToCopy);
        }

        lpData += nBytesToCopy;

        if (lRet == XQUEUE_ERROR_MORE_DATA) {
            break;
        }

        // is there another block?
        if ((pXDB->DataSize & XQUEUE_CONTINUATION_FLAG) == 0) {
            break;
        }

        block_no = pXDB->ForwardLink;
    }

    // if there's room left, append some zeros
    DWORD nSize2 = nSize;
    while ((nSize2 < *lpcbSize) && ((nSize2 - nSize) < 4)) {
        *lpData++ = 0;
        nSize2++;
    }

    if (lRet == XQUEUE_ERROR_MORE_DATA) {
        *lpcbSize = GetEntrySize(pXDB);
    }
    else if (lRet == XQUEUE_ERROR_INTERNAL) {
        *lpcbSize = 0;
    }
    else {
        *lpcbSize = nSize;

        lRet = Lock(dwTimeOut);    //===================================

        if (lRet == XQUEUE_ERROR_SUCCESS)
            ReleaseEntry();

        m_pVariableBlock->NoReads++;

        Unlock();                //===================================
    }

    return lRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// WriteToMMF() - private function
//
// Purpose:     Write memory-mapped file
//
// Parameters:  lpData    - [out] Pointer to the buffer that contains the
//                          data to write
//              nDataSize - [in] number of bytes of data in lpData
//
// Returns:     LONG      - XQueue error code (see XQueue.h)
//
// Notes:       Does not lock access to MMF.
//
LONG CXQueue::WriteToMMF(LPBYTE lpData, DWORD nDataSize)
{
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::WriteToMMF\n"));
#endif

    // verify parameters
    if ((lpData == NULL) ||
        (nDataSize == 0) ||
        IsBadReadPtr(lpData, nDataSize))
    {        
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR: bad parameters\n"));
#endif
        _ASSERTE(FALSE);
        return XQUEUE_ERROR_PARAMETER;
    }

    LONG lRet = XQUEUE_ERROR_SUCCESS;

    DWORD message_id = m_pVariableBlock->MessageId;
    DWORD nBlockNo = XQUEUE_INVALID_BLOCK;
    XQUEUE_DATA_BLOCK * pPrevious = NULL;

    while (nDataSize > 0) {
        XQUEUE_DATA_BLOCK * pXDB = GetFreeBlock();
        if (!pXDB) {
            lRet = XQUEUE_ERROR_NO_FREE;
            break;
        }

        AddUsed(pXDB);

        DWORD nBytesToCopy = (nDataSize > m_nDataSize) ?
                                m_nDataSize : nDataSize;

        memcpy(pXDB->Data, lpData, nBytesToCopy);
        pXDB->DataSize = nBytesToCopy;
        pXDB->MessageId = message_id;

        nBlockNo = pXDB->BlockNo;

        pXDB->Dump();

        lpData += nBytesToCopy;
        nDataSize -= nBytesToCopy;

        // if more bytes to copy, set continuation flag in this block
        if (nDataSize > 0) {
            pXDB->DataSize |= XQUEUE_CONTINUATION_FLAG;
        }

        if (pPrevious) {
            pPrevious->ForwardLink = nBlockNo;
        }

        pPrevious = pXDB;
    };

    m_pVariableBlock->Dump();

    return lRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// ReleaseEntry() - private function
//
// Purpose:     Release all blocks in a queue entry, add them to free list.
//
// Parameters:  None
//
// Returns:     LONG - XQueue error code (see XQueue.h)
//
LONG CXQueue::ReleaseEntry()
{
    LONG lRet = XQUEUE_ERROR_SUCCESS;

    XQUEUE_DATA_BLOCK *pXDB = NULL;

    int nCount = 0;

    for (;;) {
        // GetUsedBlock gets first block from front of list
        pXDB = GetUsedBlock();
        if (pXDB == NULL) {
            break;
        }

        nCount++;

        DWORD DataSize = pXDB->DataSize;

        pXDB->DataSize = 0;

        AddFree(pXDB);        // add to end of free list

        // check if there is a continuation block
        if ((DataSize & XQUEUE_CONTINUATION_FLAG) == 0) {
            break;
        }
    }
#if _DEBUG_XQUEUE
    TRACE(_T("ReleaseEntry:  freed %d blocks, FreeDataBlocks=%d\n"),
          nCount, m_pVariableBlock->FreeDataBlocks);
#endif   

    return lRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// Lock() - private function
//
// Purpose:     Wait for write mutex.
//
// Parameters:  dwTimeOut - [in] specifies the time-out interval, in
//                          milliseconds
//
// Returns:     LONG      - XQueue error code (see XQueue.h)
//
LONG CXQueue::Lock(DWORD dwTimeOut)
{
    _ASSERTE(m_hWriteMutex);
    if (!m_hWriteMutex) {
        return XQUEUE_ERROR_INTERNAL;
    }

    DWORD dwWaitResult = WaitForSingleObject(m_hWriteMutex, dwTimeOut);
    if (dwWaitResult == WAIT_OBJECT_0) {
        return XQUEUE_ERROR_SUCCESS;
    }
    else {
#if _DEBUG_XQUEUE
        TRACE(_T("TIMEOUT for thread %X -----\n"), GetCurrentThreadId());
#endif        
        return XQUEUE_ERROR_TIMEOUT;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Unlock() - private function
//
// Purpose:     Unlock write mutex.
//
// Parameters:  None
//
// Returns:     LONG - XQueue error code (see XQueue.h)
//
LONG CXQueue::Unlock()
{
    if (!m_hWriteMutex) {
        return XQUEUE_ERROR_SUCCESS;
    }

    ReleaseMutex(m_hWriteMutex);

    return XQUEUE_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// SetQueueAvailable() - private function
//
// Purpose:     Allow clients to write.
//
// Parameters:  None
//
// Returns:     LONG - XQueue error code (see XQueue.h)
//
LONG CXQueue::SetQueueAvailable()
{
    _ASSERTE(m_hClientWriteEvent);

    if (!m_hClientWriteEvent) {
        return XQUEUE_ERROR_NOT_OPEN;
    }

    // allow other clients to write
    SetEvent(m_hClientWriteEvent);

    return XQUEUE_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// WaitForQueueIdle() - private function
//
// Purpose:     Wait until no clients are writing.
//
// Parameters:  dwTimeOut - [in] specifies the time-out interval, in
//                          milliseconds
//
// Returns:     LONG - XQueue error code (see XQueue.h)
//
LONG CXQueue::WaitForQueueIdle(DWORD dwTimeOut)
{
    _ASSERTE(m_hClientWriteEvent);

    if (!m_hClientWriteEvent) {
        return XQUEUE_ERROR_NOT_OPEN;
    }

    DWORD dwWaitResult = WaitForSingleObject(m_hClientWriteEvent, dwTimeOut);
    if (dwWaitResult == WAIT_OBJECT_0) {
        return XQUEUE_ERROR_SUCCESS;
    }
    else {
        return XQUEUE_ERROR_TIMEOUT;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// WaitForQueueWrite() - private function
//
// Purpose:     Wait for event that signals something has been written to the
//              queue.
//
// Parameters:  dwTimeOut - [in] specifies the time-out interval, in
//                          milliseconds
//
// Returns:     LONG      - XQueue error code (see XQueue.h)
//
LONG CXQueue::WaitForQueueWrite(DWORD dwTimeOut)
{
    _ASSERTE(m_hServerNotifyEvent);

    if (!m_hServerNotifyEvent) {
        return XQUEUE_ERROR_NOT_OPEN;
    }

    DWORD dwWaitResult = WaitForSingleObject(m_hServerNotifyEvent, dwTimeOut);
    if (dwWaitResult == WAIT_OBJECT_0) {
        return XQUEUE_ERROR_SUCCESS;
    }

    return XQUEUE_ERROR_TIMEOUT;
}

///////////////////////////////////////////////////////////////////////////////
//
// CreateSyncObjects() - private function
//
// Purpose:     Create the mutex and event objects used by XQueue; called by
//              XQueue server.
//
// Parameters:  None
//
// Returns:     LONG - XQueue error code (see XQueue.h)
//
LONG CXQueue::CreateSyncObjects()
{
    TCHAR  szName[_MAX_PATH*2] = { 0 };
    size_t len = 0;

    // first close any open handles

    if (m_hWriteMutex) {
        ::CloseHandle(m_hWriteMutex);
        m_hWriteMutex = NULL;
    }

    if (m_hClientWriteEvent) {
        ::CloseHandle(m_hClientWriteEvent);
        m_hClientWriteEvent = NULL;
    }

    if (m_hServerNotifyEvent) {
        ::CloseHandle(m_hServerNotifyEvent);
        m_hServerNotifyEvent = NULL;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create a mutex object. The server or client sets this to nonsignaled
    // when it is writing to the mmf.
    _tcscpy_s(szName, MAX_PATH * 2, m_szQueueName);
    len = _tcslen(szName);    
    _tcsncat_s(szName, MAX_PATH * 2, WRITE_MUTEX_NAME, _countof(szName) - len - 1);

    m_hWriteMutex = CreateMutex( &m_tDefSecAttr,    // default security attributes
                                 FALSE,                // initially not owned
                                 szName );            // mutex name

    _ASSERTE(m_hWriteMutex);
    if (!m_hWriteMutex) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  CreateMutex failed for m_hWriteMutex (%s)\n"), szName);
#endif        
        return XQUEUE_ERROR_INTERNAL;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create an auto reset event object. Clients set this to be signaled
    // after writing to the queue, to notify the server.
    _tcscpy_s(szName, MAX_PATH * 2, m_szQueueName);
    len = _tcslen(szName);
    _tcsncat_s(szName, MAX_PATH * 2, SERVER_NOTIFY_EVENT_NAME, _countof(szName) - len - 1);

    m_hServerNotifyEvent = CreateEvent( &m_tDefSecAttr, // no security attributes
                                        FALSE,            // auto reset event
                                        FALSE,            // initial state is nonsignaled
                                        szName);        // event name

    _ASSERTE(m_hServerNotifyEvent);
    if (!m_hServerNotifyEvent) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  CreateEvent failed for m_hServerNotifyEvent (%s)\n"), szName);
#endif        
        ::CloseHandle(m_hWriteMutex);
        m_hWriteMutex = NULL;
        return XQUEUE_ERROR_INTERNAL;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create an auto reset event object. Clients wait for this to be signaled
    // before starting a new write.  This ensures that the server will free
    // some blocks if there are insufficient free blocks.
    _tcscpy_s(szName, MAX_PATH * 2, m_szQueueName);
    len = _tcslen(szName);
    _tcsncat_s(szName, MAX_PATH * 2, CLIENT_WRITE_EVENT_NAME, _countof(szName) - len - 1);

    m_hClientWriteEvent = CreateEvent( &m_tDefSecAttr,  // no security attributes
                                       FALSE,           // auto reset event
                                       TRUE,            // initial state is signaled
                                       szName );        // event name

    _ASSERTE(m_hClientWriteEvent);
    if (!m_hClientWriteEvent) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  CreateEvent failed for m_hClientWriteEvent (%s)\n"), szName);
#endif 
        ::CloseHandle(m_hWriteMutex);
        m_hWriteMutex = NULL;
        ::CloseHandle(m_hServerNotifyEvent);
        m_hServerNotifyEvent = NULL;
        return XQUEUE_ERROR_INTERNAL;
    }

    return XQUEUE_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenSyncObjects() - private function
//
// Purpose:     Open the mutex and event objects used by XQueue;  called by
//              XQueue clients to open objects created by server.
//
// Parameters:  None
//
// Returns:     LONG - XQueue error code (see XQueue.h)
//
LONG CXQueue::OpenSyncObjects()
{
    TCHAR  szName[_MAX_PATH*2] = { 0 };
    size_t len = 0;

    // first close any open handles

    if (m_hWriteMutex) {
        ::CloseHandle(m_hWriteMutex);
        m_hWriteMutex = NULL;
    }

    if (m_hClientWriteEvent) {
        ::CloseHandle(m_hClientWriteEvent);
        m_hClientWriteEvent = NULL;
    }

    if (m_hServerNotifyEvent) {
        ::CloseHandle(m_hServerNotifyEvent);
        m_hServerNotifyEvent = NULL;
    }


    ///////////////////////////////////////////////////////////////////////////
    // Open the mutex object that was created by the server. The server or
    // client sets this to nonsignaled when it is writing to the mmf.
    _tcscpy_s(szName, MAX_PATH * 2, m_szQueueName);
    len = _tcslen(szName);
    _tcsncat_s(szName, MAX_PATH * 2, WRITE_MUTEX_NAME, _countof(szName) - len - 1);

    m_hWriteMutex = ::OpenMutex( MUTEX_ALL_ACCESS,        // all access
                                 FALSE,                // not inheritable
                                 szName );                // mutex name

    if (!m_hWriteMutex) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  OpenMutex failed for m_hWriteMutex (%s)\n"), szName);
#endif 
        return XQUEUE_ERROR_INTERNAL;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Open the event object that was created by the server.  The
    // client sets this to signaled when it writes to the queue.
    _tcscpy_s(szName, MAX_PATH * 2, m_szQueueName);
    len = _tcslen(szName);
    _tcsncat_s(szName, MAX_PATH * 2, SERVER_NOTIFY_EVENT_NAME, _countof(szName) - len - 1);

    m_hServerNotifyEvent = ::OpenEvent( EVENT_ALL_ACCESS,    // all access
                                        FALSE,               // not inheritable
                                        szName );            // event name

    if (!m_hServerNotifyEvent) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  OpenEvent failed for m_hServerNotifyEvent (%s)\n"), szName);
#endif 
        ::CloseHandle(m_hWriteMutex);
        m_hWriteMutex = NULL;
        return XQUEUE_ERROR_INTERNAL;
    }


    ///////////////////////////////////////////////////////////////////////////
    // Open the auto reset event object. A client sets this to
    // nonsignaled before attempting to write to the queue;
    // other clients will check this event before attempting to write.
    // This ensures that the server will free some blocks if there
    // are insufficient free blocks.
    _tcscpy_s(szName, MAX_PATH * 2, m_szQueueName);
    len = _tcslen(szName);
    _tcsncat_s(szName, MAX_PATH * 2, CLIENT_WRITE_EVENT_NAME, _countof(szName) - len - 1);

    m_hClientWriteEvent = OpenEvent( EVENT_ALL_ACCESS,    // all access
                                     FALSE,               // not inheritable
                                     szName );            // event name

    if (!m_hClientWriteEvent) {
#if _DEBUG_XQUEUE
        TRACE(_T("ERROR:  OpenEvent failed for m_hClientWriteEvent (%s)\n"), szName);
#endif 
        ::CloseHandle(m_hWriteMutex);
        m_hWriteMutex = NULL;
        ::CloseHandle(m_hServerNotifyEvent);
        m_hServerNotifyEvent = NULL;
        return XQUEUE_ERROR_INTERNAL;
    }

    return XQUEUE_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// CalculateNoBlocks() - private function
//
// Purpose:     Calculate number of blocks it will take for the specified
//              message size.
//
// Parameters:  nDataBytes - [in] number of bytes in message
//
// Returns:     DWORD     - number of blocks required for message
//
DWORD CXQueue::CalculateNoBlocks(DWORD nDataBytes)
{
#if _DEBUG_XQUEUE
    TRACE(_T("in CXQueue::CalculateNoBlocks:  nDataBytes=%d  m_nDataSize=%d\n"), nDataBytes, m_nDataSize);
#endif 
    DWORD nBlocks = 0;

    nBlocks = nDataBytes / m_nDataSize;

    if ((nDataBytes % m_nDataSize) != 0) {
        nBlocks++;
    }

    if (nBlocks == 0) {
        nBlocks = 1;
    }
#if _DEBUG_XQUEUE
    TRACE(_T("CXQueue::CalculateNoBlocks returning %d =====\n"), nBlocks);
#endif
    return nBlocks;
}

