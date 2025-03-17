#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <iostream>
#include <thread>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <functional> //for std::function
#include <algorithm>  //for std::generate_n, std::sort
#include <chrono>
#include <cstdlib>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include "crypt.h"
#include "settings.h"
#include "database.h"

// C stuff:
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>

/*
 * we need a way to globally store app
 * state(e.g if we display/hide a button or window etc...).
 * Depending on how complex the UI gets this struct
 * could also grow in complexity.
 */
struct AppState {
    bool show_add_form;
    bool show_main_window;
    bool show_console;
    bool show_secret;
    bool show_settings;
    bool exit_app_loop;
    bool can_edit = false;

    /*
     * due to how ImGui::InputText works with str buffers under the hood
     * we need these in order to make our db update functionality work.
     */
    std::string updated_title;
    std::string updated_url;
    std::string updated_username;
    std::string updated_password;
    std::string updated_category;
    std::string updated_notes;

    void clearUpdatedStrings() {
        updated_title.clear();
        updated_url.clear();
        updated_username.clear();
        updated_password.clear();
        updated_category.clear();
        updated_notes.clear();
    }

    /*
     * FormState will be used for storing text need for
     * creating & editing, and displaying secrets.
     */
    struct FormState {
        std::string titleBuf    = "";
        std::string urlBuf      = "";
        std::string usernameBuf = "";
        std::string passwordBuf = "";
        std::string categoryBuf = "";
        std::string notesBuf    = "";
        
        void printFormState() {
            std::cout << "===[ FormState ]===" << std::endl;
            std::cout << "TITLE_BUF: " << titleBuf << std::endl;
            std::cout << "URL_BUF: " << urlBuf << std::endl;
            std::cout << "USERNAME_BUF: " << usernameBuf << std::endl;
            std::cout << "PASSWORD_BUF: " << passwordBuf << std::endl;
            std::cout << "CATEGORY_BUF: " << categoryBuf << std::endl;
            std::cout << "NOTES_BUF: " << notesBuf << std::endl;
        }
    };
    FormState formState;

    void resetFormState() {
        formState.titleBuf    = "";
        formState.urlBuf      = "";
        formState.usernameBuf = "";
        formState.passwordBuf = "";
        formState.categoryBuf = "";
        formState.notesBuf    = "";
    }

    std::unique_ptr<CipherSafe::Settings> settings;

    /*
     * This struct bundles the various states of our window
     * that need state from SDL + ImGui so we can
     * easily reference them through out our program.
     */
    struct WindowContext {
        SDL_GLContext* gl_context;
        SDL_Window* window;
        ImGuiIO* imgui_io;
    };
    WindowContext windowContext;

    CipherSafe::Database::Entry *currentActiveEntry;
    std::unique_ptr<CipherSafe::Database> db;

    std::string consoleText = "Idle...";
    std::string filterQuery = u8"";
    int selectedEntryId;
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_ReadOnly;
    std::string edit_label = "Edit";
    int click_step = 1;
    
    std::string delete_label = "Delete";
    int delete_click_step = 0;

    CipherSafe::Crypt crypt; 
};

// ====[ HELPERS ]====
static void h_print_callback_data(ImGuiInputTextCallbackData* data);
static void h_print_callback_data(ImGuiInputTextCallbackData* data) {
    std::cout << "====[ TextCallBackData ]====" << std::endl;
    std::cout << "Buf: " << data->Buf << std::endl;
    std::cout << "BufTextLen: " << data->BufTextLen << std::endl;
    std::cout << "BufSize (bytes + 1): " << data->BufSize << std::endl;
    std::cout << "BufDirty: " << data->BufDirty << std::endl;
    std::cout << "CursorPos: " << data->CursorPos << std::endl;
    std::cout << "SelectionStart: " << data->SelectionStart << std::endl;
    std::cout << "SelectionEnd: " << data->SelectionEnd << std::endl;
}

