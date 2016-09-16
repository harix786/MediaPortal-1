// Copyright (C) 2006-2015 Team MediaPortal
// http://www.team-mediaportal.com
// 
// MediaPortal is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// MediaPortal is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with MediaPortal. If not, see <http://www.gnu.org/licenses/>.

/**
*  MultiFileWriter.cpp
*  Copyright (C) 2006-2007      nate
*
*  This file is part of TSFileSource, a directshow push source filter that
*  provides an MPEG transport stream output.
*
*  TSFileSource is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  TSFileSource is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with TSFileSource; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*  nate can be reached on the forums at
*    http://forums.dvbowners.com/
*/
#define _WIN32_WINNT 0x0502

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)
#include <streams.h>
#include "MultiFileWriter.h"
#include <atlbase.h>
#include <windows.h>
#include <stdio.h>
extern void LogDebug(const char *fmt, ...) ;
extern void LogDebug(const wchar_t *fmt, ...) ;

///*******************************************
///Class which holds a single disk buffer
///

CDiskBuff::CDiskBuff(int size)
{
  m_iLength=0;
  m_pBuffer = new byte[size];
  m_iSize = size;
}

CDiskBuff::~CDiskBuff()
{
  delete [] m_pBuffer;
  m_pBuffer=NULL;
  m_iLength=0;
}

// Adds data contained to this buffer
int CDiskBuff::Add(byte* data, int len)
{
  if((m_iSize >= m_iLength + len ) && data) 
  {
    memcpy(&m_pBuffer[m_iLength], data, len);
    m_iLength+=len;
    return 0; //All data written
  }
  else
  {
    return len; //No data written/consumed
  }
}

// returns the length in bytes of the buffer
int CDiskBuff::Length()
{
  return m_iLength;
}

// returns the buffer
byte* CDiskBuff::Data()
{
  if (m_pBuffer==NULL)
  {
    return NULL;
  }
  return m_pBuffer;
}

///*******************************************
/// MultiFileWriter code 
///

MultiFileWriter::MultiFileWriter(MultiFileWriterParam *pWriterParams) :
	m_hTSBufferFile(INVALID_HANDLE_VALUE),
	m_pTSBufferFileName(NULL),
	m_pCurrentTSFile(NULL),
	m_bThreadRunning(FALSE),
	m_hThreadProc(NULL),
	m_filesAdded(0),
	m_filesRemoved(0),
	m_currentFilenameId(0),
	m_currentFileId(0),
	m_minTSFiles(pWriterParams->minFiles),
	m_maxTSFiles(pWriterParams->maxFiles),
	m_maxTSFileSize(pWriterParams->maxSize),	
	m_chunkReserve(pWriterParams->chunkSize),
	m_maxBuffersUsed(0),
  m_pDiskBuffer(NULL),
	m_bDiskFull(FALSE),
	m_bBufferFull(FALSE)
{
}

MultiFileWriter::~MultiFileWriter()
{
	if (m_pTSBufferFileName != NULL)
	{
  	Close();
  }
			
  LogDebug("MultiFileWriter::Dtor() end");
}

HRESULT MultiFileWriter::GetFileName(LPOLESTR *lpszFileName)
{
	CAutoLock lock(&m_Lock);
	*lpszFileName = m_pTSBufferFileName;
	return S_OK;
}

