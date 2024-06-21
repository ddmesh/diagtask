# Diagtask

**Version**: 1.1<br/>
**Author**: Stephan Enderlein<br/>
**License**: BSD-3-Clause

**Description**:
DiagTask is a debugging tool that allows to register "hooks".
On serial console diagtask waits for user input and calles registerred
functions. Those functions can then print infos or control the behavior
of the software.

diagtask supports **tab-completion, search, printing a separator line**.
At moment input functions are prepared but not implemented yet.
As an alternative to pass parameters, diagtask supports "**wildcard**".

When a name for a hook is registerred simply add "*****" (e.g.: "_hook_ *).
The user can then enter "_hook_" followed by space and any text. The line
must then be finished by '**\n**' (pressing Enter). The complete input line
is passed as 'input' paramter to hook, that can parse it (e.g.: _scanf_).


# Usage example
<pre>
static DiagTask diagtask(getch, &osSysGetTick); // see diagtask.hpp for parameters

// enable some features (feature_Reboot is disabled in diagtask.hpp because of dependencies to OS functions)
diagtask.enableFeatures(  DiagTask::feature_Help | DiagTask::feature_Search
                             | DiagTask::feature_Seperator | DiagTask::feature_TabCompletion
                             | DiagTask::feature_Reboot);

static void _a_hook_function(const char* input)
{
    printf("hook called with [%s]\n", input);
}

// register a function that is called when user enters "hook-name" on serial console
diagtask.registerHook("hook-name", _a_hook_function, "Description");

// diagtask.process(); should be called repeatly to get characters from serial console/port.
// serial port driver should be implemented via interrupt that stores in comming bytes in internal
// buffer. getch() will retrieve one character in non-blocking way.
// Call this function every 250ms is sufficient.
void _taskDiagTask()
{
  diagtask.process();
}
</pre>

**Example Output**

<pre>
>?
? - help
# - separator
! - reboot
hook-name                   Description
>hook-name
hook called with [hook-name]

</pre>
