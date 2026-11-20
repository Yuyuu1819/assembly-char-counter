//contador de caracteres - Ventana de Windows (Win32)
//sustituye a la version de consola.
//el conteo lo hace contador.asm, a traves de interfaz.cpp


#ifndef UNICODE
#define UNICODE
#endif


#include <windows.h>
#include <string>
#include "interfaz.hpp"

//identificadores de los controles
#define ID_EDIT      101
#define ID_ANALIZAR  102
#define ID_LIMPIAR   103

//colores tomados del diseno original
static const COLORREF COL_MARFIL = RGB(248, 246, 242);
static const COLORREF COL_TINTA = RGB(40, 37, 34);
static const COLORREF COL_GRIS = RGB(116, 109, 103);
static const COLORREF COL_VINO = RGB(119, 47, 68);
static const COLORREF COL_CAMPO = RGB(252, 251, 249);

//estado de la aplicacion
static int   g_dpi = 96;                 //escala de la pantalla
static HWND  g_edit = nullptr;           //cuadro de texto
static HFONT g_fTitulo = nullptr, g_fSub = nullptr;
static HFONT g_fEtiqueta = nullptr, g_fNumero = nullptr, g_fTexto = nullptr;
static HBRUSH g_brFondo = nullptr, g_brTarjeta = nullptr, g_brCampo = nullptr;
static HPEN  g_penTarjeta = nullptr;
static bool  g_hayDatos = false;         //false = todavia no se analiza nada
static ResultadoConteo g_r;              //resultados que devuelve el ensamblador
static std::wstring g_estado;            //mensaje bajo "RESULTADOS"

//escala una medida del diseno al tamano real de la pantalla.
static int S(int v) { return MulDiv(v, g_dpi, 96); }

//las 5 tarjetas de resultados: etiqueta y posicion
struct Tarjeta { const wchar_t* etiqueta; int x; int y; };
static const Tarjeta TARJETAS[5] = {
    { L"TOTAL",         44, 444 },
    { L"LETRAS",       304, 444 },
    { L"NÚMEROS",  564, 444 },
    { L"ESPACIOS",      44, 553 },
    { L"ESPECIALES",   304, 553 },
};
static const int TARJETA_ANCHO = 242;
static const int TARJETA_ALTO = 98;

static HFONT crearFuente(int puntos, bool negrita) {
    return CreateFontW(-MulDiv(puntos, g_dpi, 72), 0, 0, 0,
        negrita ? FW_SEMIBOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"Segoe UI");
}