HRESULT MultiFileWriter::Open(LPCWSTR pszFileName)
{
	CAutoLock lock(&m_Lock);

  // Are we already open?
  if (m_pTSBufferFileName != NULL)
  {
    return E_FAIL;
  }

	// Is this a valid filename supplied
	CheckPointer(pszFileName,E_POINTER);

	if(wcslen(pszFileName) > MAX_PATH)
  {
    LogDebug(L"MultiFileWriter: filename too long");
		return ERROR_FILENAME_EXCED_RANGE;
  }
	// Take a copy of the filename
	m_pTSBufferFileName = new WCHAR[1+lstrlenW(pszFileName)];
	if (m_pTSBufferFileName == NULL)
		return E_OUTOFMEMORY;
	wcscpy(m_pTSBufferFileName, pszFileName);

  try 
  {
  	m_pCurrentTSFile = new FileWriter();
  }
  catch(...)
  {
    m_pCurrentTSFile = NULL;
		delete[] m_pTSBufferFileName;
		m_pTSBufferFileName = NULL;
    return E_FAIL;
  }
  
	m_pCurrentTSFile->SetChunkReserve(m_chunkReserve, m_maxTSFileSize);

	StartThread();

  m_WakeThreadEvent.Set(); //Trigger thread to open file
	
	return S_OK;
}

HRESULT MultiFileWriter::OpenFile()
{
	CAutoLock lock(&m_Lock);

	// Is the file already opened
	if (m_hTSBufferFile != INVALID_HANDLE_VALUE)
	{
		return E_FAIL;
	}

  // Has a filename been set yet?
  if (m_pTSBufferFileName == NULL)
  {
    return ERROR_INVALID_NAME;
  }

  ::DeleteFileW((LPCWSTR) m_pTSBufferFileName);
	
	//check disk space first
	__int64 llDiskSpaceAvailable = 0;
	if (SUCCEEDED(GetAvailableDiskSpace(&llDiskSpaceAvailable)) && (__int64)llDiskSpaceAvailable < (__int64)(m_maxTSFileSize*2))
  {
    LogDebug("MultiFileWriter: not enough free diskspace");
		return E_FAIL;
  }

	// Try to open the file
	m_hTSBufferFile = CreateFileW(m_pTSBufferFileName,              // The filename
								 (DWORD) GENERIC_WRITE,             // File access
								 (DWORD) (FILE_SHARE_READ | FILE_SHARE_WRITE),           // Share access
								 NULL,                              // Security
								 (DWORD) CREATE_ALWAYS,             // Open flags
								 (DWORD) (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS),     // More flags
								 //(DWORD) FILE_ATTRIBUTE_NORMAL,     // More flags
								 NULL);                             // Template

	if (m_hTSBufferFile == INVALID_HANDLE_VALUE)
	{
        LogDebug("MultiFileWriter: fail to create buffer file");
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
	}
	
	m_maxBuffersUsed = 0;
	m_bDiskFull = FALSE;
	m_bBufferFull = FALSE; 

  LogDebug(L"MultiFileWriter: OpenFile() succeeded, filename: %s", m_pTSBufferFileName);

	return S_OK;
}

//
// Close
//
HRESULT MultiFileWriter::Close()
{	
  //Wait for all buffers to be written to disk
  PushBuffer(); //Force temp buffer onto queue
  m_WakeThreadEvent.Set();
  for (;;)
  { 
  	if (m_bDiskFull || (m_hTSBufferFile == INVALID_HANDLE_VALUE) || !m_hThreadProc)
  	{
  	  //If we can't flush the buffers to disk, then just discard the data
  	  ClearBuffers();
  	}
  	
    {//Context for CAutoLock
      CAutoLock lock(&m_qLock);
      if (m_writeQueue.size() == 0)
      {
        break;
      }
    }
    Sleep(1);
  }

  //Don't m_Lock before this point to avoid deadlock when m_writeQueue.size() > 0
  
  StopThread();
  
  { //Context for CAutoLock
  	CAutoLock lock(&m_Lock);
  
  	ClearBuffers();
  	  
  	if (m_hTSBufferFile != INVALID_HANDLE_VALUE)
  	{
    	CloseHandle(m_hTSBufferFile);
    	m_hTSBufferFile = INVALID_HANDLE_VALUE;
    }
  
  	m_pCurrentTSFile->CloseFile();
  	
  	CleanupFiles();
  	
  	if (m_pCurrentTSFile)
  	{
  		delete m_pCurrentTSFile;		
      m_pCurrentTSFile = NULL;
    }

  	if (m_pTSBufferFileName != NULL)
  	{
  	  LogDebug(L"MultiFileWriter: Close(), filename: %s, Max buffers used: %d", m_pTSBufferFileName, m_maxBuffersUsed);
  		delete[] m_pTSBufferFileName;
 		  m_pTSBufferFileName = NULL;
 	  } 	  
  }
		
	return S_OK;
}

