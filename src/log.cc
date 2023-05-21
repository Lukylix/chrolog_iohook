/*
  Copyleft (ɔ) 2009 Kernc
  This program is free software. It comes with absolutely no warranty whatsoever.
  See COPYING for further information.

  Project homepage: https://github.com/kernc/chrolog_iohook
*/

#ifdef _WIN32 // Windows platform
#include <algorithm>
#include <cstdio>
#include <cerrno>
#include <cwchar>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <io.h>       // For file-related functions on Windows
#include <winsock2.h> // For socket-related functions on Windows
#include <windows.h>  // Other Windows-specific headers

#include <napi.h> // Assuming this is a library that's compatible with Windows
#include <iostream>
#include "keyconstants.h"
#include <thread>
// Additional Windows-specific headers and libraries

#else // Unix-like platforms (Linux, macOS, etc.)
#include <algorithm>
#include <cstdio>
#include <cerrno>
#include <cwchar>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <error.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <wctype.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/input.h>
#include <thread>
#include <iostream>
#include <locale>
#include <codecvt>
#include <libinput.h>
#include <unistd.h>
#include <fcntl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <atomic>

#include <napi.h>

// Additional Unix-like platform-specific headers and libraries

#endif
#ifdef HAVE_CONFIG_H
#include <config.h> // include config produced from ./configure
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "0.1.2" // if PACKAGE_VERSION wasn't defined in <config.h>
#endif

// the following path-to-executable macros should be defined in config.h;
#ifndef EXE_PS
#define EXE_PS "/bin/ps"
#endif

#ifndef EXE_GREP
#define EXE_GREP "/bin/grep"
#endif

#ifndef EXE_DUMPKEYS
#define EXE_DUMPKEYS "/usr/bin/dumpkeys"
#endif

#define COMMAND_STR_DUMPKEYS (EXE_DUMPKEYS " -n | " EXE_GREP " '^\\([[:space:]]shift[[:space:]]\\)*\\([[:space:]]altgr[[:space:]]\\)*keycode'")
#define COMMAND_STR_GET_PID ((std::string(EXE_PS " ax | " EXE_GREP " '") + program_invocation_name + "' | " EXE_GREP " -v grep").c_str())
#define COMMAND_STR_CAPSLOCK_STATE ("{ { xset q 2>/dev/null | grep -q -E 'Caps Lock: +on'; } || { setleds 2>/dev/null | grep -q 'CapsLock on'; }; } && echo on")

#define INPUT_EVENT_PATH "/dev/input/" // standard path
#define PID_FILE "/var/run/chrolog_iohook.pid"

#include "usage.cc"     // usage() function
#include "args.cc"      // global arguments struct and arguments parsing
#include "keytables.cc" // character and function key tables and helper functions

#ifdef _WIN32 // Windows platform

std::map<WPARAM, int> stateMap{
    {WM_KEYDOWN, 0}, // down
    {WM_KEYUP, 1},   // up
    {WM_LBUTTONDOWN, 0},
    {WM_LBUTTONUP, 1},
    {WM_RBUTTONDOWN, 0},
    {WM_RBUTTONUP, 1},
    {WM_MBUTTONDOWN, 0},
    {WM_MBUTTONUP, 1},
    {WM_XBUTTONDOWN, 0},
    {WM_XBUTTONUP, 1},
    {WM_MOUSEWHEEL, 1},  // Assuming scrolling up as "up"
    {WM_MOUSEHWHEEL, 1}, // Assuming scrolling left as "up"
    {WM_MOUSELEAVE, 1},  // Assuming leaving as "up"
    {WM_MOUSEHOVER, 0},  // Assuming hovering as "down"
};

// Define the map for wparam values
std::map<WPARAM, std::string> wparamMap{
    {WM_MOUSEMOVE, "Move"},
    {WM_LBUTTONDOWN, "leftButton"},
    {WM_LBUTTONUP, "leftButton"},
    {WM_RBUTTONDOWN, "rightButton"},
    {WM_RBUTTONUP, "rightButton"},
    {WM_MBUTTONDOWN, "middleButton"},
    {WM_MBUTTONUP, "middleButton"},
    {WM_XBUTTONDOWN, "xButton"},
    {WM_XBUTTONUP, "xButton"},
    {WM_MOUSEWHEEL, "Wheel"},
    {WM_MOUSEHWHEEL, "HorizontalWheel"},
    {WM_MOUSELEAVE, "Leave"},
    {WM_MOUSEHOVER, "Hover"},
    {WM_MOUSEHWHEEL, "HorizontalWheel"},
    {WM_MOUSEHWHEEL, "HorizontalWheel"},
    // Add more mappings as needed
};

extern std::atomic<bool> g_should_exit;

namespace chrolog_iohook
{
  std::string keylog = ""; // where store all key strokes are stored
  std::string latestKey;
  Napi::ThreadSafeFunction tsfn_mouse;
  Napi::ThreadSafeFunction tsfn_keyboard;

  HHOOK eHook = NULL; // pointer to our hook
  HHOOK mHook = NULL; // pointer to our hook

