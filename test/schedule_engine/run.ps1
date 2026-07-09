param(
  [string]$CasePath = (Join-Path $PSScriptRoot "cases.json")
)

$ErrorActionPreference = "Stop"

function Parse-MinuteOfDay([string]$Value) {
  if ($Value -notmatch '^\d{2}:\d{2}$') {
    throw "invalid_time:$Value"
  }
  $hour = [int]$Value.Substring(0, 2)
  $minute = [int]$Value.Substring(3, 2)
  if ($hour -lt 0 -or $hour -gt 23 -or $minute -lt 0 -or $minute -gt 59) {
    throw "invalid_time:$Value"
  }
  return ($hour * 60) + $minute
}

function Test-RuleActive([int]$NowMinute, [int]$OnMinute, [int]$OffMinute) {
  if ($OnMinute -lt $OffMinute) {
    return $NowMinute -ge $OnMinute -and $NowMinute -lt $OffMinute
  }
  return $NowMinute -ge $OnMinute -or $NowMinute -lt $OffMinute
}

function Get-MinutesUntil([int]$NowMinute, [int]$EventMinute) {
  if ($EventMinute -ge $NowMinute) {
    return $EventMinute - $NowMinute
  }
  return (1440 - $NowMinute) + $EventMinute
}

function Format-Minute([int]$MinuteOfDay) {
  return "{0:D2}:{1:D2}" -f [int][Math]::Floor($MinuteOfDay / 60), [int]($MinuteOfDay % 60)
}

function Invoke-ScheduleCase($Case) {
  $relayState = if ($Case.mode -eq "disabled") { $false } else { [bool]$Case.initial_on }
  $write = $false
  $scheduleState = $Case.mode
  $nextEvent = "none"

  if (-not [bool]$Case.time_valid) {
    $nextEvent = "time_invalid"
    if ($Case.mode -eq "schedule" -or $Case.mode -eq "astro") {
      $scheduleState = "paused_time_invalid"
    }
    return @{
      write = $write
      state = if ($relayState) { "on" } else { "off" }
      schedule_state = $scheduleState
      next_event = $nextEvent
    }
  }

  if ($Case.mode -eq "schedule") {
    $nowMinute = Parse-MinuteOfDay $Case.time
    $onMinute = Parse-MinuteOfDay $Case.rule.on
    $offMinute = Parse-MinuteOfDay $Case.rule.off
    $targetOn = Test-RuleActive $nowMinute $onMinute $offMinute
    $relayState = $targetOn
    $write = $true
    $scheduleState = if ($targetOn) { "scheduled_on" } else { "scheduled_off" }
    $eventMinute = if ($targetOn) { $offMinute } else { $onMinute }
    $delta = Get-MinutesUntil $nowMinute $eventMinute
    if ($delta -eq 0) {
      $delta = 1440
    }
    $eventTime = Format-Minute $eventMinute
    $eventState = if ($targetOn) { "off" } else { "on" }
    $nextEvent = "$($Case.rule.relay_id)@$eventTime=$eventState"
  }

  return @{
    write = $write
    state = if ($relayState) { "on" } else { "off" }
    schedule_state = $scheduleState
    next_event = $nextEvent
  }
}

$casesDoc = Get-Content -Raw -LiteralPath $CasePath | ConvertFrom-Json -ErrorAction Stop
$failures = 0

foreach ($case in $casesDoc.cases) {
  $actual = Invoke-ScheduleCase $case
  $expected = $case.expected
  $caseFailures = @()

  if ($actual.write -ne [bool]$expected.write) {
    $caseFailures += "write expected=$($expected.write) actual=$($actual.write)"
  }
  if ($actual.state -ne $expected.state) {
    $caseFailures += "state expected=$($expected.state) actual=$($actual.state)"
  }
  if ($actual.schedule_state -ne $expected.schedule_state) {
    $caseFailures += "schedule_state expected=$($expected.schedule_state) actual=$($actual.schedule_state)"
  }
  if ($actual.next_event -ne $expected.next_event) {
    $caseFailures += "next_event expected=$($expected.next_event) actual=$($actual.next_event)"
  }
  if ([bool]$expected.write -and [string]::IsNullOrWhiteSpace($actual.next_event)) {
    $caseFailures += "next_event must not be empty for schedule write cases"
  }

  if ($caseFailures.Count -eq 0) {
    Write-Host "PASS $($case.name)"
  } else {
    Write-Host "FAIL $($case.name): $($caseFailures -join '; ')"
    $failures++
  }
}

if ($failures -gt 0) {
  exit 1
}

Write-Host "PASS schedule engine boundary cases"