HRESULT MultiFileWriter::GetFileSize(__int64 *lpllsize)
{
	CAutoLock lock(&m_Lock);
	
	if (m_hTSBufferFile == INVALID_HANDLE_VALUE)
		*lpllsize = 0;
	else
		*lpllsize = max(0, (__int64)(((__int64)(m_filesAdded - m_filesRemoved - 1) * m_maxTSFileSize) + m_pCurrentTSFile->GetFilePointer()));

	return S_OK;
}

HRESULT MultiFileWriter::WriteToDisk(PBYTE pbData, ULONG lDataLength)
{
	CAutoLock lock(&m_Lock);

  // Is the file open yet?
  if (m_hTSBufferFile == INVALID_HANDLE_VALUE)
  {
    return S_FALSE;
  }

	HRESULT hr;

	CheckPointer(pbData,E_POINTER);
	if (lDataLength == 0)
		return S_OK;

	if (m_pCurrentTSFile->IsFileInvalid())
	{
		//::LogDebug("Creating first file");
		if FAILED(hr = PrepareTSFile())
			return hr;
	}

	//Get File Position
	__int64 filePosition = m_pCurrentTSFile->GetFilePointer();

	// See if we will need to create more ts files.
	if (filePosition + lDataLength > m_maxTSFileSize)
	{
		__int64 dataToWrite = m_maxTSFileSize - filePosition;

		// Write some data to the current file if it's not full
		if (dataToWrite > 0)
		{
  		if FAILED(hr = m_pCurrentTSFile->WriteWithRetry(pbData, (ULONG)dataToWrite, FILE_WRITE_RETRIES))
  		{
			  // We might be running out of disk space, so just drop the data
  			return hr;
  		}
		}

		// Try to create a new file
		if FAILED(hr = PrepareTSFile())
		{
			// Buffer is probably full and the oldest file is still locked.
			// We'll just start dropping data
			return hr;
		}

		// Try writing the remaining data now that a new file has been created.
		pbData += dataToWrite;
		lDataLength -= (ULONG)dataToWrite;
		return WriteToDisk(pbData, lDataLength);
	}
	else
	{
		if FAILED(hr = m_pCurrentTSFile->WriteWithRetry(pbData, lDataLength, FILE_WRITE_RETRIES))
		{
			// We might be running out of disk space, so just drop the data
			return hr;
		}
	}

	WriteTSBufferFile();
	
	return S_OK;
}

