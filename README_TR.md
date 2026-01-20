# ApiBridge (DayZ Server Mod + HTTP API Extension)

Bu proje **@GameLabs benzeri** bir yapı sunar:
- **DayZ server-side mod (Enforce Script)**: oyuncu/sunucu durumunu `$profile:ApiBridge/` altına JSON olarak yazar ve komutları uygular.
- **Native Extension (DLL)**: DayZ tarafından `CallExtension()` ile başlatılır, **HTTP port** dinleyerek bu JSON'ları API olarak servis eder ve gelen POST komutlarını `commands.json` kuyruğuna yazar.

> Mod extension olmadan da çalışır: `apibridge.cfg`, `state.json` oluşturur ve `commands.json` dosyasını işleyebilir. HTTP için extension gereklidir.

## Üretilen dosyalar
Server açıldıktan sonra:
```
$profile:ApiBridge/apibridge.cfg
$profile:ApiBridge/state.json
$profile:ApiBridge/commands.json
$profile:ApiBridge/responses.json
```

## HTTP API
Header:
- `x-api-key: <apibridge.cfg içindeki ApiKey>`

GET:
- `GET /v1/health`
- `GET /v1/status`
- `GET /v1/state`
- `GET /v1/responses`

POST:
- `POST /v1/players/{uid}/teleport` body: `{ "x": 7500, "y": 0, "z": 7500 }`
- `POST /v1/players/{uid}/inventory/add` body: `{ "type": "BandageDressing", "quantity": 2 }`
- `POST /v1/players/{uid}/inventory/remove` body: `{ "type": "BandageDressing", "count": 1 }`
- `POST /v1/players/{uid}/inventory/setQuantity` body: `{ "type": "WaterBottle", "quantity": 100 }`
- `POST /v1/players/{uid}/stats` body: `{ "health": 80, "water": 2000 }`

## Kurulum
### 1) Modu PBO yap
Kaynak klasör:
```
@ApiBridge/ApiBridge/
```
DayZ Tools → Addon Builder ile **@ApiBridge/ApiBridge** klasöründen `ApiBridge.pbo` üret:
```
@ApiBridge/Addons/ApiBridge.pbo
```

### 2) Server'a kopyala
Server root:
```
DayZServer_x64.exe
@ApiBridge/
  Addons/
    ApiBridge.pbo
```

### 3) Server parametreleri
Örnek:
```
-servermod=@ApiBridge
-profiles=D:\DayZProfiles
```

> ÖNEMLİ: Duplicate class hatası yaşamamak için prod'da `-filePatching` kullanma ve server'da PBO dışındaki `.c` script kopyalarını bırakma.

## Extension (Windows)
### Build
Visual Studio 2022 + C++ workload + CMake kurulu olmalı.

Developer Command Prompt:
```
cd Extension\ApiBridgeExtension
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Çıktı:
- `build\Release\ApiBridge_x64.dll`

### Kurulum
DLL'i `DayZServer_x64.exe` ile **aynı klasöre** koy.

## Test (curl)
```
curl -H "x-api-key: <KEY>" http://127.0.0.1:8192/v1/status
curl -H "x-api-key: <KEY>" http://127.0.0.1:8192/v1/state
```
