#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>
#include <fstream>
#include "../ext_libs/mINI/src/mini/ini.h"
#include <string>

namespace CipherSafe {
  class Settings {

  public:
    Settings(const std::string& work_path);

    std::string dark_mode = "dark";
    std::string font_path = "";
    std::string japanese_font_path = "";
    std::string greek_font_path = "";
    std::string korean_font_path = "";
    std::string chinese_font_path = "";
    std::string cyrillic_font_path = "";
    std::string thai_font_path = "";
    std::string viet_font_path = "";
    std::string backup_dir = "";
    double font_size = 18.0f;
    int console_height = 24;
    int password_length = 18;

    bool Save();

  private:
    std::string work_dir = "";
    std::string settings_file_path = "";
    bool Load(); 
    bool ini_file_exists();
  };

}
#endif 
