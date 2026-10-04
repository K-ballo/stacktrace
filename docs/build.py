"""Convert docs/*.md to docs/*.html."""

import pathlib
import re
import shutil
import sys
import markdown

DOCS = pathlib.Path(__file__).parent
ASSETS = DOCS / 'html'
OUTPUT = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ASSETS
VERSION = sys.argv[2] if len(sys.argv) > 2 else ''

PAGES = [
    ('index.html',     'Index'),
    ('intro.html',     'Introduction'),
    ('design.html',    'Design'),
    ('usage.html',     'Usage'),
    ('cmake.html',     'CMake'),
    ('reference.html', 'Reference'),
]

TEMPLATE = """\
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{title}</title>
    <link rel="stylesheet" type="text/css" href="css/normalize.css">
    <link rel="stylesheet" type="text/css" href="css/highlight.css">
    <link rel="stylesheet" type="text/css" href="css/main.css">
    <link rel="shortcut icon" href="css/img/favicon.png"
          type="image/png">
</head>
<body>
    <div id="container">
        <header id="head">
            <div class="top">
                <img src="logo.png" class="logo" alt="">
                <a href="index.html">Eggs.Stacktrace</a>
            </div>
        </header>
        <div id="head_post">
{text_nav}
        </div>
        <div class="{page_class}">
            <section>
{body}
            </section>
        </div>
        <hr>
        <p>
            Copyright &copy; 2026 Agustin Berge
        </p>
        <p>
            Distributed under the Boost Software License, Version 1.0.<br>
            See accompanying file LICENSE.txt or copy at
            <a href="http://www.boost.org/LICENSE_1_0.txt">http://www.boost.org/LICENSE_1_0.txt</a>
        </p>
    </div>
</body>
</html>
"""

def build_text_nav(current_href):
    items = []
    for href, label in PAGES[1:]:  # skip index (already the logo link)
        cls = 'name title' if href == current_href else 'name'
        items.append(
            f'            <a class="{cls}" href="{href}">{label}</a>')
    return ' &middot;\n'.join(items)

def strip_html_comment(text):
    return re.sub(r'^\s*<!--.*?-->\s*', '', text, flags=re.DOTALL)

def add_heading_anchors(html):
    """Wrap heading text in a self-link so headings are blue/clickable."""
    def replace(m):
        tag, attrs, content = m.group(1), m.group(2), m.group(3)
        id_match = re.search(r'\bid="([^"]+)"', attrs)
        if not id_match:
            return m.group(0)
        hid = id_match.group(1)
        return (f'<{tag}{attrs}>'
                f'<a href="#{hid}">{content}</a>'
                f'</{tag}>')
    return re.sub(
        r'<(h[1-6])( [^>]+)>(.*?)</\1>',
        replace, html, flags=re.DOTALL)

def md_to_html(src):
    src = strip_html_comment(src)
    src = re.sub(
        r'\[([^\]]+)\]\(([^)]+)\.md\)', r'[\1](\2.html)', src)
    md = markdown.Markdown(
        extensions=['tables', 'fenced_code', 'codehilite', 'toc'],
        extension_configs={
            'codehilite': {'guess_lang': False},
        })
    return add_heading_anchors(md.convert(src))

def build():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    if OUTPUT != ASSETS:
        shutil.copytree(ASSETS / 'css', OUTPUT / 'css', dirs_exist_ok=True)
        shutil.copy2(ASSETS / 'logo.png', OUTPUT / 'logo.png')
    by_stem = {pathlib.Path(h).stem: l for h, l in PAGES}
    for md_path in sorted(DOCS.glob('*.md')):
        stem = md_path.stem
        html_name = stem + '.html'
        src = md_path.read_text(encoding='utf-8-sig')
        if VERSION:
            src = src.replace('@VERSION@', VERSION)
        body = md_to_html(src)
        label = by_stem.get(stem, stem)
        title = f'{label} - Eggs.Stacktrace'
        html = TEMPLATE.format(
            title=title,
            text_nav=build_text_nav(html_name),
            page_class=stem,
            body=body,
        )
        (OUTPUT / html_name).write_text(html, encoding='utf-8')
        print(f'  {html_name}')

if __name__ == '__main__':
    build()
    print('done')
