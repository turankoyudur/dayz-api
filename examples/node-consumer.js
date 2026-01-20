// Ornek Node.js tuketici (API Node tabanli degil; sadece ornek client)
// node examples/node-consumer.js

const API = process.env.API || 'http://127.0.0.1:8192/v1';
const KEY = process.env.API_KEY || 'PUT_YOUR_KEY_HERE';

async function req(path, opts = {}) {
  const res = await fetch(API + path, {
    ...opts,
    headers: {
      'x-api-key': KEY,
      'content-type': 'application/json',
      ...(opts.headers || {})
    }
  });
  const text = await res.text();
  // /v1/state raw json, responses also json
  return { status: res.status, text };
}

(async () => {
  console.log('Status...');
  console.log(await req('/status'));

  console.log('State...');
  const state = await req('/state');
  console.log(state.status);
  const st = JSON.parse(state.text);
  console.log('Players:', st.server?.playerCount, 'records:', st.players?.length);

  if (st.players?.length) {
    const uid = st.players[0].uid;
    console.log('Teleport first player (example)...');
    const tp = await req(`/players/${uid}/teleport`, {
      method: 'POST',
      body: JSON.stringify({ x: st.players[0].pos[0] + 2, y: st.players[0].pos[1], z: st.players[0].pos[2] + 2 })
    });
    console.log(tp);
  }
})();
