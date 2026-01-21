# ApiBridge ↔ Node.js Entegrasyon (Dosya Köprüsü)

Bu entegrasyon **tamamen dosya üzerinden** çalışır:

- Node tarafı `commands.json` yazar.
- DayZ mod, `commands.json` okur, işler, dosyayı siler (ack), `results.json` yazar.
- Mod ayrıca `state.json` ve `link.json` dosyalarını periyodik yazar.

> Not: Node uygulamanız DayZ sunucuyla aynı makinede çalışmıyorsa, bu dizine erişim için SMB/NFS paylaşımı veya sunucuda çalışan küçük bir “agent” gerekir.

## Dosya yolları
Hepsi **server -profiles** altında oluşur:

- `profiles/ApiBridge/apibridge.cfg`
- `profiles/ApiBridge/commands.json`
- `profiles/ApiBridge/results.json`
- `profiles/ApiBridge/state.json`
- `profiles/ApiBridge/node_heartbeat.json`
- `profiles/ApiBridge/link.json`

## 1) Node → DayZ Komut Gönderme

### commands.json şeması

```json
{
  "apiVersion": 1,
  "apiKey": "CHANGE_ME",
  "commands": [
    {
      "id": "uuid-1",
      "type": "server.message",
      "message": "Merhaba!"
    }
  ]
}
```

### Atomic write (çok önemli)
DayZ mod dosyayı okurken yarım JSON görürse hata verebilir.
Bu yüzden Node tarafında yazma işlemini **temp + rename** ile yap:

1. `commands.json.tmp` yaz
2. `fs.rename(tmp, commands.json)`

Mod, işledikten sonra `commands.json` dosyasını siler.
Node yeni komut yazmadan önce `commands.json` yok mu diye kontrol et.

## 2) DayZ → Node Sonuç Okuma

### results.json şeması

```json
{
  "apiVersion": 1,
  "serverTimeMs": 123456,
  "results": [
    {
      "id": "uuid-1",
      "ok": true,
      "error": "",
      "data": "sent",
      "serverState": null
    }
  ]
}
```

- `serverState` sadece `server.status` komutunda dolu döner.

## 3) Durum Takibi

### state.json
`state.json` periyodik snapshot’tır (player list + temel stats).

### link.json (bağlantı kontrolü)
Node uygulaması “sunucu beni görüyor mu?” kontrolü için `node_heartbeat.json` yazar.
Mod bunu okuyup `link.json` içine yazar.

#### node_heartbeat.json
```json
{
  "t": 123456,
  "nonce": "random-string"
}
```

#### link.json
```json
{
  "apiVersion": 1,
  "serverTimeMs": 123789,
  "lastNodeHeartbeatMs": 123456,
  "lastNodeNonce": "random-string",
  "status": "linked"
}
```

`status`:
- `waiting_node`: henüz node_heartbeat.json yok
- `linked`: mod node heartbeat’i okuyor

## 4) Örnek Komutlar

### Sunucu durumu
```json
{ "id": "1", "type": "server.status" }
```

### Mesaj yayınla
```json
{ "id": "2", "type": "server.message", "message": "Restart 5 dk sonra" }
```

### Oyuncu kick
```json
{ "id": "3", "type": "player.kick", "playerPlainId": "7656119..." }
```

### Soft-ban
```json
{ "id": "4", "type": "ban.add", "playerPlainId": "7656119..." }
```

### Restart / Shutdown
```json
{ "id": "5", "type": "server.restart" }
{ "id": "6", "type": "server.shutdown" }
```

## 5) Node tarafı örnek pseudo-code

```js
import fs from "fs";
import path from "path";

const base = "C:/DayZServer/profiles/ApiBridge"; // örnek

function writeAtomic(file, obj) {
  const tmp = file + ".tmp";
  fs.writeFileSync(tmp, JSON.stringify(obj, null, 2));
  fs.renameSync(tmp, file);
}

function sendCommands(apiKey, commands) {
  const commandsPath = path.join(base, "commands.json");
  if (fs.existsSync(commandsPath)) return false; // ack bekle

  const env = { apiVersion: 1, apiKey, commands };
  writeAtomic(commandsPath, env);
  return true;
}

function readJson(file) {
  if (!fs.existsSync(file)) return null;
  return JSON.parse(fs.readFileSync(file, "utf8"));
}

// bağlantı kontrolü
function heartbeat() {
  writeAtomic(path.join(base, "node_heartbeat.json"), {
    t: Date.now() % 2147483647,
    nonce: Math.random().toString(16).slice(2)
  });
}
```
