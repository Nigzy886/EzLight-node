param(
  [string]$FixtureDir = $PSScriptRoot
)

$ErrorActionPreference = "Stop"

$expectedResults = [ordered]@{
  "valid_default_config.json" = $true
  "invalid_malformed.json" = $false
  "invalid_wrong_shape.json" = $false
  "invalid_missing_node_id.json" = $false
  "invalid_wrong_node_type.json" = $false
  "invalid_wrong_profile.json" = $false
  "invalid_wrong_time_source.json" = $false
  "invalid_rtc_required_true.json" = $false
  "invalid_wrong_astro_convention.json" = $false
  "invalid_missing_relays.json" = $false
  "invalid_too_few_relays.json" = $false
  "invalid_too_many_relays.json" = $false
  "invalid_duplicate_relay_id.json" = $false
  "invalid_duplicate_gpio.json" = $false
  "invalid_wrong_gpio.json" = $false
  "invalid_wrapped_gpio.json" = $false
  "invalid_active_low_false.json" = $false
  "invalid_bad_mode.json" = $false
  "invalid_unsupported_feature_pir.json" = $false
  "invalid_bad_astro_location.json" = $false
  "valid_astro_location.json" = $true
  "valid_astro_rule_config.json" = $true
  "invalid_astro_location_set_not_boolean.json" = $false
  "invalid_astro_missing_location.json" = $false
  "invalid_bad_astro_latitude.json" = $false
  "invalid_bad_astro_longitude.json" = $false
  "invalid_astro_rules_not_array.json" = $false
  "invalid_astro_rule_unknown_relay.json" = $false
  "invalid_astro_rule_targeting_manual_relay.json" = $false
  "invalid_astro_rule_targeting_schedule_relay.json" = $false
  "invalid_astro_rule_targeting_disabled_relay.json" = $false
  "invalid_astro_unsupported_event_sunrise.json" = $false
  "invalid_astro_unsupported_offset.json" = $false
  "invalid_duplicate_astro_rule.json" = $false
  "valid_schedule_config.json" = $true
  "valid_overnight_schedule.json" = $true
  "invalid_schedule_bad_hhmm.json" = $false
  "invalid_schedule_24_00.json" = $false
  "invalid_schedule_12_60.json" = $false
  "invalid_schedule_enabled_not_boolean.json" = $false
  "invalid_schedule_overlap.json" = $false
  "invalid_schedule_rules_not_array.json" = $false
  "invalid_schedule_too_many_rules.json" = $false
  "invalid_schedule_wrapped_time_value.json" = $false
  "invalid_schedule_wrong_shape.json" = $false
  "invalid_duplicate_relay_schedule.json" = $false
  "invalid_schedule_targeting_astro_relay.json" = $false
  "invalid_schedule_unknown_relay.json" = $false
  "invalid_schedule_targeting_manual_relay.json" = $false
  "invalid_schedule_targeting_disabled_relay.json" = $false
}

$expectedRelays = @(
  @{ id = "path_lights"; gpio = 14 },
  @{ id = "driveway_lights"; gpio = 27 },
  @{ id = "entrance_lights"; gpio = 26 },
  @{ id = "spare_lights"; gpio = 25 }
)

$validModes = @("manual", "schedule", "astro", "disabled")
$validAstroEvents = @("civil_dusk", "civil_dawn")
$unsupportedFeatures = @("pir", "lux_sensor", "current_sensing", "holiday_mode", "random_holiday_mode", "wall_switches")

function Reject([string]$Reason) {
  throw $Reason
}

function Has-Property($Object, [string]$Name) {
  if ($null -eq $Object -or $null -eq $Object.PSObject) {
    return $false
  }
  return $Object.PSObject.Properties.Name -contains $Name
}

function Is-StringValue($Value) {
  return $Value -is [string] -and $Value.Length -gt 0
}

function Is-BoolValue($Value) {
  return $Value -is [bool]
}

function Is-NumberValue($Value) {
  return $Value -is [byte] -or $Value -is [int16] -or $Value -is [int32] -or $Value -is [int64] -or $Value -is [float] -or $Value -is [double] -or $Value -is [decimal]
}

function Parse-TimeOfDay([string]$Value) {
  if ($Value -notmatch '^\d{2}:\d{2}$') {
    Reject "invalid_schedule_time"
  }
  $hour = [int]$Value.Substring(0, 2)
  $minute = [int]$Value.Substring(3, 2)
  if ($hour -lt 0 -or $hour -gt 23 -or $minute -lt 0 -or $minute -gt 59) {
    Reject "invalid_schedule_time"
  }
  return ($hour * 60) + $minute
}

