/*
 *      Copyright (C) 2014 Arne Morten Kvarving
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "libXBMC_addon.h"
#include <iostream>

extern "C" {
#include "wsr_player.h"
#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include "kodi_audiodec_dll.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

short* sample_buffer=NULL;

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct WSRContext
{
  short sample_buffer[576*2];
  size_t pos;
  uint64_t timepos;
};

int Load_WSR(const char *name)
{
  if (ROM) { free(ROM); ROM = NULL; }

  void* f = XBMC->OpenFile(name, 0);
  if (!f)
    return 0;

  ROMSize = XBMC->GetFileLength(f);
  ROMBank = (ROMSize+0xFFFF)>>16;
  ROM = (BYTE*)malloc(ROMBank*0x10000);
  if (!ROM) { 
    XBMC->CloseFile(f);
    return 0;
  }
  XBMC->ReadFile(f, ROM, ROMSize);
  XBMC->CloseFile(f);

  Init_WSR();

  return 1;
}


void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  WSRContext* result = new WSRContext;
  result->pos = 576*2;
  result->timepos = 0;

  int track=0;
  std::string toLoad(strFile);
  if (toLoad.find(".wsrstream") != std::string::npos)
  {
    size_t iStart=toLoad.rfind('-') + 1;
    track = atoi(toLoad.substr(iStart, toLoad.size()-iStart-10).c_str())-1;
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    size_t slash = toLoad.rfind('\\');
    if (slash == std::string::npos)
      slash = toLoad.rfind('/');
    toLoad = toLoad.substr(0, slash);
  }

  if (!Load_WSR(toLoad.c_str())) {
    delete result;
    return NULL;
  }

  static enum AEChannel map[3] = {
    AE_CH_FL, AE_CH_FR, AE_CH_NULL
  };
  *format = AE_FMT_S16NE;
  *channelinfo = map;
  *channels = 2;
  *bitspersample = 16;
  *samplerate = 48000;
  *totaltime = 5*60*1000; // 5 minutes
  *bitrate = 0;
  Reset_WSR(track);

  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  WSRContext* wsr = (WSRContext*)context;
  if (wsr->timepos >= 5*60*48000*2)
    return 1;

  if (wsr->pos == 576*2) {
    sample_buffer = wsr->sample_buffer;
    Update_WSR(40157, 576);
    wsr->pos = 0;
  }

  size_t tocopy = std::min((size_t)size, (576U*2-wsr->pos)*2);
  memcpy(pBuffer, wsr->sample_buffer+wsr->pos, tocopy);
  wsr->pos += tocopy/2;
  wsr->timepos += tocopy/2;
  *actualsize = tocopy;

  return 0;
}

int64_t Seek(void* context, int64_t time)
{
  return -1;
}

bool DeInit(void* context)
{
  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist, int* length)
{
  strcpy(title,"title");
  strcpy(artist,"artist");
  *length = 5*60;
  return true;
}

int TrackCount(const char* strFile)
{
  if (!Load_WSR(strFile))
    return 0;
  return 255-Get_FirstSong();
}
}
