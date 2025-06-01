#pragma once

#include <string>
#include <memory>
#include <vector>
#include <IntuiCAM/Common/Types.h>

namespace IntuiCAM {
namespace Plugin {

// Base interface for all plugins
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Plugin metadata
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getAuthor() const = 0;
    
    // Plugin lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
    
    // Plugin capabilities
    virtual std::vector<std::string> getSupportedFeatures() const = 0;
    virtual bool supportsFeature(const std::string& feature) const = 0;
};

// Plugin manager for loading and managing plugins
class PluginManager {
private:
    std::vector<std::unique_ptr<IPlugin>> plugins_;
    
public:
    PluginManager() = default;
    ~PluginManager();
    
    // Plugin loading
    bool loadPlugin(const std::string& pluginPath);
    bool loadPluginsFromDirectory(const std::string& directory);
    void unloadPlugin(const std::string& pluginName);
    void unloadAllPlugins();
    
    // Plugin access
    IPlugin* getPlugin(const std::string& name) const;
    std::vector<IPlugin*> getPlugins() const;
    std::vector<IPlugin*> getPluginsWithFeature(const std::string& feature) const;
    
    // Plugin information
    std::vector<std::string> getLoadedPluginNames() const;
    size_t getPluginCount() const { return plugins_.size(); }
    
    // Singleton access
    static PluginManager& getInstance();
};

// Macro for plugin registration
#define INTUICAM_REGISTER_PLUGIN(PluginClass) \
    extern "C" { \
        IntuiCAM::Plugin::IPlugin* createPlugin() { \
            return new PluginClass(); \
        } \
        void destroyPlugin(IntuiCAM::Plugin::IPlugin* plugin) { \
            delete plugin; \
        } \
        const char* getPluginName() { \
            static PluginClass instance; \
            return instance.getName().c_str(); \
        } \
    }

} // namespace Plugin
} // namespace IntuiCAM 