HRESULT MultiFileWriter::PrepareTSFile()
{
	USES_CONVERSION;
  CAutoLock lock(&m_Lock);
	HRESULT hr;

	//LogDebug("PrepareTSFile()");
	
	// Make sure the old file is closed
	m_pCurrentTSFile->CloseFile();

	__int64 llDiskSpaceAvailable = 0;
	if (SUCCEEDED(GetAvailableDiskSpace(&llDiskSpaceAvailable)) && (__int64)llDiskSpaceAvailable < (__int64)(m_maxTSFileSize*2))
	{
	  //Not enough free disk space, so try to reuse the oldest file
		hr = ReuseTSFile();
	}
	else
	{
		if (m_tsFileNames.size() >= (UINT)m_minTSFiles) 
		{
			if FAILED(hr = ReuseTSFile())
			{
				if (m_tsFileNames.size() < (UINT)m_maxTSFiles)
				{
					if (hr != HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION)) // ERROR_SHARING_VIOLATION means file is being read by another process
						LogDebug("MultiFileWriter: Failed to reopen old file. Unexpected reason. Trying to create a new file.");

					hr = CreateNewTSFile();
				}
				else
				{
					if (hr != HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION)) // ERROR_SHARING_VIOLATION means file is being read by another process
					{
						LogDebug("MultiFileWriter: Failed to reopen old file. Unexpected reason. Dropping data!");
				  }
					else if (!m_bDiskFull) //If we have reached the max files limit then suppress this logging
					{
						LogDebug("MultiFileWriter: Failed to reopen old file. It's currently in use. Dropping data!");
				  }

					Sleep(500);
				}
			}
		}	
		else
		{
			hr = CreateNewTSFile();
		}
	}

  if (FAILED(hr) != m_bDiskFull)
  {
    m_bDiskFull = FAILED(hr);
	  if (!m_bDiskFull)
  	{
	    LogDebug("MultiFileWriter: Timeshift space available - clearing buffers");
	    
	    //As the buffer file has only just been released (by the read process)
	    //we need to create space (time) between the read and write positions
	    //to avoid stutter issues at the next buffer file changeover, so 
	    //sleep for a short time then discard all the queued write data.
	    Sleep(1500);
	    
  	  //Discard all stored data in the queued so we restart file writing with current data
  	  ClearBuffers();
  	  
	    LogDebug("MultiFileWriter: Timeshifting resumed");
  	  
  	  //Return S_FALSE so that the current write data is discarded (it is 'old' data from the queue)
  	  return S_FALSE;
  	}
  	else
  	{
	    LogDebug("MultiFileWriter: Timeshift full !!");
  	}
  }

	return hr;
}

HRESULT MultiFileWriter::CreateNewTSFile()
{
	CAutoLock lock(&m_Lock);
	HRESULT hr;

	LPWSTR pFilename = new wchar_t[MAX_PATH];
	WIN32_FIND_DATAW findData;
	HANDLE handleFound = INVALID_HANDLE_VALUE;

	//LogDebug("CreateNewTSFile.");
	while (TRUE)
	{
		// Create new filename
		m_currentFilenameId++;
		swprintf(pFilename, L"%s%i.ts", m_pTSBufferFileName, m_currentFilenameId);

		// Check if file already exists
		handleFound = FindFirstFileW(pFilename, &findData);
		if (handleFound == INVALID_HANDLE_VALUE)
			break;

		//LogDebug("Newly generated filename already exists.");

		// If it exists we loop and try the next number
		FindClose(handleFound);
	}
	
	if FAILED(hr = m_pCurrentTSFile->SetFileName(pFilename))
	{
		LogDebug("Failed to set filename for new file.");
		delete[] pFilename;
		return hr;
	}

	if FAILED(hr = m_pCurrentTSFile->OpenFile())
	{
		LogDebug("Failed to open new file");
		delete[] pFilename;
		return hr;
	}

	m_tsFileNames.push_back(pFilename);
	m_filesAdded++;

	LPWSTR pos = pFilename + wcslen(m_pTSBufferFileName);
	if (pos)
		m_currentFileId = _wtoi(pos);
	wchar_t msg[MAX_PATH];
	swprintf(msg, L"New file created : %s\n", pFilename);
	::OutputDebugStringW(msg);

	//LogDebug("new file created");
	return S_OK;
}

