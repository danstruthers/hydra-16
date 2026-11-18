// ****************************************************************************
// objcache.js - the build's cache of what cc65, ca65 and od65 make (reborn/games/dq0/GAME-CRITIQUE.md's 7.2): a run
// whose every input is as it was at a run before gets that run's outputs, copied, instead of running the tool again.
// A wrong hit is worse than a slow build, so a run is taken from the cache only when nothing it read can differ:
//   - its key: the cache's format, the tool (its file's sha1: its version, however it was built), the folder it runs
//     in, the environment it reads (every CC65_*, CA65_* and LD65_* variable), every argument as given (the flags,
//     the defines, the include folders, the outputs' names), and its input file's name and sha1;
//   - under the key, the runs kept (VARIANTS of them), each with every file the tool read (its own list of them:
//     cc65's --create-full-dep, ca65's --create-dep (its full list has the C files cc65's .dbg lines name too, which
//     it never reads): the headers, the .include and .incbin files, the .macpack files, cc65's own headers) and its
//     sha1, and every place an #include, .include, .incbin or .macpack name could have been
//     found (each name in each of those files, in every folder the tool may look in: the folder it runs in, each
//     file's own, each -I and --bin-include-dir, cc65's own folders) and whether a file was there: a run is taken
//     only if each file read has its sha1 still, and each place has a file or none as it had (a header put in a
//     folder searched before the one the run found it in is a new place, and a miss);
//   - a run is kept only if each file it read is explained by such a name (else the tools search somewhere this
//     doesn't know: it's run every time), and none of the sources (outside an obj folder: the build's own outputs,
//     made by it before the run) was changed since a moment before the run began (an edit during the run could
//     otherwise be kept with the outputs of what was there before it).
// Within one build (one cache opened), a source's sha1 is found once while its size and time stay, and whether a
// place outside obj/ has a file once; the build's own outputs in obj/ are looked at every time, and a run's record
// is made from what it read, looked at anew.
// od65's runs (the build's --dump-segsize, --dump-imports, --dump-exports of objects) are kept by the tool, the
// arguments and each object's sha1: its output is its stdout.  A run whose arguments aren't of the forms the build
// gives (TOOLS below) is run, never kept; a run that fails is never kept.
//   A moved tree, or another's copy of the cache: the build gives cc65 and ca65 the repository's files by their names
// from the folder they run in (relative), so what they make names them so, the same wherever the tree is; and the
// keys and records name the repository's files from its top (@/reborn/...), so a run kept in one place is found in
// another.  A file named by its full path (one outside the repository, a run given one) is in its key as it is.
//   What it keeps is the outputs as the tool made them: an object made then has the time ca65 made it (each .o carries
// the time of its making) and the sources' times as they were then (-g's debug information, cc65's .dbg file lines);
// the code, the data and every image built from them are the same byte for byte.
//   The cache: obj/cache under the build's folder (reborn/obj/cache, the base's own build base/obj/cache): m/ the keys'
// records (JSON), b/ the outputs by their sha1.  It's held to LIMIT bytes (HYDRA_BUILD_CACHE_MB, 1024 MB) as a build
// ends, the least recently used records and the outputs only they had going first.  HYDRA_BUILD_CACHE_DEBUG=1 says
// each run not kept, and why.
//
// use (dir)          the cache in dir for the build's runs from now on (null: none, every tool run)
// current ()         the cache in use, or null
// knows (exe)        cc65 or ca65: a tool whose runs it keeps
// relative (args, cwd)   args with each absolute path in the repository made relative to cwd (the build's run)
// A cache: run (exe, args, cwd) (cc65's, ca65's: their outputs made, from the cache or by the tool, which throws as
// execFileSync does; OUT: true if from the cache), ahead (list) (runs the build is about to ask for, [{ exe, args,
// cwd }]: those it hasn't run side by side, HYDRA_BUILD_JOBS at a time (the cores), by a worker thread (objjobs.js),
// and kept), output (exe, args) (od65's: its stdout), stats { hits, runs, kept, unkept, plain }, prune (limit), clear ().
// selfCheck ({ dir, ca65, cc65, od65 }): its rules held on sources of its own (the test buildcache).
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { execFileSync } = require('child_process');

