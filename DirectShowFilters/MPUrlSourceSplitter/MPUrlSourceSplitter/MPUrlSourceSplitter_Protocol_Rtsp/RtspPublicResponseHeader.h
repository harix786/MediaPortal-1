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

#ifndef __RTSP_PUBLIC_RESPONSE_HEADER_DEFINED
#define __RTSP_PUBLIC_RESPONSE_HEADER_DEFINED

#include "RtspResponseHeader.h"

#define RTSP_PUBLIC_RESPONSE_HEADER_TYPE                              L"Public"

#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_DESCRIBE                   L"DESCRIBE"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_ANNOUNCE                   L"ANNOUNCE"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_GET_PARAMETER              L"GET_PARAMETER"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_OPTIONS                    L"OPTIONS"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_PAUSE                      L"PAUSE"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_PLAY                       L"PLAY"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_RECORD                     L"RECORD"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_REDIRECT                   L"REDIRECT"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_SETUP                      L"SETUP"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_SET_PARAMETER              L"SET_PARAMETER"
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_TEARDOWN                   L"TEARDOWN"

#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_DESCRIBE_LENGTH            8
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_ANNOUNCE_LENGTH            8
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_GET_PARAMETER_LENGTH       13
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_OPTIONS_LENGTH             7
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_PAUSE_LENGTH               5
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_PLAY_LENGTH                4
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_RECORD_LENGTH              6
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_REDIRECT_LENGTH            8
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_SETUP_LENGTH               5
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_SET_PARAMETER_LENGTH       13
#define RTSP_PUBLIC_RESPONSE_HEADER_METHOD_TEARDOWN_LENGTH            8

#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_DESCRIBE              (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 0))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_ANNOUNCE              (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 1))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_GET_PARAMETER         (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 2))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_OPTIONS               (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 3))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_PAUSE                 (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 4))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_PLAY                  (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 5))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_RECORD                (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 6))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_REDIRECT              (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 7))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_SETUP                 (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 8))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_SET_PARAMETER         (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 9))
#define RTSP_PUBLIC_RESPONSE_HEADER_FLAG_METHOD_TEARDOWN              (1 << (MAX_RTSP_RESPONSE_HEADER_FLAG + 10))

class CRtspPublicResponseHeader : public CRtspResponseHeader
{
public:
  CRtspPublicResponseHeader(void);
  virtual ~CRtspPublicResponseHeader(void);

  /* get methods */

  /* set methods */

  /* other methods */

  // tests if method DESCRIBE is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedDescribeMethod(void);

  // tests if method ANNOUNCE is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedAnnounceMethod(void);

  // tests if method GET_PARAMETER is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedGetParameterMethod(void);

  // tests if method OPTIONS is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedOptionsMethod(void);

  // tests if method PAUSE is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedPauseMethod(void);

  // tests if method PLAY is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedPlayMethod(void);

  // tests if method RECORD is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedRecordMethod(void);

  // tests if method REDIRECT is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedRedirectMethod(void);

  // tests if method SETUP is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedSetupMethod(void);

  // tests if method SET_PARAMETER is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedSetParameterMethod(void);

  // tests if method TEARDOWN is defined
  // @return : true if method is defined, false otherwise
  virtual bool IsDefinedTeardownMethod(void);

  // deep clones of current instance
  // @return : deep clone of current instance or NULL if error
  virtual CRtspPublicResponseHeader *Clone(void);

  // parses header and stores name and value to internal variables
  // @param header : header to parse
  // @param length : the length of header
  // @return : true if successful, false otherwise
  virtual bool Parse(const wchar_t *header, unsigned int length);

protected:

  // deeply clones current instance to cloned header
  // @param  clonedHeader : cloned header to hold clone of current instance
  // @return : true if successful, false otherwise
  virtual bool CloneInternal(CHttpHeader *clonedHeader);

  // returns new header object to be used in cloning
  // @return : header object or NULL if error
  virtual CHttpHeader *GetNewHeader(void);
};

#endif