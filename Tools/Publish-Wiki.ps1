# Copy Doc/Wiki pages into GitHub's separate Wiki Git repository.
# From an up-to-date Sonheim main checkout:
#   git switch main
#   git pull --ff-only
#   powershell -ExecutionPolicy Bypass -File .\Tools\Publish-Wiki.ps1 -Preview
#   powershell -ExecutionPolicy Bypass -File .\Tools\Publish-Wiki.ps1
# -Preview only converts pages to a temporary folder. -NoPush creates a
# local Wiki commit without pushing. Existing unrelated Wiki pages are kept.
[CmdletBinding()]
param(
    [switch]$Preview,
    [switch]$NoPush,
    [string]$WikiRemote = "https://github.com/chungheonLee0325/Sonheim.wiki.git"
)
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$sourceRoot = Join-Path $repoRoot "Doc/Wiki"
$wikiBase = "https://github.com/chungheonLee0325/Sonheim/wiki"
$assetBase = "https://raw.githubusercontent.com/chungheonLee0325/Sonheim/main/Doc"
if (-not (Test-Path -LiteralPath $sourceRoot -PathType Container)) {
    throw "Missing Doc/Wiki: $sourceRoot"
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw "Install Git for Windows first." }
$branch = (& git -C $repoRoot branch --show-current | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $branch -ne "main") {
    throw "Use merged main first: git switch main; git pull --ff-only"
}
# Publish current chapters plus Home/Sidebar; skip legacy files with retired links.
$pages = @(Get-ChildItem -LiteralPath $sourceRoot -File -Filter "*.md" |
    Where-Object { $_.Name -match '^(Home|_Sidebar|[0-9]{2}_.+)\.md$' })
if ($pages.Count -lt 20) { throw "Expected at least Home, Sidebar and 18 Wiki chapters." }
$knownPages = @{}
foreach ($page in $pages) { $knownPages[$page.BaseName] = $true }

function Convert-WikiPage {
    param([string]$Content)
    # GitHub Wiki has a different repo root; link images to source main.
    $Content = $Content.Replace("../Media/", "$assetBase/Media/")
    $Content = $Content.Replace("../Gifs/", "$assetBase/Gifs/")
    $Content = $Content.Replace("../../Sonheim/Source/",
        "https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/")
    $Content = $Content.Replace(
        "https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/",
        "https://github.com/chungheonLee0325/Sonheim/blob/main/")

    # Avoid [[label|page]] in Markdown tables: the pipe splits table cells.
    $Content = [regex]::Replace($Content, '\[\[([^\]|]+)\|([^\]]+)\]\]', {
        param($m)
        $label = $m.Groups[1].Value
        $page = $m.Groups[2].Value -replace '\.md$', ''
        return ('[{0}]({1}/{2})' -f $label, $wikiBase,
            [Uri]::EscapeDataString($page))
    })
    # Leave bare [[...]] untouched: Mermaid also uses this syntax for nodes.

    # Convert Doc/Wiki relative Markdown links to Wiki page URLs.
    $Content = [regex]::Replace($Content,
        '\]\((?:\./)?([A-Za-z0-9_.-]+)\.md(#[^)]*)?\)', {
        param($m)
        $page = $m.Groups[1].Value
        $fragment = $m.Groups[2].Value
        return (']({0}/{1}{2})' -f $wikiBase,
            [Uri]::EscapeDataString($page), $fragment)
    })
    return $Content
}

$checkout = Join-Path ([IO.Path]::GetTempPath()) (
    "sonheim-wiki-" + [Guid]::NewGuid().ToString("N"))
$success = $false
try {
    if ($Preview) {
        New-Item -ItemType Directory -Path $checkout | Out-Null
    } else {
        & git clone --quiet $WikiRemote $checkout
        if ($LASTEXITCODE -ne 0) {
            throw "Wiki clone failed. Enable Wiki and create its first Home page in GitHub, then retry with Git Credential Manager authentication."
        }
    }

    $invalid = New-Object 'System.Collections.Generic.List[string]'
    foreach ($file in $pages) {
        $body = [IO.File]::ReadAllText($file.FullName)
        $converted = Convert-WikiPage -Content $body
        if ($converted -match '\[\[[^\]]+\|[^\]]+\]\]' -or
            $converted.Contains("../Media/") -or
            $converted.Contains("../Gifs/") -or
            $converted.Contains("/blob/feat-ui-dex/")) {
            throw "Unresolved link in $($file.Name)"
        }
        $refs = [regex]::Matches($converted,
            [regex]::Escape($wikiBase) + '/([A-Za-z0-9_.-]+)')
        foreach ($ref in $refs) {
            $destPage = [Uri]::UnescapeDataString($ref.Groups[1].Value)
            if (-not $knownPages.ContainsKey($destPage)) {
                $invalid.Add("$($file.Name) -> $destPage")
            }
        }
        [IO.File]::WriteAllText((Join-Path $checkout $file.Name),
            $converted, [System.Text.UTF8Encoding]::new($false))
    }
    if ($invalid.Count -gt 0) {
        throw ("Missing linked Wiki pages: " + (($invalid | Sort-Object -Unique) -join "; "))
    }

    Write-Host "Converted $($pages.Count) Wiki pages; media uses public main URLs."
    if ($Preview) {
        Write-Host "Preview saved to $checkout (no network write)."
        $success = $true
    } else {
        & git -C $checkout add -- "*.md"
        if ($LASTEXITCODE -ne 0) { throw "Git add failed." }
        & git -C $checkout diff --cached --quiet
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Wiki already up to date."
            $success = $true
        } elseif ($LASTEXITCODE -eq 1) {
            & git -C $checkout commit -m "docs: sync Sonheim Wiki from main"
            if ($LASTEXITCODE -ne 0) {
                throw "Git commit failed. Set git config user.name and user.email."
            }
            if ($NoPush) {
                Write-Host "Prepared Wiki commit (not pushed): $checkout"
                $success = $true
            } else {
                $wikiBranch = (& git -C $checkout branch --show-current | Out-String).Trim()
                if ($LASTEXITCODE -ne 0 -or -not $wikiBranch) {
                    throw "Could not determine Wiki branch."
                }
                & git -C $checkout push origin "HEAD:refs/heads/$wikiBranch"
                if ($LASTEXITCODE -ne 0) { throw "Wiki git push failed." }
                Write-Host "Published: $wikiBase"
                $success = $true
            }
        } else {
            throw "Git diff --cached failed."
        }
    }
}
finally {
    if ($success -and -not $Preview -and -not $NoPush) {
        Remove-Item -LiteralPath $checkout -Recurse -Force
    } elseif (Test-Path -LiteralPath $checkout) {
        Write-Host "Retained working files: $checkout"
    }
}
