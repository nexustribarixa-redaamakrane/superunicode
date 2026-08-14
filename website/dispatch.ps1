<# ============================================================================
   SuperUnicode website dispatcher
   ----------------------------------------------------------------------------
   Builds the site (static HTML mirroring unicode.org) and generates the
   repository directory-listing pages. Optionally deploys to GitHub Pages.

   Usage:
     ./dispatch.ps1                 # build only, into website/_site/
     ./dispatch.ps1 -Deploy         # build + push to the gh-pages branch

   Requirements for -Deploy:
     * `git` on PATH
     * a `gh-pages` branch (created automatically if missing)
     * GitHub Pages configured to serve from the gh-pages branch
   ==========================================================================#>
[CmdletBinding()]
param(
    [switch]$Deploy,
    [string]$Branch = 'gh-pages'
)

$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot
$SiteRoot  = $ScriptDir
$RepoRoot  = Split-Path $ScriptDir -Parent
$SiteOut   = Join-Path $SiteRoot '_site'

# ---------------------------------------------------------------------------
# 1. Raster assets (only regenerated when missing; SVG is the source of truth)
# ---------------------------------------------------------------------------
Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue

function New-RoundRectPath([float]$w, [float]$h, [float]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $r * 2
    $p.AddArc(0, 0, $d, $d, 180, 90);  $p.AddArc($w - $d, 0, $d, $d, 270, 90)
    $p.AddArc($w - $d, $h - $d, $d, $d, 0, 90); $p.AddArc(0, $h - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}

function New-SucsTile([int]$size, [string]$path) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size)
    $g   = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'; $g.TextRenderingHint = 'AntiAliasGridFit'
    $rect = New-Object System.Drawing.RectangleF(0, 0, $size, $size)
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect,
        [System.Drawing.Color]::FromArgb(255, 255, 122, 0),
        [System.Drawing.Color]::FromArgb(255, 168, 85, 247), 45.0)
    $g.FillPath($brush, (New-RoundRectPath $size $size ([float]($size * 0.22))))
    $font = New-Object System.Drawing.Font('Segoe UI', [float]($size * 0.46),
        [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $sb = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = 'Center'; $sf.LineAlignment = 'Center'
    $layout = New-Object System.Drawing.RectangleF(0, [float]($size * 0.04), $size, [float]($size * 0.88))
    $g.DrawString('SU', $font, $sb, $layout, $sf)
    $g.Dispose(); $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
}

function New-Ico([string[]]$pngPaths, [string]$outPath) {
    $pngs = $pngPaths | ForEach-Object { [System.IO.File]::ReadAllBytes($_) }
    $count = $pngs.Count; $offset = 6 + 16 * $count
    $ms = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter($ms)
    $bw.Write([UInt16]0); $bw.Write([UInt16]1); $bw.Write([UInt16]$count)
    for ($i = 0; $i -lt $count; $i++) {
        $dim = switch ($i) { 0 { 16 } 1 { 32 } default { 48 } }
        $sz = $pngs[$i].Length
        $bw.Write([Byte]$dim); $bw.Write([Byte]$dim); $bw.Write([Byte]0); $bw.Write([Byte]0)
        $bw.Write([UInt16]1); $bw.Write([UInt16]32); $bw.Write([UInt32]$sz); $bw.Write([UInt32]$offset)
        $offset += $sz
    }
    foreach ($p in $pngs) { $bw.Write($p) }
    $bw.Flush()
    [System.IO.File]::WriteAllBytes($outPath, $ms.ToArray())
    $bw.Dispose(); $ms.Dispose()
}

New-Item -ItemType Directory -Force -Path (Join-Path $SiteRoot 'assets\img') | Out-Null
$logo = Join-Path $SiteRoot 'assets\img\logo.png'
$icon = Join-Path $SiteRoot 'favicon.ico'
if (-not (Test-Path $logo)) { New-SucsTile 256 $logo }
if (-not (Test-Path $icon)) {
    $t = Join-Path $env:TEMP 'sucs'; New-Item -ItemType Directory -Force -Path $t | Out-Null
    New-SucsTile 16 (Join-Path $t '16.png'); New-SucsTile 32 (Join-Path $t '32.png'); New-SucsTile 48 (Join-Path $t '48.png')
    New-Ico @((Join-Path $t '16.png'), (Join-Path $t '32.png'), (Join-Path $t '48.png')) $icon
}

# ---------------------------------------------------------------------------
# 2. Generate the module directory-listing pages
# ---------------------------------------------------------------------------
$Modules = @(
    @{ slug = 'unicode';          title = 'Unicode (Base SUCS) module';        desc = 'Base SuperUnicode: core, framing, Public/ tree (SUCD, sudat, names, collation, charts, mappings, partitions).';            src = 'superunicode' }
    @{ slug = 'unicode-extended'; title = 'Unicode Extended (ExtSUCS) module'; desc = 'Extended SuperUnicode: extSUTF, plugin subsystem, SDK, and the Extended Public/ tree (registry, plugins, transport).';        src = 'superunicode_extended' }
    @{ slug = 'sutf';             title = 'SUTF module';                        desc = 'SUCS UTF-8 / UTF-16 / UTF-32 framing reference implementations and tests.';                                                    src = 'sutf' }
    @{ slug = 'extsutf';          title = 'extSUTF module';                     desc = 'Extended transport: vector layout, vsutf and esutf encoders, over sucs_ex_char_t (64-bit).';                               src = 'superunicode_extended'; filter = 'extsutf|vsutf|esutf|transport' }
    @{ slug = 'unified';          title = 'Unified module';                     desc = 'The one translation unit that compiles every public header of every module, plus its build wiring.';                        src = 'unified' }
)

$Ignore = @('build', 'bin', '.git', 'CMakeFiles', '_site', '.vs', '.vscode')

function Get-FilteredChildren([string]$abs, [string]$rel, [string]$filter) {
    # Filter keeps a directory when the dir itself matches OR any descendant
    # matches, so an "extsutf"-style filter still shows its ancestor chain.
    $items = Get-ChildItem -LiteralPath $abs | Where-Object { $_.Name -notin $Ignore }
    $out = @()
    foreach ($item in $items) {
        $itemRel = "$rel/$($item.Name)"
        if ($item.PSIsContainer) {
            $sub = Get-FilteredChildren $item.FullName $itemRel $filter
            if (-not $filter -or $itemRel -match $filter -or $sub.Count -gt 0) {
                $out += @{ Item = $item; Sub = $sub }
            }
        } else {
            if (-not $filter -or $itemRel -match $filter) {
                $out += @{ Item = $item; Sub = @() }
            }
        }
    }
    return $out
}

function New-DirSection([string]$abs, [string]$rel, [int]$depth, [string]$filter, [System.Text.StringBuilder]$sb, [ref]$dirCount) {
    $anchor = ($rel -replace '[\\/]', '_')
    if ($depth -gt 0) {
        [void]$sb.AppendLine("        <h3 id='$anchor' class='mono'>$rel/</h3>")
    }
    [void]$sb.AppendLine('        <div class="table-wrap"><table class="dir"><thead><tr><th>Name</th><th>Last modified</th><th>Size</th></tr></thead><tbody>')
    if ($depth -gt 0) {
        [void]$sb.AppendLine("          <tr><td><a href='#' onclick='history.back();return false;'>Parent directory</a></td><td></td><td>-</td></tr>")
    }
    $children = Get-FilteredChildren $abs $rel $filter
    $dirs  = $children | Where-Object { $_.Item.PSIsContainer } | Sort-Object { $_.Item.Name }
    $files = $children | Where-Object { -not $_.Item.PSIsContainer } | Sort-Object { $_.Item.Name }
    foreach ($d in $dirs) {
        $subRel = "$rel/$($d.Item.Name)"
        $dAnchor = ($subRel -replace '[\\/]', '_')
        $dirCount.Value++
        [void]$sb.AppendLine("          <tr><td><a href='#$dAnchor'>$($d.Item.Name)/</a></td><td>$($d.Item.LastWriteTime.ToString('yyyy-MM-dd HH:mm'))</td><td>-</td></tr>")
    }
    foreach ($f in $files) {
        $size = if ($f.Item.Length -ge 1048576) { "{0:N1} MB" -f ($f.Item.Length / 1048576) } else { "$($f.Item.Length) B" }
        [void]$sb.AppendLine("          <tr><td class='mono'>$($f.Item.Name)</td><td>$($f.Item.LastWriteTime.ToString('yyyy-MM-dd HH:mm'))</td><td>$size</td></tr>")
    }
    [void]$sb.AppendLine('        </tbody></table></div>')
    foreach ($d in $dirs) {
        New-DirSection (Join-Path $abs $d.Item.Name) "$rel/$($d.Item.Name)" ($depth + 1) $filter $sb $dirCount
    }
}

function New-ModulePage($mod) {
    $slug = $mod.slug; $srcRel = $mod.src
    $srcAbs = Join-Path $RepoRoot $srcRel
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine("<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'>")
    [void]$sb.AppendLine("<meta name='viewport' content='width=device-width, initial-scale=1'>")
    [void]$sb.AppendLine("<title>$($mod.title) — directory listing</title>")
    [void]$sb.AppendLine("<meta name='description' content='$($mod.desc)'>")
    [void]$sb.AppendLine("<link rel='icon' href='../../favicon.ico' sizes='16x16 32x32 48x48'>")
    [void]$sb.AppendLine("<link rel='icon' href='../../favicon.svg' type='image/svg+xml'>")
    [void]$sb.AppendLine("<link rel='apple-touch-icon' href='../../assets/img/logo.png'>")
    [void]$sb.AppendLine("<link rel='manifest' href='../../manifest.webmanifest'>")
    [void]$sb.AppendLine("<link rel='preconnect' href='https://fonts.googleapis.com'>")
    [void]$sb.AppendLine("<link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>")
    [void]$sb.AppendLine("<link href='https://fonts.googleapis.com/css2?family=Noto+Sans:ital,wght@0,100..900;1,100..900&display=swap' rel='stylesheet'>")
    [void]$sb.AppendLine("<link rel='stylesheet' href='../../assets/css/main.css'>")
    [void]$sb.AppendLine("</head><body>")
    [void]$sb.AppendLine("<header class='site-header'><div class='masthead'>")
    [void]$sb.AppendLine("<a class='brand' href='../../main.html'><img src='../../assets/img/logo.png' alt='SuperUnicode logo'>")
    [void]$sb.AppendLine("<span><span class='brand-name'>Super<em>Unicode</em></span><br><span class='brand-tag'>Native character encoding of the OpenWindows kernel</span></span></a>")
    [void]$sb.AppendLine('</div><nav class="site-nav">')
    [void]$sb.AppendLine('<a href="../../main.html">Home</a><a href="../../standard.html">The Standard</a><a href="../../extended.html">Extended</a><a href="../../sucd.html">Machine-Instruction Database</a><a href="../../reports.html">Technical Reports</a><a href="../../versions.html">Versions</a><a href="../../charts.html">Charts</a><a href="../../faq.html">FAQ</a><a href="../index.html" class="active">Repositories</a>')
    [void]$sb.AppendLine('</nav></header>')
    [void]$sb.AppendLine("<main class='page'>")
    [void]$sb.AppendLine("<nav class='breadcrumbs'><a href='../../main.html'>Home</a> &raquo; <a href='../index.html'>Repositories</a> &raquo; $($mod.title)</nav>")
    [void]$sb.AppendLine("<h1>Directory listing &mdash; <span class='grad'>$($mod.title)</span></h1>")
    [void]$sb.AppendLine("<p class='subtitle'>$($mod.desc)</p>")
    [void]$sb.AppendLine("<p class='note'>Physical source: <span class='mono'>$srcRel/</span> in the repository. Structure mirrors the versioned <span class='mono'>/Public</span> trees. Parent-directory rows jump to the enclosing directory section on this page.</p>")
    $dc = 0; $ref = [ref]$dc
    New-DirSection $srcAbs '' 0 $mod.filter $sb $ref
    [void]$sb.AppendLine("</main>")
    [void]$sb.AppendLine("<footer class='site-footer'><div class='footer-inner'><span>SuperUnicode&reg; &mdash; native character encoding of the OpenWindows kernel.</span><span><a href='../../main.html'>Home</a> &middot; <a href='../../standard.html'>Standard</a> &middot; <a href='../../extended.html'>Extended</a> &middot; <a href='../../sucd.html'>SUCD</a></span></div></footer>")
    [void]$sb.AppendLine('</body></html>')
    $outDir = Join-Path $SiteRoot "modules\$slug"
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $outDir 'index.html'), $sb.ToString())
    Write-Host "generated modules/$slug/ ($($ref.Value) directories)"
}

