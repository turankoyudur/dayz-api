# ApiBridge <-> Node.js Entegrasyon Rehberi (File Bridge)

Bu mod DayZ sunucusunun `$profile:\ApiBridge\` klasoru altinda JSON dosyalari yazar/okur.
Node.js projen HTTP API'yi kendisi servis eder (Express/Nest/Fastify vb.) ve veriyi bu dosyalardan alir.

## Dosyalar
DayZ modunun urettigi/okudugu dosyalar:

- `apibridge.cfg` : Mod konfigurasyonu (JSON, uzanti .cfg)
- `state.json` : Sunucu + oyuncu snapshot (konum, temel statlar, inventory)
- `commands.json` : Node.js tarafinin yazdigi komutlar (mod okur ve siler)
- `command_results.json` : Modun yazdigi komut sonuclari

Baglanti/iletisim kontrolu icin ek dosyalar:
- `node_heartbeat.json` : Node.js'in yazdigi heartbeat (mod okur)
- `bridge_heartbeat.json` : Modun yazdigi heartbeat (Node.js okur)

Klasor yolu:
- Server'inda `-profiles=D:\DayZProfiles` verdiysen:
  - `D:\DayZProfiles\ApiBridge\state.json`

## Guvenlik (ApiKey)
`apibridge.cfg` icindeki `ApiKey` degeri, komutlarin kabul edilmesi icin zorunludur.
Node tarafinda her komuta ayni `apiKey` alanini eklemelisin.

Heartbeat dosyalarinda da `apiKey` kullanilir.

## Baglanti kontrolu (Node <-> Mod)
Hedef: Node uygulamasi **modun aktif oldugunu** ve modun da **Node'u gordugunu** anlayabilsin.

Bu sistem iki yonlu calisir:
1) Node.js -> `node_heartbeat.json` yazar
2) Mod -> `bridge_heartbeat.json` yazar (nonce echo ile)

### node_heartbeat.json (Node.js yazar)
Yol: `$profile:\ApiBridge\node_heartbeat.json`

Ornek:
```json
{
  "apiKey": "CFG_ICINDEKI_KEY",
  "nodeId": "api-server-1",
  "nonce": "7f3e1c...",
  "sentAt": "1700000000000"
}
```

### bridge_heartbeat.json (Mod yazar)
Yol: `$profile:\ApiBridge\bridge_heartbeat.json`

Ornek:
```json
{
  "modVersion": "filebridge-v2.1",
  "serverTimeMs": 123456,
  "lastNodeSeenServerTimeMs": 123450,
  "nodeId": "api-server-1",
  "nonceEcho": "7f3e1c..."
}
```

### Link OK kriteri (onerilen)
Node tarafinda link'i "OK" say:
- `bridge_heartbeat.json` dosyasi son X saniye icinde guncellenmis olmali (X = 3 * SnapshotIntervalSec)
- `nonceEcho`, son gonderdigin `nonce` ile ayni olmali

Ek kontrol (iki yon): `state.json` icindeki `bridge.lastNodeSeenServerTimeMs` degerinin artiyor olmasi.

### Atomik heartbeat yazma
Node tarafinda yarim yazim riskini azaltmak icin ayni pattern:
1) `node_heartbeat.tmp.json` yaz
2) sonra `node_heartbeat.json` uzerine rename

### Interval ayari
Mod `node_heartbeat.json` dosyasini `CommandPollIntervalSec` periyodunda okur.
Node'un 2 saniyede bir heartbeat atmasi icin `CommandPollIntervalSec` degerini 1.0-2.0 araliginda tutman onerilir.

## state.json okuma
Node projen periyodik olarak `state.json` okur.
Oneri:
- Dosyayi okurken JSON parse hatasi alirsan 50-100ms sonra tekrar dene (DayZ tam yazarken denk gelmis olabilirsin).

Ornek (Node.js - pseudo):
```js
import fs from 'fs/promises';

const STATE = 'D:/DayZProfiles/ApiBridge/state.json';

async function readState() {
  const txt = await fs.readFile(STATE, 'utf8');
  return JSON.parse(txt);
}
```

## commands.json yazma (komut gonderme)
Node tarafinda komut gondermek icin `commands.json` dosyasini yazarsin.
Mod dosyayi okur, komutlari isler ve `commands.json` dosyasini siler.

### Atomik yazma (cok onemli)
Yarim yazilmis JSON riskini azaltmak icin:
1) once `commands.tmp.json` yaz
2) sonra `commands.json` uzerine **rename** yap

Windows'ta ayni dizin icinde rename atomik davranir.

Ornek komut formati:
```json
{
  "commands": [
    {
      "id": "cmd-001",
      "apiKey": "CFG_ICINDEKI_KEY",
      "type": "teleport",
      "playerId": "STEAM64",
      "x": 7500,
      "y": 0,
      "z": 7500
    }
  ]
}
```

## Desteklenen komut tipleri
- `teleport` : x,y,z
- `inv_add` : itemType, quantity
- `inv_remove` : itemType, count
- `inv_setqty` : itemType, quantity
- `set_health` : value
- `set_blood` : value
- `set_shock` : value

## command_results.json okuma
Komutun sonucunu almak icin Node tarafinda `command_results.json` okursun.
Dosya her isleme turunda yeniden yazilir.

Ornek sonuc:
```json
{
  "serverTimeMs": 123456,
  "results": [
    {"id":"cmd-001","ok":true,"message":"ok"}
  ]
}
```

Oneri:
- Node API tarafinda `POST /cmd` -> komutu yaz -> `command_results.json` icinden ilgili `id` gorunene kadar kisa polling yap (ornegin 1-2sn).
