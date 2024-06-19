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
* @file diagtask.hpp
*
* @brief  Declarations: DiagTask is a debugging tool that allows to register "hooks".
*
*         On serial console diagtask waits for user input and calles registerred
*         functions. Those functions can then print infos or control the behavior
*         of the software.
*
*
* @author Stephan Enderlein
*
************************************************************************/


#ifndef _INCLUDED_DIAGTASK_HPP_
#define _INCLUDED_DIAGTASK_HPP_

#ifndef DIAGTASK_USE_ETL
  /// @brief diagtask uses STL (std::vector) instead of etl::vector (https://www.etlcpp.com/)
  #define DIAGTASK_USE_ETL                    0
#endif


// these defines can be defined by user before including diagtask and compiling
#ifndef DIAGTASK_ENABLE_HELP
  /// @brief Enables support to print registerred hooks via '?'.
  #define DIAGTASK_ENABLE_HELP                1
#endif

#ifndef DIAGTASK_ENABLE_SEPARATOR
  /// @brief Enables support to print a separator line with time stamp and counter
  ///        via '#'
  #define DIAGTASK_ENABLE_SEPARATOR           1
#endif

#ifndef DIAGTASK_ENABLE_SEARCH
  /// @brief Enables support to search through all hook names via '/'
  #define DIAGTASK_ENABLE_SEARCH              0
#endif

#ifndef DIAGTASK_ENABLE_REBOOT
  /// @brief Enables support to reboot device via character '!'
  #define DIAGTASK_ENABLE_REBOOT              1
#endif

#ifndef DIAGTASK_ENABLE_TAB_COMPLETION
  /// @brief Enables support for tab-competion when entering hook names
  #define DIAGTASK_ENABLE_TAB_COMPLETION      1
#endif

// following read functions are blocking and can cause system watchdog events or
// stop main loop.
#ifndef DIAGTASK_ENABLE_READ_KEY
  /// @brief Enables support to read one character from console (not implemented yet)
  #define DIAGTASK_ENABLE_READ_KEY            0
#endif

#ifndef DIAGTASK_ENABLE_READ_INTEGER
  /// @brief Enables support to read one decimal integer from console (not implemented yet)
  #define DIAGTASK_ENABLE_READ_INTEGER        0
#endif

#ifndef DIAGTASK_ENABLE_READ_HEX_INTEGER
  /// @brief Enables support to read one hexadecimal integer from console (not implemented yet)
  #define DIAGTASK_ENABLE_READ_HEX_INTEGER    0
#endif

#ifndef DIAGTASK_ENABLE_READ_STRING
  /// @brief Enables support to read a string from console (not implemented yet)
  #define DIAGTASK_ENABLE_READ_STRING         0
#endif

// some adjustments
#ifndef DIAGTASK_MAX_HOOKNAME_LEN
  /// @brief defines the maximal hook name length (static array)
  #define DIAGTASK_MAX_HOOKNAME_LEN       20
#endif

#ifndef DIAGTASK_MAX_HOOK_INPUT_LEN
  /// @brief defines the maximal hook input length. This should be larger than
  ///        DIAGTASK_MAX_HOOKNAME_LEN if wildcard hooks are used and parameters are added to
  ///        hook name
  #define DIAGTASK_MAX_HOOK_INPUT_LEN     (DIAGTASK_MAX_HOOKNAME_LEN + 10)
#endif


#ifndef DIAGTASK_MIN_HOOKNAME_LEN
  /// @brief defines the minimal hook name length. Must be 1 if single character hook names are used.
  #define DIAGTASK_MIN_HOOKNAME_LEN       1
#endif

#ifndef DIAGTASK_HOOKDESC_LEN
  /// @brief defines the maximal hook description  length (static array)
  #define DIAGTASK_HOOKDESC_LEN           20
#endif

#ifndef DIAGTASK_MAX_HOOKS
  /// @brief defines the maximal number of hooks. currently only used when using ETL library
  #define DIAGTASK_MAX_HOOKS              20
#endif

#include <stdint.h>

#if DIAGTASK_USE_ETL
  #include <etl/vector.h>
#else
#include <vector>
#endif


class DiagTask
{

  public:
    enum features_t
    {
      feature_None          = 0x00,
      feature_Help          = 0x01,
      feature_Seperator     = 0x02,
      feature_Search        = 0x04,
      feature_Reboot        = 0x08,
      feature_TabCompletion = 0x10
    };

  private:

    /// @cond
    struct hookEntry_t
    {
      char name[DIAGTASK_MAX_HOOKNAME_LEN+1];
      char description[DIAGTASK_HOOKDESC_LEN+1];
      void(*hook)(const char* input);  // current input line
    };

    #if DIAGTASK_USE_ETL
    typedef etl::vector<DiagTask::hookEntry_t, DIAGTASK_MAX_HOOKS> diagtask_vector;
    #else
    typedef std::vector<DiagTask::hookEntry_t> diagtask_vector;
    #endif

    diagtask_vector mHooks;

    unsigned int mFeatures;

    // hold user input that still matches any hook name in array.
    // if user inputs a new character, all hook names are checked. if
    // new potential hook name is not found, mCurrentValidInput is reset.
    char mCurrentValidInput[DIAGTASK_MAX_HOOK_INPUT_LEN+1]; // add space for '\0'
    int (*mGetchar)();
    uint32_t (*mUptime)();
    void (*mReboot)();
  /// @endcond

  public:

