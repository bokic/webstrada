/**
 * @file tag_silent.cpp
 * @brief <cfsilent> runtime (cf_silent_begin/cf_silent_end).
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/string.h>

#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace cfml {

string *cf_silent_begin(string *realOut)
{
    // Remember the real (non-discard) output buffer so a <cfcontent> reset
    // inside the silent body can clear the whole page like CF (see
    // response_apply_cfcontent / silent_real_out).
    string *discard = silent_buf_push();
    if (realOut) silent_set_real_out(realOut);
    return discard;
}

void cf_silent_end()
{
    silent_buf_pop();
}

} // namespace cfml