HRESULT MultiFileWriter::ReuseTSFile()
{
	CAutoLock lock(&m_Lock);
	HRESULT hr;
	DWORD Tmo=10 ;

	LPWSTR pFilename = m_tsFileNames.at(0);

	if FAILED(hr = m_pCurrentTSFile->SetFileName(pFilename))
	{
		LogDebug(L"Failed to set filename to reuse old file");
		return hr;
	}

	// Check if file is being read by something.
	// Can be locked temporarily to update duration or definitely (!) if timeshift is paused.
	do
	{
	  hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
	  if (IsFileLocked(pFilename) == FALSE)
	  {
  		DeleteFileW(pFilename);	// Stupid function, return can be ok and file not deleted ( just tagged for deleting )!!!!
  		hr = m_pCurrentTSFile->OpenFile() ;
  		if (!FAILED(hr)) break ;
	  }
	  
		Sleep(20) ;
	}
	while (--Tmo) ;

  if (Tmo)
  {
    if (Tmo<4) // 1 failed + 1 succeded is quasi-normal, more is a bit suspicious ( disk drive too slow or problem ? )
			LogDebug(L"MultiFileWriter: %d tries to succeed deleting and re-opening %s.", 6-Tmo, pFilename);
  }
  else
	{
	  if (!m_bDiskFull) //If we have reached the max files limit then suppress this logging
	  {
      LogDebug(L"MultiFileWriter: failed to create file %s", pFilename);
    }
		return hr ;
	}

	// if stuff worked then move the filename to the end of the files list
	m_tsFileNames.erase(m_tsFileNames.begin());
	m_filesRemoved++;

	m_tsFileNames.push_back(pFilename);
	m_filesAdded++;

	LPWSTR pos = pFilename + wcslen(m_pTSBufferFileName);
	if (pos)
		m_currentFileId = _wtoi(pos);
	wchar_t msg[MAX_PATH];
	swprintf(msg, L"Old file reused : %s\n", pFilename);
	::OutputDebugStringW(msg);

	//LogDebug("reuse old file");
	return S_OK;
}


HRESULT MultiFileWriter::WriteTSBufferFile()
{
	CAutoLock lock(&m_Lock);
	LARGE_INTEGER li;
	DWORD written = 0;

	// Move to the start of the file
	li.QuadPart = 0;
	SetFilePointer(m_hTSBufferFile, li.LowPart, &li.HighPart, FILE_BEGIN);

	// Write current position of most recent file.
	__int64 currentPointer = m_pCurrentTSFile->GetFilePointer();

	BYTE* writeBuffer = new BYTE[65536];
	BYTE* writePointer = writeBuffer;

	*((__int64*)writePointer) = currentPointer;
	writePointer += sizeof(__int64);
	
	*((long*)writePointer) = m_filesAdded;
	writePointer += sizeof(long);
	
	*((long*)writePointer) = m_filesRemoved;
	writePointer += sizeof(long);

	// Write out all the filenames (null terminated)
	std::vector<LPWSTR>::iterator it = m_tsFileNames.begin();
	for ( ; it < m_tsFileNames.end() ; it++ )
	{
		LPWSTR pFilename = *it;
		long length = wcslen(pFilename)+1;
		length *= sizeof(wchar_t);

		memcpy(writePointer, pFilename, length);
		writePointer += length;

		if((writePointer - writeBuffer) > 60000)
		{
			LogDebug("MultiFileWriter: TS buffer file has exceeded maximum length.  Reduce the number of timeshifting files");
			delete[] writeBuffer;
			return S_FALSE;
		}
	}

	// Finish up with a unicode null character in case we want to put stuff after this in the future.
	wchar_t temp = 0;
	*((wchar_t*)writePointer) = temp;
	writePointer += sizeof(wchar_t);
	
	
	*((long*)writePointer) = m_filesAdded;
	writePointer += sizeof(long);
	
	*((long*)writePointer) = m_filesRemoved;
	writePointer += sizeof(long);

	WriteFile(m_hTSBufferFile, writeBuffer, writePointer - writeBuffer, &written, NULL);
	delete[] writeBuffer;


	return S_OK;
}

