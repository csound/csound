//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from his VSTHost program and from the vst~
//  object by Thomas Grill (no license), which in turn borrows from the Psycle
//  tracker (also based on VSTHost).
//
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Andres Cabrera, Michael Gogins
//
//  The vst4cs library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The vst4cs library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with The vst4cs library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//  02110-1301 USA
//
//  Linking vst4cs statically or dynamically with other modules is making a
//  combined work based on vst4cs. Thus, the terms and conditions of the GNU
//  Lesser General Public License cover the whole combination.
//
//  In addition, as a special exception, the copyright holders of vst4cs,
//  including the Csound developers and Hermann Seib, the original author of
//  VSTHost, give you permission to combine vst4cs with free software programs
//  or libraries that are released under the GNU LGPL and with code included
//  in the standard release of the VST SDK version 2 under the terms of the
//  license stated in the VST SDK version 2 files. You may copy and distribute
//  such a system following the terms of the GNU LGPL for vst4cs and the
//  licenses of the other code concerned. The source code for the VST SDK
//  version 2 is available in the VST SDK hosted at
//  https://github.com/steinbergmedia/vst3sdk.
//
//  Note that people who make modified versions of vst4cs are not obligated to
//  grant this special exception for their modified versions; it is their
//  choice whether to do so. The GNU Lesser General Public License gives
//  permission to release a modified version without this exception; this
//  exception also makes it possible to release a modified version which
//  carries forward this exception.

#ifdef _WIN32
  #pragma warning(disable:4786) //gab
#endif

#include "fxbank.h"
#include <stdio.h>

const int MyVersion = 1;                /* highest known VST FX version      */

CFxBank::CFxBank(char *pszFile)
{
    Init();                                 /* initialize data areas             */
    if (pszFile)                            /* if a file name has been passed    */
      LoadBank(pszFile);                    /* load the corresponding bank       */
}

CFxBank::CFxBank(int nPrograms, int nParams)
{
    Init();                                 /* initialize data areas             */
    SetSize(nPrograms, nParams);            /* set new size                      */
}

CFxBank::CFxBank(int nChunkSize)
{
    Init();                                 /* initialize data areas             */
    SetSize(nChunkSize);                    /* set new size                      */
}

/*****************************************************************************/
/* Init : initializes all data areas                                         */
/*****************************************************************************/

void CFxBank::Init()
{
    static char szChnk[] = "CcnK";          /* set up swapping flag              */
    static long lChnk = 'CcnK';

    NeedsBSwap = !!memcmp(szChnk, &lChnk, 4);

    bBank = NULL;                           /* no bank data loaded               */
    Unload();                               /* reset all parameters              */
}

CFxBank::~CFxBank()
{
    Unload();                               /* unload all data                   */
}

/*****************************************************************************/
/* DoCopy : combined for copy constructor and assignment operator            */
/*****************************************************************************/

CFxBank& CFxBank::DoCopy(const CFxBank& org)
{
    unsigned char *nBank = NULL;

    if (org.nBankLen) {
      unsigned char *nBank = new unsigned char[org.nBankLen];
      if (!nBank)
        throw (int) 1;
      memcpy(nBank, org.bBank, org.nBankLen);
    }
    Unload();                               /* remove previous data              */
    bBank = nBank;                          /* and copy in the other one's       */
    bChunk = org.bChunk;
    nBankLen = org.nBankLen;
    strcpy(szFileName, org.szFileName);
    return *this;
}

/*****************************************************************************/
/* SetSize : sets new size                                                   */
/*****************************************************************************/

