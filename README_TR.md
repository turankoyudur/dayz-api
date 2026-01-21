# ApiBridge (File-Based) – DayZ Sunucu Yönetim Köprüsü

Bu mod, DayZ sunucu içinden **$profile:ApiBridge/** dizinine durum dosyaları yazar ve aynı dizinden **commands.json** okuyarak komut çalıştırır.

- HTTP/RCon yok.
- Windows ve Linux dedicated server’da çalışacak şekilde tasarlanmıştır.

## Üretilen dosyalar (server -profiles altında)

- `apibridge.cfg` – ayarlar + ApiKey
- `state.json` – sunucu + oyuncu snapshot (periyodik)
- `link.json` – Node heartbeat/bağlantı durumu
- `results.json` – son komutların sonuçları
- `commands.json` – dış araç (Node) tarafından yazılır, mod tüketir

## Kurulum

### 1) PBO build
DayZ Tools > **Addon Builder** ile:

- Source directory: `ApiBridge/Scripts`
- Output: `@ApiBridge/addons/apibridge.pbo`

> Linux için `@ApiBridge/addons` ve PBO adı **küçük harf** olmalı.

### 2) Sunucuya kopyala
Sunucuya aşağıdaki yapıyı koy:

```
@ApiBridge/
  addons/
    apibridge.pbo
  mod.cpp
  meta.cpp
```

### 3) Server parametreleri
Örnek:

- Windows:
  ```
  DayZServer_x64.exe -config=serverDZ.cfg -profiles=profiles -mod=@ApiBridge
  ```

- Linux:
  ```
  ./DayZServer -config=serverDZ.cfg -profiles=profiles -mod=@apibridge
  ```

### 4) ApiKey ayarla
Sunucuyu 1 kez çalıştır.

`profiles/ApiBridge/apibridge.cfg` oluşacak.

`ApiKey` değerini değiştir (Node da aynı key’i kullanmalı).

## Komutlar
Desteklenen komutlar (v0.1.0):

- `ping`
- `server.status`
- `server.message`
- `player.kick`
- `ban.add` / `ban.remove` (soft-ban)
- `server.restart`
- `server.shutdown`

Detaylı şema için: **INTEGRATION_NODE_TR.md**