// ====[FUNCTION DECLARATIONS]====
static void MainWindowTearDown(std::unique_ptr<AppState>& app_state);
static void DisplayAddForm(std::unique_ptr<AppState>& app_state);
static void DisplayTable(std::unique_ptr<AppState>& app_state);
static void DisplayConsole(std::unique_ptr<AppState>& app_state);
static void DisplaySecret(std::unique_ptr<AppState>& app_state);
static void DisplaySettings(std::unique_ptr<AppState>& app_state);
static void InitSDL(std::unique_ptr<AppState>& app_state);
static void ShowMainWindow(std::unique_ptr<AppState>& app_state);
static bool createAppDir(const std::string& dirPath);
static std::string getUserHomeDir();
static bool InitApp();
static std::string randomString(size_t length);
static void openURL(const std::string& url);

// ====[FUNCTION DEFINITIONS]====
static bool startsWith(const std::string& str, const std::string& prefix) {
    return str.substr(0, prefix.size()) == prefix;
}

static bool isValidURL(const std::string& url) {
    return startsWith(url, "http://") || startsWith(url, "https://");
}

static void openURL(const std::string& url) {
#ifdef _WIN32
    std::string command = "start " + url;
#elif __APPLE__
    std::string command = "open " + url;
#elif __linux__
    std::string command = "xdg-open " + url;
#else
    #error "Unsupported OS"
#endif
    std::system(command.c_str());
}

static bool InitApp() {
    bool did_init = false;
    std::string homeDir = getUserHomeDir();

    if (homeDir.empty()) {
        return did_init;
    }

    if (!homeDir.empty()) {
        std::string appDir = homeDir + "/.CipherSafe";
        createAppDir(appDir);
        did_init = true;
    }

    return did_init;
}

static std::string getUserHomeDir() {
    const char* homeEnv = std::getenv("HOME");

    if (homeEnv != nullptr) {
        return std::string(homeEnv);
    } else {
        // Fallback option: Use getpwuid to get home directory
        struct passwd* pw = getpwuid(getuid());

        if (pw != nullptr) {
            return std::string(pw->pw_dir);
        } else {
            std::cerr << "Failed to retrieve user's home directory" << std::endl;
            return "";
        }
    }
}

static bool createAppDir(const std::string& dirPath) {
    char* app_work_dir_from_env = std::getenv("CIPHERSAFE_WORKDIR");

    if (app_work_dir_from_env) {
        std::cout << "skipping creating home dir as alternate workdir specfied by ENV var." << std::endl;
        return true;
    }

    if (mkdir(dirPath.c_str(), 0755) != 0) {
        if (errno != EEXIST) {
            std::cerr << "Failed to create directory: " << strerror(errno) << std::endl;
            return false;
        }
    }
    std::cout << "Directory created: " << dirPath << std::endl;
    return true;
}

static bool stringContainsSubstringIgnoreCase(const std::string& str, const std::string& substr) {
    std::string strLower = str;
    std::string substrLower = substr;

    std::transform(strLower.begin(), strLower.end(), strLower.begin(), [](unsigned char c){ return std::tolower(c); });
    std::transform(substrLower.begin(), substrLower.end(), substrLower.begin(), [](unsigned char c){ return std::tolower(c); });

    return strLower.find(substrLower) != std::string::npos;
}

static bool canLoadFont(const std::string& fontPath) {
    if (fontPath.empty()) {
        return false;
    }
    
    std::ifstream font_file(fontPath);

    if (font_file.good() && !fontPath.empty()) {
        return true;
    } else {
        return false;
    }
}

