========================================================================
    STATIC LIBRARY : Utils Project Overview
========================================================================

Utils is a library of APIs and API wrappers that has many utilities one may 
find useful. The utils library is based on the following libraries:
1. Boost library
2. Wombat for the POSIX like function

Those libraries should be used instead of their counterparts that exists
on boost C/C++ standard libraries in order to keep the same codebase 
compatible with other parts of it

How to use utils libraries

1. add utils as a reference to your project
2. add utils as dependency to your project
3. add .\PropertySheets\utils.props to your project property manager