# Modules index page
function New-ModulesIndex {
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine("<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'>")
    [void]$sb.AppendLine("<meta name='viewport' content='width=device-width, initial-scale=1'>")
    [void]$sb.AppendLine('<title>Repositories — directory listings</title>')
    [void]$sb.AppendLine("<link rel='icon' href='../favicon.ico' sizes='16x16 32x32 48x48'>")
    [void]$sb.AppendLine("<link rel='icon' href='../favicon.svg' type='image/svg+xml'>")
    [void]$sb.AppendLine("<link rel='apple-touch-icon' href='../assets/img/logo.png'>")
    [void]$sb.AppendLine("<link rel='manifest' href='../manifest.webmanifest'>")
    [void]$sb.AppendLine("<link rel='preconnect' href='https://fonts.googleapis.com'>")
    [void]$sb.AppendLine("<link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>")
    [void]$sb.AppendLine("<link href='https://fonts.googleapis.com/css2?family=Noto+Sans:ital,wght@0,100..900;1,100..900&display=swap' rel='stylesheet'>")
    [void]$sb.AppendLine("<link rel='stylesheet' href='../assets/css/main.css'>")
    [void]$sb.AppendLine('</head><body>')
    [void]$sb.AppendLine("<header class='site-header'><div class='masthead'>")
    [void]$sb.AppendLine("<a class='brand' href='../main.html'><img src='../assets/img/logo.png' alt='SuperUnicode logo'>")
    [void]$sb.AppendLine("<span><span class='brand-name'>Super<em>Unicode</em></span><br><span class='brand-tag'>Native character encoding of the OpenWindows kernel</span></span></a>")
    [void]$sb.AppendLine('</div><nav class="site-nav">')
    [void]$sb.AppendLine('<a href="../main.html">Home</a><a href="../standard.html">The Standard</a><a href="../extended.html">Extended</a><a href="../sucd.html">Machine-Instruction Database</a><a href="../reports.html">Technical Reports</a><a href="../versions.html">Versions</a><a href="../charts.html">Charts</a><a href="../faq.html">FAQ</a><a href="index.html" class="active">Repositories</a>')
    [void]$sb.AppendLine('</nav></header>')
    [void]$sb.AppendLine("<main class='page'>")
    [void]$sb.AppendLine("<nav class='breadcrumbs'><a href='../main.html'>Home</a> &raquo; Repositories</nav>")
    [void]$sb.AppendLine("<h1>Repositories &mdash; <span class='grad'>directory listings</span></h1>")
    [void]$sb.AppendLine('<p class="subtitle">Live listings of every module in the SuperUnicode project.</p>')
    [void]$sb.AppendLine('<div class="grid">')
    foreach ($m in $Modules) {
        $name = $m.title
        [void]$sb.AppendLine("<div class='card'><h3><a href='$($m.slug)/index.html'>$name</a></h3><p>$($m.desc)</p></div>")
    }
    [void]$sb.AppendLine('</div></main>')
    [void]$sb.AppendLine("<footer class='site-footer'><div class='footer-inner'><span>SuperUnicode&reg; &mdash; native character encoding of the OpenWindows kernel.</span><span><a href='../main.html'>Home</a> &middot; <a href='../standard.html'>Standard</a> &middot; <a href='../extended.html'>Extended</a> &middot; <a href='../sucd.html'>SUCD</a></span></div></footer>")
    [void]$sb.AppendLine('</body></html>')
    New-Item -ItemType Directory -Force -Path (Join-Path $SiteRoot 'modules') | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $SiteRoot 'modules\index.html'), $sb.ToString())
}