static void loadFontsTask(ImGuiIO* io, std::unique_ptr<AppState>& app_state) {
    (void)*io;

    // Handle font loading:
    if (canLoadFont(app_state->settings->font_path)) {
        std::cout << "loading main app font..." << std::endl;
        io->Fonts->AddFontFromFileTTF(app_state->settings->font_path.c_str(), app_state->settings->font_size, nullptr, io->Fonts->GetGlyphRangesDefault());
    } else {
        std::cout << "no main font found default font..." << std::endl;
        io->Fonts->AddFontDefault();
    }

    ImFontConfig fontConfig;
    fontConfig.MergeMode = true;

    // load non-latin fonts here:
    if (canLoadFont(app_state->settings->japanese_font_path)) {
        io->Fonts->AddFontFromFileTTF(app_state->settings->japanese_font_path.c_str(), app_state->settings->font_size, &fontConfig, io->Fonts->GetGlyphRangesJapanese());
    }

    if (canLoadFont(app_state->settings->korean_font_path)) {
        io->Fonts->AddFontFromFileTTF(app_state->settings->korean_font_path.c_str(), app_state->settings->font_size, &fontConfig, io->Fonts->GetGlyphRangesKorean());
    }

    if (canLoadFont(app_state->settings->chinese_font_path)) {
        io->Fonts->AddFontFromFileTTF(app_state->settings->chinese_font_path.c_str(), app_state->settings->font_size, &fontConfig, io->Fonts->GetGlyphRangesChineseFull());
    }

    if (canLoadFont(app_state->settings->thai_font_path)) {
        io->Fonts->AddFontFromFileTTF(app_state->settings->thai_font_path.c_str(), app_state->settings->font_size, &fontConfig, io->Fonts->GetGlyphRangesThai());
    }

    if (canLoadFont(app_state->settings->viet_font_path)) {
        io->Fonts->AddFontFromFileTTF(app_state->settings->viet_font_path.c_str(), app_state->settings->font_size, &fontConfig, io->Fonts->GetGlyphRangesVietnamese());
    }

    if (canLoadFont(app_state->settings->cyrillic_font_path)) {
        io->Fonts->AddFontFromFileTTF(app_state->settings->cyrillic_font_path.c_str(), app_state->settings->font_size, &fontConfig, io->Fonts->GetGlyphRangesCyrillic());
    }

    if (canLoadFont(app_state->settings->greek_font_path)) {
        io->Fonts->AddFontFromFileTTF(app_state->settings->greek_font_path.c_str(), app_state->settings->font_size, &fontConfig, io->Fonts->GetGlyphRangesGreek());
    }

    io->Fonts->Build();
}