HRESULT MultiFileWriter::CleanupFiles()
{
	CAutoLock lock(&m_Lock);

	m_filesAdded = 0;
	m_filesRemoved = 0;
	m_currentFilenameId = 0;
	m_currentFileId = 0;

	// Check if .tsbuffer file is being read by something.
	if (IsFileLocked(m_pTSBufferFileName) == TRUE)
		return S_OK;

	std::vector<LPWSTR>::iterator it;
	for (it = m_tsFileNames.begin() ; it < m_tsFileNames.end() ; it++ )
	{
		if (IsFileLocked(*it) == TRUE)
		{
			// If any of the files are being read then we won't
			// delete any so that the full buffer stays intact.
			wchar_t msg[MAX_PATH];
			swprintf(msg, L"CleanupFiles: A file is still locked : %s\n", *it);
			::OutputDebugStringW((LPWSTR)&msg);
			LogDebug(L"CleanupFiles: A file is still locked");
			return S_OK;
		}
	}

	// Now we know we can delete all the files.

	for (it = m_tsFileNames.begin() ; it < m_tsFileNames.end() ; it++ )
	{
		if (DeleteFileW(*it) == FALSE)
		{
			wchar_t msg[MAX_PATH];
			swprintf(msg, L"Failed to delete file %s : 0x%x\n", *it, GetLastError());
			::OutputDebugStringW(msg);
			LogDebug(L"CleanupFiles: Failed to delete file");
		}
		delete[] *it;
	}
	m_tsFileNames.clear();

	if (DeleteFileW(m_pTSBufferFileName) == FALSE)
	{
		wchar_t msg[MAX_PATH];
		swprintf(msg, L"Failed to delete tsbuffer file : 0x%x\n", GetLastError());
		::OutputDebugStringW(msg);
			LogDebug(L"CleanupFiles: Failed to delete tsbuffer file: 0x%x\n", GetLastError());

	}
	m_filesAdded = 0;
	m_filesRemoved = 0;
	m_currentFilenameId = 0;
	m_currentFileId = 0;
	return S_OK;
}

BOOL MultiFileWriter::IsFileLocked(LPWSTR pFilename)
{
	CAutoLock lock(&m_Lock);
	HANDLE hFile;
	hFile = CreateFileW(pFilename,               // The filename
					   (DWORD) GENERIC_READ,             // File access
					   (DWORD) NULL,                     // Share access
					   NULL,                             // Security
					   (DWORD) OPEN_EXISTING,            // Open flags
					   (DWORD) 0,                        // More flags
					   NULL);                            // Template

	if (hFile == INVALID_HANDLE_VALUE)
		return TRUE;

	CloseHandle(hFile);
	return FALSE;
}

HRESULT MultiFileWriter::GetAvailableDiskSpace(__int64* llAvailableDiskSpace)
{
	if (!llAvailableDiskSpace)
		return E_INVALIDARG;

	CAutoLock lock(&m_Lock);
	HRESULT hr;

	wchar_t	*pszDrive = NULL;
	wchar_t	szDrive[4];
	if (m_pTSBufferFileName[1] == L':')
	{
		szDrive[0] = (wchar_t)m_pTSBufferFileName[0];
		szDrive[1] = L':';
		szDrive[2] = L'\\';
		szDrive[3] = 0;
		pszDrive = szDrive;
	}

	ULARGE_INTEGER uliDiskSpaceAvailable;
	ULARGE_INTEGER uliDiskSpaceTotal;
	ULARGE_INTEGER uliDiskSpaceFree;
	uliDiskSpaceAvailable.QuadPart= 0;
	uliDiskSpaceTotal.QuadPart= 0;
	uliDiskSpaceFree.QuadPart= 0;
	hr = GetDiskFreeSpaceExW(pszDrive, &uliDiskSpaceAvailable, &uliDiskSpaceTotal, &uliDiskSpaceFree);
	if SUCCEEDED(hr)
		*llAvailableDiskSpace = uliDiskSpaceAvailable.QuadPart;
	else
		*llAvailableDiskSpace = 0;

	return hr;
}

FileWriter* MultiFileWriter::getCurrentTSFile(void)
{
	CAutoLock lock(&m_Lock);
	return m_pCurrentTSFile;
}

long MultiFileWriter::getNumbFilesAdded(void)
{
	CAutoLock lock(&m_Lock);
	return m_filesAdded;
}

long MultiFileWriter::getNumbFilesRemoved(void)
{
	CAutoLock lock(&m_Lock);
	return m_filesRemoved;
}