    //// @brief Constructor for class.
    /**
     * Only one instance of this class should be created, because
     * getch() will be called concurently and only one instance will get the character.
     * @param getch - function pointer to function to read character from serial console.
     *                It is non-blocking and returns -1
                      If no character was read.
    */
    explicit DiagTask(int (*getch)());

    /// @brief Constructor for class.
    /**
     * Only one instance of this class should be created, because
     * getch() will be called concurently and only one instance will get the character.
     * @param getch - function pointer to function to read character from serial console.
     *                It is non-blocking and returns -1
     *                if no character was read.
     * @param uptime - function pointer to function that returns uptime in seconds
     * @param reboot - function pointer to function that reboots the system
     * @see DIAGTASK_ENABLE_SEPARATOR
    */
    explicit DiagTask(int (*getch)(), uint32_t (*uptime)(), void (*reboot)());
    explicit DiagTask(int (*getch)(), uint32_t (*uptime)());

    /// @brief enables some build-in features
    void enableFeatures(unsigned int features);

    /// @brief main diag task process
    /**
     * This task should be called repeatly to process hook inputs and call registerred functions.
     * It may be called in a loop of main task or an own task. If using RTOS it must be
     * ensured, that any data access is protected (multi threading)
     * This function must be called only from one execution context (thread)
     */
    void process(void);

    /** @brief registers a new hook
     * @param name name of the hook
     * @param hook function pointer to function that is called when hook is actiated
     * @param description description is displayed when all hooks are listed (press "?")
     * \return returns true on success, else false
     */
    bool registerHook( const char * name, void(*hook)(const char* input)
                     , const char * description = "");

    /** @brief calles a hook function (allows to call named hook)
     * @param name name of the hook
     * \return returns true on success, else false
     */
    bool executeHook(const char * name);

#if DIAGTASK_ENABLE_READ_KEY
    /*!
      * @brief  Reads a character from serial console
      *
      * Reads a character from the serial console. User may press ESC to abort the input.
      * In this case the storage where "out" points to is set to zero.
      * Pressing ENTER will return the code for "Enter".
      *
      * @param  out  where the character is stored
      * @param  echo Enables/Disables the echo of input
      * \return true and pressed character on success
      *         \li false when Parameter is NULL
      *         \li false when user has pressed ESC to abort
      * @see DIAGTASK_ENABLE_READ_KEY
      */
    bool readKey(/*@out@*/ char & out, bool echo = true);
#endif // DIAGTASK_ENABLE_READ_KEY

#if DIAGTASK_ENABLE_READ_INTEGER
    /**
      * @brief  Reads an decimal integer from serial console
      *
      * Reads a integer from the serial console. User may press ESC to abort the input.
      * In this case the storage where "out" points to is set to zero.
      * Pressing ENTER will return the code for "Enter".
      *
      * @param  out  where the integer is stored
      * @param  echo Enables/Disables the echo of input
      * \return true and integer on success
      *         \li false when Parameter is NULL
      *         \li false when user has pressed ESC to abort
      * @see DIAGTASK_ENABLE_READ_INTEGER
      */
    bool readInteger(/*@out@*/ int32_t & out, bool echo = true);
#endif //DIAGTASK_ENABLE_READ_INTEGER

#if DIAGTASK_ENABLE_READ_HEX_INTEGER
    /*!
      * @brief  Reads an hexadecimal integer from serial console
      *
      * Reads a integer from the serial console. User may press ESC to abort the input.
      * In this case the storage where "out" points to is set to zero.
      * Pressing ENTER will return the code for "Enter".
      *
      * @param  out  where the integer is stored
      * @param  echo Enables/Disables the echo of input
      * \return true and integer on success
      *         \li false when Parameter is NULL
      *         \li false when user has pressed ESC to abort
      * @see DIAGTASK_ENABLE_READ_HEX_INTEGER
      */
    bool readHexInteger(/*@out@*/ uint32_t & out, bool echo = true);
#endif // DIAGTASK_ENABLE_READ_HEX_INTEGER

#if DIAGTASK_ENABLE_READ_STRING
    /*!
      * @brief  Reads a string from serial console (zero terminated)
      *
      * Reads a string from the serial console. User may press ESC to abort the input.
      * The returned string is zero terminated.
      * In this case the storage where "out" points to is set to zero.
      * Pressing ENTER will return the code for "Enter".
      *
      * @param  out  Pointer where the integer is stored
      * @param  echo Enables/Disables the echo of input
      * \return true and string on success
      *         \li false when Parameter is NULL
      *         \li false when user has pressed ESC to abort
      * @see DIAGTASK_ENABLE_READ_STRING
      */
    bool readString(/*@out@*/ char *out, uint16_t maxlen, bool echo = true);
#endif // DIAGTASK_ENABLE_READ_STRING

  private:

    /// @cond

    // findes and returns all hooks that start with prefix
    diagtask_vector privFindHooks( const char * prefix);

    // returns true if a special function was executed (help,search,...)
    // input should hold current inserted character.
    // privCheckAndProcessSpecialChars() also access mCurrentValidInput
    bool privCheckAndProcessSpecialChars(char input);

    // returns true if a special function was executed (help,search,...)
    // input should hold current inserted character.
    // processTabCompetion() also access mCurrentValidInput
    bool processTabCompetion(char input);

    #if DIAGTASK_ENABLE_HELP
      void privHelp();
    #endif // DIAGTASK_ENABLE_HELP

    #if DIAGTASK_ENABLE_SEPARATOR
      void  privDisplaySeparator();
    #endif // DIAGTASK_ENABLE_SEPARATOR

  /// @endcond
}; // class diagtask

#endif // _INCLUDED_DIAGTASK_HPP_
