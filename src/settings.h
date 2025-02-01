#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>
#include <fstream>
#include "../lib/mINI/src/mini/ini.h"
#include <string>

namespace CipherSafe {
  class Settings {

  public:
    Settings(const std::string& work_path);

    std::string dark_mode = "dark";
    std::string font_path = "";
    std::string backup_dir = "";
    int console_height = 24;
    int password_length = 18;

    bool Save();

  private:
    std::string work_dir = "";
    std::string settings_file_path = "";
    bool Load(); 
    bool file_exists();
  };

}
#endif 
