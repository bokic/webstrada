import { Component, inject, OnInit, signal } from '@angular/core';
import { FormsModule } from '@angular/forms';

import { AdminApiService, AdminSettings } from '../../services/admin-api.service';

interface SettingRow {
  key: string;
  label: string;
  type: 'text' | 'number' | 'bool' | 'charset' | 'timespan';
  value: string | number | boolean;
}

interface SettingGroup {
  title: string;
  rows: SettingRow[];
}

// Charset choices offered by the Default Encoding / Default Template Charset
// comboboxes.
const CHARSETS = [
  'UTF-8',
  'ISO-8859-1',
  'ISO-8859-15',
  'Windows-1252',
  'US-ASCII',
  'UTF-16',
  'Shift_JIS',
  'EUC-JP',
  'GB2312',
  'Big5',
];

const ROW_DEFS: { key: keyof AdminSettings; label: string; type: 'text' | 'number' | 'bool' | 'charset' | 'timespan'; group: string }[] = [
  { key: 'defaultOutputCharset', label: 'Default Encoding', type: 'charset', group: 'Basic Settings' },
  { key: 'defaultInputCharset', label: 'Default Template Charset', type: 'charset', group: 'Basic Settings' },
  { key: 'charsetDetectionMinConfidence', label: 'Charset Detection Min Confidence', type: 'number', group: 'Basic Settings' },
  { key: 'enableWhitespaceManagement', label: 'Whitespace Management', type: 'bool', group: 'Basic Settings' },
  { key: 'defaultApplicationTimeoutSeconds', label: 'Default Application Timeout', type: 'timespan', group: 'Timeouts' },
  { key: 'defaultSessionTimeoutSeconds', label: 'Default Session Timeout', type: 'timespan', group: 'Timeouts' },
  { key: 'enableQueryLogging', label: 'Query Logging', type: 'bool', group: 'Diagnostics' },
  { key: 'debugEnabled', label: 'Debugging', type: 'bool', group: 'Diagnostics' },
  { key: 'compileExtForInclude', label: 'Compile Extensions for Include', type: 'text', group: 'Basic Settings' },
];

@Component({
  selector: 'app-settings',
  imports: [FormsModule],
  templateUrl: './settings.html',
  styleUrl: './settings.css',
})
export class Settings implements OnInit {
  private api = inject(AdminApiService);

  // Signals so the zoneless app re-renders when async API responses land.
  protected readonly groups = signal<SettingGroup[]>([]);
  protected readonly loaded = signal(false);
  protected readonly saving = signal(false);
  protected readonly message = signal('');
  protected readonly charsets = CHARSETS;

  ngOnInit() {
    this.load();
  }

  private load() {
    this.api.getConfig().subscribe({
      next: (cfg) => {
        this.groups.set(this.buildGroups(cfg.settings));
        this.loaded.set(true);
      },
      error: () => {
        this.loaded.set(true);
      },
    });
  }

  private buildGroups(settings: AdminSettings): SettingGroup[] {
    const groups: SettingGroup[] = [];
    for (const def of ROW_DEFS) {
      let group = groups.find((g) => g.title === def.group);
      if (!group) {
        group = { title: def.group, rows: [] };
        groups.push(group);
      }
      group.rows.push({
        key: def.key,
        label: def.label,
        type: def.type,
        value: settings[def.key],
      });
    }
    return groups;
  }

  // ---- Timespan (Days / Hours / Minutes spin boxes) helpers ----
  // The row value is the total number of seconds; the three spin boxes edit
  // its day/hour/minute parts.

  protected timespanDays(row: SettingRow): number {
    return Math.floor((Number(row.value) || 0) / 86400);
  }

  protected timespanHours(row: SettingRow): number {
    return Math.floor(((Number(row.value) || 0) % 86400) / 3600);
  }

  protected timespanMinutes(row: SettingRow): number {
    return Math.floor(((Number(row.value) || 0) % 3600) / 60);
  }

  protected setTimespanPart(row: SettingRow, part: 'd' | 'h' | 'm', value: number) {
    const days = this.timespanDays(row);
    const hours = this.timespanHours(row);
    const minutes = this.timespanMinutes(row);
    let total = 0;
    if (part === 'd') total = (value * 86400) + (hours * 3600) + (minutes * 60);
    else if (part === 'h') total = (days * 86400) + (value * 3600) + (minutes * 60);
    else total = (days * 86400) + (hours * 3600) + (value * 60);
    row.value = total;
  }

  protected save() {
    this.saving.set(true);
    this.message.set('');
    const settings: Partial<AdminSettings> = {};
    for (const group of this.groups()) {
      for (const row of group.rows) {
        (settings as Record<string, unknown>)[row.key] =
          row.type === 'number' || row.type === 'timespan' ? Number(row.value) : row.value;
      }
    }
    this.api.updateConfig({ settings }).subscribe({
      next: (cfg) => {
        this.saving.set(false);
        this.message.set('Settings saved.');
        this.groups.set(this.buildGroups(cfg.settings));
      },
      error: (err) => {
        this.saving.set(false);
        this.message.set(err.error?.error ?? 'Save failed.');
      },
    });
  }

  protected restoreDefaults() {
    this.saving.set(true);
    this.message.set('');
    this.api.resetConfig().subscribe({
      next: (cfg) => {
        this.saving.set(false);
        this.message.set('Settings restored to defaults.');
        this.groups.set(this.buildGroups(cfg.settings));
      },
      error: (err) => {
        this.saving.set(false);
        this.message.set(err.error?.error ?? 'Restore failed.');
      },
    });
  }
}