New-ModulesIndex
foreach ($m in $Modules) { New-ModulePage $m }

# ---------------------------------------------------------------------------
# 3. Assemble _site/
# ---------------------------------------------------------------------------
if (Test-Path $SiteOut) { Remove-Item -LiteralPath $SiteOut -Recurse -Force }
New-Item -ItemType Directory -Path $SiteOut | Out-Null
Get-ChildItem -LiteralPath $SiteRoot | Where-Object { $_.Name -notin @('_site', 'dispatch.ps1') } | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $SiteOut -Recurse -Force
}
New-Item -ItemType File -Path (Join-Path $SiteOut '.nojekyll') -Force | Out-Null
Write-Host "site built at $SiteOut"

# ---------------------------------------------------------------------------
# 4. Deploy to GitHub Pages (gh-pages branch)
# ---------------------------------------------------------------------------
if ($Deploy) {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'git not found on PATH' }
    $work = Join-Path $env:TEMP 'sucs-ghpages'
    git -C $RepoRoot worktree remove --force $work 2>$null | Out-Null
    $branchExists = git -C $RepoRoot rev-parse --verify "$Branch" 2>$null
    if (-not $branchExists) {
        Write-Host "creating $Branch branch..."
        git -C $RepoRoot branch $Branch
    }
    git -C $RepoRoot worktree add $work $Branch 2>&1 | Out-Null
    Get-ChildItem -LiteralPath $work | Where-Object { $_.Name -notin @('.git') } | Remove-Item -Recurse -Force
    Copy-Item -Path (Join-Path $SiteOut '*') -Destination $work -Recurse -Force
    git -C $work add -A
    git -C $work commit -m "deploy site $(Get-Date -Format 'yyyy-MM-dd HHmm')" 2>&1 | Out-Null
    git -C $work push origin $Branch 2>&1 | Out-Null
    git -C $RepoRoot worktree remove --force $work
    Write-Host "deployed to origin/$Branch — enable GitHub Pages with source = '$Branch' (branch root)"
}
