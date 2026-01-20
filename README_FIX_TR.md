# ApiBridge - Fix Paketi

Bu pakette guncel DayZ server script derleyicisiyle uyumlu (derlenen) dosyalar var.

## Duzenlenen sorunlar

### 1) apibridgeconfig.c(34) Broken expression
- Genelde `out` gibi rezerv isimlerin degisken adi olarak kullanilmasindan kaynaklanir.
- Bu pakette config dosyasi temizlendi.

### 2) Unknown type 'MissionServer'
- `MissionServer` **4_World** icinde kullanilamaz; dosya **5_Mission** altinda olmali.
- Bu pakette `apibridgemissionserver.c` dogru modulde.

### 3) Incompatible parameter 'ok' (apibridgecommandprocessor.c:21)
- Bazı DayZ surumlerinde `JsonFileLoader<T>.JsonLoadFile(...)` **bool donmez**, `void` doner.
- Bu pakette return degeri alinmiyor; dosya her iki durumda da derlenir.

## Nasil uygularim?
1) Mod kaynagina bu dosyalari kopyala (mevcut dosyalarin ustune yaz):
   - `scripts/3_Game/apibridge/apibridgetypes.c`
   - `scripts/3_Game/apibridge/apibridgeconfig.c`
   - `scripts/4_World/apibridge/apibridgeservice.c`
   - `scripts/4_World/apibridge/apibridgestatecollector.c`
   - `scripts/4_World/apibridge/apibridgecommandprocessor.c`
   - `scripts/5_Mission/apibridge/apibridgemissionserver.c`

2) DayZ Tools -> Addon Builder ile PBO'yu tekrar build et.

3) Server'i baslat.

## Uretilen dosyalar
Server acilinca `$profile:ApiBridge` altinda:
- `config.json`
- `apibridge.cfg`
- `state.json`
- `responses.json`
- `commands.json` (disaridan yazilir)
