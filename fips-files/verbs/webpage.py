"""fips verb to build the samples webpage"""

import os
import yaml
import shutil
import subprocess
import glob
from string import Template

from mod import log, util, project

BuildConfig = 'wasm-ninja-release'

systems = [
    'mo5'
]

asset_dirs = [ 'mo5' ]

items = [
    { 'type':'emu',  'title':'Thomson MO5',                 'system':'mo5', 'url':'mo5.html', 'img':'mo5/mo5.jpg', 'note':'' },
    { 'type':'game', 'title':"3D Fight (tape)",             'system':'mo5', 'url':'mo5.html?file=mo5/3dfight_mo5.k7&input=run%22%22%0A', 'img':'mo5/3dfight_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Arkanoïd (tape)",             'system':'mo5', 'url':'mo5.html?file=mo5/arkanoid_mo5.k7&input=run%22%22%0A', 'img':'mo5/arkanoid_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Androïdes (tape)",            'system':'mo5', 'url':'mo5.html?file=mo5/androide-mo5.k7&input=loadm%0A', 'img':'mo5/androide-mo5.jpg', 'note':''},
    { 'type':'game', 'title':"L'aigle dor (tape)",          'system':'mo5', 'url':'mo5.html?file=mo5/l-aigle-d-or_mo5.k7&input=run%22%22%0A', 'img':'mo5/l-aigle-d-or_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Classiques vol1 (tape)",      'system':'mo5', 'url':'mo5.html?file=mo5/classiques-vol1_mo5.k7&input=run%22%22%0A', 'img':'mo5/classiques-vol1_mo5.jpg', 'note':''},
    { 'type':'demo', 'title':"F-14 - Forever Young (tape)", 'system':'mo5', 'url':'mo5.html?file=mo5/forever14.k7&input=loadm%22%22,,r%0A', 'img':'mo5/forever14.jpg', 'note':'by Shinra and Just For Fun'},
    { 'type':'game', 'title':"Formule 1 (tape)",            'system':'mo5', 'url':'mo5.html?file=mo5/formule-1_mo5.k7&input=run%22%22%0A', 'img':'mo5/formule-1_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Glouton (tape)",              'system':'mo5', 'url':'mo5.html?file=mo5/glouton_mo5.k7&input=run%22%22%0A', 'img':'mo5/glouton_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Green Beret (tape)",          'system':'mo5', 'url':'mo5.html?file=mo5/green-beret_mo5.k7&input=run%22%22%0A', 'img':'mo5/green-beret_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Judoka (tape)",               'system':'mo5', 'url':'mo5.html?file=mo5/judoka_mo5.k7&input=run%22%22%0A', 'img':'mo5/judoka_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Karate (tape)",               'system':'mo5', 'url':'mo5.html?file=mo5/karate_mo5.k7&input=loadm%0A', 'img':'mo5/karate_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Krakout (tape)",              'system':'mo5', 'url':'mo5.html?file=mo5/krakout_mo5.k7&input=run%22%22%0A', 'img':'mo5/krakout_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"le-5eme-axe (tape)",          'system':'mo5', 'url':'mo5.html?file=mo5/le-5eme-axe_mo5.k7&input=loadm%0A', 'img':'mo5/le-5eme-axe_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Mission Pas Possible (tape)", 'system':'mo5', 'url':'mo5.html?file=mo5/mission-pas-possible_mo5.k7&input=run%22%22%0A', 'img':'mo5/mission-pas-possible_mo5.jpg', 'note':''},
    #{ 'type':'game', 'title':"Mission En Rafale (tape)",    'system':'mo5', 'url':'mo5.html?file=mo5/missions-en-rafale_mo5.k7&input=run%22%22%0A', 'img':'mo5/missions-en-rafale_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Numéro 10 (tape)",            'system':'mo5', 'url':'mo5.html?file=mo5/numero-10_mo5.k7&input=run%22%22%0A', 'img':'mo5/numero-10_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Raid sur Tenere (tape)",      'system':'mo5', 'url':'mo5.html?file=mo5/raid-sur-tenere_mo5.k7&input=run%22%22%0A', 'img':'mo5/raid-sur-tenere_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Renegade (tape)",             'system':'mo5', 'url':'mo5.html?file=mo5/renegade_mo5.k7&input=run%22%22%0A', 'img':'mo5/renegade_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Runway (tape)",               'system':'mo5', 'url':'mo5.html?file=mo5/runway_mo5.k7&input=run%22%22%0A', 'img':'mo5/runway_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Runway 2 (tape)",             'system':'mo5', 'url':'mo5.html?file=mo5/runway2_mo5.k7&input=run%22%22%0A', 'img':'mo5/runway2_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Sorcery (tape)",              'system':'mo5', 'url':'mo5.html?file=mo5/sorcery_mo5.k7&input=run%22%22%0A', 'img':'mo5/sorcery_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Stanley (tape)",              'system':'mo5', 'url':'mo5.html?file=mo5/stanley_mo5.k7&input=loadm%22%22,,r%0A', 'img':'mo5/stanley_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Super tennis (tape)",         'system':'mo5', 'url':'mo5.html?file=mo5/super-tennis-fil_mo5.k7&input=run%22%22%0A', 'img':'mo5/super-tennis-fil_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Tout Schuss (tape)",          'system':'mo5', 'url':'mo5.html?file=mo5/tout-schuss_mo5.k7&input=run%22%22%0A', 'img':'mo5/tout-schuss_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Vampire (tape)",              'system':'mo5', 'url':'mo5.html?file=mo5/vampire_mo5.k7&input=loadm%0A', 'img':'mo5/vampire_mo5.jpg', 'note':''},
    { 'type':'game', 'title':"Zéro de conduite (tape)",     'system':'mo5', 'url':'mo5.html?file=mo5/zero-de-conduite_mo5.k7&input=run%22%22%0A', 'img':'mo5/zero-de-conduite_mo5.jpg', 'note':''}
]

#-------------------------------------------------------------------------------
def cwebp(src_path, dst_path):
    cmd_line = 'cwebp -quiet -q 80 {} -o {}'.format(src_path, dst_path)
    print('> {}'.format(cmd_line))
    subprocess.call(cmd_line, shell=True)

#-------------------------------------------------------------------------------
def to_webp_ext(path):
    return os.path.splitext(path)[0] + '.webp'

#-------------------------------------------------------------------------------
def deploy_webpage(fips_dir, proj_dir, webpage_dir) :
    emsc_deploy_dir = util.get_deploy_dir(fips_dir, 'yame', BuildConfig)

    # build the thumbnail gallery
    content = ''
    for item in items :
        if item['type'] == 'test':
            continue
        title = item['title']
        system = item['system']
        url = item['url']
        ui_url = url.replace(".html", "-ui.html")
        image = to_webp_ext(item['img'])
        note = item['note']
        log.info('> adding thumbnail for {}'.format(url))
        content += '<div class="thumb-frame">\n'
        content += '  <div class="thumb-title">{}</div>\n'.format(title)
        if os.path.exists(emsc_deploy_dir + '/' + system + '-ui.js'):
            content += '<a class="ui-btn-link" href="{}"><div class="ui-btn">UI</div></a>'.format(ui_url)
        content += '  <a class="image-frame" href="{}"><img class="image" src="{}"></img></a>\n'.format(url,image)
        if note != '':
            content += '  <div class="thumb-footer">{}</div>\n'.format(note)
        content += '</div>\n'

    # populate the html template, and write to the build directory
    with open(proj_dir + '/webpage/index.html', 'r') as f :
        templ = Template(f.read())
    html = templ.safe_substitute(samples=content)
    with open(webpage_dir + '/index.html', 'w') as f :
        f.write(html)

    # and the same with the CSS template
    with open(proj_dir + '/webpage/style.css', 'r') as f :
        templ = Template(f.read())
    css = templ.safe_substitute()
    with open(webpage_dir +'/style.css', 'w') as f :
        f.write(css)

    # copy other required files
    for name in ['favicon.png'] :
        log.info('> copy file: {}'.format(name))
        shutil.copy(proj_dir + '/webpage/' + name, webpage_dir + '/' + name)

    # generate emu HTML pages
    for system in systems :
        log.info('> generate emscripten HTML page: {}'.format(system))
        for ext in ['wasm', 'js'] :
            src_path = '{}/{}.{}'.format(emsc_deploy_dir, system, ext)
            if os.path.isfile(src_path) :
                shutil.copy(src_path, '{}/'.format(webpage_dir))
        with open(proj_dir + '/webpage/emsc.html', 'r') as f :
            templ = Template(f.read())
        html = templ.safe_substitute(name=system, prog=system)
        with open('{}/{}.html'.format(webpage_dir, system), 'w') as f :
            f.write(html)

    # copy data files and images
    for asset_dir in asset_dirs:
        src_dir = proj_dir + '/webpage/' + asset_dir
        dst_dir = webpage_dir + '/' + asset_dir
        if os.path.isdir(src_dir):
            if os.path.isdir(dst_dir):
                shutil.rmtree(dst_dir)
            os.makedirs(dst_dir)
            for filename in os.listdir(src_dir):
                src_file = src_dir + '/' + filename
                dst_file = dst_dir + '/' + filename
                if filename.endswith('.jpg'):
                    dst_file = to_webp_ext(dst_file)
                    cwebp(src_file, dst_file)
                else:
                    print('> copy {} => {}'.format(src_file, dst_file))
                    shutil.copyfile(src_file, dst_file)

#-------------------------------------------------------------------------------
def build_deploy_webpage(fips_dir, proj_dir, rebuild) :
    # if webpage dir exists, clear it first
    proj_build_dir = util.get_deploy_root_dir(fips_dir, 'yame')
    webpage_dir = '{}/yame-webpage'.format(proj_build_dir)
    if rebuild :
        if os.path.isdir(webpage_dir) :
            shutil.rmtree(webpage_dir)
    if not os.path.isdir(webpage_dir) :
        os.makedirs(webpage_dir)

    # compile samples
    project.gen(fips_dir, proj_dir, BuildConfig)
    project.build(fips_dir, proj_dir, BuildConfig)

    # deploy the webpage
    deploy_webpage(fips_dir, proj_dir, webpage_dir)

    log.colored(log.GREEN, 'Generated Samples web page under {}.'.format(webpage_dir))

#-------------------------------------------------------------------------------
def serve_webpage(fips_dir, proj_dir) :
    proj_build_dir = util.get_deploy_root_dir(fips_dir, 'yame')
    webpage_dir = '{}/yame-webpage'.format(proj_build_dir)
    p = util.get_host_platform()
    if p == 'osx' :
        try :
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt :
            pass
    elif p == 'win':
        try:
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass
    elif p == 'linux':
        try:
            subprocess.call(
                'http-server -c-1 -g -o'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass

#-------------------------------------------------------------------------------
def run(fips_dir, proj_dir, args) :
    if len(args) > 0 :
        if args[0] == 'build' :
            build_deploy_webpage(fips_dir, proj_dir, False)
        elif args[0] == 'rebuild' :
            build_deploy_webpage(fips_dir, proj_dir, True)
        elif args[0] == 'serve' :
            serve_webpage(fips_dir, proj_dir)
        else :
            log.error("Invalid param '{}', expected 'build' or 'serve'".format(args[0]))
    else :
        log.error("Param 'build' or 'serve' expected")

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
             'fips webpage build\n' +
             'fips webpage rebuild\n' +
             'fips webpage serve\n' +
             log.DEF +
             '    build ysame samples webpage')