const FORMAT = 'hydra objcache 1';              // (The keys' and records' form: another, another cache)
const VARIANTS = 6;                             // (A key's runs kept: the newest)
const SLACK = 2000;                             // (A source changed this soon before a run began isn't trusted, ms)
const WIN = process.platform === 'win32';
const LIMIT = (+process.env.HYDRA_BUILD_CACHE_MB || 1024) * 1048576;
const DEBUG = !!process.env.HYDRA_BUILD_CACHE_DEBUG;
const REPO = path.resolve(__dirname, '..', '..');            // (The repository's top: base/tools/../..)
const JOBS = +process.env.HYDRA_BUILD_JOBS || (require('os').availableParallelism || (() => require('os').cpus().length))();   // (ahead's at a time)

const sha1 = (...parts) => { const h = crypto.createHash('sha1'); for (const p of parts) h.update(p); return h.digest('hex'); };
const same = p => { const r = path.resolve(p); return WIN ? r.toLowerCase() : r; };   // (A path as the file system tells them apart)
const kind = p => { try { const s = fs.statSync(p); return s.isFile() ? 1 : 2; } catch (e) { return 0; } };   // (0 none, 1 a file, 2 else)
const under = (top, p) => { const r = path.relative(top, path.resolve(p)); return !r.startsWith('..') && !path.isAbsolute(r) ? r : null; };
// A path as the records keep it: a file of the repository's from its top, "@/reborn/games/..."; another as it is
const rel = (p, top = REPO) => { const r = under(top, p); return r === null ? path.resolve(p) : '@/' + r.split(path.sep).join('/'); };
const abs = (r, top = REPO) => r.startsWith('@/') ? path.join(top, ...r.slice(2).split('/')) : r;

