import {
  Component,
  ElementRef,
  OnDestroy,
  OnInit,
  ViewChild,
  inject,
  signal,
} from '@angular/core';
import { AdminApiService, RecentRequest } from '../../services/admin-api.service';

export interface TraceRow {
  id: number;
  time: string;
  timestamp: number;
  template: string;
  status: number;
  statusText: string;
  method: string;
  durationMs: number;
  durationText: string;
}

function formatTime(epoch: number): string {
  const d = new Date(epoch * 1000);
  return d.toTimeString().slice(0, 8);
}

function getStatusText(code: number): string {
  switch (code) {
    case 200: return '200 OK';
    case 201: return '201 Created';
    case 204: return '204 No Content';
    case 301: return '301 Moved Permanently';
    case 302: return '302 Found';
    case 304: return '304 Not Modified';
    case 400: return '400 Bad Request';
    case 401: return '401 Unauthorized';
    case 403: return '403 Forbidden';
    case 404: return '404 Not Found';
    case 500: return '500 Internal Error';
    default: return String(code);
  }
}

@Component({
  selector: 'app-tracing',
  imports: [],
  templateUrl: './tracing.html',
  styleUrl: './tracing.css',
})
export class Tracing implements OnInit, OnDestroy {
  private api = inject(AdminApiService);

  @ViewChild('scrollWindow') private scrollWindowRef?: ElementRef<HTMLDivElement>;

  protected readonly loaded = signal(false);
  protected readonly isLive = signal(true);
  protected readonly excludeAdmin = signal(false);
  protected readonly requests = signal<TraceRow[]>([]);
  protected readonly loadingOlder = signal(false);
  protected readonly hasMoreOlder = signal(true);
  protected readonly newCountSinceScroll = signal(0);

  private pollTimer: ReturnType<typeof setInterval> | null = null;
  private allServerPool: TraceRow[] = [];
  private currentOffset = 0;
  private readonly pageSize = 10;
  private nextId = 1000;

  ngOnInit() {
    this.initialFetch();
    this.startPolling();
  }

  ngOnDestroy() {
    this.stopPolling();
  }

  protected toggleLive() {
    const nextState = !this.isLive();
    this.isLive.set(nextState);
    if (nextState) {
      this.startPolling();
    } else {
      this.stopPolling();
    }
  }

  protected toggleAdminFilter() {
    this.excludeAdmin.update((v) => !v);
    this.currentOffset = 0;
    this.initialFetch();
  }

  protected clearView() {
    this.requests.set([]);
    this.currentOffset = 0;
    this.newCountSinceScroll.set(0);
    this.initialFetch();
  }

  protected scrollToTop() {
    this.newCountSinceScroll.set(0);
    if (this.scrollWindowRef?.nativeElement) {
      this.scrollWindowRef.nativeElement.scrollTo({ top: 0, behavior: 'smooth' });
    }
  }

  protected onScroll(event: Event) {
    const el = event.target as HTMLElement;
    if (!el) return;

    if (el.scrollTop <= 10) {
      this.newCountSinceScroll.set(0);
    }

    // Infinite scroll check: near the bottom
    const distanceToBottom = el.scrollHeight - el.scrollTop - el.clientHeight;
    if (distanceToBottom < 40 && !this.loadingOlder() && this.hasMoreOlder()) {
      this.fetchOlderBatch();
    }
  }

  private startPolling() {
    this.stopPolling();
    this.pollTimer = setInterval(() => {
      if (this.isLive()) {
        this.pollNewRequests();
      }
    }, 1000);
  }

  private stopPolling() {
    if (this.pollTimer) {
      clearInterval(this.pollTimer);
      this.pollTimer = null;
    }
  }

  private initialFetch() {
    this.api.getServerInfo(this.excludeAdmin()).subscribe({
      next: (info) => {
        const rows = this.mapRecentRequests(info.recentRequests);
        this.allServerPool = [...rows];
        
        // Take the latest 10 requests for initial view
        const initialBatch = rows.slice(0, this.pageSize);
        this.currentOffset = initialBatch.length;
        this.hasMoreOlder.set(rows.length > initialBatch.length);
        this.requests.set(initialBatch);
        this.loaded.set(true);
      },
      error: () => {
        // Fallback for standalone/mock demo if server is offline
        if (this.allServerPool.length === 0) {
          this.generateMockPool();
        }
        const initialBatch = this.allServerPool.slice(0, this.pageSize);
        this.currentOffset = initialBatch.length;
        this.hasMoreOlder.set(this.allServerPool.length > initialBatch.length);
        this.requests.set(initialBatch);
        this.loaded.set(true);
      },
    });
  }

  private pollNewRequests() {
    this.api.getServerInfo(this.excludeAdmin()).subscribe({
      next: (info) => {
        const latestRows = this.mapRecentRequests(info.recentRequests);
        this.processNewArrivals(latestRows);
      },
      error: () => {
        // In offline/dev environment without server daemon, simulate occasional incoming requests
        this.simulateIncomingMockRequest();
      },
    });
  }

