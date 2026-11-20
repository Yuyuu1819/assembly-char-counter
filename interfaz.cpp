#include "interfaz.hpp"

extern "C" void contar(const char* texto, int* resultados);

bool solicitarConteoBackend(const std::string& texto, ResultadoConteo& resultado) {
    if (texto.length() > 100) return false;
    int r[5];
    contar(texto.c_str(), r);
    resultado.letras = r[0];
    resultado.numeros = r[1];
    resultado.espacios = r[2];
    resultado.signos = r[3];
    resultado.otros = 0;
    resultado.total = r[4];
    return true;
}