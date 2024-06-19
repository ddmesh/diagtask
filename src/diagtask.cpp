 /*
Copyright (c) 2021, Stephan Enderlein. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/
/*!
************************************************************************
*
* @file   diagtask.cpp
*
* @brief  Definitions: DiagTask is a debugging tool that allows to register "hooks".
*
*         On serial console diagtask waits for user input and calles registerred
*         functions. Those functions can then print infos or control the behavior
*         of the software.
*
* @author Stephan Enderlein
*
************************************************************************/

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "diagtask.hpp"

#define ENABLE_ECHO 1

// --- constants
/// @defgroup hook_names Special hook commands
/// @{
#define SPECIAL_KEYWORD_HELP      '?'
#define SPECIAL_KEYWORD_SEPARATOR '#'
#define SPECIAL_KEYWORD_SEARCH    '/'
#define SPECIAL_KEYWORD_TAB       '\t'
#define SPECIAL_KEYWORD_REBOOT    '!'

#define SPECIAL_KEYWORD_WILDCARD    '*'
/// @}

// --- local types


// --- local data
// --- functions

DiagTask::DiagTask(int (*getch)())
      : mFeatures(feature_None), mCurrentValidInput(""), mGetchar(getch), mUptime(NULL), mReboot(NULL)
{
};


DiagTask::DiagTask(int (*getch)(), uint32_t (*uptime)())
      : mFeatures(feature_None), mCurrentValidInput(""), mGetchar(getch), mUptime(uptime), mReboot(NULL)
{
};

DiagTask::DiagTask(int (*getch)(), uint32_t (*uptime)(), void (*reboot)())
      : mFeatures(feature_None), mCurrentValidInput(""), mGetchar(getch), mUptime(uptime), mReboot(reboot)
{
};

void DiagTask::process(void)
{
  int c;

  // try to read one character
  if(!mGetchar) return;  // error, no function defined

  c = mGetchar();
  if( c < 0 ) return;  // no byte was received

  // replace '\0' and '\r' with '\n'. those are only used for wildcard hooks.
  // this overcomes windows/linux eol and allows also '\0' to be used as end marker
  if ( c=='\0' || c== '\r')
  { c = '\n'; }

#if ENABLE_ECHO
  if(    (c != SPECIAL_KEYWORD_TAB       )
      && (c != SPECIAL_KEYWORD_SEPARATOR )
      && (c != SPECIAL_KEYWORD_HELP      )
      && (c != SPECIAL_KEYWORD_REBOOT    )
    )
  {
    putchar(c);
    fflush(stdout);
  }
#endif

  // only process if valid character was received
  while(c >= 0)
  {

    // add new input to mCurrentValidInput
    auto len = strlen(mCurrentValidInput);
    if( len == DIAGTASK_MAX_HOOK_INPUT_LEN) //full?
    { mCurrentValidInput[0] = '\0';
      len = 0;
      break;
    }

  //printf("%d, [%s]\n", len , mCurrentValidInput);

    // special characters are only valid, if those are the first character.
    // if a valid hook is recognized but this hook uses such characters somewhere, those
    // characters should be used as normal hook characters and not processed as special value.
    if(mCurrentValidInput[0] == '\0' && privCheckAndProcessSpecialChars(c))
    { break; }

    // if special character '\t' was pressed, do not add it to input buffer or process it futher
    if(processTabCompetion(c))
    { break; }

    mCurrentValidInput[len] = c;
    mCurrentValidInput[len+1] = '\0';

    // if we still have user input, try to call hook
    diagtask_vector filterredHooks = privFindHooks(mCurrentValidInput);
    
    // no hook found
    if(filterredHooks.size() == 0)
    {
      // reset input; c did not select a hook
      mCurrentValidInput[0] = '\0';
      break;
    }
    // found hook with at least correct length (could be a wildcard hook)
    else if(filterredHooks.size() == 1
              // ensure that we got the complete hook length. avoid calling hook if only one hook
              // exisits that starts with current input. But hook can be longer because of wildcard
              && strlen(mCurrentValidInput) >= strlen(filterredHooks[0].name) )
    {
      // check for wildcard hook
      if(strchr(filterredHooks[0].name, SPECIAL_KEYWORD_WILDCARD))
      {
        // if we get '\n' then stop and execute hook
        if(    mCurrentValidInput[len] == '\n' )
        {
#if ENABLE_ECHO
          putchar('\n');
#endif // #if ENABLE_ECHO

          mCurrentValidInput[len] = '\0'; // remove '\n'
          // find wildcard position at which the user argument starts
          unsigned int argIdx = strchr(filterredHooks[0].name, SPECIAL_KEYWORD_WILDCARD)
                            - filterredHooks[0].name;
          filterredHooks[0].hook(&mCurrentValidInput[argIdx]);
          mCurrentValidInput[0] = '\0'; // reset input
        }
      }
      else
      {
#if ENABLE_ECHO
        putchar('\n');
#endif // #if ENABLE_ECHO
        filterredHooks[0].hook("");
        mCurrentValidInput[0] = '\0'; // reset input
      }
    }

    // character was processed, leave loop
    break;
  } //while (c>0);

}