function Validate-EzLightConfig($Config) {
  if ($Config -is [array] -or $null -eq $Config -or $Config.GetType().Name -ne "PSCustomObject") {
    Reject "config_wrong_shape"
  }

  foreach ($feature in $unsupportedFeatures) {
    if (Has-Property $Config $feature) {
      Reject "unsupported_v0_1_feature"
    }
  }

  if (-not (Has-Property $Config "node_id") -or -not (Has-Property $Config "node_type") -or -not (Has-Property $Config "profile")) {
    Reject "missing_identity_fields"
  }
  if (-not (Is-StringValue $Config.node_id) -or -not (Is-StringValue $Config.node_type) -or -not (Is-StringValue $Config.profile)) {
    Reject "missing_identity_fields"
  }
  if ($Config.node_id -ne "ezlight_001" -or $Config.node_type -ne "ezlight" -or $Config.profile -ne "experimental.ezlight.v1") {
    Reject "invalid_identity"
  }

  if (-not (Has-Property $Config "timezone") -or -not (Has-Property $Config "time_source") -or -not (Has-Property $Config "rtc_required") -or -not (Has-Property $Config "astro_convention")) {
    Reject "missing_time_fields"
  }
  if (-not (Is-StringValue $Config.timezone)) {
    Reject "missing_timezone"
  }
  if ($Config.time_source -ne "ntp") {
    Reject "unsupported_time_source"
  }
  if (-not (Is-BoolValue $Config.rtc_required)) {
    Reject "missing_time_fields"
  }
  if ($Config.rtc_required) {
    Reject "rtc_not_supported_v0_1"
  }
  if ($Config.astro_convention -ne "civil_dusk_civil_dawn") {
    Reject "unsupported_astro_convention"
  }

  if (-not (Has-Property $Config "relays") -or $Config.relays -isnot [array]) {
    Reject "missing_relays"
  }
  $relays = @($Config.relays)
  if ($relays.Count -ne 4) {
    Reject "invalid_relay_count"
  }

  $seenIds = @{}
  $seenGpios = @{}
  $relayModes = @{}
  for ($i = 0; $i -lt 4; $i++) {
    $relay = $relays[$i]
    if ($relay.GetType().Name -ne "PSCustomObject") {
      Reject "invalid_relay_shape"
    }
    if (-not (Has-Property $relay "id") -or -not (Has-Property $relay "gpio") -or -not (Has-Property $relay "active_low") -or -not (Has-Property $relay "mode")) {
      Reject "invalid_relay_shape"
    }
    if (-not (Is-StringValue $relay.id) -or -not (Is-NumberValue $relay.gpio) -or -not (Is-BoolValue $relay.active_low) -or -not (Is-StringValue $relay.mode)) {
      Reject "invalid_relay_shape"
    }
    if ($seenIds.ContainsKey($relay.id)) {
      Reject "duplicate_relay_id"
    }
    $seenIds[$relay.id] = $true
    if ($seenGpios.ContainsKey([string]$relay.gpio)) {
      Reject "duplicate_relay_gpio"
    }
    $seenGpios[[string]$relay.gpio] = $true
    if ($relay.id -ne $expectedRelays[$i].id) {
      Reject "invalid_relay_id"
    }
    if ([int]$relay.gpio -ne $expectedRelays[$i].gpio) {
      Reject "invalid_relay_gpio"
    }
    if (-not $relay.active_low) {
      Reject "invalid_relay_polarity"
    }
    if ($validModes -notcontains $relay.mode) {
      Reject "invalid_relay_mode"
    }
    $relayModes[$relay.id] = $relay.mode
  }

  if (-not (Has-Property $Config "schedule") -or $Config.schedule.GetType().Name -ne "PSCustomObject") {
    Reject "missing_schedule"
  }
  if (-not (Has-Property $Config.schedule "enabled")) {
    Reject "invalid_schedule_config"
  }
  if (-not (Is-BoolValue $Config.schedule.enabled)) {
    Reject "invalid_schedule_config"
  }
  if ((Has-Property $Config.schedule "storage") -and $Config.schedule.storage -ne "littlefs_json") {
    Reject "invalid_schedule_config"
  }
  if (Has-Property $Config.schedule "events") {
    if ($Config.schedule.events -isnot [array]) {
      Reject "missing_schedule_events"
    }
    if (@($Config.schedule.events).Count -gt 0) {
      Reject "schedule_execution_not_supported_v0_1"
    }
  }
  $hasRules = Has-Property $Config.schedule "rules"
  if ($Config.schedule.enabled -and -not $hasRules) {
    Reject "missing_schedule_rules"
  }
  if ($hasRules) {
    if ($Config.schedule.rules -isnot [array]) {
      Reject "invalid_schedule_rules"
    }
    $rules = @($Config.schedule.rules)
    if ($rules.Count -gt 8) {
      Reject "too_many_schedule_rules"
    }
    if ($Config.schedule.enabled -and $rules.Count -eq 0) {
      Reject "missing_schedule_rules"
    }
    $scheduledRelays = @{}
    foreach ($rule in $rules) {
      if ($rule.GetType().Name -ne "PSCustomObject") {
        Reject "invalid_schedule_rule"
      }
      if (-not (Has-Property $rule "relay_id") -or -not (Has-Property $rule "on") -or -not (Has-Property $rule "off")) {
        Reject "invalid_schedule_rule"
      }
      if (-not (Is-StringValue $rule.relay_id) -or -not (Is-StringValue $rule.on) -or -not (Is-StringValue $rule.off)) {
        Reject "invalid_schedule_rule"
      }
      if (-not $relayModes.ContainsKey($rule.relay_id)) {
        Reject "unknown_schedule_relay"
      }
      if ($relayModes[$rule.relay_id] -eq "disabled") {
        Reject "schedule_targets_disabled_relay"
      }
      if ($relayModes[$rule.relay_id] -ne "schedule") {
        Reject "schedule_targets_non_schedule_relay"
      }
      if ($scheduledRelays.ContainsKey($rule.relay_id)) {
        Reject "duplicate_schedule_rule"
      }
      $scheduledRelays[$rule.relay_id] = $true
      $onMinute = Parse-TimeOfDay $rule.on
      $offMinute = Parse-TimeOfDay $rule.off
      if ($onMinute -eq $offMinute) {
        Reject "invalid_schedule_window"
      }
    }
  }

  if (-not (Has-Property $Config "astro") -or $Config.astro.GetType().Name -ne "PSCustomObject") {
    Reject "missing_astro"
  }
  if (-not (Has-Property $Config.astro "location_set") -or -not (Is-BoolValue $Config.astro.location_set)) {
    Reject "missing_astro_location_flag"
  }
  if ($Config.astro.location_set) {
    if (-not (Has-Property $Config.astro "latitude") -or -not (Has-Property $Config.astro "longitude")) {
      Reject "missing_astro_location"
    }
    if (-not (Is-NumberValue $Config.astro.latitude) -or -not (Is-NumberValue $Config.astro.longitude)) {
      Reject "missing_astro_location"
    }
    if ($Config.astro.latitude -lt -90.0 -or $Config.astro.latitude -gt 90.0 -or $Config.astro.longitude -lt -180.0 -or $Config.astro.longitude -gt 180.0) {
      Reject "invalid_astro_location"
    }
  }
  if (Has-Property $Config.astro "rules") {
    if ($Config.astro.rules -isnot [array]) {
      Reject "invalid_astro_rules"
    }
    if (-not $Config.astro.location_set -and @($Config.astro.rules).Count -gt 0) {
      Reject "astro_rules_require_location"
    }
    $astroRules = @($Config.astro.rules)
    if ($astroRules.Count -gt 4) {
      Reject "too_many_astro_rules"
    }
    $astroRelays = @{}
    foreach ($rule in $astroRules) {
      if ($rule.GetType().Name -ne "PSCustomObject") {
        Reject "invalid_astro_rule"
      }
      if (-not (Has-Property $rule "relay_id") -or -not (Has-Property $rule "on") -or -not (Has-Property $rule "off")) {
        Reject "invalid_astro_rule"
      }
      if ((Has-Property $rule "offset") -or (Has-Property $rule "offset_minutes") -or (Has-Property $rule "on_offset") -or (Has-Property $rule "off_offset")) {
        Reject "unsupported_astro_offset"
      }
      if (-not (Is-StringValue $rule.relay_id) -or -not (Is-StringValue $rule.on) -or -not (Is-StringValue $rule.off)) {
        Reject "invalid_astro_rule"
      }
      if (-not $relayModes.ContainsKey($rule.relay_id)) {
        Reject "unknown_astro_relay"
      }
      if ($relayModes[$rule.relay_id] -eq "disabled") {
        Reject "astro_targets_disabled_relay"
      }
      if ($relayModes[$rule.relay_id] -ne "astro") {
        Reject "astro_targets_non_astro_relay"
      }
      if ($astroRelays.ContainsKey($rule.relay_id)) {
        Reject "duplicate_astro_rule"
      }
      if ($validAstroEvents -notcontains $rule.on -or $validAstroEvents -notcontains $rule.off) {
        Reject "unsupported_astro_event"
      }
      if ($rule.on -eq $rule.off) {
        Reject "invalid_astro_window"
      }
      $astroRelays[$rule.relay_id] = $true
    }
  }

  return "ok"
}

$failures = 0

foreach ($caseName in $expectedResults.Keys) {
  $path = Join-Path $FixtureDir $caseName
  $expectedAccepted = $expectedResults[$caseName]
  $accepted = $false
  $reason = "ok"

  try {
    if (-not (Test-Path -LiteralPath $path)) {
      Reject "fixture_missing"
    }
    $raw = Get-Content -Raw -LiteralPath $path
    $config = $raw | ConvertFrom-Json -ErrorAction Stop
    $reason = Validate-EzLightConfig $config
    $accepted = $true
  } catch {
    $reason = $_.Exception.Message
    $accepted = $false
  }

  if ($accepted -eq $expectedAccepted) {
    Write-Host "PASS $caseName ($reason)"
  } else {
    $expectedText = if ($expectedAccepted) { "accepted" } else { "rejected" }
    $actualText = if ($accepted) { "accepted" } else { "rejected" }
    Write-Host "FAIL $caseName expected $expectedText but was $actualText ($reason)"
    $failures++
  }
}

if ($failures -gt 0) {
  exit 1
}

Write-Host "PASS config validation fixtures"
