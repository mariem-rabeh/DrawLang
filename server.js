const express = require('express');
const { execFile } = require('child_process');
const fs   = require('fs');
const os   = require('os');
const path = require('path');
const crypto = require('crypto');

const app = express();
app.use(express.json({ limit: '1mb' }));

// ── CORS : autoriser l'HTML ouvert depuis file:// ou n'importe quel origin ──
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin',  '*');
  res.header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Content-Type');
  if (req.method === 'OPTIONS') return res.sendStatus(204);
  next();
});

// ── Détection Windows : le binaire porte l'extension .exe ──
const IS_WIN  = process.platform === 'win32';
const BIN_NAME = IS_WIN ? 'drawlang.exe' : 'drawlang';
const BINARY  = path.join(__dirname, BIN_NAME);

console.log(`Plateforme : ${process.platform}`);
console.log(`Binaire attendu : ${BINARY}`);
console.log(`Binaire présent : ${fs.existsSync(BINARY)}`);

// ── GET /ping ── sanity-check depuis le navigateur ──
app.get('/ping', (_, res) => {
  res.json({ status: 'ok', binary: fs.existsSync(BINARY), platform: process.platform });
});

// ── POST /run ── compilation + exécution DrawLang ──
app.post('/run', (req, res) => {
  const code = req.body && req.body.code;
  if (!code || typeof code !== 'string') {
    return res.status(400).json({ error: 'Aucun code fourni' });
  }

  if (!fs.existsSync(BINARY)) {
    return res.json({
      ok:   false,
      logs: `[ERREUR] Binaire introuvable : ${BINARY}\n` +
            `Compilez d'abord avec :\n` +
            (IS_WIN
              ? `  flex draw.l && bison -d draw.y\n  gcc -o drawlang.exe draw.tab.c ast.c svg.c -lm`
              : `  flex draw.l && bison -d draw.y\n  gcc -o drawlang draw.tab.c ast.c svg.c -lm -lfl`),
      svg: null
    });
  }

  // Fichiers temporaires uniques
  const id      = crypto.randomBytes(8).toString('hex');
  const srcFile = path.join(os.tmpdir(), `draw_${id}.draw`);
  const svgFile = path.join(os.tmpdir(), `draw_${id}.svg`);

  try {
    fs.writeFileSync(srcFile, code, 'utf8');
  } catch (e) {
    return res.json({ ok: false, logs: `[ERREUR] Impossible d'écrire le fichier temp: ${e.message}`, svg: null });
  }

  execFile(BINARY, [srcFile, svgFile], { timeout: 10000 }, (err, stdout, stderr) => {
    // Nettoyage du fichier source
    try { fs.unlinkSync(srcFile); } catch (_) {}

    // Supprimer les séquences ANSI (couleurs terminal)
    const stripAnsi = s => s.replace(/\x1b\[[0-9;]*m/g, '');
    const logs = stripAnsi(((stdout || '') + (stderr || '')).trim());

    // Lire le SVG généré même en cas d'erreur partielle
    let svg = null;
    if (fs.existsSync(svgFile)) {
      try {
        svg = fs.readFileSync(svgFile, 'utf8');
      } catch (_) {}
      try { fs.unlinkSync(svgFile); } catch (_) {}
    }

    // ok = pas d'erreur fatale ET svg produit
    const ok = !err && svg !== null
           && !logs.match(/\[ERREUR\]|\[SYNTAXE\]/i);

    res.json({ ok, logs, svg });
  });
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, '127.0.0.1', () => {
  console.log(`\nDrawLang backend démarré → http://localhost:${PORT}`);
  console.log(`Ouvrez drawlang_ide.html dans votre navigateur.\n`);
});