bool CFxBank::SetSize(int nPrograms, int nParams)
{
    int nTotLen = sizeof(fxSet) - sizeof(fxProgram);
    int nProgLen = sizeof(fxProgram) + (nParams - 1) * sizeof(float);
    nTotLen += nPrograms * nProgLen;
    unsigned char *nBank = new unsigned char[nTotLen];
    if (!nBank)
      return false;

    Unload();
    bBank = nBank;
    nBankLen = nTotLen;
    bChunk = false;

    memset(nBank, 0, nTotLen);              /* initialize new bank               */

    fxSet   *pSet = (fxSet *) bBank;
    pSet->chunkMagic = cMagic;
    pSet->byteSize = 0;
    pSet->fxMagic = bankMagic;
    pSet->version = MyVersion;
    pSet->numPrograms = nPrograms;

    unsigned char *bProg = (unsigned char *) pSet->programs;

    for (int i = 0; i < nPrograms; i++) {
      fxProgram *pProg = (fxProgram *) (bProg + i * nProgLen);

      pProg->chunkMagic = cMagic;
      pProg->byteSize = 0;
      pProg->fxMagic = fMagic;
      pProg->version = 1;
      pProg->numParams = nParams;
      for (int j = 0; j < nParams; j++)
        pProg->params[j] = 0.0;
    }
    return true;
}

bool CFxBank::SetSize(int nChunkSize)
{
    int nTotLen = sizeof(fxChunkSet) + nChunkSize - 8;
    unsigned char *nBank = new unsigned char[nTotLen];
    if (!nBank)
      return false;

    Unload();
    bBank = nBank;
    nBankLen = nTotLen;
    bChunk = true;

    memset(nBank, 0, nTotLen);              /* initialize new bank               */
    fxChunkSet *pSet = (fxChunkSet *)bBank;
    pSet->chunkMagic = cMagic;
    pSet->byteSize = 0;
    pSet->fxMagic = chunkBankMagic;
    pSet->version = MyVersion;
    pSet->numPrograms = 1;
    pSet->chunkSize = nChunkSize;

    return true;
}

/*****************************************************************************/
/* SwapBytes : swaps bytes for big/little-endian difference                  */
/*****************************************************************************/

void CFxBank::SwapBytes(long &l)
{
    unsigned char *b = (unsigned char *)&l;
    long intermediate =  ((long)b[0] << 24) |
                          ((long)b[1] << 16) |
                          ((long)b[2] << 8) |
                          (long)b[3];
    l = intermediate;
}

void CFxBank::SwapBytes(float &f)
{
    long *pl = (long *)&f;
    SwapBytes(*pl);
}

/*****************************************************************************/
/* LoadBank : loads a bank file                                              */
/*****************************************************************************/

