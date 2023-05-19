{
  "targets": [
    {
      "target_name": "chrolog_iohook",
      "sources": [ "chrolog_iohook.cc", "src/log.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "libraries": [ "-ludev", "-linput" , "-lwayland-client", "-lwayland-server", "-lwlroots"],
      "conditions": [
        ['OS=="win"', {
          "defines": ["_WIN32"],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1
            }
          }
        }]
      ],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.7"
      }
    }
  ],
  "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"]
}
