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

#include <kodi/addon-instance/AudioDecoder.h>
#include <kodi/Filesystem.h>
#include <kodi/General.h>
#include <kodi/tools/DllHelper.h>
#include <algorithm>
#include <iostream>

extern "C" {
#include "wsr_player.h"
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
}

struct WSRContext
{
  short sample_buffer[576*2];
  size_t pos;
  uint64_t timepos;
};

class CMyAddon;

class CWSRCodec : public kodi::addon::CInstanceAudioDecoder,
                  private CDllHelper
{
public:
  CWSRCodec(KODI_HANDLE instance, CMyAddon* addon, bool useChild) :
    CInstanceAudioDecoder(instance),
    m_addon(addon), m_useChild(useChild)
  {
    std::string source = kodi::GetAddonPath(LIBRARY_PREFIX "in_wsr" LIBRARY_SUFFIX);
    if (m_useChild)
    {
      char buffer[50];
      snprintf(buffer, 50, "%sin_wsr-%p%s", LIBRARY_PREFIX, this, LIBRARY_SUFFIX);
      if (!kodi::vfs::CopyFile(source, buffer))
      {
        kodi::Log(ADDON_LOG_ERROR, "Failed to create in_wsr copy");
        return;
      }
    }
    else
      m_usedLibName = source;
  }

  virtual ~CWSRCodec() = default;

  bool Init(const std::string& filename, unsigned int filecache,
            int& channels, int& samplerate,
            int& bitspersample, int64_t& totaltime,
            int& bitrate, AEDataFormat& format,
            std::vector<AEChannel>& channellist) override
  {
    if (!LoadDll(m_usedLibName)) return false;
    if (!REGISTER_DLL_SYMBOL(Init_WSR)) return false;
    if (!REGISTER_DLL_SYMBOL(Reset_WSR)) return false;
    if (!REGISTER_DLL_SYMBOL(Update_WSR)) return false;
    if (!REGISTER_DLL_SYMBOL(Get_FirstSong)) return false;
    if (!REGISTER_DLL_SYMBOL(ROM)) return false;
    if (!REGISTER_DLL_SYMBOL(ROMSize)) return false;
    if (!REGISTER_DLL_SYMBOL(ROMBank)) return false;
    if (!REGISTER_DLL_SYMBOL(sample_buffer)) return false;

    ctx.pos = 576*2;
    ctx.timepos = 0;

    int track=0;
    std::string toLoad(filename);
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

    if (!Load_WSR(toLoad.c_str()))
      return false;

    format = AE_FMT_S16NE;
    channellist = { AE_CH_FL, AE_CH_FR };
    channels = 2;
    bitspersample = 16;
    samplerate = 48000;
    totaltime = 5*60*1000; // 5 minutes
    bitrate = 0;
    Reset_WSR(track);

    return true;
  }

  int ReadPCM(uint8_t* buffer, int size, int& actualsize) override
  {
    if (ctx.timepos >= 5*60*48000*2)
      return 1;

    if (ctx.pos == 576*2) {
      *sample_buffer = ctx.sample_buffer;
      Update_WSR(40157, 576);
      ctx.pos = 0;
    }

    size_t tocopy = std::min((size_t)size, (576U*2-ctx.pos)*2);
    memcpy(buffer, ctx.sample_buffer+ctx.pos, tocopy);
    ctx.pos += tocopy/2;
    ctx.timepos += tocopy/2;
    actualsize = tocopy;

    return 0;
  }

  int64_t Seek(int64_t time) override
  {
    return -1;
  }

  int TrackCount(const std::string& fileName) override
  {
    if (fileName.find(".wsrstream") != std::string::npos)
      return 0;

    if (!LoadDll(m_usedLibName)) return false;
    if (!REGISTER_DLL_SYMBOL(Init_WSR)) return false;
    if (!REGISTER_DLL_SYMBOL(Reset_WSR)) return false;
    if (!REGISTER_DLL_SYMBOL(Update_WSR)) return false;
    if (!REGISTER_DLL_SYMBOL(Get_FirstSong)) return false;
    if (!REGISTER_DLL_SYMBOL(ROM)) return false;
    if (!REGISTER_DLL_SYMBOL(ROMSize)) return false;
    if (!REGISTER_DLL_SYMBOL(ROMBank)) return false;
    if (!REGISTER_DLL_SYMBOL(sample_buffer)) return false;

    if (!Load_WSR(fileName.c_str()))
      return 0;
    return 255-Get_FirstSong();
  }

private:
  int Load_WSR(const char *name)
  {
    kodi::vfs::CFile file;
    if (!file.OpenFile(name, 0))
      return 0;

    *ROMSize = file.GetLength();
    *ROMBank = (*ROMSize+0xFFFF)>>16;
    *ROM = (BYTE*)malloc(*ROMBank*0x10000);
    if (!*ROM)
    {
      file.Close();
      return 0;
    }
    file.Read(*ROM, *ROMSize);
    file.Close();

    Init_WSR();

    return 1;
  }

  WSRContext ctx;
  CMyAddon* m_addon;
  bool m_useChild;
  std::string m_usedLibName;

  void (*Init_WSR)();
  void (*Reset_WSR)(int SongNo);
  void (*Update_WSR)(int cycles, int Length);
  int (*Get_FirstSong)();
  unsigned char** ROM;
  int* ROMSize;
  int* ROMBank;
  short** sample_buffer;
};


class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CWSRCodec(instance, this, ++m_usedAmount > 1);
    return ADDON_STATUS_OK;
  }

  void DecreaseUsedAmount()
  {
    if (m_usedAmount > 0)
      --m_usedAmount;
  }

  virtual ~CMyAddon() = default;

private:
  int m_usedAmount = 0;
};


ADDONCREATOR(CMyAddon)