  LRESULT CALLBACK MouseProc(int nCode, WPARAM wparam, LPARAM lparam) // intercept mouse events
  {
    if (nCode >= 0)
    {

      if (wparam == WM_MOUSEMOVE)
      {
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        int x = cursorPos.x;
        int y = cursorPos.y;

        // Process mouse move event using x and y coordinates
        tsfn_mouse.BlockingCall([x, y](Napi::Env env, Napi::Function jsCallback)
                                {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("x", Napi::Number::New(env, x));
    obj.Set("y", Napi::Number::New(env, y));
    jsCallback.Call({Napi::String::New(env, "Move"), obj}); });
      }
      else
      {
        std::string wparamStr = wparamMap[wparam];
        int state = stateMap[wparam] || 1;

        tsfn_mouse.BlockingCall([wparamStr, state](Napi::Env env, Napi::Function jsCallback)
                                { jsCallback.Call({Napi::String::New(env, wparamStr), Napi::Number::New(env, state)}); });
      }
    }
  }

  LRESULT KeyBoardProc(int nCode, WPARAM wparam, LPARAM lparam) // intercept key presses
  {
    // wparam - key type, lparam - type of KBDLLHOOKSTRUCT
    // look in KeyConstants.h for key mapping
    if (nCode < 0)
      CallNextHookEx(eHook, nCode, wparam, lparam);

    KBDLLHOOKSTRUCT *kbs = (KBDLLHOOKSTRUCT *)lparam;
    std::string KeyName = Keys::KEYS[kbs->vkCode].Name;
    tsfn_keyboard.BlockingCall(&KeyName, [](Napi::Env env, Napi::Function jsCallback, std::string *keylog)
                               {
  // Find the index of the last newline character in keylog
  size_t lastNewlineIndex = keylog->rfind('\n');

  // Extract the latest key from keylog
  if (lastNewlineIndex != std::string::npos) {
    latestKey = keylog->substr(lastNewlineIndex + 1);

    // Remove the latest key from keylog
    keylog->erase(lastNewlineIndex + 1);
  } else {
    latestKey = *keylog;
    keylog->clear();
  }

  jsCallback.Call({Napi::String::New(env, latestKey)}); });

    return CallNextHookEx(eHook, nCode, wparam, lparam);
  }

  bool InstallHook()
  {

    // WH_KEYBOARD_LL - indicates we use keyboard hook and LL is low level -> global hook, value 13
    // OurKeyBoardProc - procedure invoked by hook system every time user press a key
    // GetModuleHandle serves for obatining H instance
    // DWTHREADID or 0 is identifier of thread which hook procedure is associated with (all existing threads)
    eHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyBoardProc, GetModuleHandle(NULL), 0);
    mHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)MouseProc, GetModuleHandle(NULL), 0);
    return eHook == NULL || mHook == NULL;
  }

  bool UninstallHook() // disable hook, does not stop keylogger
  {
    bool b = UnhookWindowsHookEx(eHook);
    eHook = NULL;
    return (bool)b;
  }

  bool IsHooked()
  {
    return (bool)(eHook == NULL);
  }

  int main(int argc, char **argv, Napi::ThreadSafeFunction mouse, Napi::ThreadSafeFunction keyboard)
  {
    tsfn_mouse = mouse;
    tsfn_keyboard = keyboard;
    InstallHook();
    MSG Msg; // msg object to be processed, but actually never is processed

    while (GetMessage(&Msg, NULL, 0, 0) && !g_should_exit.load()) // empties console window
    {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
    return 0;
  }
}

#else // Unix-like platforms (Linux, macOS, etc.)

extern std::atomic<bool> g_should_exit;

namespace chrolog_iohook
{

#define TIME_FORMAT "%F %T%z > " // results in YYYY-mm-dd HH:MM:SS+ZZZZ

  struct key_state_t
  {
    wchar_t key;
    unsigned int repeats; // count_repeats differs from the actual number of repeated characters! afaik, only the OS knows how these two values are related (by respecting configured repeat speed and delay)
    bool repeat_end;
    input_event event;
    bool scancode_ok;
    bool capslock_in_effect = false;
    bool shift_in_effect = false;
    bool altgr_in_effect = false;
    bool ctrl_in_effect = false; // used for identifying Ctrl+C / Ctrl+D
  } key_state;

  std::mutex file_size_mutex;
  int mouseX = 0;
  int mouseY = 0;
  // executes cmd and returns string ouput
  std::string execute(const char *cmd)
  {
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
      error(EXIT_FAILURE, errno, "Pipe error");
    char buffer[128];
    std::string result = "";
    while (!feof(pipe))
      if (fgets(buffer, 128, pipe) != NULL)
        result += buffer;
    pclose(pipe);
    return result;
  }

  int input_fd_keyboard = -1; // input event device file descriptor; global so that signal_handler() can access it
  int input_fd_mouse = -1;

  Napi::ThreadSafeFunction tsfn_mouse;
  Napi::ThreadSafeFunction tsfn_keyboard;

  struct Point
  {
    int x;
    int y;
  };

  void set_utf8_locale()
  {
    // set locale to common UTF-8 for wchars to be recognized correctly
    if (setlocale(LC_CTYPE, "en_US.UTF-8") == NULL)
    {                                         // if en_US.UTF-8 isn't available
      char *locale = setlocale(LC_CTYPE, ""); // try the locale that corresponds to the value of the associated environment variable LC_CTYPE
      if (locale != NULL &&
          (strstr(locale, "UTF-8") != NULL || strstr(locale, "UTF8") != NULL ||
           strstr(locale, "utf-8") != NULL || strstr(locale, "utf8") != NULL))
        ; // if locale has "UTF-8" in its name, it is cool to do nothing
      else
        error(EXIT_FAILURE, 0, "LC_CTYPE locale must be of UTF-8 type, or you need en_US.UTF-8 availabe");
    }
  }

