// Minimal Node.js consumer example
// npm i undici

import { request } from 'undici';

const BASE = process.env.DAYZ_API || 'http://127.0.0.1:8192';
const KEY = process.env.DAYZ_API_KEY;

async function get(path) {
  const { body } = await request(`${BASE}${path}`, {
    headers: { 'x-api-key': KEY }
  });
  return body.json();
}

async function main() {
  const state = await get('/v1/state');
  console.log('playerCount', state.playerCount);
  console.log('players', state.players?.map(p => ({ uid: p.uid, name: p.name, x:p.x, y:p.y, z:p.z })));
}

main().catch(e => {
  console.error(e);
  process.exit(1);
});
