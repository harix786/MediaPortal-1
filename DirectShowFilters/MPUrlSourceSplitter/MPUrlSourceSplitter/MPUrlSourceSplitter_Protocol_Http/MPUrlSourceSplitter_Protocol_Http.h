/*
    Copyright (C) 2007-2010 Team MediaPortal
    http://www.team-mediaportal.com

    This file is part of MediaPortal 2

    MediaPortal 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MediaPortal 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaPortal 2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef __MP_URL_SOURCE_SPLITTER_PROTOCOL_HTTP_DEFINED
#define __MP_URL_SOURCE_SPLITTER_PROTOCOL_HTTP_DEFINED

#include "Logger.h"
#include "ProtocolPlugin.h"
#include "HttpCurlInstance.h"
#include "MediaPacketCollection.h"
#include "CacheFile.h"

#include <WinSock2.h>

#define PROTOCOL_NAME                                                         L"HTTP"

#define TOTAL_SUPPORTED_PROTOCOLS                                             1
wchar_t *SUPPORTED_PROTOCOLS[TOTAL_SUPPORTED_PROTOCOLS] =                     { L"HTTP" };

#define MINIMUM_RECEIVED_DATA_FOR_SPLITTER                                    1 * 1024 * 1024

#define MP_URL_SOURCE_SPLITTER_PROTOCOL_HTTP_FLAG_NONE                        PROTOCOL_PLUGIN_FLAG_NONE

#define MP_URL_SOURCE_SPLITTER_PROTOCOL_HTTP_FLAG_RANGES_SUPPORTED            (1 << (PROTOCOL_PLUGIN_FLAG_LAST + 0))
#define MP_URL_SOURCE_SPLITTER_PROTOCOL_HTTP_FLAG_SEEKING_SUPPORT_DETECTION   (1 << (PROTOCOL_PLUGIN_FLAG_LAST + 1))

#define MP_URL_SOURCE_SPLITTER_PROTOCOL_HTTP_FLAG_LAST                        (PROTOCOL_PLUGIN_FLAG_LAST + 2)

class CMPUrlSourceSplitter_Protocol_Http : public CProtocolPlugin
{
public:
  // constructor
  // create instance of CMPUrlSourceSplitter_Protocol_Http class
  CMPUrlSourceSplitter_Protocol_Http(HRESULT *result, CLogger *logger, CParameterCollection *configuration);

  // destructor
  ~CMPUrlSourceSplitter_Protocol_Http(void);

  // IProtocol interface

  // gets connection state
  // @return : one of protocol connection state values
  ProtocolConnectionState GetConnectionState(void);

  // parse given url to internal variables for specified protocol
  // errors should be logged to log file
  // @param parameters : the url and connection parameters
  // @return : S_OK if successfull
  HRESULT ParseUrl(const CParameterCollection *parameters);

  // receives data and process stream package request
  // the method can't block call (method is called within thread which can be terminated anytime)
  // @param streamPackage : the stream package request to process
  // @return : S_OK if successful, error code only in case when error is not related to processing request
  HRESULT ReceiveData(CStreamPackage *streamPackage);

  // gets current connection parameters (can be different as supplied connection parameters)
  // @param parameters : the reference to parameter collection to be filled with connection parameters
  // @return : S_OK if successful, error code otherwise
  HRESULT GetConnectionParameters(CParameterCollection *parameters);

  // ISimpleProtocol interface

  // get timeout (in ms) for receiving data
  // @return : timeout (in ms) for receiving data
  unsigned int GetReceiveDataTimeout(void);

  // starts receiving data from specified url and configuration parameters
  // @param parameters : the url and parameters used for connection
  // @return : S_OK if url is loaded, false otherwise
  HRESULT StartReceivingData(CParameterCollection *parameters);

  // request protocol implementation to cancel the stream reading operation
  // @return : S_OK if successful
  HRESULT StopReceivingData(void);

  // retrieves the progress of the stream reading operation
  // @param streamProgress : reference to instance of class that receives the stream progress
  // @return : S_OK if successful, VFW_S_ESTIMATED if returned values are estimates, E_INVALIDARG if stream ID is unknown, E_UNEXPECTED if unexpected error
  HRESULT QueryStreamProgress(CStreamProgress *streamProgress);
  
  // clear current session
  // @return : S_OK if successfull
  HRESULT ClearSession(void);

  // gets duration of stream in ms
  // @return : stream duration in ms or DURATION_LIVE_STREAM in case of live stream or DURATION_UNSPECIFIED if duration is unknown
  int64_t GetDuration(void);

  // gets stream count
  // receiving data is disabled until protocol reports valid stream count (at least one)
  // @return : stream count or STREAM_COUNT_UNKNOWN if not known
  unsigned int GetStreamCount(void);

  // ISeeking interface

  // gets seeking capabilities of protocol
  // @return : bitwise combination of SEEKING_METHOD flags
  unsigned int GetSeekingCapabilities(void);

  // request protocol implementation to receive data from specified time (in ms) for specified stream
  // this method is called with same time for each stream in protocols with multiple streams
  // @param streamId : the stream ID to receive data from specified time
  // @param time : the requested time (zero is start of stream)
  // @return : time (in ms) where seek finished or lower than zero if error
  int64_t SeekToTime(unsigned int streamId, int64_t time);

  // CPlugin implementation

  // return reference to null-terminated string which represents plugin name
  // errors should be logged to log file and returned NULL
  // @return : reference to null-terminated string
  virtual const wchar_t *GetName(void);

  // get plugin instance ID
  // @return : GUID, which represents instance identifier or GUID_NULL if error
  virtual GUID GetInstanceId(void);

  // initialize plugin implementation with configuration parameters
  // @param configuration : the reference to additional configuration parameters (created by plugin's hoster class)
  // @return : S_OK if successfull, error code otherwise
  virtual HRESULT Initialize(CPluginConfiguration *configuration);

protected:
  // holds connection state
  ProtocolConnectionState connectionState;

  // mutex for locking access to file, buffer, ...
  HANDLE lockMutex;
  // mutex for locking access to internal buffer of CURL instance
  HANDLE lockCurlMutex;

  // holds receive data timeout
  unsigned int receiveDataTimeout;

  // main instance of CURL
  CHttpCurlInstance *mainCurlInstance;
  // holds current cookies of CURL instance
  CParameterCollection *currentCookies;
  // holds media packets with received data
  CMediaPacketCollection *mediaPackets;
  // holds cache file
  CCacheFile *cacheFile;
  // start, end and current stream position
  int64_t startStreamPosition;
  int64_t endStreamPosition;
  int64_t currentStreamPosition;
  // holds current stream length
  int64_t streamLength;
  // holds last store time to cache file
  unsigned int lastStoreTime;
  // holds last receive data time
  unsigned int lastReceiveDataTime;

  /* methods */

  // gets store file name
  // @return : store file name or NULL if error
  wchar_t *GetStoreFile(void);
};

#endif
