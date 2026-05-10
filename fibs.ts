import { Configurer, Builder } from 'jsr:@floooh/fibs@^1';
import { addWebpageCommand } from './fibs-scripts/webpage.ts';

export function configure(c: Configurer) {
    c.addImport({
        name: 'dcimgui',
        url: 'https://github.com/floooh/dcimgui',
        files: ['fibs-docking.ts'],
    });
    c.addImport({
        name: 'libs',
        url: 'https://github.com/floooh/fibs-libs',
        files: ['sokol.ts', 'stb.ts'],
    });
    c.addImport({
        name: 'extras',
        url: 'https://github.com/floooh/fibs-extras',
        files: [
            'emscripten.ts',
            'macos.ts',
            'windows.ts',
            'sokolshdc.ts',
            'embedfiles.ts',
            'stdoptions.ts',
            'linux-threads.ts',
            'vscode.ts',
        ],
    });

    addWebpageCommand(c);
}

export function build(b: Builder) {
    b.addCmakeVariable('CMAKE_CXX_STANDARD', '20');
    if (b.isMsvc()) {
        b.addCompileOptions([
            '/wd4244',      // conversion from X to Y, possible loss of data
            '/wd4267',      // ditto
            '/wd4324',      // structure was padded due to alignment specifier
            '/wd4200',      // non-standard extension used: zero sized array
            '/wd4201',      // non-standard extension used: nameless struct/union
            '/wd4702',      // unreachable code
        ]);
    } else if (b.isGcc()) {
        b.addCompileOptions([
            '-Wno-stringop-overflow',   // possible false positives
        ]);
    }

    addCommon(b);

    // emulator without UI
    b.addTarget('mo5', 'windowed-exe', (t) => {
        t.setDir('src');
        t.setIdeFolder('src');
        t.addSources([`main.c`, `mo5.c`, `keybuf.c`, `m6809.c`, `mo5rom.c`]);
        t.addDependencies(['common']);
        t.addIncludeDirectories({ dirs: ['../libs/sokol']});
    });
    // emulator with UI
    b.addTarget(`mo5-ui`, 'windowed-exe', (t) => {
        t.setDir('src');
        t.setIdeFolder('src');
        t.addSources([`main.c`, `mo5.c`, `mo5-ui-impl.cc`, `keybuf.c`, `m6809.c`, `mo5rom.c`]);
        t.addCompileDefinitions({ EMU_USE_UI: '1' });
        t.addDependencies(['ui']);
        t.addIncludeDirectories({ dirs: ['../libs/sokol']});
    });
}

function addCommon(b: Builder) {
    const dir = 'libs/sokol';
    const ideFolder = 'common';
    b.addTarget('common', 'lib', (t) => {
        t.setDir(dir);
        t.setIdeFolder(ideFolder);
        t.addSources([
            'sokol.c',
            'clock.c', 'clock.h',
            'fs.c', 'fs.h',
            'gfx.c', 'gfx.h',

        ]);
        t.addJob({ job: 'sokolshdc', args: { src: 'shaders.glsl', outDir: t.buildDir() } });
        t.addIncludeDirectories({ dirs: [t.buildDir()], scope: 'private'});
        t.addDependencies(['sokol']);
    });
    b.addTarget('ui', 'lib', (t) => {
        t.setDir(dir);
        t.setIdeFolder(ideFolder);
        t.addSources([ 'ui.cc', 'ui.h' ]);
        t.addDependencies(['imgui-docking', 'common']);
    });
}

