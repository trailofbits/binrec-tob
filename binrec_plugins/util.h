#ifndef UTIL_H
#define UTIL_H

#include <sstream>

#define PLUGIN_STATE(cls)                                                                          \
    cls();                                                                                         \
    virtual ~cls();                                                                                \
    virtual cls *clone() const                                                                     \
    {                                                                                              \
        return new cls(*this);                                                                     \
    }                                                                                              \
    static PluginState *factory(Plugin *p, S2EExecutionState *s)                                   \
    {                                                                                              \
        return new cls();                                                                          \
    }

template <class T> inline std::string to_string(const T &t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

#endif // UTIL_H