long MultiFileWriter::getCurrentFileId(void)
{
	CAutoLock lock(&m_Lock);
	return m_currentFileId;//m_currentFilenameId;
}

long MultiFileWriter::getMinTSFiles(void)
{
	CAutoLock lock(&m_Lock);
	return m_minTSFiles;
}

void MultiFileWriter::setMinTSFiles(long minFiles)
{
	CAutoLock lock(&m_Lock);
	m_minTSFiles = minFiles;
}

long MultiFileWriter::getMaxTSFiles(void)
{
	CAutoLock lock(&m_Lock);
	return m_maxTSFiles;
}

void MultiFileWriter::setMaxTSFiles(long maxFiles)
{
	CAutoLock lock(&m_Lock);
	m_maxTSFiles = maxFiles;
}

__int64 MultiFileWriter::getMaxTSFileSize(void)
{
	CAutoLock lock(&m_Lock);
	return m_maxTSFileSize;
}

void MultiFileWriter::setMaxTSFileSize(__int64 maxSize)
{
	CAutoLock lock(&m_Lock);
	m_maxTSFileSize = maxSize;
	m_pCurrentTSFile->SetChunkReserve(m_chunkReserve, m_maxTSFileSize);
}

__int64 MultiFileWriter::getChunkReserve(void)
{
	CAutoLock lock(&m_Lock);
	return m_chunkReserve;
}

void MultiFileWriter::setChunkReserve(__int64 chunkSize)
{
	CAutoLock lock(&m_Lock);
	m_chunkReserve = chunkSize;
	m_pCurrentTSFile->SetChunkReserve(m_chunkReserve, m_maxTSFileSize);
}

void MultiFileWriter::GetPosition(__int64 * position)
{
	CAutoLock lock(&m_Lock);
	if (m_pCurrentTSFile==NULL)
	{
		*position=-1;
		return;
	}
	if (m_pCurrentTSFile->IsFileInvalid())
	{
		*position=-1;
		return;
	}
	*position=m_pCurrentTSFile->GetFilePointer();
}


void MultiFileWriter::ClearBuffers()
{
  CAutoLock qlock(&m_qLock);
  ivecDiskBuff it = m_writeQueue.begin();
  while (it != m_writeQueue.end())
  {
    CDiskBuff* diskBuffer = *it;
    delete diskBuffer;
    it = m_writeQueue.erase(it);
  }
  m_bBufferFull = FALSE;
}



////**************************************
//// Temporary Disk Buffer methods
////

HRESULT MultiFileWriter::NewBuffer(int size)
{
  if (m_pDiskBuffer != NULL) return S_FALSE;
  	
  try 
  {
    m_pDiskBuffer = new CDiskBuff(size);    
  }
  catch(...)
  {
    m_pDiskBuffer = NULL;
    LogDebug("MultiFileWriter::NewBuffer() buffer allocation exception, size = %d", size);
    return E_FAIL;
  }
  return S_OK;
}

HRESULT MultiFileWriter::AddToBuffer(byte* pbData, int len, int newBuffSize)
{
  if (m_pDiskBuffer == NULL)
  {
    if (NewBuffer(newBuffSize) != S_OK)
    {
      return E_FAIL;    
    }
  }   
    
	if (m_pDiskBuffer->Add(pbData,len) > 0)
	{
	  //Not enough space to add data to current buffer
    PushBuffer(); //Push the current buffer onto the disk write queue
    if (NewBuffer(newBuffSize) != S_OK)
    {
      return E_FAIL;    
    }
  	if (m_pDiskBuffer->Add(pbData,len) != 0)
  	{
      return E_FAIL;
 	  }
    return S_FALSE;
	}
	
  return S_OK;
}

HRESULT MultiFileWriter::PushBuffer()
{
  // Has a filename been set yet?
  if (m_pTSBufferFileName == NULL)
  {
		DiscardBuffer();
    return S_FALSE;
  }
  
  if (m_pDiskBuffer == NULL) return S_FALSE;
  { //Context for CAutoLock
    CAutoLock lock(&m_qLock);
    m_writeQueue.push_back(m_pDiskBuffer);
    m_pDiskBuffer = NULL;   
  }	  
  m_WakeThreadEvent.Set();    
	return S_OK;
}

