#include <config.h>

struct field_desc;

#ifdef SCHEDULER_JLINK_PLUGIN_SUPPORT
#define enable_debugger_support() extern const void *_debugger_layouts; __unused const void *_layouts = _debugger_layouts;
#else
#define enable_debugger_support()
#endif
