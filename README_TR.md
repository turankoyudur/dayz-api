# ApiBridge (File Bridge) - Kurulum

Bu paket, DayZ sunucusunda **harici bir servise (Node.js dahil)** veri aktarabilmek icin **$profile** altina JSON dosyalari yazan, minimum ve uyumlu bir DayZ server modudur.

## Ne yapar?
- Ilk calismada `$profile:\\ApiBridge\\apibridge.cfg` olusturur (JSON formatinda, uzantisi .cfg).
- Periyodik olarak `$profile:\\ApiBridge\\state.json` dosyasina sunucu + oyuncu snapshot yazar.

## 1) PBO build (DayZ Tools)
1. DayZ Tools -> **Addon Builder** ac.
2. `ApiBridge/` klasorunu (workdrive) Source olarak sec.
3. Output olarak `@ApiBridge/Addons/ApiBridge.pbo` hedefini ver.
4. Build et.

## 2) Sunucuya kurulum
1. `@ApiBridge` klasorunu sunucuya kopyala.
2. `@ApiBridge/Addons/ApiBridge.pbo` var mi kontrol et.
3. Server start parametresine ekle:
   - Sadece server tarafi icin onerilen: `-serverMod=@ApiBridge`
   - Profil yolu icin: `-profiles=profiles`

## 3) Dogrulama
- Server acildiktan sonra su dosyalar olusmali:
  - `profiles/ApiBridge/apibridge.cfg`
  - `profiles/ApiBridge/state.json`

Not: Profil klasoru, serveri nasil calistirdigina gore farkli yerde olabilir. `-profiles=...` ile netlestirebilirsin.
