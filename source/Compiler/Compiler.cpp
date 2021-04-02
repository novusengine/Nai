#include <pch/Build.h>
#include "Compiler.h"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"

#include <Utils/FileReader.h>
#include <Utils/DebugHandler.h>

bool Compiler::Start()
{
    if (_stageInfo.stage != Stage::STOPPED)
        return false;

    _stageInfo.stage = Stage::IDLE;
    _stageInfo.paths.reserve(32);

    std::thread runThread = std::thread(&Compiler::Run, this);
    runThread.detach();

    return true;
}

bool Compiler::Stop()
{
    Message stopMessage;
    stopMessage.type = Message::Type::STOP;

    _messages.enqueue(stopMessage);
    return true;
}

bool Compiler::Process()
{
    if (_stageInfo.stage != Stage::IDLE)
        return false;

    if (_stageInfo.paths.size() == 0)
        return false;

    Message processMessage;
    processMessage.type = Message::Type::PROCESS;

    _messages.enqueue(processMessage);
    return true;
}

bool Compiler::AddIncludePath(fs::path path)
{
    for (fs::path& p : _includePaths)
    {
        if (path == p)
            return false;
    }

    _includePaths.push_back(path);
    return true;
}

bool Compiler::AddPaths(std::vector<fs::path> paths)
{
    if (_stageInfo.stage != Stage::IDLE)
        return false;

    size_t numPaths = _stageInfo.paths.size();
    size_t numNewPaths = paths.size();

    _stageInfo.paths.reserve(numPaths + numNewPaths);

    for (fs::path& path : paths)
    {
        path = fs::absolute(path);
        _stageInfo.paths.push_back(path);
    }
    return true;
}

bool Compiler::AddImport(fs::path path)
{
    if (_stageInfo.stage != Stage::LEXER_SYNTAX_IMPORT)
        return false;

    _stageInfo.paths.push_back(path);
    return true;
}

bool Compiler::HasModule(uint32_t hash)
{
    return _moduleHashToIndex.find(hash) != _moduleHashToIndex.end();
}

bool Compiler::HasModule(uint32_t hash, uint32_t& index)
{
    auto moduleItr = _moduleHashToIndex.find(hash);
    if (moduleItr == _moduleHashToIndex.end())
        return false;

    index = moduleItr->second;
    return true;
}

bool Compiler::GetModuleByIndex(uint32_t index, ModuleInfo& moduleInfo)
{
    if (index >= _modules.size())
        return false;

    moduleInfo = _modules[index];
    return true;
}

void Compiler::Run()
{
    bool isRunning = true;

    while (isRunning)
    {
        Message message;
        while (_messages.try_dequeue(message))
        {
            if (message.type == Message::Type::STOP)
            {
                isRunning = false;
                break;
            }
            else if (message.type == Message::Type::PROCESS)
            {
                _stageInfo.stage = Stage::LEXER_SYNTAX_IMPORT;
            }
        }

        if (isRunning == false)
            break;

        Stage currentStage = _stageInfo.stage;
        if (currentStage == Stage::IDLE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        else if (currentStage == Stage::LEXER_SYNTAX_IMPORT)
        {
            for (auto itr = _stageInfo.paths.begin(); itr != _stageInfo.paths.end();)
            {
                std::string fileName = itr->filename().string();
                std::string path = itr->string();
                if (fs::exists(*itr))
                {
                    if (fs::is_directory(*itr))
                    {
                        for (auto& entry : fs::directory_iterator(*itr))
                        {
                            const fs::path& subPath = entry.path();
                            if (fs::is_regular_file(subPath) && subPath.extension() == ".nai")
                                _stageInfo.paths.push_back(subPath);
                        }
                    }
                    else if (fs::is_regular_file(*itr) && itr->extension() == ".nai")
                    {
                        uint32_t strHash = StringUtils::hash_djb2(path.c_str(), path.length());
                        if (HasModule(strHash) == false)
                        {
                            FileReader reader(itr->string(), itr->filename().string());
                            if (!reader.Fetch())
                            {
                                NC_LOG_ERROR("Compiler failed to read script (%s)", itr->string().c_str());
                                continue;
                            }

                            ModuleInfo& moduleInfo = _modules.emplace_back();
                            moduleInfo.Init(*itr, reader.GetBuffer(), reader.Length());

                            if (!Lexer::Process(moduleInfo) || !Parser::CheckSyntax(moduleInfo) || !Parser::ResolveImports(this, moduleInfo))
                                _modules.pop_back();
                            else
                                _moduleHashToIndex[strHash] = _modules.size() - 1;
                        }
                    }

                    itr = _stageInfo.paths.erase(itr);
                    continue;
                }

                NC_LOG_ERROR("Compiler failed to find path (%s)", path.c_str());
            }

            _stageInfo.stage = Stage::AST;
        }
        else if (currentStage == Stage::AST)
        {
            for (ModuleInfo& module : _modules)
            {
                if (!Parser::CreateAst(module))
                {
                    NC_LOG_ERROR("Module(%s) failed to create ast", module.name.string().c_str());
                    // Report failed 
                }
            }

            _stageInfo.stage = Stage::SEMANTICS;
        }
        else
        {
            // Unsupported Stage
            break;
        }

        std::this_thread::yield();
    }

    _stageInfo.stage = Stage::STOPPED;
}

bool Compiler::Compile(std::string& path)
{
    fs::path filePath = path;
    
    return true;
}
