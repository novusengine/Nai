#pragma once
#include <pch/Build.h>
#include <thread>
#include <shared_mutex>
#include <Compiler/ModuleInfo.h>
#include <Utils/ConcurrentQueue.h>

#include <filesystem>
#include <robin_hood.h>
namespace fs = std::filesystem;

class Compiler
{
public:
    enum class Stage
    {
        STOPPED,
        IDLE,
        LEXER_SYNTAX_IMPORT,
        AST,
        SEMANTICS,
        CODEGEN
    };

    bool Start();
    bool Stop();
    bool Process();

    const std::vector<fs::path>& GetIncludePaths() { return _includePaths; }
    bool AddIncludePath(fs::path path);

    bool AddPaths(std::vector<fs::path> paths);
    bool AddImport(fs::path path);

    Stage GetStage() { return _stageInfo.stage; }
    void SetStage(Stage stage) { _stageInfo.stage = stage; }

    bool HasModule(uint32_t hash);
    bool HasModule(uint32_t hash, uint32_t& index);
    bool GetModuleByIndex(uint32_t index, ModuleInfo& moduleInfo);

private:
    void Run();
    bool Compile(std::string& path);

private:
    std::vector<fs::path> _includePaths;
    std::vector<ModuleInfo> _modules;
    robin_hood::unordered_map<uint32_t, uint32_t> _moduleHashToIndex;

    struct StageState
    {
        Stage stage = Stage::STOPPED;

        std::vector<fs::path> paths;
        std::shared_mutex mutex;
    };

    StageState _stageInfo;

    struct Message
    {
        enum class Type
        {
            NONE,
            STOP,
            PROCESS
        };

        Type type;
    };
    moodycamel::ConcurrentQueue<Message> _messages;
};