bool DiagTask::registerHook( const char * name, void(*hook)(const char* input)
                           , const char * description)
{
  if(!name || !hook)
  { return false; }

  auto len = strlen(name);

  if(len < DIAGTASK_MIN_HOOKNAME_LEN || len > DIAGTASK_MAX_HOOKNAME_LEN )
  { return false; }

  hookEntry_t entry;
  strncpy(entry.name, name, DIAGTASK_MAX_HOOKNAME_LEN);

  entry.name[DIAGTASK_MAX_HOOKNAME_LEN] = '\0';
  entry.hook = hook;
  if(description)
  {
    strncpy(entry.description, description, DIAGTASK_HOOKDESC_LEN);
    entry.description[DIAGTASK_HOOKDESC_LEN] = '\0';
  }
  else
  {
    entry.description[0] = '\0';
  }

  //TODO insert sorted
  mHooks.push_back(entry);
  return true;
}

void DiagTask::enableFeatures(unsigned int features)
{
 mFeatures = features;
}

bool DiagTask::executeHook(const char * name)
{ //TODO: implement DiagTask::executeHook()
  return false;
}

#if DIAGTASK_ENABLE_READ_KEY
bool DiagTask::readKey(char & out, bool echo)
{
  int c = getchar();
  if( c > 0 && c != 27) //ESC
  {
    out = c;
    if(echo) putchar(c);
    return true;
  }
  return false;
}
#endif //DIAGTASK_ENABLE_READ_KEY

#if DIAGTASK_ENABLE_READ_INTEGER
bool DiagTask::readInteger(int32_t & out, bool echo)
{
  //TODO: implement DiagTask::readInteger()
  return false;
}
#endif // DIAGTASK_ENABLE_READ_INTEGER

#if DIAGTASK_ENABLE_READ_HEX_INTEGER
bool DiagTask::readHexInteger(uint32_t & out, bool echo)
{ //TODO: implement DiagTask::readHexInteger()
  return false;
}
#endif // DIAGTASK_ENABLE_READ_HEX_INTEGER

#if DIAGTASK_ENABLE_READ_STRING
bool DiagTask::readString(char * out, uint16_t maxlen, bool echo)
{ //TODO: implement DiagTask::readString()
  return false;
}
#endif

// diagtask.hpp excludes following files explicitly
/// @cond
DiagTask::diagtask_vector DiagTask::privFindHooks( const char * prefix)
{
  diagtask_vector filterredHooks;
  uint16_t lenHook;
  uint16_t lenInput;
  uint16_t lenMin;
  const char *posWildcard;

  lenInput= strlen(prefix);

  for (auto & h : mHooks)
  {
    //check until end of hookname and ignore wildcards. whildcards are later used to read
    //until line end '\n'
    //all hooks without wildcards do not have a '\n'
    posWildcard = strchr(h.name, SPECIAL_KEYWORD_WILDCARD);
    lenHook = posWildcard == NULL ? strlen(h.name) : ( posWildcard - h.name );
    lenMin = std::min(lenHook, lenInput);

    if( strncmp(h.name, prefix, lenMin) == 0 )
    {
      filterredHooks.push_back(h);
    }
  }
  return filterredHooks;
}