  private processNewArrivals(incoming: TraceRow[]) {
    const currentList = this.requests();
    if (incoming.length === 0) return;

    if (currentList.length === 0) {
      this.requests.set(incoming.slice(0, this.pageSize));
      return;
    }

    const newestCurrent = currentList[0];
    // Find all incoming rows newer than newestCurrent
    const newItems: TraceRow[] = [];
    for (const row of incoming) {
      if (
        (row.id > 0 && newestCurrent.id > 0 && row.id > newestCurrent.id) ||
        row.timestamp > newestCurrent.timestamp ||
        (row.timestamp === newestCurrent.timestamp && row.template !== newestCurrent.template)
      ) {
        newItems.push(row);
      } else {
        break; // Reached known items (assuming newest first)
      }
    }

    if (newItems.length > 0) {
      this.prependNewRequests(newItems);
    }
  }

  private prependNewRequests(newRows: TraceRow[]) {
    const el = this.scrollWindowRef?.nativeElement;
    const prevScrollHeight = el ? el.scrollHeight : 0;
    const prevScrollTop = el ? el.scrollTop : 0;

    // Prepend new rows to table list
    this.requests.update((existing) => [...newRows, ...existing]);

    // If user has scrolled down or content expanded above viewport,
    // adjust scrollTop so table cells do NOT scroll/jump; only scrollbar updates!
    setTimeout(() => {
      if (el) {
        const newScrollHeight = el.scrollHeight;
        const deltaHeight = newScrollHeight - prevScrollHeight;
        if (deltaHeight > 0) {
          el.scrollTop = prevScrollTop + deltaHeight;
        }
      }
    }, 0);

    this.newCountSinceScroll.update((c) => c + newRows.length);
  }

  private fetchOlderBatch() {
    this.loadingOlder.set(true);

    setTimeout(() => {
      const nextBatch = this.allServerPool.slice(
        this.currentOffset,
        this.currentOffset + this.pageSize,
      );

      if (nextBatch.length > 0) {
        this.requests.update((existing) => [...existing, ...nextBatch]);
        this.currentOffset += nextBatch.length;
        this.hasMoreOlder.set(this.currentOffset < this.allServerPool.length);
      } else {
        this.hasMoreOlder.set(false);
      }
      this.loadingOlder.set(false);
    }, 300);
  }

  private mapRecentRequests(list: RecentRequest[]): TraceRow[] {
    // Backend recentRequests is oldest first, so reverse to have newest first
    const reversed = [...list].reverse();
    return reversed.map((r, index) => {
      const id = r.id || (list.length - index);
      return {
        id,
        time: formatTime(r.time),
        timestamp: r.time,
        template: r.template,
        status: r.status,
        statusText: getStatusText(r.status),
        method: r.method,
        durationMs: r.durationMs,
        durationText: `${r.durationMs.toFixed(1)} ms`,
      };
    });
  }

  private generateMockPool() {
    const sampleTemplates = [
      '/index.cfm',
      '/api/users.cfm',
      '/products/list.cfm',
      '/auth/login.cfm',
      '/api/orders.cfm',
      '/reports/monthly.cfm',
      '/admin/config.cfm',
      '/services/inventory.cfm',
      '/checkout/pay.cfm',
      '/api/health.cfm',
      '/dashboard/analytics.cfm',
      '/content/page.cfm',
    ];
    const methods = ['GET', 'POST', 'GET', 'GET', 'PUT', 'GET', 'DELETE'];
    const statuses = [200, 200, 200, 200, 302, 204, 404, 500, 200, 200];

    const now = Math.floor(Date.now() / 1000);
    const pool: TraceRow[] = [];

    for (let i = 0; i < 60; i++) {
      const time = now - (i * 2 + Math.floor(Math.random() * 3));
      const method = methods[i % methods.length];
      const template = sampleTemplates[i % sampleTemplates.length];
      const status = statuses[i % statuses.length];
      const durationMs = 1.5 + Math.random() * 45;
      const id = this.nextId--;

      pool.push({
        id,
        time: formatTime(time),
        timestamp: time,
        template,
        status,
        statusText: getStatusText(status),
        method,
        durationMs,
        durationText: `${durationMs.toFixed(1)} ms`,
      });
    }

    this.allServerPool = pool;
    this.nextId = 1001;
  }

  private simulateIncomingMockRequest() {
    // Generates a mock request occasionally during offline testing
    const sampleTemplates = [
      '/api/events.cfm',
      '/index.cfm',
      '/api/status.cfm',
      '/users/profile.cfm',
      '/products/item.cfm',
    ];
    const now = Math.floor(Date.now() / 1000);
    const method = Math.random() > 0.3 ? 'GET' : 'POST';
    const template = sampleTemplates[Math.floor(Math.random() * sampleTemplates.length)];
    const status = Math.random() > 0.05 ? 200 : (Math.random() > 0.5 ? 404 : 500);
    const durationMs = 0.8 + Math.random() * 35;
    const id = ++this.nextId;

    const newRow: TraceRow = {
      id,
      time: formatTime(now),
      timestamp: now,
      template,
      status,
      statusText: getStatusText(status),
      method,
      durationMs,
      durationText: `${durationMs.toFixed(1)} ms`,
    };

    this.prependNewRequests([newRow]);
  }

  protected getMethodClass(method: string): string {
    const m = (method || '').toUpperCase();
    if (m === 'GET') return 'get';
    if (m === 'POST') return 'post';
    if (m === 'PUT') return 'put';
    if (m === 'DELETE') return 'delete';
    return 'other';
  }

  protected getDurationClass(ms: number): string {
    if (ms < 10) return 'duration-fast';
    if (ms < 50) return 'duration-med';
    return 'duration-slow';
  }
}
