# ApiBridgeExtension - Build (TR)

Bu klasor, DayZ server tarafinda calisan **native extension** (DLL/SO) kaynagidir.
DayZ Enforce script bir "mod" ile bu extension'i `CallExtension("ApiBridge", ...)` ile baslatir.
Extension ise HTTP API'yi acip `profiles/ApiBridge/...` altindaki JSON dosyalarini serve eder.

## Windows (MSVC + CMake)
1. Visual Studio 2022 (Desktop development with C++) ve CMake kurulu olsun.
2. `x64 Native Tools Command Prompt` ac:

```bat
cd Extension\ApiBridgeExtension
mkdir build & cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Cikti:
- `build/Release/ApiBridge_x64.dll`

Bunu DayZ server exe ile ayni klasore koy (ornek: `DayZServer_x64.exe` yanina).

## Linux (GCC/Clang + CMake)
```bash
cd Extension/ApiBridgeExtension
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Cikti genelde:
- `build/libApiBridge.so` veya `build/ApiBridge.so` (sistemine gore)

DayZ Linux server extension isimlendirmesi dagitime gore degisebiliyor; gerekirse dosyayi **ApiBridge.so** olarak yeniden adlandir.

## Port / Security
- Default bind IP: `127.0.0.1`
- Default port: `8192`
- API key: `profiles/ApiBridge/apibridge.cfg` icinde otomatik uretilir.

Uretimde (public) acacaksan:
- BindIp=127.0.0.1 birak (onerilen) ve Node projen ayni sunucudan baglansin
- ya da firewall / reverse proxy ile dis dunya erisimini kilitle
