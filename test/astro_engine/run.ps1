param(
  [string]$CasePath = (Join-Path $PSScriptRoot "cases.json")
)

$ErrorActionPreference = "Stop"
$civilZenithDegrees = 96.0

function Convert-ToMinute([string]$Value) {
  if ($Value -notmatch '^\d{2}:\d{2}$') {
    throw "invalid_time:$Value"
  }
  return ([int]$Value.Substring(0, 2) * 60) + [int]$Value.Substring(3, 2)
}

function Format-Minute([int]$MinuteOfDay) {
  return "{0:D2}:{1:D2}" -f [int][Math]::Floor($MinuteOfDay / 60), [int]($MinuteOfDay % 60)
}

function Normalize-Degrees([double]$Value) {
  while ($Value -lt 0.0) { $Value += 360.0 }
  while ($Value -ge 360.0) { $Value -= 360.0 }
  return $Value
}

function Normalize-Hours([double]$Value) {
  while ($Value -lt 0.0) { $Value += 24.0 }
  while ($Value -ge 24.0) { $Value -= 24.0 }
  return $Value
}

function ConvertTo-Radians([double]$Degrees) {
  return $Degrees * [Math]::PI / 180.0
}

function ConvertTo-Degrees([double]$Radians) {
  return $Radians * 180.0 / [Math]::PI
}

function Get-DayOfYear([string]$DateText) {
  $date = [DateTime]::ParseExact($DateText, "yyyy-MM-dd", [Globalization.CultureInfo]::InvariantCulture)
  return $date.DayOfYear
}

function Get-CivilEventMinute($Case, [bool]$Dawn) {
  $dayOfYear = [double](Get-DayOfYear $Case.date)
  $latitude = [double]$Case.latitude
  $longitude = [double]$Case.longitude
  $lngHour = $longitude / 15.0
  $baseHour = 18.0
  if ($Dawn) {
    $baseHour = 6.0
  }
  $approxTime = $dayOfYear + (($baseHour - $lngHour) / 24.0)
  $meanAnomaly = (0.9856 * $approxTime) - 3.289
  $trueLongitude = $meanAnomaly + (1.916 * [Math]::Sin((ConvertTo-Radians $meanAnomaly))) + (0.020 * [Math]::Sin((ConvertTo-Radians (2.0 * $meanAnomaly)))) + 282.634
  $trueLongitude = Normalize-Degrees $trueLongitude

  $rightAscension = ConvertTo-Degrees ([Math]::Atan(0.91764 * [Math]::Tan((ConvertTo-Radians $trueLongitude))))
  $rightAscension = Normalize-Degrees $rightAscension
  $longitudeQuadrant = [Math]::Floor($trueLongitude / 90.0) * 90.0
  $rightAscensionQuadrant = [Math]::Floor($rightAscension / 90.0) * 90.0
  $rightAscension = ($rightAscension + $longitudeQuadrant - $rightAscensionQuadrant) / 15.0

  $sinDec = 0.39782 * [Math]::Sin((ConvertTo-Radians $trueLongitude))
  $cosDec = [Math]::Cos([Math]::Asin($sinDec))
  $cosHour = ([Math]::Cos((ConvertTo-Radians $civilZenithDegrees)) - ($sinDec * [Math]::Sin((ConvertTo-Radians $latitude)))) / ($cosDec * [Math]::Cos((ConvertTo-Radians $latitude)))
  if ($cosHour -gt 1.0 -or $cosHour -lt -1.0) {
    throw "civil_twilight_unavailable"
  }

  $hourAngle = if ($Dawn) { 360.0 - (ConvertTo-Degrees ([Math]::Acos($cosHour))) } else { ConvertTo-Degrees ([Math]::Acos($cosHour)) }
  $hourAngle = $hourAngle / 15.0
  $localMeanTime = $hourAngle + $rightAscension - (0.06571 * $approxTime) - 6.622
  $utcHour = Normalize-Hours ($localMeanTime - $lngHour)
  $localHour = Normalize-Hours ($utcHour + ([double]$Case.utc_offset_minutes / 60.0))
  return [int]([Math]::Floor(($localHour * 60.0) + 0.5) % 1440)
}

$casesDoc = Get-Content -Raw -LiteralPath $CasePath | ConvertFrom-Json -ErrorAction Stop
$failures = 0

foreach ($case in $casesDoc.cases) {
  $caseFailures = @()
  $dawnMinute = Get-CivilEventMinute $case $true
  $duskMinute = Get-CivilEventMinute $case $false
  $expected = $case.expected

  if ($dawnMinute -lt (Convert-ToMinute $expected.civil_dawn_min) -or $dawnMinute -gt (Convert-ToMinute $expected.civil_dawn_max)) {
    $caseFailures += "civil_dawn $(Format-Minute $dawnMinute) outside $($expected.civil_dawn_min)-$($expected.civil_dawn_max)"
  }
  if ($duskMinute -lt (Convert-ToMinute $expected.civil_dusk_min) -or $duskMinute -gt (Convert-ToMinute $expected.civil_dusk_max)) {
    $caseFailures += "civil_dusk $(Format-Minute $duskMinute) outside $($expected.civil_dusk_min)-$($expected.civil_dusk_max)"
  }
  if ($dawnMinute -ge $duskMinute) {
    $caseFailures += "civil_dawn must occur before civil_dusk for this sanity case"
  }

  if ($caseFailures.Count -eq 0) {
    Write-Host "PASS $($case.name) dawn=$(Format-Minute $dawnMinute) dusk=$(Format-Minute $duskMinute)"
  } else {
    Write-Host "FAIL $($case.name): $($caseFailures -join '; ')"
    $failures++
  }
}

if ($failures -gt 0) {
  exit 1
}

Write-Host "PASS astro civil twilight sanity cases"
