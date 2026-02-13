

//version that provides more context about the assertion failure:
template <typename... Args>
void Log::PrintAssertMessage(Log::Type type, std::string_view prefix, std::format_string<Args...> message, Args &&...args)
{
    auto logger = (type == Type::Core) ? GetCoreLogger() : (type == Type::Editor) ? GetEditorLogger() : GetLauncherLogger();
    auto formatted = std::format(message, std::forward<Args>(args)...);
    
    // Enhanced error logging with more context
    logger->error("{}: {}", prefix, formatted);
    
#ifdef SEDX_ASSERT_MESSAGE_BOX
    // Prepare a more informative message for the dialog
    std::string dialogTitle = std::format("Scenery Editor X | {} Assert", 
                                          type == Type::Core ? "Core" : 
                                          type == Type::Editor ? "Editor" : "Launcher");
    
    std::string dialogMessage = std::format("ASSERTION FAILED\n\n"
                                           "Module: {}\n"
                                           "Message: {}\n\n"
                                           "Details:\n{}\n\n"
                                           "See log for more information.",
                                           type == Type::Core ? "Core" : 
                                           type == Type::Editor ? "Editor" : "Launcher",
                                           prefix,
                                           formatted);
    
    // Display a more detailed message box
    MessageBoxA(nullptr, dialogMessage.c_str(), dialogTitle.c_str(), MB_OK | MB_ICONERROR);
#endif
}

//For even more context, you could add file and line information:
template <typename... Args>
void Log::PrintAssertMessage(Log::Type type, std::string_view prefix, const char* file, int line, 
                            std::format_string<Args...> message, Args &&...args)
{
    auto logger = (type == Type::Core) ? GetCoreLogger() : (type == Type::Editor) ? GetEditorLogger() : GetLauncherLogger();
    auto formatted = std::format(message, std::forward<Args>(args)...);
    
    // Enhanced error logging with file and line info
    logger->error("{}:{}: {}: {}", file, line, prefix, formatted);
    
#ifdef SEDX_ASSERT_MESSAGE_BOX
    // Extract filename from path
    std::string filename = file;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        filename = filename.substr(lastSlash + 1);
    
    std::string dialogTitle = std::format("Scenery Editor X | {} Assert", 
                                          type == Type::Core ? "Core" : 
                                          type == Type::Editor ? "Editor" : "Launcher");
    
    std::string dialogMessage = std::format("ASSERTION FAILED\n\n"
                                           "Location: {}:{}\n"
                                           "Module: {}\n"
                                           "Condition: {}\n\n"
                                           "Details:\n{}\n\n"
                                           "See log for more information.",
                                           filename, line,
                                           type == Type::Core ? "Core" : 
                                           type == Type::Editor ? "Editor" : "Launcher",
                                           prefix,
                                           formatted);
    
    MessageBoxA(nullptr, dialogMessage.c_str(), dialogTitle.c_str(), MB_OK | MB_ICONERROR);
#endif
}

// Using MessageBoxW for Unicode Support
// International character support, you can use MessageBoxW with proper UTF-16 conversion:
#ifdef SEDX_ASSERT_MESSAGE_BOX
    // Convert to wide strings for MessageBoxW
    std::wstring wTitle = std::wstring (dialogTitle.begin (), dialogTitle.end ());
    std::wstring wMessage = std::wstring (dialogMessage.begin (), dialogMessage.end ());

    MessageBoxW (nullptr, wMessage.c_str (), wTitle.c_str (), MB_OK | MB_ICONERROR);
#endif


//For more complex UTF-8 to UTF-16 conversion:
#ifdef SEDX_ASSERT_MESSAGE_BOX
    // Convert UTF-8 to UTF-16 for proper Unicode handling
    int wTitleSize = MultiByteToWideChar (CP_UTF8, 0, dialogTitle.c_str (), -1, nullptr, 0);
    std::wstring wTitle (wTitleSize, 0);
    MultiByteToWideChar (CP_UTF8, 0, dialogTitle.c_str (), -1, &wTitle[0], wTitleSize);
    
    int wMessageSize = MultiByteToWideChar (CP_UTF8, 0, dialogMessage.c_str (), -1, nullptr, 0);
    std::wstring wMessage (wMessageSize, 0);
    MultiByteToWideChar (CP_UTF8, 0, dialogMessage.c_str (), -1, &wMessage[0], wMessageSize);
    
    MessageBoxW (nullptr, wMessage.c_str (), wTitle.c_str (), MB_OK | MB_ICONERROR);