static void InitSDL(std::unique_ptr<AppState>& app_state) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        throw std::runtime_error("Error: " + std::string( SDL_GetError() ));
    }

    // From 2.0.18: Enable native IME.
    #ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    #endif

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("CipherSafe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 750, 600, window_flags);

    if (window == nullptr) {
        throw std::runtime_error("Error: SDL_CreateWindow(): " + std::string( SDL_GetError() ));
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    auto start = std::chrono::high_resolution_clock::now();
    std::thread load_fonts(loadFontsTask, &io, std::ref(app_state));
    //loadFontsTask(&io, std::ref(app_state));

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    app_state->windowContext.imgui_io = &io;

    // Setup Dear ImGui style
    if (app_state->settings->dark_mode == "dark")
        ImGui::StyleColorsDark();

    if (app_state->settings->dark_mode == "light")
        ImGui::StyleColorsLight();

    // Rounded window corners
    ImGui::GetStyle().WindowRounding = 0.0f;

    // Setup Platform/Renderer backends
    if (ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
        ImGui_ImplOpenGL2_Init();
    }
    else {
        throw std::runtime_error("Error: Could not initialize OpenGL");
    }

    app_state->windowContext.gl_context = &gl_context;
    app_state->windowContext.window = window;
}

static void DisplayTable(std::unique_ptr<AppState>& app_state) {
    std::vector<std::unique_ptr<CipherSafe::Database::Entry>> dbEntries;

    if (app_state->filterQuery.empty()) {
        dbEntries = app_state->db->GetAll();
    }
    else {
        dbEntries = app_state->db->Filter(app_state->filterQuery);
    }

    int entriesSize = dbEntries.size();
    bool selected = false;

    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##search", &app_state->filterQuery);
    ImGui::PopItemWidth();

    if (entriesSize > 0) {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::BeginTable("##secrets_list", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("TITLE");
            ImGui::TableSetupColumn("URL/SERVICE/APP");
            ImGui::TableSetupColumn("CATEGORY");
            ImGui::TableHeadersRow();

            //ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();

            for(auto& entry: dbEntries) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                if (ImGui::Selectable(std::to_string(entry->id).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    std::cout << "selected: " << "id: " << std::to_string(entry->id) << std::endl;
                    app_state->show_secret = true;
                    app_state->selectedEntryId = entry->id;
                }

                ImGui::TableNextColumn();

                ImGui::Text(entry->title.c_str());
                ImGui::TableNextColumn();

                ImGui::Text(entry->url.c_str());
                ImGui::TableNextColumn();

                ImGui::Text(entry->category.c_str());
                ImGui::TableNextColumn();
            }

            ImGui::EndTable();
        }
    }
    else if (!app_state->filterQuery.empty() && entriesSize <= 0) {
        ImGui::Spacing();
        std::string noResultsMsg = "No secrets found searching with: " + app_state->filterQuery;
        ImGui::Text(noResultsMsg.c_str());
    }
    else {
        ImGui::Spacing();
        ImGui::Text("No Secrets to display... Click the \"Add\" button to get started.");
    }
}

static void DisplayConsole(std::unique_ptr<AppState>& app_state) {
    if (app_state->show_console) {
        ImGui::SeparatorText("CipherSafe Console");
        ImGui::Text(app_state->consoleText.c_str());

        for (auto i = 0; i < app_state->settings->console_height; i++) {
            ImGui::Spacing();
        }
    }
}

static void ShowMainWindow(std::unique_ptr<AppState>& app_state) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("window", &app_state->show_main_window, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    if (ImGui::Button("Add")) {
        app_state->consoleText = "add secret initiated...";
        app_state->show_add_form = true;
    }

    ImGui::SetItemTooltip("add a new secret...");

    ImGui::SameLine(); // make the settings button be on the same line as the remove button

    if (ImGui::Button("Settings")) {
        app_state->show_settings = true;
        std::cout << "settings button clicked" << std::endl;
    }

    ImGui::SeparatorText("Secrets List");

    DisplayTable(app_state);
    DisplayConsole(app_state);

    ImGui::End();
}

static void DisplayAddForm(std::unique_ptr<AppState>& app_state) {
    bool didSave = false;

    if (app_state->show_add_form) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Add New Secret", &app_state->show_add_form, ImGuiWindowFlags_NoDecoration);
        ImGui::SeparatorText("Add New Secret");

        if (ImGui::Button("Save")) {
            if(app_state->formState.urlBuf.empty()) {
                app_state->consoleText = "the url/app/service text field cannot be empty.";
            } else {
                std::unique_ptr<CipherSafe::Database::Entry> entry(new CipherSafe::Database::Entry());
                entry->title    = app_state->formState.titleBuf;
                entry->url      = app_state->formState.urlBuf;
                entry->username = app_state->formState.usernameBuf;
                entry->password = app_state->formState.passwordBuf;
                entry->category = app_state->formState.categoryBuf;
                entry->notes    = app_state->formState.notesBuf;

                if (app_state->db->Add(std::move(entry))) {
                    didSave = true;
                    app_state->consoleText = "successfully saved new secret to database...";
                }
            }

            if(didSave) {
                app_state->show_add_form = false;
                app_state->resetFormState();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            app_state->consoleText = "add secret cancelled...";
            app_state->show_add_form = false;
            app_state->resetFormState();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushItemWidth(-1);
        ImGui::Text("Title");
        ImGui::InputText("##title", &app_state->formState.titleBuf);

        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::PushItemWidth(-1);
        ImGui::Text("URL/App/Service");
        ImGui::InputText("##url", &app_state->formState.urlBuf);
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::PushItemWidth(-1);
        ImGui::Text("Username");
        ImGui::InputText("##username", &app_state->formState.usernameBuf);
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::PushItemWidth(-1);
        ImGui::Text("Password");
        ImGui::InputText("##password", &app_state->formState.passwordBuf);
        if (ImGui::Button("Generate Password")) {
            int password_length = app_state->settings->password_length;
            if (password_length <= 0) {
                password_length = 18; // if 0 is set in settings, then back to default value
            }

            if (password_length > 200) {
                password_length = 200; // the max chars that we generate for a password.
            }
            std::string generatedRandomString = randomString(password_length);
            app_state->formState.passwordBuf = generatedRandomString;
            app_state->consoleText = "new password successfully generated...";
        }
        ImGui::Spacing();
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::PushItemWidth(-1);
        ImGui::Text("Category");
        ImGui::InputText("##category", &app_state->formState.categoryBuf);
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::PushItemWidth(-1);
        ImGui::Text("Notes:");
        ImGui::InputTextMultiline("##notes", &app_state->formState.notesBuf, ImVec2(450, 85), ImGuiInputTextFlags_AllowTabInput);
        ImGui::PopItemWidth();

        DisplayConsole(app_state);

        ImGui::End();
    }
}

static int TitleInputTextUpdateCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        AppState *app_state = static_cast<AppState*>(data->UserData);

        if (data->Buf) { 
            app_state->currentActiveEntry->title = std::string(data->Buf);

            if (app_state->db->Update( app_state->currentActiveEntry )) {
                app_state->consoleText = "successfully updated secret.";
                return 0;
            } else {
                app_state->consoleText = "failed to update secret.";
                return 1;
            }
        }
    }

    return 0;
}