bool CFxBank::LoadBank(char *pszFile)
{
    FILE *fp = fopen(pszFile, "rb");        /* try to open the file              */

    if (!fp) {                              /* upon error                        */
      printf("Error loading bank: %s \n", pszFile);
      return false;                         /* return an error                   */
    }
    bool brc = true;                        /* default to OK                     */
    unsigned char *nBank = NULL;
    //printf("Bank Loaded\n");
    //try
    {
      fseek(fp, 0, SEEK_END);               /* get file size                     */
      size_t tLen = (size_t)ftell(fp);
      //printf("File Size = %f\n", (float) tLen);
      rewind(fp);

      nBank = new unsigned char[tLen];      /* allocate storage                  */
      //if (!nBank)
      //  throw (int)1;
                                            /* read chunk set to determine cnt.  */
      if (fread(nBank, 1, tLen, fp) != tLen)
        return false;
      //  throw (int)1;
      fxSet *pSet = (fxSet *)nBank;         /* position on set                   */
      //printf("pSet created");
      if (NeedsBSwap)                       /* eventually swap necessary bytes   */
        {
          //printf("Swap Needed\n");
          SwapBytes(pSet->chunkMagic);
          SwapBytes(pSet->byteSize);
          SwapBytes(pSet->fxMagic);
          SwapBytes(pSet->version);
          SwapBytes(pSet->fxID);
          SwapBytes(pSet->fxVersion);
          SwapBytes(pSet->numPrograms);
        }
      if ((pSet->chunkMagic != cMagic) ||   /* if erroneous data in there        */
          (pSet->version > MyVersion) ||
          ((pSet->fxMagic != bankMagic) &&
           (pSet->fxMagic != chunkBankMagic)))
        {
          printf("Erroneous data.\n");
          return 0;
        }                    /* get out                           */
      //printf ("Swap OK\n");
      if (pSet->fxMagic == bankMagic)
        {
          //printf("bankMagic\n");
          fxProgram * pProg = pSet->programs; /* position on 1st program   */
          //printf ("Number of programs = %i",numPrograms);
          int nProg = 0;
          while (nProg < pSet->numPrograms)   /* walk program list         */
            {
              if (NeedsBSwap)                 /* eventually swap necessary bytes */
                {
                  SwapBytes(pProg->chunkMagic);
                  SwapBytes(pProg->byteSize);
                  SwapBytes(pProg->fxMagic);
                  SwapBytes(pProg->version);
                  SwapBytes(pProg->fxID);
                  SwapBytes(pProg->fxVersion);
                  SwapBytes(pProg->numParams);
                }
              //printf("bankMagic-Swapped\n");
              if ((pProg->chunkMagic != cMagic)||
                  (pProg->fxMagic != fMagic))
                {                              /* if erroneous data             */
                  printf("Erroneous data.\n");
                  return 0;                    /* get out                       */
                };
              if (NeedsBSwap)                  /* if necessary                  */
                {                              /* swap all parameter bytes      */
                  int j;
                  for (j = 0; j < pProg->numParams; j++)
                    SwapBytes(pProg->params[j]);
                }
              //printf("bankMagic-swap parameter\n");
              unsigned char *pNext = (unsigned char *)(pProg + 1);
              pNext += (sizeof(float) * (pProg->numParams - 1));
              //if (pNext > nBank + tLen)      /* VERY simple fuse              */
              //  throw (int)1;

              pProg = (fxProgram *)pNext;
              nProg++;
            }
          printf("bankMagic-swap parameter\n");
        }
      /* if it's a chunk file              */
      else if (pSet->fxMagic == chunkBankMagic)
        {
          //printf("chunkBankMagic\n");
          fxChunkSet * pCSet = (fxChunkSet *)nBank;
          if (NeedsBSwap)                   /* eventually swap necessary bytes */
            {
              SwapBytes(pCSet->chunkSize);
              /* size check - must not be too large*/
              //if (pCSet->chunkSize + sizeof(*pCSet) - 8 > tLen)
              //throw (int)1;
              //printf("Chunk OK\n");
            }
        }
      else printf ("No Magic match\n");
      Unload();                             /* otherwise remove eventual old data*/
      //printf("Unloaded");
      bBank = nBank;                        /* and put in new data               */
      nBankLen = (int)tLen;
      bChunk = (pSet->fxMagic == chunkBankMagic);
    }
    //catch(...)
    //  {
    //  brc = false;                        /* if any error occured, say NOPE */
    //  if (nBank)                          /* and remove loaded data         */
    //    delete[] nBank;
    //  }

    fclose(fp);                             /* close the file                    */
    //printf("File closed\n");
    return brc;                             /* and return                        */
}

/*****************************************************************************/
/* SaveBank : save bank to file                                              */
/*****************************************************************************/

