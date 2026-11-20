#pragma once
#include <string>

// Datos que la interfaz espera recibir del backend en ensamblador.
struct ResultadoConteo {
    int total = 0;
    int letras = 0;
    int numeros = 0;
    int espacios = 0;
    int signos = 0;
    int otros = 0;
};

// Punto de integracion con el backend. La interfaz no realiza el conteo.
// Devuelve true solo cuando el backend proporciona resultados validos.
bool solicitarConteoBackend(const std::string& texto, ResultadoConteo& resultado);