  void exit_cleanup(int status, void *discard)
  {
    // TODO:
  }

  void create_PID_file()
  {
    // create temp file carrying PID for later retrieval
    int pid_fd = open(PID_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (pid_fd != -1)
    {
      char pid_str[16] = {0};
      sprintf(pid_str, "%d", getpid());
      if (write(pid_fd, pid_str, strlen(pid_str)) == -1)
        error(EXIT_FAILURE, errno, "Error writing to PID file '" PID_FILE "'");
      close(pid_fd);
    }
    else
    {
      if (errno == EEXIST) // This should never happen
        error(EXIT_FAILURE, errno, "Another process already running? Quitting. (" PID_FILE ")");
      else
        error(EXIT_FAILURE, errno, "Error opening PID file '" PID_FILE "'");
    }
  }

  void kill_existing_process()
  {
    pid_t pid;
    bool via_file = true;
    bool via_pipe = true;
    FILE *temp_file = fopen(PID_FILE, "r");

    via_file &= (temp_file != NULL);

    if (via_file)
    { // kill process with pid obtained from PID file
      via_file &= (fscanf(temp_file, "%d", &pid) == 1);
      fclose(temp_file);
    }

    if (!via_file)
    { // if reading PID from temp_file failed, try ps-grep pipe
      via_pipe &= (sscanf(execute(COMMAND_STR_GET_PID).c_str(), "%d", &pid) == 1);
      via_pipe &= (pid != getpid());
    }

    if (via_file || via_pipe)
    {
      remove(PID_FILE);
      kill(pid, SIGINT);

      exit(EXIT_SUCCESS); // process killed successfully, exit
    }
    error(EXIT_FAILURE, 0, "No process killed");
  }

  sigset_t set;
  int signal_pipe[2]; //  Pipe for self-pipe technique

  struct ThreadArgs
  {
    std::thread *keyboard_thread;
    std::thread *mouse_thread;
    std::thread *mouse_pos_thread;
  };

  void *signal_handling_thread(void *args)
  {
    // Extract the thread pointers from the arguments
    ThreadArgs *thread_args = static_cast<ThreadArgs *>(args);
    std::thread *keyboard_thread = thread_args->keyboard_thread;
    std::thread *mouse_thread = thread_args->mouse_thread;
    std::thread *mouse_pos_thread = thread_args->mouse_pos_thread;

    // Create a signal set with the desired signals to handle
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);

    // Unblock the signals for this thread
    if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0)
    {
      perror("pthread_sigmask");
      exit(EXIT_FAILURE);
    }