//dibuja un texto dentro del rectangulo (x, y, ancho, alto) del diseno.
static void dibujarTexto(HDC hdc, const wchar_t* s, int x, int y, int ancho, int alto,
    HFONT fuente, COLORREF color) {
    RECT rc{ S(x), S(y), S(x + ancho), S(y + alto) };
    SelectObject(hdc, fuente);
    SetTextColor(hdc, color);
    DrawTextW(hdc, s, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

//dibuja una tarjeta blanca con esquinas redondeadas.
static void dibujarTarjeta(HDC hdc, int x, int y, int ancho, int alto, int radio) {
    SelectObject(hdc, g_brTarjeta);
    SelectObject(hdc, g_penTarjeta);
    RoundRect(hdc, S(x), S(y), S(x + ancho), S(y + alto), S(radio * 2), S(radio * 2));
}

//lee el texto del cuadro y lo convierte a bytes ANSI (1 byte por caracter),
//que es lo que espera la rutina en ensamblador.
static std::string leerTexto(HWND edit) {
    const int n = GetWindowTextLengthW(edit);
    if (n <= 0) return std::string();

    std::wstring w(static_cast<size_t>(n) + 1, L'\0');
    GetWindowTextW(edit, &w[0], n + 1);
    w.resize(static_cast<size_t>(n));

    const int bytes = WideCharToMultiByte(CP_ACP, 0, w.c_str(), n, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return std::string();

    std::string s(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), n, &s[0], bytes, nullptr, nullptr);
    return s;
}

//Acciones de los botones
static void alAnalizar(HWND hwnd) {
    const std::string texto = leerTexto(g_edit);

    if (solicitarConteoBackend(texto, g_r)) {
        g_hayDatos = true;
        g_estado = L"Análisis completado.";
    }
    else {
        g_hayDatos = false;
        g_estado = L"El texto supera los 100 caracteres permitidos.";
    }
    InvalidateRect(hwnd, nullptr, TRUE);   //vuelve a dibujar la ventana
}

static void alLimpiar(HWND hwnd) {
    SetWindowTextW(g_edit, L"");
    g_hayDatos = false;
    g_estado.clear();
    SetFocus(g_edit);
    InvalidateRect(hwnd, nullptr, TRUE);
}

//Dibujado de la ventana

static void pintar(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    SetBkMode(hdc, TRANSPARENT);

    //encabezado
    dibujarTexto(hdc, L"PROYECTO ENSAMBLADOR", 44, 33, 470, 24, g_fEtiqueta, COL_VINO);
    dibujarTexto(hdc, L"Contador de caracteres", 44, 62, 740, 48, g_fTitulo, COL_TINTA);
    dibujarTexto(hdc, L"Escribe un texto y descubre cómo se compone.",
        46, 116, 700, 24, g_fSub, COL_GRIS);

    //panel del cuadro de texto
    dibujarTarjeta(hdc, 44, 164, 762, 198, 16);
    dibujarTexto(hdc, L"TEXTO A ANALIZAR", 64, 179, 500, 18, g_fEtiqueta, COL_GRIS);

    //seccion de resultados
    dibujarTexto(hdc, L"RESULTADOS", 46, 379, 400, 22, g_fEtiqueta, COL_VINO);
    dibujarTexto(hdc, g_estado.c_str(), 46, 406, 740, 20, g_fTexto, COL_GRIS);

    //as 5 tarjetas
    const int valores[5] = { g_r.total, g_r.letras, g_r.numeros, g_r.espacios, g_r.signos };
    for (int i = 0; i < 5; ++i) {
        const Tarjeta& t = TARJETAS[i];
        dibujarTarjeta(hdc, t.x, t.y, TARJETA_ANCHO, TARJETA_ALTO, 13);
        dibujarTexto(hdc, t.etiqueta, t.x + 16, t.y + 12, 205, 18, g_fEtiqueta, COL_GRIS);

        wchar_t numero[16];
        if (g_hayDatos) wsprintfW(numero, L"%d", valores[i]);
        else            wcscpy_s(numero, L"—");   //raya larga cuando no hay datos

        dibujarTexto(hdc, numero, t.x + 16, t.y + 36, 210, 48, g_fNumero, COL_TINTA);
    }

    EndPaint(hwnd, &ps);
}
//controles
static void crearControles(HWND hwnd, HINSTANCE hInst) {
    g_fTitulo = crearFuente(33, true);
    g_fSub = crearFuente(14, false);
    g_fEtiqueta = crearFuente(11, true);
    g_fNumero = crearFuente(32, true);
    g_fTexto = crearFuente(12, false);

    g_brTarjeta = CreateSolidBrush(RGB(255, 255, 255));
    g_brCampo = CreateSolidBrush(COL_CAMPO);
    g_penTarjeta = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

    HFONT fCampo = crearFuente(15, false);

    g_edit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
        S(62), S(206), S(726), S(98),
        hwnd, (HMENU)ID_EDIT, hInst, nullptr);
    SendMessageW(g_edit, WM_SETFONT, (WPARAM)fCampo, TRUE);
    SendMessageW(g_edit, EM_SETLIMITTEXT, 100, 0);   // maximo 100 caracteres

    HWND bAnalizar = CreateWindowExW(0, L"BUTTON", L"Analizar texto",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        S(62), S(314), S(158), S(33),
        hwnd, (HMENU)ID_ANALIZAR, hInst, nullptr);
    SendMessageW(bAnalizar, WM_SETFONT, (WPARAM)g_fTexto, TRUE);

    HWND bLimpiar = CreateWindowExW(0, L"BUTTON", L"Limpiar",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        S(230), S(314), S(100), S(33),
        hwnd, (HMENU)ID_LIMPIAR, hInst, nullptr);
    SendMessageW(bLimpiar, WM_SETFONT, (WPARAM)g_fTexto, TRUE);
}

//procedimiento de ventana: aqui llegan todos los mensajes

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        crearControles(hwnd, ((LPCREATESTRUCTW)lParam)->hInstance);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_ANALIZAR) alAnalizar(hwnd);
        else if (LOWORD(wParam) == ID_LIMPIAR) alLimpiar(hwnd);
        return 0;

    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wParam, COL_CAMPO);
        SetTextColor((HDC)wParam, COL_TINTA);
        return (LRESULT)g_brCampo;

    case WM_PAINT:
        pintar(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//punto de entrada (sustituye a main)

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    SetProcessDPIAware();

    //calcula la escala: usa la de la pantalla, pero sin pasarse del escritorio.
    HDC screen = GetDC(nullptr);
    int dpiPantalla = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(nullptr, screen);

    RECT wa{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    const int dpiAlto = MulDiv(96, (wa.bottom - wa.top) - 70, 690);
    const int dpiAncho = MulDiv(96, (wa.right - wa.left) - 30, 850);

    g_dpi = dpiPantalla;
    if (dpiAlto < g_dpi)  g_dpi = dpiAlto;
    if (dpiAncho < g_dpi) g_dpi = dpiAncho;
    if (g_dpi < 72)       g_dpi = 72;

    g_brFondo = CreateSolidBrush(COL_MARFIL);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = g_brFondo;
    wc.lpszClassName = L"ContadorCaracteresASM";
    RegisterClassW(&wc);

    //area util de 850 x 690, igual que el diseno original.
    const DWORD estilo = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc{ 0, 0, S(850), S(690) };
    AdjustWindowRect(&rc, estilo, FALSE);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Contador de caracteres",
        estilo, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {   //permite moverse con Tab
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return 0;
}