static int URLInputTextUpdateCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        AppState *app_state = static_cast<AppState*>(data->UserData);

        if (data->Buf) { 
            app_state->currentActiveEntry->url = std::string(data->Buf);

            if (app_state->db->Update( app_state->currentActiveEntry )) {
                app_state->consoleText = "successfully updated secret.";
                return 0;
            } else {
                app_state->consoleText = "failed to update secret.";
                return 1;
            }
        }
    }

    return 0;
}

static int UsernameInputTextUpdateCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        AppState *app_state = static_cast<AppState*>(data->UserData);

        if (data->Buf) { 
            app_state->currentActiveEntry->username = std::string(data->Buf);

            if (app_state->db->Update( app_state->currentActiveEntry )) {
                app_state->consoleText = "successfully updated secret.";
                return 0;
            } else {
                app_state->consoleText = "failed to update secret.";
                return 1;
            }
        }
    }

    return 0;
}

static int PasswordInputTextUpdateCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        AppState *app_state = static_cast<AppState*>(data->UserData);

        if (data->Buf) { 
            app_state->currentActiveEntry->password = std::string(data->Buf);

            if (app_state->db->Update( app_state->currentActiveEntry )) {
                app_state->consoleText = "successfully updated secret.";
                return 0;
            } else {
                app_state->consoleText = "failed to update secret.";
                return 1;
            }
        }
    }

    return 0;
}

static int CategoryInputTextUpdateCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        AppState *app_state = static_cast<AppState*>(data->UserData);

        if (data->Buf) { 
            app_state->currentActiveEntry->category = std::string(data->Buf);

            if (app_state->db->Update( app_state->currentActiveEntry )) {
                app_state->consoleText = "successfully updated secret.";
                return 0;
            } else {
                app_state->consoleText = "failed to update secret.";
                return 1;
            }
        }
    }

    return 0;
}

