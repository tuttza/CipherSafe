#include "settings.h"

using namespace CipherSafe;

Settings::Settings(const std::string& work_path) {
  this->work_dir = work_path;
  this->settings_file_path = this->work_dir + "/settings.ini";
  this->Load();
}

bool Settings::Load() {
  std::cout << "Loading the settings.ini file..." << std::endl;

  bool did_load = false;

  if (!this->file_exists()) {
    std::cout << "settings.ini file did not exist, now creating a new settings.ini file: " << this->settings_file_path << std::endl;

    mINI::INIFile file(this->settings_file_path);
    mINI::INIStructure ini;

    ini["ciphersafe_settings"]["console_height"] = std::to_string(this->console_height);
    ini["ciphersafe_settings"]["password_length"] = std::to_string(this->password_length);
    ini["ciphersafe_settings"]["dark_mode"] = this->dark_mode;
    ini["ciphersafe_settings"]["font"] = this->font_path;

    file.generate(ini);
  }

  mINI::INIFile file(this->settings_file_path);
  mINI::INIStructure ini;

  if (file.read(ini)) {
    this->console_height = std::stoi(ini["ciphersafe_settings"]["console_height"]);
    this->password_length = std::stoi(ini["ciphersafe_settings"]["password_length"]);
    this->dark_mode = ini["ciphersafe_settings"]["dark_mode"];
    this->font_path = ini["ciphersafe_settings"]["font"];
    did_load = true;
  }

  return did_load;
}

bool Settings::Save() {
  bool did_save = false;

  if (!this->file_exists()) {
    return did_save;
  }

  mINI::INIFile file(this->settings_file_path);
  mINI::INIStructure ini;

  if (this->console_height <= 0 || this->console_height > 50) {
    this->console_height = 24;
  }
  ini["ciphersafe_settings"]["console_height"] = std::to_string(this->console_height);


  if (this->password_length <= 0 || this->password_length > 200) {
    this->password_length = 18;
  }
  ini["ciphersafe_settings"]["password_length"] = std::to_string(this->password_length);

  if (this->dark_mode.empty()) {
    this->dark_mode = "dark";
  }
  ini["ciphersafe_settings"]["dark_mode"] = this->dark_mode;

  std::ifstream font_file(this->font_path);
  if (!font_file.good()) {
    this->font_path = "";
  }

  ini["ciphersafe_settings"]["font"] = this->font_path;
  
  if (file.write(ini)) {
    did_save = true;
  }

  return did_save;
}

bool Settings::file_exists() {
  if (this->settings_file_path.empty()) {
    std::cerr << "Could not find settings.ini file: path is empty" << std::endl;
    return false;
  }

  std::ifstream file(this->settings_file_path);
  return file.good();
}
