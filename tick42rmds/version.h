/*
 * Tick42 RMDS bridge identification macros
 *
 * This file is used both by the bridge C++ code and
 * by the Microsoft (Windows) resource compiler.
 * 
 * For some reason, the Microsoft resource compiler
 * requires the FILE_VERSION as a CSV-list of numbers
 * and the PRODUCT_VERSION as a string resource. It
 * doesn't seem to like nested macros, so it is not
 * possible to stringize the number-based versions.
 * This is the reason why we have duplicate copies
 * of the version information here
 */

#define BRIDGE_NAME_STRING "tick42rmds"

#define BRIDGE_VERSION_MAJOR 1
#define BRIDGE_VERSION_MINOR 3
#define BRIDGE_VERSION_REVISION 0
#define BRIDGE_VERSION_BUILD 0
#define BRIDGE_VERSION_STRING "1.3.0.0"
