/******************************************************************************
 *
 * Copyright (C) 1997-2015 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby 
 * granted. No representations are made about the suitability of this software 
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#include <stdio.h>

#include <qdir.h>
#include <qdict.h>

#include "htags.h"
#include "util.h"
#include "message.h"
#include "config.h"
#include "portable.h"


bool Htags::useHtags = FALSE;

static QDir g_inputDir;
static QDict<QCString> g_symbolDict(10007);

/*! constructs command line of htags(1) and executes it.
 *  \retval TRUE success
 *  \retval FALSE an error has occurred.
 */
bool Htags::execute(const QCString &htmldir)
{
  static QStrList &inputSource = Config_getList(INPUT);
  static bool quiet = Config_getBool(QUIET);
  static bool warnings = Config_getBool(WARNINGS);
  static QCString htagsOptions = ""; //Config_getString(HTAGS_OPTIONS);
  static QCString projectName = Config_getString(PROJECT_NAME);
  static QCString projectNumber = Config_getString(PROJECT_NUMBER);

  QCString cwd = QDir::currentDirPath().utf8();
  if (inputSource.isEmpty())
  {
    g_inputDir.setPath(cwd);
  }
  else if (inputSource.count()==1)
  {
    g_inputDir.setPath(inputSource.first());
    if (!g_inputDir.exists())
      err("Cannot find directory %s. "
          "Check the value of the INPUT tag in the configuration file.\n",
          inputSource.first()
         );
  }
  else
  {
    err("If you use USE_HTAGS then INPUT should specify a single directory.\n");
    return FALSE;
  }

  /*
   * Construct command line for htags(1).
   */
  QCString commandLine = " -g -s -a -n ";
  if (!quiet)   commandLine += "-v ";
  if (warnings) commandLine += "-w ";
  if (!htagsOptions.isEmpty()) 
  {
    commandLine += ' ';
    commandLine += htagsOptions;
  }
  if (!projectName.isEmpty()) 
  {
    commandLine += "-t \"";
    commandLine += projectName;
    if (!projectNumber.isEmpty()) 
    {
      commandLine += '-';
      commandLine += projectNumber;
    }
    commandLine += "\" ";
  }
  commandLine += " \"" + htmldir + "\"";
  QCString oldDir = QDir::currentDirPath().utf8();
  QDir::setCurrent(g_inputDir.absPath());
  //printf("CommandLine=[%s]\n",commandLine.data());
  Portables::sysTimerStart();
  bool result=Portables::system("htags",commandLine,FALSE)==0;
  if (!result)
  {
    err("Problems running %s. Check your installation\n", "htags");
  }
  Portables::sysTimerStop();
  QDir::setCurrent(oldDir);
  return result;
}


/*! load filemap and make index.
 *  \param htmlDir of HTML directory generated by htags(1).
 *  \retval TRUE success
 *  \retval FALSE error
 */
bool Htags::loadFilemap(const QCString &htmlDir)
{
  QCString fileMapName = htmlDir+"/HTML/FILEMAP";
  QFileInfo fi(fileMapName);
  /*
   * Construct FILEMAP dictionary using QDict.
   *
   * In FILEMAP, URL includes 'html' suffix but we cut it off according
   * to the method of FileDef class.
   *
   * FILEMAP format:
   * <NAME>\t<HREF>.html\n
   * QDICT:
   * dict[<NAME>] = <HREF>
   */
  if (fi.exists() && fi.isReadable())
  {
    QFile f(fileMapName);
    const int maxlen = 8192;
    QCString line(maxlen+1);
    line.at(maxlen)='\0';
    if (f.open(IO_ReadOnly))
    {
      int len;
      while ((len=f.readLine(line.rawData(),maxlen))>0)
      {
        line.at(len)='\0';
        //printf("Read line: %s",line.data());
        int sep = line.find('\t');
        if (sep!=-1)
        {
          QCString key   = line.left(sep).stripWhiteSpace();
          QCString value = line.mid(sep+1).stripWhiteSpace();
          int ext=value.findRev('.');
          if (ext!=-1) value=value.left(ext); // strip extension
          g_symbolDict.setAutoDelete(TRUE);
          g_symbolDict.insert(key,new QCString(value));
          //printf("Key/Value=(%s,%s)\n",key.data(),value.data());
        }
      }
      return TRUE;
    }
    else
    {
      err("file %s cannot be opened\n",fileMapName.data()); 
    }
  }
  return FALSE;
}

/*! convert path name into the url in the hypertext generated by htags.
 *  \param path path name
 *  \returns URL NULL: not found.
 */
QCString Htags::path2URL(const QCString &path)
{
  QCString url,symName=path;
  QCString dir = g_inputDir.absPath().utf8();
  int dl=dir.length();
  if ((int)symName.length()>dl+1)
  {
    symName = symName.mid(dl+1);
  }
  if (!symName.isEmpty())
  {
    QCString *result = g_symbolDict[symName];
    //printf("path2URL=%s symName=%s result=%p\n",path.data(),symName.data(),result);
    if (result)
    {
      url = "HTML/" + *result;
    }
  }
  return url;
}