HRESULT MultiFileWriter::DiscardBuffer()
{
  if (m_pDiskBuffer != NULL)
  {
    delete m_pDiskBuffer;
    m_pDiskBuffer = NULL;
  }
	return S_OK;
}

////**************************************
//// Write Thread methods
////

unsigned MultiFileWriter::thread_function(void* p)
{
	MultiFileWriter *thread = reinterpret_cast<MultiFileWriter *>(p);
	thread->ThreadProc();
	_endthreadex(0);
  return 0;
}

unsigned __stdcall MultiFileWriter::ThreadProc()
{
  LogDebug("MultiFileWriter::ThreadProc() started");
  CDiskBuff* diskBuffer = NULL;
  UINT qsize = 0;
  
  while (m_bThreadRunning)
  {    
    if (m_hTSBufferFile == INVALID_HANDLE_VALUE) //Open the file
    {
      // Has a filename been set yet ?
      if (m_pTSBufferFileName != NULL)
      {
        OpenFile();
      }
    }

    { //Context for CAutoLock
      CAutoLock lock(&m_qLock);
      qsize = m_writeQueue.size();
      if (qsize > 0) 
      {              
        //get the next buffer
        ivecDiskBuff it = m_writeQueue.begin();
        diskBuffer=*it;
        m_writeQueue.erase(it);
      }
      else
      {
        diskBuffer = NULL;   
      }
      
      if (qsize < NOT_FULL_BUFFERS)
      {
        m_bBufferFull = FALSE;
      }
    }
    
    if (diskBuffer != NULL)
    {
      WriteToDisk(diskBuffer->Data(), diskBuffer->Length());  
      delete diskBuffer;
      diskBuffer = NULL;
    }

    if (qsize > m_maxBuffersUsed) 
    {     
      m_maxBuffersUsed = qsize;
      //LogDebug("MultiFileWriter::ThreadProc(), Max buffers used = %d", m_maxBuffersUsed);
    }
    
    if (qsize < 2) //this is the pre 'pop' qsize value
    {
      //Sleep for 100ms, unless thread gets an event
      m_WakeThreadEvent.Wait(100);
    }
    //else there are more buffers to process, so go round again
  }
  
  if (diskBuffer != NULL)
  {
    delete diskBuffer;
  }
  LogDebug("MultiFileWriter::ThreadProc() finished");
  return 0;
}


void MultiFileWriter::StartThread()
{
  m_bThreadRunning = TRUE;
  UINT id;
  m_hThreadProc = (HANDLE)_beginthreadex(NULL, 0, &MultiFileWriter::thread_function, (void *) this, 0, &id);
  SetThreadPriority(m_hThreadProc, THREAD_PRIORITY_NORMAL);
  
  // Set timer resolution to SYS_TIMER_RES (if possible)
  TIMECAPS tc; 
  m_dwTimerResolution = 0; 
  if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == MMSYSERR_NOERROR)
  {
    m_dwTimerResolution = min(max(tc.wPeriodMin, SYS_TIMER_RES), tc.wPeriodMax);
    if (m_dwTimerResolution)
    {
      timeBeginPeriod(m_dwTimerResolution);
    }
  }
}


void MultiFileWriter::StopThread()
{
  if (m_hThreadProc)
  {
    m_bThreadRunning = FALSE;
    m_WakeThreadEvent.Set();
    WaitForSingleObject(m_hThreadProc, INFINITE);	
    m_WakeThreadEvent.Reset();
    m_hThreadProc = NULL;
  }
  
  // Reset timer resolution (if we managed to set it originally)
  if (m_dwTimerResolution)
  {
    timeEndPeriod(m_dwTimerResolution);
    m_dwTimerResolution = 0;
  }
}