    int signal;
    bool should_exit = false;
    ssize_t read_result;
    while (!should_exit && !g_should_exit.load())
    {
      read_result = read(signal_pipe[0], &signal, sizeof(signal));
      if (read_result == -1)
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          continue;
        }
        else
        {
          perror("read");
          break;
        }
      }

      should_exit = true;
      break; // Exit the while loop
    }
    // remove pid file
    remove(PID_FILE);
    // Terminate all threads
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pthread_kill(keyboard_thread->native_handle(), SIGTERM);
    pthread_kill(mouse_thread->native_handle(), SIGTERM);
    pthread_kill(mouse_pos_thread->native_handle(), SIGTERM);
  }
  void signal_handler(int signum)
  {
    if (input_fd_keyboard != -1)
      close(input_fd_keyboard); // closing input file will break the infinite while loop
    if (input_fd_mouse != -1)
      close(input_fd_mouse);
    // Write the signal number to the pipe
    int write_result = write(signal_pipe[1], reinterpret_cast<const void *>(&signum), sizeof(signum));
    if (write_result == -1)
    {
      perror("write");
    }
  }

  void set_signal_handling()
  {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0; // Note: we're not setting SA_RESTART
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
      perror("sigaction(SIGINT)");
      exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
      perror("sigaction(SIGTERM)");
      exit(EXIT_FAILURE);
    }

    // Block the signals in the main thread
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
  }

  int parse_char_keycode(std::string string, int offset)
  {
    while (string[offset] == ' ')
      offset++;
    std::string numeric = string.substr(offset, 3);
    unsigned int keycode = atoi(numeric.c_str());
    if (keycode >= sizeof(char_or_func) - 1)
      return -1; // only ever map keycodes up to 128 (currently N_KEYS_DEFINED are used)
    if (!is_char_key(keycode))
      return -1; // only map character keys of keyboard
    return keycode;
  }

  void determine_system_keymap()
  {
    // custom map will be used; erase existing US keymapping
    memset(char_keys, '\0', sizeof(char_keys));
    memset(shift_keys, '\0', sizeof(shift_keys));
    memset(altgr_keys, '\0', sizeof(altgr_keys));

    // get keymap from dumpkeys
    // if one knows of a better, more portable way to get wchar_t-s from symbolic keysym-s from `dumpkeys` or `xmodmap` or another, PLEASE LET ME KNOW! kthx
    std::stringstream ss, dump(execute(COMMAND_STR_DUMPKEYS)); // see example output after i.e. `loadkeys slovene`
    std::string line;

    int index;
    int utf8code; // utf-8 code of keysym answering keycode i

    while (std::getline(dump, line))
    {
      int keycode;
      ss.clear();
      ss.str("");
      utf8code = 0;

      // replace any U+#### with 0x#### for easier parsing
      index = line.find("U+", 0);
      while (static_cast<std::string::size_type>(index) != std::string::npos)
      {
        line[index] = '0';
        line[index + 1] = 'x';
        index = line.find("U+", index);
      }

      assert(line.size() > 0);
      if (line[0] == 'k')
      {                                        // if line starts with 'keycode'
        keycode = parse_char_keycode(line, 8); // skip "keycode "
        if (keycode < 0)
          continue;
        index = to_char_keys_index(keycode);

        ss << &line[14]; // 1st keysym starts at index 14 (skip "keycode XXX = ")
        ss >> std::hex >> utf8code;
        // 0XB00CLUELESS: 0xB00 is added to some keysyms that are preceeded with '+'; I don't really know why; see `man keymaps`; `man loadkeys` says numeric keysym values aren't to be relied on, orly?
        if (line[14] == '+' && (utf8code & 0xB00))
          utf8code ^= 0xB00;
        char_keys[index] = static_cast<wchar_t>(utf8code);

        // if there is a second keysym column, assume it is a shift column
        if (ss >> std::hex >> utf8code)
        {
          if (line[14] == '+' && (utf8code & 0xB00))
            utf8code ^= 0xB00;
          shift_keys[index] = static_cast<wchar_t>(utf8code);
        }

        // if there is a third keysym column, assume it is an altgr column
        if (ss >> std::hex >> utf8code)
        {
          if (line[14] == '+' && (utf8code & 0xB00))
            utf8code ^= 0xB00;
          altgr_keys[index] = static_cast<wchar_t>(utf8code);
        }
      }
      else
      {                                         // else if line starts with 'shift i'
        keycode = parse_char_keycode(line, 15); // skip "\tshift\tkeycode " or "\taltgr\tkeycode "
        if (keycode < 0)
          continue;
        index = to_char_keys_index(keycode);
        ss << &line[21]; // 1st keysym starts at index 21 (skip "\tshift\tkeycode XXX = " or "\taltgr\tkeycode XXX = ")
        ss >> std::hex >> utf8code;
        if (line[21] == '+' && (utf8code & 0xB00))
          utf8code ^= 0xB00; // see line 0XB00CLUELESS

        if (line[1] == 's') // if line starts with "shift"
          shift_keys[index] = static_cast<wchar_t>(utf8code);
        if (line[1] == 'a') // if line starts with "altgr"
          altgr_keys[index] = static_cast<wchar_t>(utf8code);
      }
    } // while (getline(dump, line))
  }

  void parse_input_keymap()
  {
    // custom map will be used; erase existing US keytables
    memset(char_keys, '\0', sizeof(char_keys));
    memset(shift_keys, '\0', sizeof(shift_keys));
    memset(altgr_keys, '\0', sizeof(altgr_keys));

    stdin = freopen(args.keymap.c_str(), "r", stdin);
    if (stdin == NULL)
      error(EXIT_FAILURE, errno, "Error opening input keymap '%s'", args.keymap.c_str());

    unsigned int i = -1;
    unsigned int line_number = 0;
    wchar_t func_string[32];
    wchar_t line[32];

    while (!feof(stdin))
    {

      if (++i >= sizeof(char_or_func))
        break; // only ever read up to 128 keycode bindings (currently N_KEYS_DEFINED are used)

      if (is_used_key(i))
      {
        ++line_number;
        if (fgetws(line, sizeof(line), stdin) == NULL)
        {
          if (feof(stdin))
            break;
          else
            error_at_line(EXIT_FAILURE, errno, args.keymap.c_str(), line_number, "fgets() error");
        }
        // line at most 8 characters wide (func lines are "1234567\n", char lines are "1 2 3\n")
        if (wcslen(line) > 8) // TODO: replace 8*2 with 8 and wcslen()!
          error_at_line(EXIT_FAILURE, 0, args.keymap.c_str(), line_number, "Line too long!");
        // terminate line before any \r or \n
        std::wstring::size_type last = std::wstring(line).find_last_not_of(L"\r\n");
        if (last == std::wstring::npos)
          error_at_line(EXIT_FAILURE, 0, args.keymap.c_str(), line_number, "No characters on line");
        line[last + 1] = '\0';
      }

      if (is_char_key(i))
      {
        unsigned int index = to_char_keys_index(i);
        if (swscanf(line, L"%lc %lc %lc", &char_keys[index], &shift_keys[index], &altgr_keys[index]) < 1)
        {
          error_at_line(EXIT_FAILURE, 0, args.keymap.c_str(), line_number, "Too few input characters on line");
        }
      }
      if (is_func_key(i))
      {
        if (i == KEY_SPACE)
          continue; // space causes empty string and trouble
        if (swscanf(line, L"%7ls", &func_string[0]) != 1)
          error_at_line(EXIT_FAILURE, 0, args.keymap.c_str(), line_number, "Invalid function key string"); // does this ever happen?
        wcscpy(func_keys[to_func_keys_index(i)], func_string);
      }
    } // while (!feof(stdin))
    fclose(stdin);

    if (line_number < N_KEYS_DEFINED)
#define QUOTE(x) #x              // quotes x so it can be used as (char*)
      error(EXIT_FAILURE, 0, "Too few lines in input keymap '%s'; There should be " QUOTE(N_KEYS_DEFINED) " lines!", args.keymap.c_str());
  }

  std::string determine_keyboard_device()
  {
    const char *cmd = EXE_GREP " -B8 -E 'KEY=.*e$' /proc/bus/input/devices | " EXE_GREP " -E 'Name|Handlers|KEY' ";
    std::stringstream output(execute(cmd));

    std::vector<std::string> devices;
    std::vector<unsigned short> keyboard_scores;
    std::string line;

    unsigned short line_type = 0;
    unsigned short score = 0;

    while (std::getline(output, line))
    {

      transform(line.begin(), line.end(), line.begin(), ::tolower);

      // Generate score based on device name
      if (line_type == 0)
      {
        if (line.find("keyboard") != std::string::npos)
        {
          score += 100;
        }
      }
      // Add the event handler
      else if (line_type == 1)
      {
        std::string::size_type i = line.find("event");
        if (i != std::string::npos)
          i += 5; // "event".size() == 5
        if (i < line.size())
        {
          int index = atoi(&line.c_str()[i]);

          if (index != -1)
          {
            std::stringstream input_dev_path;
            input_dev_path << INPUT_EVENT_PATH;
            input_dev_path << "event";
            input_dev_path << index;

            devices.push_back(input_dev_path.str());
          }
        }
      }
      // Generate score based on size of key bitmask
      else if (line_type == 2)
      {
        std::string::size_type i = line.find("=");
        std::string full_key_map = line.substr(i + 1);
        score += full_key_map.length();
        keyboard_scores.push_back(score);
        score = 0;
      }
      line_type = (line_type + 1) % 3;
    }

    if (devices.size() == 0)
    {
      error(0, 0, "Couldn't determine keyboard device. :/");
      error(EXIT_FAILURE, 0, "Please post contents of your /proc/bus/input/devices file as a new bug report. Thanks!");
    }

    int max_keyboard_device = std::max_element(keyboard_scores.begin(), keyboard_scores.end()) - keyboard_scores.begin();
    return devices[max_keyboard_device];
  }
  std::string determine_mouse_device()
  {
    const std::string device_file = "/proc/bus/input/devices";
    std::ifstream infile(device_file);

    if (!infile)
    {
      std::cerr << "Failed to open " << device_file << std::endl;
      return "";
    }

    std::vector<std::string> devices;
    std::string line;

    bool is_mouse = false;

    while (std::getline(infile, line))
    {
      std::string lowercase_line = line;
      std::transform(lowercase_line.begin(), lowercase_line.end(), lowercase_line.begin(),
                     [](unsigned char c)
                     { return std::tolower(c); });

      if (lowercase_line.find("name") != std::string::npos)
      {
        if (lowercase_line.find("mouse") != std::string::npos ||
            lowercase_line.find("touchpad") != std::string::npos ||
            lowercase_line.find("trackpad") != std::string::npos)
        {
          is_mouse = true;
        }
      }
      else if (line.find("H:") == 0 && is_mouse)
      {
        std::string::size_type i = line.find("event");
        if (i != std::string::npos)
          i += 5; // "event".size() == 5
        if (i < line.size())
        {
          int index = atoi(&line.c_str()[i]);

          if (index != -1)
          {
            std::stringstream input_dev_path;
            input_dev_path << "/dev/input/event";
            input_dev_path << index;
            devices.push_back(input_dev_path.str());
            is_mouse = false; // Reset for the next potential mouse device section
          }
        }
      }
    }

    if (devices.empty())
    {
      std::cerr << "No suitable mouse device found." << std::endl;
      return "";
    }

    // Return the last found device
    return devices.back();
  }

  void determine_input_device()
  {
    // Safe switch user to nobody
    setegid(65534);
    seteuid(65534);

    std::string keyboard_device = determine_keyboard_device();
    std::string mouse_device = determine_mouse_device();
    if (!keyboard_device.empty())
      args.keyboard_device = keyboard_device;
    if (!mouse_device.empty())
      args.mouse_device = mouse_device;
    // Reclaim root privileges
    seteuid(0);
    setegid(0);

    // Handle the obtained devices as needed
    if (!keyboard_device.empty())
    {
      std::cout << "Keyboard device found: " << keyboard_device << std::endl;
    }

    if (!mouse_device.empty())
    {
      std::cout << "Mouse device found: " << mouse_device << std::endl;
    }
  }
  // Maintain the libinput and udev context globally or as member variables
  struct libinput *li = nullptr;
  struct udev *udev = nullptr;

  bool setupContext()
  {
    struct libinput_interface interface = {
        .open_restricted = [](const char *path, int flags, void *user_data) -> int
        {
          return open(path, flags);
        },
        .close_restricted = [](int fd, void *user_data) -> void
        {
          close(fd);
        },
    };

    udev = udev_new();
    if (!udev)
    {
      // handle error
      return false;
    }

    li = libinput_udev_create_context(&interface, nullptr, udev);
    if (!li)
    {
      // handle error
      udev_unref(udev);
      return false;
    }

    if (libinput_udev_assign_seat(li, "seat0") == -1)
    {
      // handle error
      libinput_unref(li);
      udev_unref(udev);
      return false;
    }

    return true;
  }

  void pollEvents()
  {
    while (true)
    {
      libinput_dispatch(li);
      while (struct libinput_event *ev = libinput_get_event(li))
      {
        if (libinput_event_get_type(ev) == LIBINPUT_EVENT_POINTER_MOTION)
        {
          struct libinput_event_pointer *pev = libinput_event_get_pointer_event(ev);
          double dx = libinput_event_pointer_get_dx(pev);
          double dy = libinput_event_pointer_get_dy(pev);
          // Calculate position relative to top left of the screen
          int screenMouseX = static_cast<int>(dx);
          int screenMouseY = static_cast<int>(dy);

          Point point;
          point.x = screenMouseX;
          point.y = screenMouseY;
          auto callback_move = [](Napi::Env env, Napi::Function jsCallback, Point *point)
          {
            Napi::Object moveObj = Napi::Object::New(env);
            moveObj.Set("x", Napi::Number::New(env, point->x));
            moveObj.Set("y", Napi::Number::New(env, point->y));
            Napi::String event = Napi::String::New(env, "move");
            jsCallback.Call({event, moveObj});
          };

          tsfn_mouse.BlockingCall(&point, callback_move);
        }
        libinput_event_destroy(ev);
      }
    }
  }

  bool handle_mouse_pos()
  {
    std::cout << "Mouse position thread started" << std::endl;
    if (!setupContext())
    {
      std::cout << "Failed to setup libinput context" << std::endl;
      return false;
    }
    pollEvents();
    return true;
  }
  std ::string last_coord_type = "";

  int triger_events()
  {
    int keymap_fd = open(args.keymap.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (keymap_fd == -1)
      error(EXIT_FAILURE, errno, "Error opening output file '%s'", args.keymap.c_str());
    char buffer[32];
    int buflen = 0;
    unsigned int index;
    for (unsigned int i = 0; i < sizeof(char_or_func); ++i)
    {
      buflen = 0;
      if (is_char_key(i))
      {
        index = to_char_keys_index(i);
        // only export non-null characters
        if (char_keys[index] != L'\0' &&
            shift_keys[index] != L'\0' &&
            altgr_keys[index] != L'\0')
          buflen = sprintf(buffer, "%lc %lc %lc\n", char_keys[index], shift_keys[index], altgr_keys[index]);
        else if (char_keys[index] != L'\0' &&
                 shift_keys[index] != L'\0')
          buflen = sprintf(buffer, "%lc %lc\n", char_keys[index], shift_keys[index]);
        else if (char_keys[index] != L'\0')
          buflen = sprintf(buffer, "%lc\n", char_keys[index]);
        else // if all \0, export nothing on that line (=keymap will not parse)
          buflen = sprintf(buffer, "\n");
      }
      else if (is_func_key(i))
      {
        buflen = sprintf(buffer, "%ls\n", func_keys[to_func_keys_index(i)]);
      }

      if (is_used_key(i))
      {
        if (write(keymap_fd, buffer, buflen) < buflen)
          error(EXIT_FAILURE, errno, "Error writing to keymap file '%s'", args.keymap.c_str());
      }
    }
    close(keymap_fd);
    error(EXIT_SUCCESS, 0, "Success writing keymap to file '%s'", args.keymap.c_str());
    exit(EXIT_SUCCESS);
    return keymap_fd;
  }

  bool update_mouse_state()
  {

    int n = read(input_fd_mouse, &key_state.event, sizeof(struct input_event));
    if (n <= 0)
    {
      // Error or end of input
      std::cout << "Failed to read mouse event" << std::endl;
      return false;
    }
    // Define the callback function to call JavaScript
    auto callback = [](Napi::Env env, Napi::Function jsCallback, std::pair<std::string, std::string> *eventData)
    {
      Napi::String eventKey = Napi::String::New(env, eventData->first);
      Napi::String eventValue = Napi::String::New(env, eventData->second);
      jsCallback.Call({eventKey, eventValue});
      delete eventData;
    };

    // Handle relative mouse movement
    if (key_state.event.type == EV_REL || key_state.event.type == EV_ABS)
    {
      return true;
    }
    else if (key_state.event.type == EV_KEY)
    {
      if (key_state.event.code == BTN_LEFT)
      {                                        // Left click
        bool is_press = key_state.event.value; // 1 for press, 0 for release
        std::pair<std::string, std::string> *eventData = new std::pair<std::string, std::string>("left_click", is_press ? "true" : "false");
        tsfn_mouse.BlockingCall(eventData, callback);
      }
      else if (key_state.event.code == BTN_RIGHT)
      {                                        // Right click
        bool is_press = key_state.event.value; // 1 for press, 0 for release
        std::pair<std::string, std::string> *eventData = new std::pair<std::string, std::string>("right_click", is_press ? "true" : "false");
        tsfn_mouse.BlockingCall(eventData, callback);
      }
      else if (key_state.event.code == BTN_MIDDLE)
      {                                        // Middle click
        bool is_press = key_state.event.value; // 1 for press, 0 for release
        std::pair<std::string, std::string> *eventData = new std::pair<std::string, std::string>("middle_click", is_press ? "true" : "false");
        tsfn_mouse.BlockingCall(eventData, callback);
      }
    }

    return true;
  }

  bool update_key_state()
  {
// these event.value-s aren't defined in <linux/input.h> ?
#define EV_MAKE 1                // when key pressed
#define EV_BREAK 0               // when key released
#define EV_REPEAT 2              // when key switches to repeating after short delay

    if (read(input_fd_keyboard, &key_state.event, sizeof(struct input_event)) <= 0)
    {
      std::cout << "Failed to read keyboard event" << std::endl;
      return false;
    }
    if (key_state.event.type != EV_KEY)
      return update_key_state(); // keyboard events are always of type EV_KEY

    unsigned short scan_code = key_state.event.code; // the key code of the pressed key (some codes are from "scan code set 1", some are different (see <linux/input.h>)

    key_state.repeat_end = false;
    if (key_state.event.value == EV_REPEAT)
    {
      key_state.repeats++;
      return true;
    }
    else if (key_state.event.value == EV_BREAK)
    {
      if (scan_code == KEY_LEFTSHIFT || scan_code == KEY_RIGHTSHIFT)
        key_state.shift_in_effect = false;
      else if (scan_code == KEY_RIGHTALT)
        key_state.altgr_in_effect = false;
      else if (scan_code == KEY_LEFTCTRL || scan_code == KEY_RIGHTCTRL)
        key_state.ctrl_in_effect = false;

      key_state.repeat_end = key_state.repeats > 0;
      if (key_state.repeat_end)
        return true;
      else
        return update_key_state();
    }
    key_state.repeats = 0;

    key_state.scancode_ok = scan_code < sizeof(char_or_func);
    if (!key_state.scancode_ok)
      return true;

    key_state.key = 0;

    if (key_state.event.value != EV_MAKE)
      return update_key_state();

    switch (scan_code)
    {
    case KEY_CAPSLOCK:
      key_state.capslock_in_effect = !key_state.capslock_in_effect;
      break;
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT:
      key_state.shift_in_effect = true;
      break;
    case KEY_RIGHTALT:
      key_state.altgr_in_effect = true;
      break;
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL:
      key_state.ctrl_in_effect = true;
      break;
    default:
      if (is_char_key(scan_code))
      {
        wchar_t wch;
        if (key_state.altgr_in_effect)
        {
          wch = altgr_keys[to_char_keys_index(scan_code)];
          if (wch == L'\0')
          {
            if (key_state.shift_in_effect)
              wch = shift_keys[to_char_keys_index(scan_code)];
            else
              wch = char_keys[to_char_keys_index(scan_code)];
          }
        }

        else if (key_state.capslock_in_effect && iswalpha(char_keys[to_char_keys_index(scan_code)]))
        {                                // only bother with capslock if alpha
          if (key_state.shift_in_effect) // capslock and shift cancel each other
            wch = char_keys[to_char_keys_index(scan_code)];
          else
            wch = shift_keys[to_char_keys_index(scan_code)];
          if (wch == L'\0')
            wch = char_keys[to_char_keys_index(scan_code)];
        }

        else if (key_state.shift_in_effect)
        {
          wch = shift_keys[to_char_keys_index(scan_code)];
          if (wch == L'\0')
            wch = char_keys[to_char_keys_index(scan_code)];
        }
        else // neither altgr nor shift are effective, this is a normal char
          wch = char_keys[to_char_keys_index(scan_code)];

        key_state.key = wch;
      }
    }
    if (key_state.key != 0)
    {
      // Define the callback function to call JavaScript
      auto callback = [](Napi::Env &env, Napi::Function &jsCallback, std::string *const &key)
      {
        Napi::String keyString = Napi::String::New(env, *key);
        jsCallback.Call({keyString});
        delete key;
      };

      // Pass the key press event to JavaScript
      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
      std::string *keyStringPtr = new std::string(converter.to_bytes(key_state.key));

      tsfn_keyboard.BlockingCall(keyStringPtr, callback);
    }
    return true;
  }

  void handle_keyboard()
  {
    bool key_state_valid = true;
    while (key_state_valid)
    {
      key_state_valid = update_key_state();
    }
  }

  void handle_mouse()
  {
    bool mouse_state_valid = true;
    while (mouse_state_valid)
    {
      mouse_state_valid = update_mouse_state();
    }
  }

  void log_loop()
  {
    char timestamp[32]; // timestamp string, long enough to hold format "\n%F %T%z > "

    struct stat st;
    stat(args.logfile.c_str(), &st);
    off_t file_size = st.st_size; // log file is currently file_size bytes "big"

    time_t cur_time;
    time(&cur_time);
    strftime(timestamp, sizeof(timestamp), TIME_FORMAT, localtime(&cur_time));

    std::thread keyboard_thread(handle_keyboard);
    std::thread mouse_thread(handle_mouse);
    std::thread mouse_pos_thread(handle_mouse_pos);

    // Create thread_args struct to hold the thread pointers
    ThreadArgs thread_args;
    thread_args.keyboard_thread = &keyboard_thread;
    thread_args.mouse_thread = &mouse_thread;
    thread_args.mouse_pos_thread = &mouse_pos_thread;

    pthread_t signal_thread;
    if (pthread_create(&signal_thread, NULL, signal_handling_thread, &thread_args) != 0)
    {
      std::cout << "Failed to create signal handling thread" << std::endl;
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }

    // keyboard_thread.join();
    // mouse_thread.join();

    if (pthread_join(signal_thread, NULL))
    {
      std::cout << "Error: pthread_join() failed for signal_thread" << std::endl;
      exit(EXIT_FAILURE);
    }

    // Close the write end of the signal pipe
    close(signal_pipe[1]);

    // Wait for the signal handling thread to finish
    pthread_join(signal_thread, NULL);

    // Close the read end of the signal pipe
    close(signal_pipe[0]);

    // append final timestamp, close files, and exit
    time(&cur_time);
    strftime(timestamp, sizeof(timestamp), "%F %T%z", localtime(&cur_time));
  }

  int main(int argc, char **argv, Napi::ThreadSafeFunction mouse, Napi::ThreadSafeFunction keyboard)

  {
    tsfn_mouse = mouse;
    tsfn_keyboard = keyboard;
    // Create the pipe for self-pipe technique
    if (pipe(signal_pipe) == -1)
    {
      perror("pipe");
      return -1;
    }

    // Set the read end of the pipe as non-blocking
    fcntl(signal_pipe[0], F_SETFL, O_NONBLOCK);

    on_exit(exit_cleanup, NULL);

    process_command_line_arguments(argc, argv);

    if (geteuid())
      error(EXIT_FAILURE, errno, "Got r00t?");
    // kill existing chrolog_iohook process
    if (args.kill)
      kill_existing_process();

    // if neither start nor export, that must be an error
    if (!args.start && !(args.flags & FLAG_EXPORT_KEYMAP))
    {
      usage();
      exit(EXIT_FAILURE);
    }

    // if posting remote and post_size not set, set post_size to default [500K bytes]
    if (args.post_size == 0 && (!args.http_url.empty() || !args.irc_server.empty()))
    {
      args.post_size = 500000;
    }

    // check for incompatible flags
    if (!args.keymap.empty() && (!(args.flags & FLAG_EXPORT_KEYMAP) && args.us_keymap))
    { // exporting uses args.keymap also
      error(EXIT_FAILURE, 0, "Incompatible flags '-m' and '-u'. See usage.");
    }

    // check for incompatible flags: if posting remote and output is set to stdout
    if (args.post_size != 0 && (args.logfile == "-"))
    {
      error(EXIT_FAILURE, 0, "Incompatible flags [--post-size | --post-http] and --output to stdout");
    }

    set_utf8_locale();

    if (args.flags & FLAG_EXPORT_KEYMAP)
    {
      if (!args.us_keymap)
        determine_system_keymap();
      triger_events();
      // = exit(0)
    }
    else if (!args.keymap.empty()) // custom keymap in use
      parse_input_keymap();
    else
      determine_system_keymap();

    if (args.keyboard_device.empty())
    { // no device given with -d switch
      determine_input_device();
    }
    else if (args.mouse_device.empty())
    {
      determine_input_device();
    }
    else
    { // event device supplied as -d argument
      std::string::size_type i = args.keyboard_device.find_last_of('/');
      args.keyboard_device = (std::string(INPUT_EVENT_PATH) + args.keyboard_device.substr(i == std::string::npos ? 0 : i + 1));

      // Normalize the mouse device path
      i = args.mouse_device.find_last_of('/');
      args.mouse_device = (std::string(INPUT_EVENT_PATH) + args.mouse_device.substr(i == std::string::npos ? 0 : i + 1));
    }

    std::cout << "Setting up signal handling..." << std::endl;

    set_signal_handling();

    close(STDIN_FILENO);

    std::cout << "Opening keyboard device..." << std::endl;
    input_fd_keyboard = open(args.keyboard_device.c_str(), O_RDONLY);
    if (input_fd_keyboard == -1)
      error(EXIT_FAILURE, errno, "Error opening input event device '%s'", args.keyboard_device);

    std::cout << "Opening mouse device..." << std::endl;
    input_fd_mouse = open(args.mouse_device.c_str(), O_RDONLY);
    if (input_fd_mouse == -1)
      error(EXIT_FAILURE, errno, "Error opening input event device '%s'", args.mouse_device.c_str());

    seteuid(getuid());
    setegid(getgid());

    if (access(PID_FILE, F_OK) != -1) // PID file already exists
      error(EXIT_FAILURE, errno, "Another process already running? Quitting. (" PID_FILE ")");

    // if (!(args.flags & FLAG_NO_DAEMON))
    // {
    //   int nochdir = 1;                    // don't change cwd to root (allow relative output file paths)
    //   int noclose = 1;                    // don't close streams (stderr used)
    //   if (daemon(nochdir, noclose) == -1) // become daemon
    //     error(EXIT_FAILURE, errno, "Failed to become daemon");
    // }

    // now we need those privileges back in order to create system-wide PID_FILE
    seteuid(0);
    setegid(0);
    if (!(args.flags & FLAG_NO_DAEMON))
    {
      create_PID_file();
    }

    // now we've got everything we need, finally drop privileges by becoming 'nobody'
    // setegid(65534); seteuid(65534);   // commented-out, I forgot why xD

    key_state.capslock_in_effect = execute(COMMAND_STR_CAPSLOCK_STATE).size() >= 2;

    log_loop();

    remove(PID_FILE);

    exit(EXIT_SUCCESS);
  } // main()

} // namespace chrolog_iohook
#endif

#ifdef _win32
int main()
{
  return 0;
}
#else
int main(int argc, char **argv, Napi::ThreadSafeFunction tsfn_mouse, Napi::ThreadSafeFunction tsfn_keyboard)
{
  return chrolog_iohook::main(argc, argv, tsfn_mouse, tsfn_keyboard);
}
#endif