static int NotesInputTextUpdateCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        AppState *app_state = static_cast<AppState*>(data->UserData);

        if (data->Buf) { 
            app_state->currentActiveEntry->notes = std::string(data->Buf);

            if (app_state->db->Update( app_state->currentActiveEntry )) {
                app_state->consoleText = "successfully updated secret.";
                return 0;
            } else {
                app_state->consoleText = "failed to update secret.";
                return 1;
            }
        }
    }

    return 0;
}

static void DisplaySettings(std::unique_ptr<AppState>& app_state) {
    if (!app_state->show_settings) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Settings", &app_state->show_settings, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    ImGui::SeparatorText("Settings");
    ImGui::Spacing();
    ImGui::Spacing();

    int selected_dark_mode = 0;
    const char* items[] = { "Light", "Dark" };

    // Set initial state from settings.ini file:
    if (app_state->settings->dark_mode == "light") {
        selected_dark_mode = 0;
    }

    if (app_state->settings->dark_mode == "dark") {
        selected_dark_mode = 1;
    }

    ImGui::Text("Dark Mode:");
    if (ImGui::Combo("##Dark Mode", &selected_dark_mode, items, IM_ARRAYSIZE(items)))
    {
        std::cout << "Selected item: " << selected_dark_mode << " - " << items[selected_dark_mode] << std::endl;
        
        if (items[selected_dark_mode] == items[0]) {
            app_state->settings->dark_mode = "light"; 
            selected_dark_mode = 0;
            ImGui::StyleColorsLight();
        }

        if (items[selected_dark_mode] == items[1]) {
            app_state->settings->dark_mode = "dark";
            selected_dark_mode = 1;
            ImGui::StyleColorsDark();
        }

        if (app_state->settings->Save()) {
            app_state->consoleText = "successfully updated settings...";
        } else {
            app_state->consoleText = "failed to update settings...";
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Password Length:");
    ImGui::InputInt("##password_length", &app_state->settings->password_length);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Console Height:");
    ImGui::InputInt("##console_height", &app_state->settings->console_height);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::SeparatorText("Font Settings");

    ImGui::Text("All font releated settings require and app restart to take effect.");
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Font Size:");
    ImGui::InputDouble("##font_size", &app_state->settings->font_size);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("App Font:");
    ImGui::InputText("##app_font", &app_state->settings->font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Japanese Font:");
    ImGui::InputText("##japanese_font", &app_state->settings->japanese_font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Korean Font:");
    ImGui::InputText("##korean_font", &app_state->settings->korean_font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Chinese Font:");
    ImGui::InputText("##chinese_font", &app_state->settings->chinese_font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Thai Font:");
    ImGui::InputText("##thai_font", &app_state->settings->thai_font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Vietnamese Font:");
    ImGui::InputText("##viet_font", &app_state->settings->viet_font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Cyrillic Font:");
    ImGui::InputText("##cyrillic_font", &app_state->settings->cyrillic_font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Greek Font:");
    ImGui::InputText("##greek_font", &app_state->settings->greek_font_path);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::SameLine();
    if (ImGui::Button("Close and Save")) {
        app_state->show_settings = false;        
        if (app_state->settings->Save()) {
            app_state->consoleText = "succesfully updated settings...";
        } else {
            app_state->consoleText = "something went wrong updating settings, updates not saved...";
        }
    }

    ImGui::End();
}

static void DisplaySecret(std::unique_ptr<AppState>& app_state) {
    if (!app_state->show_secret) {
        return;
    }

    app_state->show_main_window = false;

    std::unique_ptr<CipherSafe::Database::Entry> secret;

    if (app_state->selectedEntryId) {
        secret = app_state->db->GetEntryById(app_state->selectedEntryId);   

        app_state->currentActiveEntry = secret.get();
        app_state->updated_title    = app_state->currentActiveEntry->title;     
        app_state->updated_url      = app_state->currentActiveEntry->url;
        app_state->updated_username = app_state->currentActiveEntry->username;
        app_state->updated_password = app_state->currentActiveEntry->password;
        app_state->updated_category = app_state->currentActiveEntry->category;
        app_state->updated_notes    = app_state->currentActiveEntry->notes;
    } else {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Secret ", &app_state->show_secret, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    if (secret->title.empty()) {
        ImGui::SeparatorText("Secret");
    } else {
        std::string separatorText = std::string("Secret - ") + secret->title;
        ImGui::SeparatorText(separatorText.c_str());
    }
    
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Title");
    ImGui::InputText("##title", &app_state->updated_title, (app_state->input_flags | ImGuiInputTextFlags_CallbackEdit), TitleInputTextUpdateCallback, app_state.get());
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("URL/App/Service");
    ImGui::InputText("##url", &app_state->updated_url, (app_state->input_flags | ImGuiInputTextFlags_CallbackEdit), URLInputTextUpdateCallback, app_state.get());
    ImGui::PopItemWidth();

    if (isValidURL(app_state->updated_url)) {
        if (ImGui::Button("Open")) {
            openURL(app_state->updated_url);
        }
    }

    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Username");
    ImGui::InputText("##username", &app_state->updated_username, app_state->input_flags | ImGuiInputTextFlags_CallbackEdit, UsernameInputTextUpdateCallback, app_state.get());
    ImGui::PopItemWidth();

    if (ImGui::Button("Copy Username")) {
        app_state->consoleText = "copied username to clipboard...";
        ImGui::SetClipboardText(secret->username.c_str());
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Password");
    ImGui::InputText("##password", &app_state->updated_password, app_state->input_flags | ImGuiInputTextFlags_CallbackEdit, PasswordInputTextUpdateCallback, app_state.get());
    ImGui::PopItemWidth();

    if (ImGui::Button("Copy Password")) {
        app_state->consoleText = "copied password to clipboard...";
        ImGui::SetClipboardText(secret->password.c_str());
    }

    ImGui::Spacing();

    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Category");
    ImGui::InputText("##category", &app_state->updated_category, app_state->input_flags | ImGuiInputTextFlags_CallbackEdit, CategoryInputTextUpdateCallback, app_state.get());
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::Text("Notes:");
    ImGui::InputTextMultiline("##notes", &app_state->updated_notes, ImVec2(450, 85), app_state->input_flags | ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackEdit, NotesInputTextUpdateCallback, app_state.get());
    ImGui::PopItemWidth();

    // Push red color styles for the delete button
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 255));
    // Darker red when hovered
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(200, 0, 0, 255));
    // Even darker red when active
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(150, 0, 0, 255));

    if (ImGui::Button(app_state->delete_label.c_str())) {
        app_state->delete_click_step++;

        if (app_state->delete_click_step == 1) {
            app_state->delete_label = "Are you sure?";
        }

        if (app_state->delete_click_step == 2) {
            app_state->delete_label = "Delete it!";
        }

        if (app_state->delete_click_step == 3) {
            if (app_state->db->RemoveEntryById(app_state->selectedEntryId)) {
                app_state->consoleText = "successfully removed secret";
                app_state->show_secret = false;
                app_state->show_main_window = true;

                app_state->delete_label = "Delete";
                app_state->delete_click_step = 0;
            }
        }
    }

    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();

    // Slightly darker yellow
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 194, 0, 255));

    // Darker yellow when hovered
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 153, 0, 255));
    
    // Even darker yellow when active
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(204, 102, 0, 255));

    if (ImGui::Button(app_state->edit_label.c_str())) {
        if (app_state->click_step > 1) {
            app_state->click_step = 0;
        }

        app_state->consoleText = "edit mode activated.";
        app_state->can_edit = true; 
        app_state->click_step++;

        if (app_state->can_edit) {
            app_state->edit_label = "Edit Mode Activated";
            app_state->input_flags = 0; // allow fields to be edited.
        }
        
        if (app_state->click_step == 1) {
            app_state->edit_label = "Edit";
            app_state->consoleText = "edit mode deactivated.";
            app_state->input_flags = ImGuiInputTextFlags_ReadOnly;
        }
    }

    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    if (ImGui::Button("Close")) {
        app_state->show_main_window = true;
        app_state->show_secret = false;

        app_state->edit_label = "Edit";
        app_state->input_flags = ImGuiInputTextFlags_ReadOnly;
        app_state->click_step = 1;
        app_state->clearUpdatedStrings();

        app_state->delete_label = "Delete";
        app_state->delete_click_step = 0;

        app_state->consoleText = "idle.";
    }

    DisplayConsole(app_state);

    ImGui::End();
}

static void MainWindowTearDown(std::unique_ptr<AppState>& app_state) {
    if (app_state->db) {
        app_state->db->Close();
    }

    app_state->crypt.encrypt_file();

    ImGui_ImplOpenGL2_Shutdown();

    ImGui_ImplSDL2_Shutdown();

    ImGui::DestroyContext();

    if (app_state->windowContext.gl_context) {
        SDL_GL_MakeCurrent(app_state->windowContext.window, nullptr); // Detach the context from the window

        SDL_GL_DeleteContext(app_state->windowContext.gl_context);
        app_state->windowContext.gl_context = nullptr;
    } 

    if (app_state->windowContext.window) {
        SDL_DestroyWindow(app_state->windowContext.window);
        app_state->windowContext.window = nullptr;
    }

    SDL_Quit();
}


static std::string randomString(size_t length) {
    RAND_poll();
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+";
    const size_t max_index = sizeof(charset) - 1;

    std::string password;
    password.reserve(length);

    // Buffer to hold random bytes
    unsigned char buffer[length];
    RAND_bytes(buffer, sizeof(buffer));

    for (size_t i = 0; i < length; ++i) {
        password += charset[buffer[i] % max_index];
    }

    return password;
}

int main(int argc, char* argv[]) {
    if (!InitApp()) {
        return 1;
    }

    char* app_work_dir_from_env = std::getenv("CIPHERSAFE_WORKDIR");
    std::string app_work_dir_value;

    if (app_work_dir_from_env) {
        std::cout << "using app_work_dir from env" << std::endl;
        app_work_dir_value = app_work_dir_from_env;
    } else {
        std::cout << "using hardcoded app_work_dir" << std::endl;
        app_work_dir_value = getUserHomeDir() + "/.CipherSafe/";
    }

    std::unique_ptr<CipherSafe::Settings> app_settings( new CipherSafe::Settings(app_work_dir_value) );

    std::unique_ptr<AppState> state(new AppState);
    //state->init("./"); // used for testing within the build dir. use when modifying crypt.cpp.
    state->crypt.init(app_work_dir_value);
    state->crypt.decrypt_file();
    state->show_main_window = true;
    state->show_console     = true;
    state->show_add_form    = false;
    state->show_secret      = false;
    state->settings         = std::move(app_settings);

    const std::string dbPath = app_work_dir_value + "core.db";
    std::unique_ptr<CipherSafe::Database> db(new CipherSafe::Database(dbPath));
    state->db = std::move(db);

    InitSDL(state);

    // Main loop
    while (!state->exit_app_loop) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                state->exit_app_loop = true;

            if (
                event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(state->windowContext.window)
                )
                state->exit_app_loop = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ShowMainWindow(state);
        DisplayAddForm(state);
        DisplaySecret(state);
        DisplaySettings(state);

        // Rendering
        ImGui::Render();

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        glViewport(
            0,
            0,
            (int)state->windowContext.imgui_io->DisplaySize.x,
            (int)state->windowContext.imgui_io->DisplaySize.y
        );
        
        glClearColor(
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        );
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(state->windowContext.window);
    }

    MainWindowTearDown(state);

    return 0;
}
