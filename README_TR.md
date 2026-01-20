# ApiBridge (File Bridge) - Kurulum ve Kullanim

Bu paket, DayZ sunucusunda harici bir servise (Node.js dahil) veri aktarabilmek icin `$profile` altina JSON dosyalari yazan ve disaridan komut alabilen, minimum ve uyumlu bir DayZ server modudur.

## Ne yapar?
- Ilk calismada `$profile:\ApiBridge\apibridge.cfg` olusturur (JSON formatinda, uzantisi .cfg).
- Periyodik olarak `$profile:\ApiBridge\state.json` dosyasina sunucu + oyuncu snapshot yazar:
  - oyuncu konumu
  - health/blood/shock
  - envanter listesi (type, quantity, location)
- Disaridan `$profile:\ApiBridge\commands.json` ile komut alir ve uygular.
- Komut sonuclarini `$profile:\ApiBridge\command_results.json` dosyasina yazar.

## Dosyalar
- `apibridge.cfg` : Config (ApiKey, interval'lar)
- `state.json` : Snapshot
- `commands.json` : Node.js yazacak, mod okuyup silecek
- `command_results.json` : Mod yazacak

## 1) PBO build (DayZ Tools)
1. DayZ Tools -> Addon Builder ac.
2. `ApiBridge/` klasorunu (workdrive) Source olarak sec.
3. Output olarak `@ApiBridge/Addons/ApiBridge.pbo` hedefini ver.
4. Build et.

## 2) Sunucuya kurulum
1. `@ApiBridge` klasorunu sunucuya kopyala.
2. `@ApiBridge/Addons/ApiBridge.pbo` var mi kontrol et.
3. Server start parametrelerine ekle:
   - Sadece server tarafi icin onerilen: `-serverMod=@ApiBridge`
   - Profil yolu icin onerilen: `-profiles=profiles`

> Not: Test ederken `-filePatching` kapali tutman daha guvenli (cift derleme hatalarini engeller).

## 3) Dogrulama
Server acildiktan sonra su dosyalar olusmali:
- `profiles/ApiBridge/apibridge.cfg`
- `profiles/ApiBridge/state.json`

## 4) Node.js entegrasyonu
`INTEGRATION_NODE_TR.md` dosyasini oku.
