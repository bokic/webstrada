import { Component, inject, OnInit, signal } from '@angular/core';

import { AdminApiService, CacheEntry } from '../../services/admin-api.service';

interface Stat {
  label: string;
  value: string;
  sub: string;
}

function formatSizeBytes(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1048576) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / 1048576).toFixed(1)} MB`;
}

function formatTimeStr(epochMs: number): string {
  const d = new Date(epochMs);
  return d.toTimeString().slice(0, 8);
}

type SortField = keyof Pick<CacheEntry, 'region' | 'id' | 'hits' | 'size' | 'createdMs' | 'expiresMs'>;

@Component({
  selector: 'app-cache',
  imports: [],
  templateUrl: './cache.html',
  styleUrl: './cache.css',
})
export class Cache implements OnInit {
  private api = inject(AdminApiService);

  protected readonly loaded = signal(false);
  protected readonly busy = signal(false);
  protected readonly message = signal('');
  protected readonly entries = signal<CacheEntry[]>([]);
  protected readonly totalEntries = signal(0);
  protected readonly totalHits = signal(0);
  protected readonly totalSize = signal(0);
  protected readonly compiledTemplates = signal(0);
  protected readonly compiledComponents = signal(0);

  protected sortField: SortField = 'id';
  protected sortDir: 1 | -1 = 1;

  ngOnInit() {
    this.refresh();
  }

  protected stats(): Stat[] {
    return [
      { label: 'Cache Entries', value: this.totalEntries().toLocaleString(), sub: 'store entries across all regions' },
      { label: 'Total Cache Hits', value: this.totalHits().toLocaleString(), sub: 'since last restart' },
      { label: 'Cache Memory', value: this.formatSize(this.totalSize()), sub: 'stored entry sizes' },
      { label: 'Compiled Templates', value: this.compiledTemplates().toLocaleString(), sub: 'JIT-compiled .cfm files' },
      { label: 'Compiled Components', value: this.compiledComponents().toLocaleString(), sub: 'JIT-compiled .cfc files' },
    ];
  }

  // Template-exposed formatters (module functions are not callable there).
  protected formatSize(bytes: number): string {
    return formatSizeBytes(bytes);
  }

  protected formatTime(epochMs: number): string {
    return formatTimeStr(epochMs);
  }

  protected refresh() {
    this.message.set('');
    this.api.getCacheInfo().subscribe({
      next: (info) => {
        this.totalEntries.set(info.totalEntries);
        this.totalHits.set(info.totalHits);
        this.totalSize.set(info.totalSize);
        this.compiledTemplates.set(info.compiledTemplates);
        this.compiledComponents.set(info.compiledComponents);
        this.entries.set([...info.entries]);
        this.applySort();
        this.loaded.set(true);
      },
      error: () => {
        this.loaded.set(true);
      },
    });
  }

  protected clearAll() {
    this.busy.set(true);
    this.message.set('');
    this.api.clearCache().subscribe({
      next: () => {
        this.busy.set(false);
        this.refresh();
        this.message.set('All caches cleared.');
      },
      error: (err) => {
        this.busy.set(false);
        this.message.set(err.error?.error ?? 'Clear failed.');
      },
    });
  }

  protected evict(entry: CacheEntry) {
    this.busy.set(true);
    this.message.set('');
    this.api.evictCacheEntry(entry.region, entry.id).subscribe({
      next: (r) => {
        this.busy.set(false);
        this.refresh();
        this.message.set(r.ok ? `Evicted '${entry.id}'.` : `Evict failed: ${r.error ?? 'unknown'}`);
      },
      error: (err) => {
        this.busy.set(false);
        this.message.set(err.error?.error ?? 'Evict failed.');
      },
    });
  }

  protected sortBy(field: SortField) {
    if (this.sortField === field) {
      this.sortDir = this.sortDir === 1 ? -1 : 1;
    } else {
      this.sortField = field;
      this.sortDir = 1;
    }
    this.applySort();
  }

  protected sortIcon(field: SortField): string {
    if (this.sortField !== field) return '';
    return this.sortDir === 1 ? ' \u25b2' : ' \u25bc';
  }

  protected sortTitle(field: SortField): string {
    if (this.sortField !== field) return 'Sort by ' + field;
    return this.sortDir === 1 ? 'Sort descending' : 'Sort ascending';
  }

  private applySort() {
    const field = this.sortField;
    const dir = this.sortDir;
    this.entries.update((list) =>
      [...list].sort((a, b) => {
        const av = a[field];
        const bv = b[field];
        if (av < bv) return -1 * dir;
        if (av > bv) return 1 * dir;
        return 0;
      }),
    );
  }

  protected expiresText(entry: CacheEntry): string {
    return entry.expiresMs === 0 ? 'permanent' : this.formatTime(entry.expiresMs);
  }
}
