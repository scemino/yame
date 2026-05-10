// fibs command to build and serve the emulators webpage
// deno-lint-ignore-file no-unversioned-import
import { Configurer, log, proj, Project, util } from "jsr:@floooh/fibs";
import { green } from "jsr:@std/fmt/colors";

export function addWebpageCommand(c: Configurer) {
    c.addCommand({ name: "webpage", help, run });
}

function help() {
    log.helpCmd([
        "webpage build",
        "webpage serve",
    ], "build or serve webpage");
}

async function run(p: Project, args: string[]): Promise<void> {
    const subcmd = args[1];
    const configName = "emsc-ninja-release";
    const config = p.config(configName);
    const srcDir = p.distDir(configName);
    const dstDir = `${p.fibsDir()}/webpage`;
    if (subcmd === "build") {
        if (util.dirExists(dstDir)) {
            if (Deno.env.get('CI') || log.ask(`Ok to delete directory ${dstDir}?`, false)) {
                Deno.removeSync(dstDir, { recursive: true });
            }
        }
        util.ensureDir(dstDir);
        await proj.generate(config);
        await proj.build({});
        await deployWebpage(p, srcDir, dstDir);
        log.info(`\n${green("Done.")} (webpage available in ${dstDir})`);
    } else if (subcmd === "serve") {
        const emsc = p.importModule("extras", "emscripten.ts");
        emsc.emrun(p, { cwd: dstDir, file: "index.html" });
    } else {
        throw new Error("subcommand 'build' or 'serve' expected (run 'fibs help webpage')");
    }
}

async function deployWebpage(p: Project, srcDir: string, dstDir: string): Promise<void> {
    // build thumbnail gallery
    let content = '';
    for (const item of items) {
        if (item.type === 'hidden') {
            continue;
        }
        const {
            title,
            url,
            img,
            note,
        } = item;
        const uiUrl = url.replaceAll('.html', '-ui.html');
        log.info(`> adding thumbnail for ${url}`);
        content += `<div class="thumb-frame">\n`;
        content += `  <div class="thumb-title">${title}</div>\n`;
        if (util.fileExists(`${srcDir}/mo5-ui.js`)) {
            content += `  <a class="ui-btn-link" href="${uiUrl}"><div class="ui-btn">UI</div></a>\n`;
        }
        content += `  <a class="image-frame" href="${url}"><img class="image" src="${toWebpExt(img)}"></img></a>\n`;
        if (note !== '') {
            content += `  <div class="thumb-footer">${note}</div>\n`;
        }
        content += '</div>\n';
    }

    // populate html template and write to build directory
    const indexTmpl = Deno.readTextFileSync(`${p.dir()}/webpage/index.html`);
    const indexHtml = indexTmpl.replaceAll('${samples}', content);
    Deno.writeTextFileSync(`${dstDir}/index.html`, indexHtml, {});

    // copy other required files
    for (const file of ['style.css', 'favicon.png']) {
        log.info(`> copy file: ${file}`);
        Deno.copyFileSync(`${p.dir()}/webpage/${file}`, `${dstDir}/${file}`);
    }

    // generate emulator html pages
    const emuTmpl = Deno.readTextFileSync(`${p.dir()}/webpage/emsc.html`);
    for (const system of ['mo5', 'mo5-ui']) {
        log.info(`> generate html page for: ${system}`);
        for (const ext of ['wasm', 'js']) {
            const srcPath = `${srcDir}/${system}.${ext}`;
            const dstPath = `${dstDir}/${system}.${ext}`;
            if (util.fileExists(srcPath)) {
                Deno.copyFileSync(srcPath, dstPath);
            }
        }
        const emuHtml = emuTmpl.replaceAll('${prog}', system);
        Deno.writeTextFileSync(`${dstDir}/${system}.html`, emuHtml);
    }

    // copy data file and images
    const assetDir  = 'mo5'
    const srcAssetDir = `${p.dir()}/webpage/${assetDir}`;
    const dstAssetDir = `${dstDir}/${assetDir}`;
    if (!util.dirExists(srcAssetDir)) {
        log.warn(`source asset directory ${srcAssetDir} not found!`);
    }
    util.ensureDir(dstAssetDir);
    for (const file of Deno.readDirSync(srcAssetDir)) {
        if (!file.isFile) {
            log.warn(`ignoring non-file asset ${file.name} in ${srcAssetDir}`);
            continue;
        }
        const srcPath = `${srcAssetDir}/${file.name}`;
        let dstPath = `${dstAssetDir}/${file.name}`;
        if (srcPath.endsWith('.jpg')) {
            dstPath = toWebpExt(dstPath);
            await cwebp(srcPath, dstPath);
        } else {
            log.info(`> copy ${srcPath} => ${dstPath}`);
            Deno.copyFileSync(srcPath, dstPath);
        }

    }
}

function toWebpExt(path: string): string {
    return path.replace(/\.jpg$/, '.webp');
}

async function cwebp(srcPath: string, dstPath: string): Promise<void> {
    log.info(`> cwebp ${srcPath} => ${dstPath}`)
    await util.runCmd('cwebp', {
        args: ['-quiet', '-q', '80', srcPath, '-o', dstPath ],
        showCmd: false,
    });
}

type Emu = {
    type: "ext" | "emu" | "demo" | "game" | "hidden",
    title: string;
    url: string;
    img: string;
    note: string;
};