bool DiagTask::privCheckAndProcessSpecialChars(char input)
{
//printf("[%c](%d) buf:[%s]\n",input,input, mCurrentValidInput);
  #if DIAGTASK_ENABLE_HELP
    if( ( mFeatures & feature_Help ) && input == SPECIAL_KEYWORD_HELP)
    {
      privHelp();
      mCurrentValidInput[0] = '\0'; // reset input
      return true;
    }
  #endif //DIAGTASK_ENABLE_HELP

  #if DIAGTASK_ENABLE_SEPARATOR
    if( ( mFeatures & feature_Seperator ) && input == SPECIAL_KEYWORD_SEPARATOR)
    {
      privDisplaySeparator();
      mCurrentValidInput[0] = '\0'; // reset input
      return true;
    }
  #endif //DIAGTASK_ENABLE_SEPARATOR

  #if DIAGTASK_ENABLE_REBOOT
    if( ( mFeatures & feature_Reboot )
          && mReboot
          && input == SPECIAL_KEYWORD_REBOOT)
    {
      printf("rebooting\n");
      mReboot();
      while(1);
    }
  #endif //DIAGTASK_ENABLE_REBOOT

  #if DIAGTASK_ENABLE_SEARCH
    if( ( mFeatures & feature_Search ) && input == SPECIAL_KEYWORD_SEARCH)
    {
      /*TODO: not implemented yet*/
      mCurrentValidInput[0] = '\0'; // reset input
      printf("net yet implemented\n");
      return true;
    }
  #endif //DIAGTASK_ENABLE_SEARCH

  return false;
}

bool DiagTask::processTabCompetion(char input)
{
#if DIAGTASK_ENABLE_TAB_COMPLETION
    if( ( mFeatures & feature_TabCompletion ) && input == SPECIAL_KEYWORD_TAB)
    {
      if(mCurrentValidInput[0])
      {
      // show all hooks that start with mCurrentValidInput
      // if we only have one hook, then call hook function
        diagtask_vector filterredHooks = privFindHooks(mCurrentValidInput);
      if(filterredHooks.size() == 1)
      {
        printf("[%s]\n", filterredHooks[0].name);
        filterredHooks[0].hook("");
        mCurrentValidInput[0] = '\0'; // reset input
      }
      else
      {
        auto len = strlen(mCurrentValidInput);
#if ENABLE_ECHO
        printf("\n");
#endif // #if ENABLE_ECHO
          for ( const auto & h: filterredHooks)
        {
          printf("[%s]%-20s\t%s\n", mCurrentValidInput, &h.name[len], h.description);
          }
        }
      }
      return true;
    }
  #endif // DIAGTASK_ENABLE_TAB_COMPLETION

  return false;
}


#if DIAGTASK_ENABLE_HELP
void DiagTask::privHelp()
{
  printf("\n");

#if DIAGTASK_ENABLE_HELP
  printf("%c - help\n", SPECIAL_KEYWORD_HELP);
#endif //DIAGTASK_ENABLE_HELP

#if DIAGTASK_ENABLE_SEARCH
  printf("%c - search\n", SPECIAL_KEYWORD_SEARCH);
#endif //DIAGTASK_ENABLE_SEARCH

#if DIAGTASK_ENABLE_SEPARATOR
  printf("%c - separator\n", SPECIAL_KEYWORD_SEPARATOR);
#endif //DIAGTASK_ENABLE_SEPARATOR

#if DIAGTASK_ENABLE_REBOOT
  printf("%c - reboot\n", SPECIAL_KEYWORD_REBOOT);
#endif //DIAGTASK_ENABLE_REBOOT

  for (const auto & h : mHooks)
  {
    printf("%-20s\t%s\n", h.name, h.description);
  }
}
#endif // DIAGTASK_ENABLE_HELP

#if DIAGTASK_ENABLE_SEPARATOR
void DiagTask::privDisplaySeparator()
{
 static uint32_t count=0;

  printf("\n\n\n\n");
  printf("###########################################\n");
  printf("### SEPARATOR %5lu ######  %10lu  ###\n", static_cast<long unsigned int>(count), static_cast<long unsigned int>(mUptime ? mUptime() : 0));
  printf("###########################################\n");
  printf("\n\n\n\n");
  count++;
}

// doxygen end condition
/// @endcond
#endif // DIAGTASK_ENABLE_SEPARATOR