bool CFxBank::SaveBank(char *pszFile)
{
    if (!IsLoaded())
      return false;
    /* create internal copy for mod      */
    unsigned char *nBank = new unsigned char[nBankLen];
    if (!nBank)                 /* if impossible                     */
      return false;
    memcpy(nBank, bBank, nBankLen);

    fxSet   *pSet = (fxSet *) nBank;    /* position on set           */
    int     numPrograms = pSet->numPrograms;

    if (NeedsBSwap) {           /* if byte-swapping needed           */
      SwapBytes(pSet->chunkMagic);
      SwapBytes(pSet->byteSize);
      SwapBytes(pSet->fxMagic);
      SwapBytes(pSet->version);
      SwapBytes(pSet->fxID);
      SwapBytes(pSet->fxVersion);
      SwapBytes(pSet->numPrograms);
    }

    if (bChunk) {
      fxChunkSet *pCSet = (fxChunkSet *) nBank;

      if (NeedsBSwap)                       /* if byte-swapping needed           */
        SwapBytes(pCSet->chunkSize);
    }
    else {
      fxProgram *pProg = pSet->programs;  /* position on 1st program */
      int numParams = pProg->numParams;
      int nProg = 0;

      while (nProg < numPrograms) { /* walk program list             */
        if (NeedsBSwap) {       /* eventually swap all necessary     */
          SwapBytes(pProg->chunkMagic);
          SwapBytes(pProg->byteSize);
          SwapBytes(pProg->fxMagic);
          SwapBytes(pProg->version);
          SwapBytes(pProg->fxID);
          SwapBytes(pProg->fxVersion);
          SwapBytes(pProg->numParams);
          for (int j = 0; j < numParams; j++)
            SwapBytes(pProg->params[j]);
        }
        unsigned char *pNext = (unsigned char *)(pProg + 1);
        pNext += (sizeof(float) * (numParams - 1));
        if (pNext > nBank + nBankLen)       /* VERY simple fuse                  */
          break;

        pProg = (fxProgram *)pNext;
        nProg++;
      }
    }

    bool    brc = true;         /* default to OK                     */
    FILE    *fp = NULL;

    try {
      fp = fopen(pszFile, "wb");            /* try to open the file              */
      if (!fp)                              /* upon error                        */
        throw (int)1;                       /* return an error                   */
      if (fwrite(nBank, 1, nBankLen, fp) != (size_t)nBankLen)
        throw (int)1;
    }
    catch (...) {
      brc = false;
    }
    if (fp)
      fclose(fp);
    delete[] nBank;

    return brc;
}

/*****************************************************************************/
/* Unload : removes a loaded bank from memory                                */
/*****************************************************************************/

void CFxBank::Unload()
{
    if (bBank)
      delete[] bBank;
    *szFileName = '\0';                     /* reset file name                   */
    bBank = NULL;                           /* reset bank pointer                */
    nBankLen = 0;                           /* reset bank length                 */
    bChunk = false;                         /* and of course it's no chunk.      */
}

/*****************************************************************************/
/* GetProgram : returns pointer to one of the loaded programs                */
/*****************************************************************************/

fxProgram * CFxBank::GetProgram(int nProgNum)
{
    if ((!IsLoaded()) || (bChunk))  /* if nothing loaded or chunk file  */
      return NULL;                          /* return OUCH                       */

    fxSet   *pSet = (fxSet *) bBank;    /* position on set              */
    fxProgram *pProg = pSet->programs;  /* position on 1st program      */
#if 1
    int nProgLen = sizeof(fxProgram) + (pProg->numParams - 1) * sizeof(float);
    unsigned char *pThatProg = ((unsigned char *)pProg) + (nProgNum * nProgLen);
    pProg = (fxProgram *)pThatProg;
#else
    /*---------------------------------------------------------------------------*/
    /* presumably, the following logic is overkill; if all programs have the     */
    /* same number of parameters, a simple multiplication would do.              */
    /* But that's not stated anywhere in the VST SDK...                          */
    /*---------------------------------------------------------------------------*/
    int i;
    for (i = 0; i < nProgNum; i++)
      {
        unsigned char *pNext = (unsigned char *)(pProg + 1);
        pNext += (sizeof(float) * (pProg->numParams - 1));
        if (pNext > bBank + nBankLen)         /* VERY simple fuse              */
          return NULL;

        pProg = (fxProgram *)pNext;
      }
#endif
    return pProg;
}


