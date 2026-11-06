#!/usr/bin/env python3
"""Run constructed late-Zerg host frame oracles. Outputs stay ignored in build/."""
import fcntl, hashlib, json, os, pathlib, shlex, shutil, subprocess, sys
ROOT = pathlib.Path(__file__).resolve().parent.parent
os.chdir(ROOT)
out = ROOT / ('build/late-render-host-custom' if '--custom' in sys.argv else 'build/late-render-host-pair')
out.mkdir(parents=True, exist_ok=True)

def run(cmd, log, **kw):
    print('+', shlex.join(map(str, cmd)), flush=True)
    with open(log, 'w') as f:
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, check=True, **kw)

def space():
    if shutil.disk_usage(ROOT).free < 3 * 1024**3 + 300 * 1024**2:
        raise RuntimeError('insufficient free space (3 GiB + 300 MiB reserve)')

with open('/tmp/sc64-native-build-20261031.lock', 'w') as lock:
    fcntl.flock(lock, fcntl.LOCK_EX)
    space()
    mk = out / 'vectors.mk'
    mk.write_text('pair-vectors:\n\t@printf "%s\\n" "$(ZERG_Z7_RELEASE_FLAGS)" "$(ZERG_Z9_RELEASE_FLAGS)" "$(CANONICAL_CACHE_FLAGS)"\n')
    vectors = subprocess.check_output(['make','-s','-f','Makefile','-f',str(mk),'pair-vectors'], text=True).splitlines()
    if len(vectors) != 3: raise RuntimeError('expected three flag vectors')
    cache = shlex.split(vectors[2])
    # Freeze source, compiler vectors, generated headers, and all headers in
    # include directories. The private files themselves never enter the repo.
    paths = set(ROOT.glob('starcraft/*.h')) | set(ROOT.glob('starcraft/*.c'))
    paths |= set(ROOT.glob('common/*.h')) | set(ROOT.glob('common/*.c'))
    paths |= {ROOT/'Makefile', ROOT/'tests/sc64_z3_frame_coverage.c', pathlib.Path(__file__).resolve()}
    for vector in vectors[:2]:
        for arg in shlex.split(vector):
            if arg.startswith('-I') and len(arg) > 2:
                directory = ROOT / arg[2:]
                paths.update(directory.glob('*.h'))
    def manifest():
        return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in sorted(paths) if p.is_file()}
    before = manifest()
    (out/'manifest-before.json').write_text(json.dumps(before, indent=2, sort_keys=True)+'\n')
    errors = []
    for profile, vector in zip(('z7','z9'), vectors[:2]):
        flags = shlex.split(vector)
        if any(x in flags for x in cache): raise RuntimeError('baseline unexpectedly has cache flags')
        if profile == 'z9': flags += ['-DSC64_FRAME_TEST_Z9']
        if '--custom' in sys.argv: flags += ['-DSC64_FRAME_TEST_CUSTOM']
        common = ['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-fsanitize=address,undefined','-DGBA_HOST','-Icommon'] + flags
        for variant in ('baseline','candidate'):
            space()
            binary = out / (profile+'-'+variant)
            cmd = common + (cache if variant == 'candidate' else []) + ['tests/sc64_z3_frame_coverage.c','common/gba.c','-o',str(binary)]
            run(cmd,out/(binary.name+'.compile.log'))
            trace = pathlib.Path(str(binary)+'.trace'); trace.unlink(missing_ok=True)
            env = os.environ.copy(); env['SC64_FRAME_TRACE'] = str(trace)
            try:
                run([str(binary)],out/(binary.name+'.run.log'),env=env)
            except subprocess.CalledProcessError as exc:
                errors.append(f'{binary.name} exited {exc.returncode}')
        a=(out/(profile+'-baseline.trace')).read_bytes(); b=(out/(profile+'-candidate.trace')).read_bytes()
        if not a or len(a) % (38400+512) or a != b: errors.append(profile+' cross-binary framebuffer/palette trace mismatch')
        print(profile, 'captures',len(a)//(38400+512),'trace bytes',len(a),'sha256',hashlib.sha256(a).hexdigest(),flush=True)
    after = manifest()
    (out/'manifest-after.json').write_text(json.dumps(after, indent=2, sort_keys=True)+'\n')
    if before != after: errors.append('source or input manifest changed during run')
    if errors: raise RuntimeError('; '.join(errors))
print('All four compiled and passed; paired traces match.')
