#ifndef ERROR_LOGGER_H
#define ERROR_LOGGER_H

#include <iostream>
#include <mutex>
#include <string>

/*
 * Es thread-safe: garantiza que los mensajes no se pisen entre hilos.
 */
class ErrorLogger {
private:
    inline static std::mutex mtx;  // inline inicializa la variable directamente

public:
    ErrorLogger() = delete;  // La voy a usar solo de manera estatica

    static void log(const std::string& msg) {
        // Bloqueamos el mutex solo durante la impresión
        std::lock_guard<std::mutex> lock(mtx);
        std::cerr << "[ERROR] " << msg << "\n";
    }
};

#endif