// deno-fmt-ignore-next-line
const items: Emu[] = [
    { 'type':'emu',  'title':'Thomson MO5',                 'url':'mo5.html', 'img':'mo5/mo5.jpg', 'note':'' },
    { 'type':'game', 'title':"3D Fight (tape)",             'url':'mo5.html?file=mo5/3dfight_mo5.k7&input=run%22%22%0A', 'img':'mo5/3dfight_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Arkanoïd (tape)",             'url':'mo5.html?file=mo5/arkanoid_mo5.k7&input=run%22%22%0A', 'img':'mo5/arkanoid_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Androïdes (tape)",            'url':'mo5.html?file=mo5/androide-mo5.k7&input=loadm%0A', 'img':'mo5/androide-mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Avenger (tape)",              'url':'mo5.html?file=mo5/avenger_mo5.k7&input=run%22%22%0A', 'img':'mo5/avenger_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"L'aigle dor (tape)",          'url':'mo5.html?file=mo5/l-aigle-d-or_mo5.k7&input=run%22%22%0A', 'img':'mo5/l-aigle-d-or_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Classiques vol1 (tape)",      'url':'mo5.html?file=mo5/classiques-vol1_mo5.k7&input=run%22%22%0A', 'img':'mo5/classiques-vol1_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"F15 strike eagle (tape)",     'url':'mo5.html?file=mo5/f15-strike-eagle_mo5.k7&input=run%22%22%0A', 'img':'mo5/f15-strike-eagle_mo5.jpg', 'note':''},
    { 'type':'demo', 'title':"F-14 - Forever Young (tape)", 'url':'mo5.html?file=mo5/forever14.k7&input=loadm%22%22,,r%0A', 'img':'mo5/forever14.jpg', 'note':'by Shinra and Just For Fun'},
    { 'type':'game', 'title':"Flipper (tape)",              'url':'mo5.html?file=mo5/flipper_mo5.k7&input=run%22%22%0A', 'img':'mo5/flipper_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Football (tape)",             'url':'mo5.html?file=mo5/football_mo5.k7&input=run%22%22%0A', 'img':'mo5/football_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Formule 1 (tape)",            'url':'mo5.html?file=mo5/formule-1_mo5.k7&input=run%22%22%0A', 'img':'mo5/formule-1_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Game Over (tape)",            'url':'mo5.html?file=mo5/game-over_mo5.k7&input=run%22%22%0A', 'img':'mo5/game-over_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Glouton (tape)",              'url':'mo5.html?file=mo5/glouton_mo5.k7&input=run%22%22%0A', 'img':'mo5/glouton_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Green Beret (tape)",          'url':'mo5.html?file=mo5/green-beret_mo5.k7&input=run%22%22%0A', 'img':'mo5/green-beret_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Judoka (tape)",               'url':'mo5.html?file=mo5/judoka_mo5.k7&input=run%22%22%0A', 'img':'mo5/judoka_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Karate (tape)",               'url':'mo5.html?file=mo5/karate_mo5.k7&input=loadm%0A', 'img':'mo5/karate_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Krakout (tape)",              'url':'mo5.html?file=mo5/krakout_mo5.k7&input=run%22%22%0A', 'img':'mo5/krakout_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"le-5eme-axe (tape)",          'url':'mo5.html?file=mo5/le-5eme-axe_mo5.k7&input=loadm%0A', 'img':'mo5/le-5eme-axe_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Mission Pas Possible (tape)", 'url':'mo5.html?file=mo5/mission-pas-possible_mo5.k7&input=run%22%22%0A', 'img':'mo5/mission-pas-possible_mo5.jpg', 'note':''},
    //{ 'type':'game', 'title':"Mission En Rafale (tape)",  'url':'mo5.html?file=mo5/missions-en-rafale_mo5.k7&input=run%22%22%0A', 'img':'mo5/missions-en-rafale_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Numéro 10 (tape)",            'url':'mo5.html?file=mo5/numero-10_mo5.k7&input=run%22%22%0A', 'img':'mo5/numero-10_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Raid sur Tenere (tape)",      'url':'mo5.html?file=mo5/raid-sur-tenere_mo5.k7&input=run%22%22%0A', 'img':'mo5/raid-sur-tenere_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Renegade (tape)",             'url':'mo5.html?file=mo5/renegade_mo5.k7&input=run%22%22%0A', 'img':'mo5/renegade_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Runway (tape)",               'url':'mo5.html?file=mo5/runway_mo5.k7&input=run%22%22%0A', 'img':'mo5/runway_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Runway 2 (tape)",             'url':'mo5.html?file=mo5/runway2_mo5.k7&input=run%22%22%0A', 'img':'mo5/runway2_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Sapiens (tape)",              'url':'mo5.html?file=mo5/sapiens_mo5.k7&input=loadm%0A', 'img':'mo5/sapiens_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Sorcery (tape)",              'url':'mo5.html?file=mo5/sorcery_mo5.k7&input=run%22%22%0A', 'img':'mo5/sorcery_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Stanley (tape)",              'url':'mo5.html?file=mo5/stanley_mo5.k7&input=loadm%22%22,,r%0A', 'img':'mo5/stanley_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Super tennis (tape)",         'url':'mo5.html?file=mo5/super-tennis-fil_mo5.k7&input=run%22%22%0A', 'img':'mo5/super-tennis-fil_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Tout Schuss (tape)",          'url':'mo5.html?file=mo5/tout-schuss_mo5.k7&input=run%22%22%0A', 'img':'mo5/tout-schuss_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Vampire (tape)",              'url':'mo5.html?file=mo5/vampire_mo5.k7&input=loadm%0A', 'img':'mo5/vampire_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Zéro de conduite (tape)",     'url':'mo5.html?file=mo5/zero-de-conduite_mo5.k7&input=run%22%22%0A', 'img':'mo5/zero-de-conduite_mo5.jpg', 'note':''}
]