#endif


template <typename... Args>
void Log::PrintAssertMessage(Log::Type type, std::string_view prefix, 
                            const char* file, int line,
                            std::format_string<Args...> message, Args &&...args)
{
    auto logger = (type == Type::Core) ? GetCoreLogger() : 
                 (type == Type::Editor) ? GetEditorLogger() : GetLauncherLogger();
    
    auto formatted = std::format(message, std::forward<Args>(args)...);
    
    // Enhanced error logging with source location
    logger->error("{}:{}: {}: {}", file, line, prefix, formatted);
    
#ifdef SEDX_ASSERT_MESSAGE_BOX
    // Extract filename from path for cleaner display
    std::string filename = file;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        filename = filename.substr(lastSlash + 1);
    
    // Format module type name
    const char* moduleTypeName = 
        (type == Type::Core) ? "Core" : 
        (type == Type::Editor) ? "Editor" : 
        (type == Type::Launcher) ? "Launcher" : "Unknown";
    
    // Create an informative dialog title
    std::string dialogTitle = std::format("Scenery Editor X | {} Assert", moduleTypeName);
    
    // Build a detailed, formatted message
    std::string dialogMessage = std::format(
        "ASSERTION FAILED\n\n"
        "Location: {}:{}\n"
        "Module: {}\n"
        "Condition: {}\n\n"
        "Details:\n{}\n\n"
        "See application log for additional information.",
        filename, line,
        moduleTypeName,
        prefix,
        formatted
    );
    
    // Show the message box with error icon
    MessageBoxA(nullptr, dialogMessage.c_str(), dialogTitle.c_str(), MB_OK | MB_ICONERROR);
#endif
}

//You can customize the dialog buttons for different assertion types:
// For critical errors
UINT dialogType = MB_OK | MB_ICONERROR;

// For warnings that can be ignored
UINT dialogType = MB_OKCANCEL | MB_ICONWARNING;

// Handle the return value
int result = MessageBoxA (nullptr, dialogMessage.c_str (), dialogTitle.c_str (), dialogType);
if (result == IDCANCEL)
{
    // User chose to ignore this warning
    logger->warn ("User ignored assertion warning");
}

// implementation with file/line information and a more helpful message format:
template <typename... Args>
void Log::PrintAssertMessage (Log::Type type, std::string_view prefix,
                            const char *file, int line,
                            std::format_string<Args...> message, Args &&...args)
{
    auto logger = (type == Type::Core) ? GetCoreLogger () :
        (type == Type::Editor) ? GetEditorLogger () : GetLauncherLogger ();

    auto formatted = std::format (message, std::forward<Args> (args)...);

    // Enhanced error logging with source location
    logger->error ("{}:{}: {}: {}", file, line, prefix, formatted);

#ifdef SEDX_ASSERT_MESSAGE_BOX
    // Extract filename from path for cleaner display
    std::string filename = file;
    size_t lastSlash = filename.find_last_of ("/\\");
    if (lastSlash != std::string::npos)
        filename = filename.substr (lastSlash + 1);

    // Format module type name
    const char *moduleTypeName =
        (type == Type::Core) ? "Core" :
        (type == Type::Editor) ? "Editor" :
        (type == Type::Launcher) ? "Launcher" : "Unknown";

    // Create an informative dialog title
    std::string dialogTitle = std::format ("Scenery Editor X | {} Assert", moduleTypeName);

    // Build a detailed, formatted message
    std::string dialogMessage = std::format (
        "ASSERTION FAILED\n\n"
        "Location: {}:{}\n"
        "Module: {}\n"
        "Condition: {}\n\n"
        "Details:\n{}\n\n"
        "See application log for additional information.",
        filename, line,
        moduleTypeName,
        prefix,
        formatted
    );

    // Show the message box with error icon
    MessageBoxA (nullptr, dialogMessage.c_str (), dialogTitle.c_str (), MB_OK | MB_ICONERROR);
#endif
}

//assertion macros to pass file and line information:
#define SEDX_CORE_ASSERT(condition, ...) \
    if(!(condition)) \
    { ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, \
      #condition, __FILE__, __LINE__, __VA_ARGS__); SEDX_DEBUGBREAK(); }