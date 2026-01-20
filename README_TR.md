# DayZ ApiBridge (Mod + Native Extension)

Bu paket, **DayZ server** icin HTTP API saglayan bir **mod + native extension** cozumudur.
Node.js projen bu API'yi normal REST cagrilariyla kullanir (API Node tabanli degildir).

## Mimari
- Enforce Script (server-only mod):
  - Ilk calismada `$profile:ApiBridge/apibridge.cfg` dosyasini otomatik olusturur (ApiKey dahil).
  - Belirli araliklarla `$profile:ApiBridge/state.json` dosyasini yazar.
  - HTTP uzerinden gelen edit istekleri icin `$profile:ApiBridge/commands.json` komutlarini okur ve uygular.
  - Komut sonucunu `$profile:ApiBridge/responses.json` icine yazar.

- Native Extension (DLL/SO):
  - HTTP server acar.
  - `state.json` / `responses.json` dosyalarini REST API olarak servis eder.
  - Komut endpointlerinde `commands.json` icine komut objesi ekler.

## HTTP API (v1)
Header: `x-api-key: <apibridge.cfg icindeki ApiKey>`

- `GET /v1/health` -> {ok:true}
- `GET /v1/status` -> extension durumu + dosya var mi
- `GET /v1/state` -> state.json (oyuncu konumu, inventory, statlar, oyuncu sayisi)
- `GET /v1/responses` -> responses.json (komut ciktilari)

Komutlar (hepsi JSON body ister):
- `POST /v1/players/{uid}/teleport` body: `{ "x":123, "y":0, "z":456 }`
- `POST /v1/players/{uid}/inventory/add` body: `{ "type":"AmmoBox_545x39_20Rnd", "quantity":1 }`
- `POST /v1/players/{uid}/inventory/remove` body: `{ "type":"BandageDressing", "count":2 }`
- `POST /v1/players/{uid}/inventory/setQuantity` body: `{ "type":"WaterBottle", "quantity":100 }`
- `POST /v1/players/{uid}/stats` body: `{ "health":80, "water":2000 }`  (sadece gonderdigin alanlar set edilir)

Komut donusu: `{ ok:true, accepted:true, id:"..." }`
Sonucu almak icin `GET /v1/responses` okuyup `id` ile filtrele.

## Kurulum Ozeti
1) Extension'i build et (bkz: `Extension/ApiBridgeExtension/README_BUILD_TR.md`).
2) DLL/SO dosyasini DayZ server exe yanina koy.
3) Modu PBO yap ve `@ApiBridge` olarak sunucuya ekle.
4) Server parametrelerine ekle:
   - `-servermod=@ApiBridge`
   - `-profiles=profiles`
5) Server acilinca `profiles/ApiBridge/apibridge.cfg` olusur. ApiKey'i buradan al.
6) API default: `http://127.0.0.1:8192/v1/state`

## Guvenlik
Default BindIp `127.0.0.1` oldugu icin sadece ayni makineden erisilir.
Dis dunyaya acacaksan firewall / reverse proxy ile mutlaka kisitla.