// The tools, by name: their arguments (flags alone, values with the next), which give outputs (a role each: s the
// compiler's assembly, o an object, l a listing), which name folders searched, their own folder (under their home),
// their environment's, and how their sources name what they take in
const TOOLS = {
  cc65: { flags: a => a === '-g' || /^-O[irs]*$/.test(a), values: ['-t', '--cpu', '-I', '-D', '-o'], outs: { '-o': 's' },
    dirs: ['-I'], own: 'include', env: 'CC65_INC', dep: '--create-full-dep', names: t => [...t.matchAll(/^[ \t]*#[ \t]*include[ \t]*["<]([^">\r\n]+)[">]/gm)].map(m => m[1]) },
  ca65: { flags: a => a === '-g', values: ['--cpu', '-I', '-D', '-l', '-o', '--bin-include-dir'], outs: { '-o': 'o', '-l': 'l' },
    dirs: ['-I', '--bin-include-dir'], own: 'asminc', env: 'CA65_INC', dep: '--create-dep', names: t => [   // (.macpack NAME: its NAME.mac, cc65's own)
      ...[...t.matchAll(/\.(?:include|incbin)[ \t]+"([^"\r\n]+)"/gi)].map(m => m[1]), ...[...t.matchAll(/\.macpack[ \t]+(\w+)/gi)].map(m => m[1] + '.mac')] },
};
const toolOf = exe => path.basename(String(exe)).toLowerCase().replace(/\.exe$/, '');
const knows = exe => !!TOOLS[toolOf(exe)];

// args with each absolute path in the repository made relative to cwd (a tool's names of its files then the same
// wherever the tree is)
const relative = (args, cwd) => args.map(a => typeof a === 'string' && path.isAbsolute(a) && under(REPO, a) !== null ? path.relative(cwd, a) || '.' : a);

// A tool's file: its path (on the PATH if it's a bare name), and its sha1, once a process
const tools = new Map();
function toolFile(exe) {
  if (tools.has(exe)) return tools.get(exe);
  let file = null;
  if (path.isAbsolute(exe) || /[\\/]/.test(exe)) file = fs.existsSync(exe) ? path.resolve(exe) : null;
  else for (const d of (process.env.PATH || '').split(path.delimiter).filter(Boolean))
    for (const f of [path.join(d, exe), path.join(d, exe + '.exe')]) if (!file && kind(f) === 1) file = f;
  const r = file ? { file, hash: sha1(fs.readFileSync(file)) } : null;
  tools.set(exe, r);
  return r;
}

// The environment the cc65 tools read
const envOf = () => Object.keys(process.env).filter(k => /^(CC65|CA65|LD65)_/i.test(k)).sort().map(k => k + '=' + process.env[k]).join('\n');

// A run's arguments read: { input, outs: { role: file }, dirs: [the folders it's told to search] }, or null (a form
// not known: run, not kept)
function parse(t, args) {
  const T = TOOLS[t], outs = {}, dirs = [];
  let input = null;
  for (let i = 0; i < args.length; i++) {
    const a = String(args[i]);
    if (T.flags(a)) continue;
    if (T.values.includes(a)) {
      if (i + 1 >= args.length) return null;
      const v = String(args[++i]);
      if (T.outs[a]) outs[T.outs[a]] = v;
      if (T.dirs.includes(a)) dirs.push(v);
      continue;
    }
    if (i !== args.length - 1 || a.startsWith('-')) return null;
    input = a;
  }
  if (!input || !outs[Object.values(T.outs)[0]]) return null;
  return { input, outs, dirs };
}

// A make rule's prerequisites (cc65's --create-full-dep, ca65's --create-dep: "target:<tab>file file ...", a space in a name
// "\ "), or null if it isn't one
function depsOf(text) {
  const first = text.replace(/\\\r?\n/g, ' ').split(/\r?\n/)[0], at = first.indexOf(':\t');
  if (at < 0) return null;
  const out = [];
  let cur = '';
  const rest = first.slice(at + 2);
  for (let k = 0; k < rest.length; k++) {
    const c = rest[k];
    if (c === '\\' && rest[k + 1] === ' ') { cur += ' '; k++; }
    else if (c === '$' && rest[k + 1] === '$') { cur += '$'; k++; }
    else if (c === ' ' || c === '\t') { if (cur) out.push(cur); cur = ''; }
    else cur += c;
  }
  if (cur) out.push(cur);
  return out;
}

function open(dir, opt = {}) {
  const say = opt.say || (s => console.log(s)), top = opt.repo || REPO;   // (opt.repo: the tree its records name from: selfCheck's)
  const inObj = p => { const r = under(top, p), c = r === null ? [] : r.split(/[\\/]/); return c[0] === 'obj' || c[1] === 'obj'; };   // (The
                                                                // build's own outputs: obj/, the base's and reborn's)
  const stats = { hits: 0, runs: 0, kept: 0, unkept: 0, plain: 0 };
  const M = k => path.join(dir, 'm', k.slice(0, 2), k + '.json'), B = h => path.join(dir, 'b', h.slice(0, 2), h);
  const put = (file, data) => {                   // (Written whole or not at all)
    fs.mkdirSync(path.dirname(file), { recursive: true });
    const tmp = file + '.' + process.pid + '.tmp';
    fs.writeFileSync(tmp, data);
    fs.renameSync(tmp, file);
  };
  const blob = data => { const h = sha1(data), f = B(h); if (!fs.existsSync(f)) put(f, data); return h; };
  const record = k => { try { const r = JSON.parse(fs.readFileSync(M(k), 'utf8')); return r && r.format === FORMAT ? r : null; } catch (e) { return null; } };
  const keep = (k, rec, v) => {
    const r = rec || { format: FORMAT, runs: [] };
    r.runs = [v, ...r.runs.filter(x => JSON.stringify(x.outs) !== JSON.stringify(v.outs))].slice(0, VARIANTS);
    put(M(k), JSON.stringify(r));
    stats.kept++;
  };
  const used = k => { try { const now = new Date(); fs.utimesSync(M(k), now, now); } catch (e) { /* (Its age only) */ } };

  // A kept run's inputs as they are now: each place with a file or none as it had, each file read the same.  A
  // source's sha1 (outside obj/) is found once a build while its size and time stay, and whether a place outside obj/
  // has a file once a build (the build writes there before it runs a tool: the generated headers; obj/'s, which it
  // writes as it goes, are looked at every time).  A run's record is made from what it read, looked at anew
  const sums = new Map();
  const sumOf = p => {
    if (inObj(p)) return sha1(fs.readFileSync(p));
    const s = fs.statSync(p), was = sums.get(p), at = s.size + ':' + s.mtimeMs;
    if (was && was.at === at) return was.h;
    const h = sha1(fs.readFileSync(p));
    sums.set(p, { at, h });
    return h;
  };
  const kinds = new Map(), kindOf = p => {        // (A place's file or none: outside obj/, once a build)
    if (inObj(p)) return kind(p);
    if (!kinds.has(p)) kinds.set(p, kind(p));
    return kinds.get(p);
  };
  const holds = v => v.probes.every(([p, k]) => kindOf(abs(p, top)) === k) && v.deps.every(([p, h]) => {
    try { return sumOf(abs(p, top)) === h; } catch (e) { return false; }
  });

  function plain(exe, args, cwd) {
    stats.plain++;
    execFileSync(exe, args, { cwd, stdio: ['ignore', 'pipe', 'pipe'] });
    return false;
  }

  // A run's key, and its record: { t, p, tf, input, src, key, rec }, or null (a run it doesn't keep)
  function look(exe, args, cwd) {
    const t = toolOf(exe), T = TOOLS[t], p = T && parse(t, args), tf = p && toolFile(exe);
    if (!p || !tf) return null;
    const input = path.resolve(cwd, p.input);
    let src;
    try { src = fs.readFileSync(input); } catch (e) { return null; }   // (The tool says why)
    const key = sha1(FORMAT, '\0', t, '\0', tf.hash, '\0', rel(cwd, top), '\0', envOf(), '\0', JSON.stringify(args.map(String)), '\0', rel(input, top), '\0', sha1(src));
    return { t, p, tf, input, src, key, rec: record(key) };
  }
  // The outputs of the kept run that holds now ([[role, bytes]]; with read false, only whether there's one), or null
  function found(l, read = true) {
    if (l.rec) for (const v of l.rec.runs) {
      if (!holds(v)) continue;
      if (!read) { if (Object.values(v.outs).every(h => fs.existsSync(B(h)))) return true; continue; }
      try { return Object.entries(v.outs).map(([role, h]) => [role, fs.readFileSync(B(h))]); } catch (e) { /* (Pruned) */ }
    }
    return null;
  }
  // A run just made (its list of what it read in dep), kept if it may be
  function settle(l, exe, cwd, dep, began) {
    let v;
    try { v = made(l.t, exe, l.p, cwd, l.input, l.src, dep, began); } catch (e) { v = e.message; }
    if (typeof v === 'string') { stats.unkept++; if (DEBUG) say('objcache: ' + l.input + ' not kept: ' + v); }
    else keep(l.key, l.rec, v);
  }
  const withDep = (exe, args, dep) => [...args.slice(0, -1), TOOLS[toolOf(exe)].dep, dep, args[args.length - 1]];

  // cc65's or ca65's run, its outputs made.  OUT: true (from the cache), false (run)
  const early = new Set();                        // (The keys ahead ran this time: taken from the cache, but run)
  function run(exe, args, cwd) {
    const l = look(exe, args, cwd);
    if (!l) return plain(exe, args, cwd);
    const outs = found(l);
    if (outs) {
      for (const [role, data] of outs) { const f = path.resolve(cwd, l.p.outs[role]); fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, data); }
      used(l.key);
      if (!early.has(l.key)) stats.hits++;
      return true;
    }
    // Run, with the files it reads listed
    const dep = path.join(dir, 'tmp', process.pid + '.d');
    fs.mkdirSync(path.dirname(dep), { recursive: true });
    const began = Date.now();
    stats.runs++;
    try {
      execFileSync(exe, withDep(exe, args, dep), { cwd, stdio: ['ignore', 'pipe', 'pipe'] });
      settle(l, exe, cwd, dep, began);
    } finally { try { fs.rmSync(dep, { force: true }); } catch (e) { /* (Left in tmp/: pruned) */ } }
    return false;
  }

  // The runs of list ([{ exe, args, cwd }]) it hasn't, run side by side (JOBS at a time: together) and kept, as the
  // build is about to ask for each in turn (it takes them from the cache then).  Not if two would write one file, or
  // one read what another writes; a run that fails is left to the build's own (which says why)
  function ahead(list) {
    const todo = [];
    for (const j of list) { const l = look(j.exe, j.args, j.cwd); if (l && !found(l, false)) todo.push(Object.assign(l, j)); }
    if (todo.length < 2 || JOBS < 2) return 0;
    const outs = todo.flatMap(j => Object.values(j.p.outs).map(f => same(path.resolve(j.cwd, f))));
    if (new Set(outs).size !== outs.length || todo.some(j => outs.includes(same(j.input)))) return 0;
    const tmp = path.join(dir, 'tmp'), began = Date.now();
    fs.mkdirSync(tmp, { recursive: true });
    todo.forEach((j, i) => { j.dep = path.join(tmp, process.pid + '.' + i + '.d'); });
    const res = together(todo.map(j => ({ exe: j.exe, args: withDep(j.exe, j.args, j.dep), cwd: j.cwd })));
    let n = 0;
    todo.forEach((j, i) => {
      if (res[i] && res[i].code === 0) { stats.runs++; n++; early.add(j.key); settle(j, j.exe, j.cwd, j.dep, began); }
      try { fs.rmSync(j.dep, { force: true }); } catch (e) { /* (Left in tmp/: pruned) */ }
    });
    return n;
  }

  // A run just made: its record { deps, probes, outs }, or why it's not to be kept (a string)
  function made(t, exe, p, cwd, input, src, dep, began) {
    const T = TOOLS[t], read = depsOf(fs.readFileSync(dep, 'latin1'));
    if (!read) return 'its list of the files it read isn\'t a make rule';
    const files = [...new Map([input, ...read.map(f => path.resolve(cwd, f))].map(f => [same(f), f])).values()];
    const texts = new Map();
    for (const f of files) {
      if (kind(f) !== 1) return f + ' isn\'t a file (a name read wrong?)';
      if (!inObj(f) && fs.statSync(f).mtimeMs >= began - SLACK) return f + ' changed as it ran, or just before';
      texts.set(f, fs.readFileSync(f));
    }
    if (sha1(texts.get(input)) !== sha1(src)) return 'its input changed as it ran';
    // Every place each name could be found: the folders the tool may search (it runs in, each file's, the ones it's
    // told, its own), each name of each file read
    const home = process.env.CC65_HOME, own = [path.join(path.dirname(toolFile(exe).file), '..', T.own), ...(home ? [path.join(home, T.own)] : []),
      ...(process.env[T.env] || '').split(path.delimiter).filter(Boolean), ...['/usr/share/cc65', '/usr/local/share/cc65', '/usr/lib/cc65'].map(d => path.join(d, T.own))];
    const dirs = [...new Map([cwd, ...files.map(f => path.dirname(f)), ...p.dirs, ...own].map(d => path.resolve(cwd, d)).map(d => [same(d), d])).values()];
    const places = new Map();
    for (const data of texts.values())
      for (const n of T.names(data.toString('latin1')))
        for (const c of path.isAbsolute(n) ? [n] : dirs.map(d => path.resolve(d, n))) places.set(same(c), c);
    const lost = files.find(f => same(f) !== same(input) && !places.has(same(f)));
    if (lost) return lost + ' was read, but no name in what it read finds it';
    const outs = {};
    for (const [role, f] of Object.entries(p.outs)) outs[role] = blob(fs.readFileSync(path.resolve(cwd, f)));
    return { deps: files.filter(f => same(f) !== same(input)).map(f => [rel(f, top), sha1(texts.get(f))]), probes: [...places.values()].map(c => [rel(c, top), kind(c)]),
      outs, t: Date.now() };
  }

  // od65's run: its output (stdout), from the cache or the tool
  function output(exe, args) {
    const tf = toolFile(exe), objs = args.filter(a => !String(a).startsWith('-'));
    if (toolOf(exe) !== 'od65' || !tf || !args.every(a => /^--dump-(segsize|imports|exports)$/.test(a) || !String(a).startsWith('-'))) {
      stats.plain++;
      return execFileSync(exe, args, { encoding: 'latin1' });
    }
    let key;
    try { key = sha1(FORMAT, '\0od65\0', tf.hash, '\0', envOf(), '\0', JSON.stringify(args.map(String)), ...objs.map(o => '\0' + sha1(fs.readFileSync(o)))); }
    catch (e) { stats.plain++; return execFileSync(exe, args, { encoding: 'latin1' }); }
    const rec = record(key);
    if (rec && rec.runs.length) {
      try { const text = fs.readFileSync(B(rec.runs[0].outs.out), 'latin1'); used(key); stats.hits++; return text; } catch (e) { /* (Pruned) */ }
    }
    stats.runs++;
    const text = execFileSync(exe, args, { encoding: 'latin1' });
    try { keep(key, null, { deps: [], probes: [], outs: { out: blob(Buffer.from(text, 'latin1')) }, t: Date.now() }); } catch (e) { /* (Not kept) */ }
    return text;
  }

  // The cache held to limit bytes: the records least recently used, then the outputs no record has, gone first
  function prune(limit = LIMIT) {
    const list = sub => {
      const out = [], top = path.join(dir, sub);
      for (const d of fs.existsSync(top) ? fs.readdirSync(top) : []) for (const f of fs.readdirSync(path.join(top, d))) {
        const file = path.join(top, d, f), s = fs.statSync(file);
        out.push({ file, f, size: s.size, t: s.mtimeMs });
      }
      return out;
    };
    const recs = list('m'), blobs = list('b'), old = Date.now() - 3600e3, tmp = path.join(dir, 'tmp');
    for (const x of [...recs, ...blobs].filter(x => x.f.endsWith('.tmp') && x.t < old)) fs.rmSync(x.file, { force: true });   // (A run cut short)
    for (const f of fs.existsSync(tmp) ? fs.readdirSync(tmp) : []) if (fs.statSync(path.join(tmp, f)).mtimeMs < old) fs.rmSync(path.join(tmp, f), { force: true });
    const total = [...recs, ...blobs].reduce((n, x) => n + x.size, 0);
    if (total <= limit) return { total, removed: 0 };
    const size = new Map(blobs.map(b => [b.f, b.size])), have = new Set();
    let bytes = 0, removed = 0;
    for (const r of recs.filter(x => x.f.endsWith('.json')).sort((a, b) => b.t - a.t)) {   // (Newest first, kept while there's room)
      let rec = null;
      try { rec = JSON.parse(fs.readFileSync(r.file, 'utf8')); } catch (e) { /* (Gone) */ }
      const hs = rec && rec.runs ? rec.runs.flatMap(v => Object.values(v.outs)) : [];
      const more = hs.filter(h => !have.has(h)).reduce((n, h) => n + (size.get(h) || 0), 0);
      if (rec && bytes + r.size + more <= limit * 0.75) { bytes += r.size + more; hs.forEach(h => have.add(h)); continue; }
      fs.rmSync(r.file, { force: true });
      removed++;
    }
    for (const b of blobs) if (!have.has(b.f)) { fs.rmSync(b.file, { force: true }); removed++; }
    return { total, removed };
  }

  // Everything in it gone (a build from nothing: node build.js --clean)
  function clear() {
    for (const sub of ['m', 'b', 'tmp']) fs.rmSync(path.join(dir, sub), { recursive: true, force: true });
  }

  return { dir, run, ahead, output, prune, clear, stats };
}

// The cache's own check (the test buildcache: reborn/tests/suites/system.js): a tree of sources in dir/a (dir emptied
// first) and a cache of its own, each rule held.  tools: { dir, ca65, cc65, od65 } (the tools' files).  OUT:
// { failures, notes }
function selfCheck({ dir, ca65, cc65, od65 }) {
  const failures = [], notes = [], A = path.join(dir, 'a'), B = path.join(dir, 'b'), old = new Date(Date.now() - 3600e3);
  fs.rmSync(dir, { recursive: true, force: true });
  const put = (top, f, text, fresh) => {                       // (Old enough to be trusted; fresh, changed as a run began)
    const p = path.join(top, f), t = fresh ? new Date(Date.now() + 600e3) : old;
    fs.mkdirSync(path.dirname(p), { recursive: true });
    fs.writeFileSync(p, text);
    fs.utimesSync(p, t, t);
  };
  put(A, 'src/m.s', '.macpack longbranch\n.include "a.inc"\n.incbin "d.bin"\n  lda #VA\n  lda #VB\n');   // (.macpack: cc65's asminc/NAME.mac)
  put(A, 'inc1/a.inc', 'VA = 1\n.include "b.inc"\n');
  put(A, 'inc2/b.inc', 'VB = 2\n');
  put(A, 'src/d.bin', 'AB');
  put(A, 'src/c.c', '#include "h.h"\nint f (void) { return H; }\n');
  put(A, 'inc1/h.h', '#define H 7\n');
  put(A, 'src/x.c', '#define NAME "h.h"\n#include NAME\nint g (void) { return H; }\n');
  fs.mkdirSync(path.join(A, 'out'), { recursive: true });
  let cache, top = A, store = path.join(dir, 'cache');
  const asmArgs = ['--cpu', 'W65C02', '-g', '-I', 'inc1', '-I', 'inc2', '-l', 'out/m.lst', '-o', 'out/m.o', 'src/m.s'];
  const step = (what, want, fn, check) => {                   // (want: 'hit', 'kept' (run, kept) or 'unkept'.  Each step a
    cache = open(store, { repo: top, say: s => notes.push(s) });   //   build of its own: a cache opened anew)
    try { fn(); } catch (e) { failures.push(what + ': ' + e.message); return; }
    const d = k => cache.stats[k], got = d('hits') ? 'hit' : d('kept') ? 'kept' : d('unkept') ? 'unkept' : 'nothing';
    if (got !== want) failures.push(what + ': ' + got + ', not ' + want);
    if (check) { const why = check(); if (why) failures.push(what + ': ' + why); }
  };
  const lst = () => fs.readFileSync(path.join(top, 'out', 'm.lst'), 'latin1');
  const has = bytes => () => lst().includes(bytes) ? null : 'its listing hasn\'t ' + bytes;
  const asm = () => cache.run(ca65, asmArgs, top);
  step('ca65 the first time', 'kept', asm, has('A9 01'));
  const first = lst(), firstObj = fs.readFileSync(path.join(A, 'out', 'm.o'));
  step('ca65 again', 'hit', asm, () => lst() === first && fs.readFileSync(path.join(A, 'out', 'm.o')).equals(firstObj) ? null : 'its outputs aren\'t the first\'s');
  put(A, 'inc2/b.inc', 'VB = 3\n');
  step('an .include\'s .include changed', 'kept', asm, has('A9 03'));
  put(A, 'inc2/b.inc', 'VB = 2\n');
  step('it changed back', 'hit', asm, () => lst() === first ? null : 'its listing isn\'t the first\'s');
  put(A, 'src/a.inc', 'VA = 5\n.include "b.inc"\n');
  step('an .include put in a folder searched first', 'kept', asm, has('A9 05'));
  fs.rmSync(path.join(A, 'src', 'a.inc'));
  step('it taken away', 'hit', asm, has('A9 01'));
  put(A, 'src/d.bin', 'AC');
  step('an .incbin\'s file changed', 'kept', asm, has('41 43'));
  put(A, 'inc2/b.inc', 'VB = 4\n', true);
  step('a source changed as the run began', 'unkept', asm, has('A9 04'));
  step('and run again', 'unkept', asm);
  const ccArgs = f => ['-g', '-O', '-t', 'none', '-I', 'inc1', '-o', 'out/' + f.replace(/\.c$/, '.s'), 'src/' + f];
  step('cc65 the first time', 'kept', () => cache.run(cc65, ccArgs('c.c'), top));
  step('cc65 again', 'hit', () => cache.run(cc65, ccArgs('c.c'), top));
  put(A, 'inc1/h.h', '#define H 8\n');
  step('a header changed', 'kept', () => cache.run(cc65, ccArgs('c.c'), top), () => /#\$08/.test(fs.readFileSync(path.join(A, 'out', 'c.s'), 'latin1')) ? null : 'its assembly hasn\'t #$08');
  fs.utimesSync(path.join(A, 'out', 'c.s'), old, old);           // (Here not in obj/: as the build's own outputs are trusted)
  step('ca65 on cc65\'s assembly (its .dbg file lines name the C\'s files, not read)', 'kept',
    () => cache.run(ca65, ['--cpu', 'W65C02', '-g', '-l', 'out/c.lst', '-o', 'out/c.o', 'out/c.s'], top));
  step('a computed #include (its header found where no name says)', 'unkept', () => cache.run(cc65, ccArgs('x.c'), top));
  step('and compiled again', 'unkept', () => cache.run(cc65, ccArgs('x.c'), top));
  put(A, 'inc2/b.inc', 'VB = 4\n');                            // (No longer just changed)
  put(A, 'src/n.s', '.include "a.inc"\n  ldx #VA\n');
  put(A, 'src/q.s', '.include "a.inc"\n  ldy #VB\n');
  const two = ['n', 'q'].map(f => ({ exe: ca65, args: ['--cpu', 'W65C02', '-g', '-I', 'inc1', '-I', 'inc2', '-l', 'out/' + f + '.lst', '-o', 'out/' + f + '.o', 'src/' + f + '.s'], cwd: A }));
  step('two runs side by side (ahead), then asked for', 'kept', () => { cache.ahead(two); two.forEach(j => cache.run(j.exe, j.args, j.cwd)); },
    () => cache.stats.runs === 2 && cache.stats.kept === 2 && fs.readFileSync(path.join(A, 'out', 'q.lst'), 'latin1').includes('A0 04') ? null :
      'runs ' + cache.stats.runs + ', kept ' + cache.stats.kept + ' (each once, not twice), or q.lst without A0 04');
  const dump = () => cache.output(od65, ['--dump-exports', path.join(top, 'out', 'm.o')]);
  step('od65 the first time', 'kept', dump);
  step('od65 again', 'hit', dump);
  // The tree moved, and its cache with it: its runs found, their outputs the same
  put(A, 'inc2/b.inc', 'VB = 2\n');
  put(A, 'src/d.bin', 'AB');
  step('the first sources again', 'hit', asm, () => lst() === first ? null : 'its listing isn\'t the first\'s');
  fs.cpSync(A, B, { recursive: true, preserveTimestamps: true });
  fs.cpSync(store, path.join(dir, 'cache2'), { recursive: true });
  for (const f of fs.readdirSync(path.join(B, 'out'))) fs.rmSync(path.join(B, 'out', f));
  store = path.join(dir, 'cache2');
  top = B;
  step('the tree moved: ca65', 'hit', asm, () => lst() === first ? null : 'its listing isn\'t the first\'s');
  step('the tree moved: cc65', 'hit', () => cache.run(cc65, ccArgs('c.c'), top));
  if (!/Main file\s*:\s*src[\\/]m\.s/.test(first)) failures.push('the listing names its source otherwise than from the folder it ran in: ' + (/Main file.*/.exec(first) || [''])[0]);
  notes.push('ca65, cc65 and od65 run through a cache of their own: ' + (failures.length ? failures.length + ' rule(s) broken' : 'each rule held'));
  return { failures, notes };
}

// Runs side by side, JOBS at a time, the build waiting (objjobs.js: a worker thread, one a process, its event loop the
// spawning's; the build's held on Atomics.wait).  OUT: a { code, said } each; [] if the worker's gone (the build runs
// each then: after a wait of WAIT for a batch, no more batches are sent it)
const WAIT = 900e3;
let pool = null;
function together(jobs) {
  if (pool === false) return [];
  const W = require('worker_threads');
  if (!pool) {
    const flag = new Int32Array(new SharedArrayBuffer(4)), ch = new W.MessageChannel();
    const w = new W.Worker(path.join(__dirname, 'objjobs.js'), { workerData: { flag, port: ch.port2 }, transferList: [ch.port2] });
    w.unref();
    ch.port1.unref();
    pool = { w, flag, port: ch.port1 };
  }
  Atomics.store(pool.flag, 0, 0);
  pool.w.postMessage({ jobs, n: JOBS });
  if (Atomics.wait(pool.flag, 0, 0, WAIT) === 'timed-out') { pool = false; return []; }
  const m = W.receiveMessageOnPort(pool.port);
  return m ? m.message : [];
}

let active = null;
function use(dir, opt) { active = dir ? open(dir, opt) : null; return active; }
const current = () => active;

module.exports = { use, current, open, knows, relative, depsOf, parse, rel, abs, selfCheck, FORMAT, REPO };
